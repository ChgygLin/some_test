/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include <fstream>
#include <string>

// g++ -o testgles2 testgles2.c -g -I/usr/local/include -L/usr/local/lib  -lSDL2

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>


#include "ext.hpp"

#define GL_COMPUTE_SHADER                 0x91B9
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_READ_WRITE                     0x88BA
#define GL_RGBA32F                        0x8814
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020

#define MAX_ATTRACTORS  64


#define PARTICLE_GROUP_COUNT    5000
#define PARTICLE_GROUP_SIZE     128
#define PARTICLE_COUNT          (PARTICLE_GROUP_COUNT*PARTICLE_GROUP_SIZE)

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

#define VERBOSE_VIDEO 0x00000001
#define VERBOSE_MODES 0x00000002
#define VERBOSE_RENDER 0x00000004
#define VERBOSE_EVENT 0x00000008
#define VERBOSE_AUDIO 0x00000010
#define VERBOSE_MOTION 0x00000020


typedef struct
{
    /* SDL init flags */
    Uint32 flags;
    Uint32 verbose;

    /* Video info */
    const char *videodriver;
    int display;
    const char *window_title;
    const char *window_icon;
    Uint32 window_flags;

    int window_x;
    int window_y;
    int window_w;
    int window_h;
    int window_minW;
    int window_minH;
    int window_maxW;
    int window_maxH;

    int depth;

    SDL_Window *windows;

    /* Renderer info */
    const char *renderdriver;
    Uint32 render_flags;
    SDL_bool skip_renderer;
    SDL_Renderer *renderers;
    SDL_Texture *targets;

    /* GL settings */
    int gl_red_size;
    int gl_green_size;
    int gl_blue_size;
    int gl_alpha_size;
    int gl_buffer_size;
    int gl_depth_size;
    int gl_stencil_size;
    int gl_double_buffer;
    int gl_accum_red_size;
    int gl_accum_green_size;
    int gl_accum_blue_size;
    int gl_accum_alpha_size;
    int gl_retained_backing;
    int gl_accelerated;
    int gl_major_version;
    int gl_minor_version;
    int gl_debug;
    int gl_profile_mask;

} CommonState;

typedef struct GLES2_Context
{
    #define SDL_PROC(ret, func, params) ret(APIENTRY *func) params;
    #include "SDL_gles2funcs.h"
    #undef SDL_PROC
} GLES2_Context;

typedef struct shader_data
{
    GLuint render_program, shader_frag, shader_vert;
    GLuint compute_program, shader_compute;

    GLuint vPosition;    // 顶点的向量坐标
    GLuint fPosition;    //纹理的向量坐标
    GLuint sampler;      // sampler2D

    GLuint textureId;

    GLuint attractor_buffer;


    GLuint tbos[2];
    // GLuint position_tbo;
    // GLuint velocity_tbo;

} shader_data;



// Mass 吸引子的质量
float attractor_masses[MAX_ATTRACTORS];
float aspect_ratio = 1.0f;      // 窗口缩放比例

static float randomFloat()
{
    float res;
    unsigned int tmp;
    static unsigned int seed = 0xFFFF0C59;

    seed *= 16807;

    tmp = seed ^ (seed >> 4) ^ (seed << 15);

    *((unsigned int *)&res) = (tmp >> 9) | 0x3F800000;

    return (res - 1.0f);
}

static glm::vec3 random_vector(float minmag = 0.0f, float maxmag = 1.0f)
{
    glm::vec3 randomvec(randomFloat() * 2.0f - 1.0f, randomFloat() * 2.0f - 1.0f, randomFloat() * 2.0f - 1.0f);
    randomvec = normalize(randomvec);
    randomvec *= (randomFloat() * (maxmag - minmag) + minmag);

    return randomvec;
}



static CommonState *state;
static SDL_GLContext context;
static int depth = 24;
static GLES2_Context ctx;

static int LoadContext(GLES2_Context *data)
{
    #define SDL_PROC(ret, func, params)                                                            \
        do                                                                                         \
        {                                                                                          \
            data->func = reinterpret_cast<decltype(data->func)>(SDL_GL_GetProcAddress(#func));                                             \
            if (!data->func)                                                                       \
            {                                                                                      \
                return SDL_SetError("Couldn't load GLES2 function %s: %s", #func, SDL_GetError()); \
            }                                                                                      \
        } while (0);

    #include "SDL_gles2funcs.h"
    #undef SDL_PROC

    return 0;
}

void CommonQuit(CommonState *state)
{
    SDL_free(state->windows);


    if (state->targets)
    {
        SDL_DestroyTexture(state->targets);
        SDL_free(state->targets);
    }

    if (state->renderers)
    {
        SDL_DestroyRenderer(state->renderers);
        SDL_free(state->renderers);
    }

    SDL_free(state);
    SDL_Quit();
}

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
    if (context)
        SDL_GL_DeleteContext(context);

    CommonQuit(state);
    exit(rc);
}

#define GL_CHECK(x)                                                                         \
    x;                                                                                      \
    {                                                                                       \
        GLenum glError = ctx.glGetError();                                                  \
        if (glError != GL_NO_ERROR)                                                         \
        {                                                                                   \
            SDL_Log("glGetError() = %i (0x%.8x) at line %i\n", glError, glError, __LINE__); \
            quit(1);                                                                        \
        }                                                                                   \
    }



/*
 * Create shader, load in source, compile, dump debug as necessary.
 *
 * shader: Pointer to return created shader ID.
 * source: Passed-in shader source code.
 * shader_type: Passed to GL, e.g. GL_VERTEX_SHADER.
 */
static void process_shader(GLuint *shader, const char *source, GLint shader_type)
{
    GLint status = GL_FALSE;
    const char *shaders[1] = {NULL};
    char buffer[1024];
    GLsizei length = 0;

    /* Create shader and load into GL. */
    *shader = GL_CHECK(ctx.glCreateShader(shader_type));

    shaders[0] = source;

    GL_CHECK(ctx.glShaderSource(*shader, 1, shaders, NULL));

    /* Clean up shader source. */
    shaders[0] = NULL;

    /* Try compiling the shader. */
    GL_CHECK(ctx.glCompileShader(*shader));
    GL_CHECK(ctx.glGetShaderiv(*shader, GL_COMPILE_STATUS, &status));

    /* Dump debug info (source and log) if compilation failed. */
    if (status != GL_TRUE)
    {
        ctx.glGetShaderInfoLog(*shader, sizeof(buffer), &length, &buffer[0]);
        buffer[length] = '\0';
        SDL_Log("Shader compilation failed: %s", buffer);
        fflush(stderr);
        quit(-1);
    }
}

static void link_program(struct shader_data *data)
{
    GLint status = GL_FALSE;
    char buffer[1024];
    GLsizei length = 0;

    // link render shader
    GL_CHECK(ctx.glAttachShader(data->render_program, data->shader_vert));
    GL_CHECK(ctx.glAttachShader(data->render_program, data->shader_frag));
    GL_CHECK(ctx.glLinkProgram(data->render_program));
    GL_CHECK(ctx.glGetProgramiv(data->render_program, GL_LINK_STATUS, &status));

    if (status != GL_TRUE)
    {
        ctx.glGetProgramInfoLog(data->render_program, sizeof(buffer), &length, &buffer[0]);
        buffer[length] = '\0';
        SDL_Log("Program linking failed: %s", buffer);
        fflush(stderr);
        quit(-1);
    }


    // link compute shader
    GL_CHECK(ctx.glAttachShader(data->compute_program, data->shader_compute));
    GL_CHECK(ctx.glLinkProgram(data->compute_program));
    GL_CHECK(ctx.glGetProgramiv(data->compute_program, GL_LINK_STATUS, &status));

    if (status != GL_TRUE)
    {
        ctx.glGetProgramInfoLog(data->compute_program, sizeof(buffer), &length, &buffer[0]);
        buffer[length] = '\0';
        SDL_Log("Program linking failed: %s", buffer);
        fflush(stderr);
        quit(-1);
    }
}


static void Render(unsigned int width, unsigned int height, shader_data *data)
{
    aspect_ratio = (float)width / (float)height;
    //设置模型视图和投影矩阵
    // 实时改变 MVP 矩阵
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 1000.0f);
    glm::mat4 mvp = projection * glm::lookAt(glm::vec3(0, 0, 100.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1.0f, 0));
    GLuint t = ctx.glGetUniformLocation(data->render_program, "mvp");
    ctx.glUniformMatrix4fv(t, 1, GL_FALSE, glm::value_ptr(mvp));


    //执行计算着色器
    ctx.glUseProgram(data->compute_program);

    ctx.glBindBuffer(GL_UNIFORM_BUFFER, data->attractor_buffer);
    for (int i = 0; i < MAX_ATTRACTORS; i++)
    {
        attractor_masses[i] = 0.5f + randomFloat()* 0.5f;
    }

    ctx.glBindBufferBase(GL_UNIFORM_BUFFER, 0, data->attractor_buffer);
    //更新引力器位置和质量的缓存
    float time = clock()/float(CLOCKS_PER_SEC);
    glm::vec4* attractors = (glm::vec4*)ctx.glMapBufferRange(GL_UNIFORM_BUFFER, 0, 32 * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    for (int i = 0; i < 32; ++i) {
        attractors[i] = glm::vec4(sinf(time*float(i + 4)*7.5f*20.0f)*50.0f,
                                  cosf(time*float(i + 7)*3.9f*20.0f)*50.0f,
                                  sinf(time*float(i + 3)*5.3f*20.0f)*cosf(time*float(i + 5)*9.1f)*100.0f,
                                  attractor_masses[i]);
    }
    ctx.glUnmapBuffer(GL_UNIFORM_BUFFER);


    //设置时间间隔
    ctx.glUniform1f(ctx.glGetUniformLocation(data->compute_program, "dt"), 0.2f);
    ctx.glBindImageTexture(0, data->tbos[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);     // velocity_tbo
    ctx.glBindImageTexture(1, data->tbos[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);     // position_tbo
    ctx.glDispatchCompute(PARTICLE_COUNT, 1, 1);

    //确保计算着色器的写入操作已经完成
    ctx.glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //
    ctx.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ctx.glUseProgram(data->render_program);
    ctx.glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);

}

int done;
Uint32 frames;
shader_data data;

static void render_window()
{
    int w, h, status;

    if (!state->windows)
        return;

    status = SDL_GL_MakeCurrent(state->windows, context);
    if (status)
    {
        SDL_Log("SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        return;
    }

    SDL_GL_GetDrawableSize(state->windows, &w, &h);
    Render(w, h, &data);
    SDL_GL_SwapWindow(state->windows);


    ++frames;
}

static void loop()
{
    SDL_Event event;

    /* Check for events */
    while (SDL_PollEvent(&event) && !done)
    {
        switch (event.type)
        {
            case SDL_QUIT:
                done = 1;
                break;
        }
    }

    if (!done)
        render_window();
}

CommonState* CommonCreateState()
{
    CommonState* state;

    state = (CommonState*)SDL_calloc(1, sizeof(*state));
    if (!state)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Initialize some defaults */
    state->flags = SDL_INIT_VIDEO;
    state->window_title = "OpenGL ES2 Test";
    state->window_flags = 0;
    state->window_x = SDL_WINDOWPOS_UNDEFINED;
    state->window_y = SDL_WINDOWPOS_UNDEFINED;
    state->window_w = DEFAULT_WINDOW_WIDTH;
    state->window_h = DEFAULT_WINDOW_HEIGHT;

    /* Set some very sane GL defaults */
    state->gl_red_size = 8;
    state->gl_green_size = 8;
    state->gl_blue_size = 8;
    state->gl_alpha_size = 0;
    state->gl_buffer_size = 0;
    state->gl_depth_size = 24;
    state->gl_stencil_size = 0;
    state->gl_double_buffer = 1;
    state->gl_accum_red_size = 0;
    state->gl_accum_green_size = 0;
    state->gl_accum_blue_size = 0;
    state->gl_accum_alpha_size = 0;
    state->gl_retained_backing = 1;
    state->gl_accelerated = -1;
    state->gl_debug = 0;

    return state;
}

static SDL_Surface *LoadIcon(const char *file)
{
    SDL_Surface *icon;

    /* Load the icon surface */
    icon = SDL_LoadBMP(file);
    if (icon == NULL)
    {
        SDL_Log("Couldn't load %s: %s\n", file, SDL_GetError());
        return (NULL);
    }

    if (icon->format->palette)
    {
        /* Set the colorkey */
        SDL_SetColorKey(icon, 1, *((Uint8 *)icon->pixels));
    }

    return (icon);
}

static void snprintfcat(SDL_OUT_Z_CAP(maxlen) char *text, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)
{
    size_t length = SDL_strlen(text);
    va_list ap;

    va_start(ap, fmt);
    text += length;
    maxlen -= length;
    SDL_vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);
}

static void PrintRendererFlag(char *text, size_t maxlen, Uint32 flag)
{
    switch (flag)
    {
        case SDL_RENDERER_SOFTWARE:
            snprintfcat(text, maxlen, "Software");
            break;
        case SDL_RENDERER_ACCELERATED:
            snprintfcat(text, maxlen, "Accelerated");
            break;
        case SDL_RENDERER_PRESENTVSYNC:
            snprintfcat(text, maxlen, "PresentVSync");
            break;
        case SDL_RENDERER_TARGETTEXTURE:
            snprintfcat(text, maxlen, "TargetTexturesSupported");
            break;
        default:
            snprintfcat(text, maxlen, "0x%8.8x", flag);
            break;
    }
}

static void PrintRenderer(SDL_RendererInfo *info)
{
    int i, count;
    char text[1024];

    SDL_Log("  Renderer %s:\n", info->name);

    snprintf(text, sizeof(text), "    Flags: 0x%x8.8", info->flags);
    snprintfcat(text, sizeof(text), " (");

    count = 0;
    for (i = 0; i < sizeof(info->flags) * 8; ++i)
    {
        Uint32 flag = (1 << i);
        if (info->flags & flag)
        {
            if (count > 0)
                snprintfcat(text, sizeof(text), " | ");

            PrintRendererFlag(text, sizeof(text), flag);
            ++count;
        }
    }
    snprintfcat(text, sizeof(text), ")");
    SDL_Log("%s\n", text);

    SDL_snprintf(text, sizeof(text), "    Texture formats (%uld): ", info->num_texture_formats);
    for (i = 0; i < (int)info->num_texture_formats; ++i)
    {
        if (i > 0)
            snprintfcat(text, sizeof(text), ", ");
    }
    SDL_Log("%s\n", text);

    if (info->max_texture_width || info->max_texture_height)
        SDL_Log("    Max Texture Size: %dx%d\n", info->max_texture_width, info->max_texture_height);

}

SDL_bool CommonInit(CommonState *state)
{
    int i, j, m, n, w, h;
    SDL_DisplayMode fullscreen_mode;
    char text[1024];

    SDL_assert((state->flags & SDL_INIT_VIDEO) != 0);

    if (state->verbose & VERBOSE_VIDEO)
    {
        n = SDL_GetNumVideoDrivers();
        if (n == 0)
            SDL_Log("No built-in video drivers\n");
        else
        {
            SDL_Log("-------------------------------------------------------------------------\n");
            SDL_snprintf(text, sizeof(text), "Built-in video drivers:");
            for (i = 0; i < n; ++i)
            {
                if (i > 0)
                    snprintfcat(text, sizeof(text), ",");

                snprintfcat(text, sizeof(text), " %s", SDL_GetVideoDriver(i));
            }
            SDL_Log("%s\n", text);
        }
    }

    if (SDL_VideoInit(state->videodriver) < 0)
    {
        SDL_Log("Couldn't initialize video driver: %s\n", SDL_GetError());
        return SDL_FALSE;
    }

    if (state->verbose & VERBOSE_VIDEO)
        SDL_Log("Video driver: %s\n", SDL_GetCurrentVideoDriver());

    /* Upload GL settings */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, state->gl_red_size);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, state->gl_green_size);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, state->gl_blue_size);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, state->gl_alpha_size);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, state->gl_double_buffer);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, state->gl_buffer_size);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, state->gl_depth_size);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, state->gl_stencil_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, state->gl_accum_red_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, state->gl_accum_green_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, state->gl_accum_blue_size);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, state->gl_accum_alpha_size);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, state->gl_retained_backing);

    if (state->gl_major_version)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, state->gl_major_version);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, state->gl_minor_version);
    }

    if (state->gl_debug)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    if (state->gl_profile_mask)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, state->gl_profile_mask);

    if (state->verbose & VERBOSE_RENDER)
    {
        SDL_RendererInfo info;

        n = SDL_GetNumRenderDrivers();
        if (n == 0)
            SDL_Log("No built-in render drivers\n");
        else
        {
            SDL_Log("-------------------------------------------------------------------------\n");
            SDL_Log("Built-in render drivers:\n");
            for (i = 0; i < n; ++i)
            {
                SDL_GetRenderDriverInfo(i, &info);
                PrintRenderer(&info);
            }
        }
    }

    SDL_zero(fullscreen_mode);

    fullscreen_mode.format = SDL_PIXELFORMAT_RGB24;

    state->windows = (SDL_Window *)SDL_calloc(1, sizeof(state->windows));
    state->renderers = (SDL_Renderer *)SDL_calloc(1, sizeof(state->renderers));
    state->targets = (SDL_Texture *)SDL_calloc(1, sizeof(state->targets));

    if (!state->windows || !state->renderers)
    {
        SDL_Log("Out of memory!\n");
        return SDL_FALSE;
    }

    char title[1024];
    SDL_Rect r;

    r.x = state->window_x;
    r.y = state->window_y;
    r.w = state->window_w;
    r.h = state->window_h;

    SDL_strlcpy(title, state->window_title, SDL_arraysize(title));

    state->windows = SDL_CreateWindow(title, r.x, r.y, r.w, r.h, state->window_flags);
    if (!state->windows)
    {
        SDL_Log("Couldn't create window: %s\n", SDL_GetError());
        return SDL_FALSE;
    }

    if (state->window_minW || state->window_minH)
        SDL_SetWindowMinimumSize(state->windows, state->window_minW, state->window_minH);

    if (state->window_maxW || state->window_maxH)
        SDL_SetWindowMaximumSize(state->windows, state->window_maxW, state->window_maxH);

    SDL_GetWindowSize(state->windows, &w, &h);
    if (!(state->window_flags & SDL_WINDOW_RESIZABLE) && (w != state->window_w || h != state->window_h))
    {
        SDL_Log("Window requested size %dx%d, got %dx%d\n", state->window_w, state->window_h, w, h);
        state->window_w = w;
        state->window_h = h;
    }

    if (SDL_SetWindowDisplayMode(state->windows, &fullscreen_mode) < 0)
    {
        SDL_Log("Can't set up fullscreen display mode: %s\n", SDL_GetError());
        return SDL_FALSE;
    }

    if (state->window_icon)
    {
        SDL_Surface *icon = LoadIcon(state->window_icon);
        if (icon)
        {
            SDL_SetWindowIcon(state->windows, icon);
            SDL_FreeSurface(icon);
        }
    }

    SDL_ShowWindow(state->windows);

    if (!state->skip_renderer && (state->renderdriver || !(state->window_flags & SDL_WINDOW_OPENGL)))
    {
        m = -1;
        if (state->renderdriver)
        {
            SDL_RendererInfo info;
            n = SDL_GetNumRenderDrivers();

            for (j = 0; j < n; ++j)
            {
                SDL_GetRenderDriverInfo(j, &info);
                if (SDL_strcasecmp(info.name, state->renderdriver) == 0)
                {
                    m = j;
                    break;
                }
            }

            if (m == -1)
            {
                SDL_Log("Couldn't find render driver named %s", state->renderdriver);
                return SDL_FALSE;
            }
        }
        state->renderers = SDL_CreateRenderer(state->windows, m, state->render_flags);
        if (!state->renderers)
        {
            SDL_Log("Couldn't create renderer: %s\n", SDL_GetError());
            return SDL_FALSE;
        }

        if (state->verbose & VERBOSE_RENDER)
        {
            SDL_RendererInfo info;

            SDL_Log("-------------------------------------------------------------------------\n");
            SDL_Log("Current renderer:\n");
            SDL_GetRendererInfo(state->renderers, &info);
            PrintRenderer(&info);
            SDL_Log("-------------------------------------------------------------------------\n");
        }
    }

    return SDL_TRUE;
}




int main(int argc, char *argv[])
{
    int value;
    SDL_DisplayMode mode;
    Uint32 then, now;
    int status;

    /* Initialize test framework */
    state = CommonCreateState();
    if (!state)
        return 1;

    /* Set OpenGL parameters */
    state->window_flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    state->gl_red_size = 8;
    state->gl_green_size = 8;
    state->gl_blue_size = 8;
    state->gl_depth_size = depth;
    state->gl_major_version = 3;
    state->gl_minor_version = 2;
    state->gl_profile_mask = SDL_GL_CONTEXT_PROFILE_ES;
    state->verbose |= VERBOSE_VIDEO | VERBOSE_MODES | VERBOSE_RENDER;
    // state->window_icon = "/home/orangepi/Codes/SDL2-2.26.5/test/icon.bmp";
    state->videodriver = "x11";     // "wayland";
    state->renderdriver = "opengles2";
    

    if (!CommonInit(state))
    {
        quit(2);
        return 0;
    }

    /* Create OpenGL ES contexts */
    context = SDL_GL_CreateContext(state->windows);
    if (!context)
    {
        SDL_Log("SDL_GL_CreateContext(): %s\n", SDL_GetError());
        quit(2);
    }

    /* Important: call this *after* creating the context */
    if (LoadContext(&ctx) < 0)
    {
        SDL_Log("Could not load GLES2 functions\n");
        quit(2);
        return 0;
    }

    SDL_GL_SetSwapInterval(0);  // 禁用垂直同步

    SDL_GetCurrentDisplayMode(0, &mode);
    SDL_Log("Screen bpp: %d\n", SDL_BITSPERPIXEL(mode.format));
    SDL_Log("\n");
    SDL_Log("Vendor     : %s\n", ctx.glGetString(GL_VENDOR));
    SDL_Log("Renderer   : %s\n", ctx.glGetString(GL_RENDERER));
    SDL_Log("Version    : %s\n", ctx.glGetString(GL_VERSION));
    SDL_Log("Extensions : %s\n", ctx.glGetString(GL_EXTENSIONS));
    SDL_Log("\n");

    status = SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
    SDL_assert(!status);
    SDL_Log("SDL_GL_RED_SIZE: requested %d, got %d\n", 8, value);

    status = SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
    SDL_assert(!status);
    SDL_Log("SDL_GL_GREEN_SIZE: requested %d, got %d\n", 8, value);

    status = SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
    SDL_assert(!status);
    SDL_Log("SDL_GL_BLUE_SIZE: requested %d, got %d\n", 8, value);

    status = SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
    SDL_assert(!status);
    SDL_Log("SDL_GL_DEPTH_SIZE: requested %d, got %d\n", depth, value);

    int w, h;
    status = SDL_GL_MakeCurrent(state->windows, context);
    if (status)
    {
        SDL_Log("SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        exit(-1);
    }

    SDL_GL_GetDrawableSize(state->windows, &w, &h);
    ctx.glViewport(0, 0, w, h);     // 渲染的目标窗口区域，决定了渲染的内容在窗口中的位置和大小


    std::ifstream vertex_file("../src/vertex_particle.glsl");
    std::string vertex_source((std::istreambuf_iterator<char>(vertex_file)), std::istreambuf_iterator<char>());

    std::ifstream fragment_file("../src/fragment_particle.glsl");
    std::string fragment_source((std::istreambuf_iterator<char>(fragment_file)), std::istreambuf_iterator<char>());

    std::ifstream compute_file("../src/compute_particle.glsl");
    std::string compute_source((std::istreambuf_iterator<char>(compute_file)), std::istreambuf_iterator<char>());

    /* Create shader_program (ready to attach shaders) */
    data.render_program = GL_CHECK(ctx.glCreateProgram());

    /* Shader Initialization */
    process_shader(&data.shader_vert, vertex_source.c_str(), GL_VERTEX_SHADER);
    process_shader(&data.shader_frag, fragment_source.c_str(), GL_FRAGMENT_SHADER);
    process_shader(&data.shader_compute, compute_source.c_str(), GL_COMPUTE_SHADER);

    /* Attach shaders and link shader_program */
    link_program(&data);

    GL_CHECK(ctx.glUseProgram(data.render_program));

    ctx.glGenTextures(2, data.tbos);
    ctx.glBindTexture(GL_TEXTURE_2D, data.tbos[0]);  // position_tbo

    glm::vec4 *pos = new glm::vec4[PARTICLE_COUNT];
    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        pos[i] = glm::vec4(random_vector(-1.0f, 1.0f), randomFloat());
    }

    ctx.glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,PARTICLE_GROUP_COUNT,PARTICLE_GROUP_SIZE,0,GL_RGBA,GL_FLOAT,pos);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    delete[] pos;

    ctx.glBindTexture(GL_TEXTURE_2D,data.tbos[1]);  // velocity_tbo

    glm::vec4 *velocities = new glm::vec4[PARTICLE_COUNT];
    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        velocities[i] = glm::vec4(random_vector(-10.0f, 10.0f), 0.0f);
    }

    ctx.glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,PARTICLE_GROUP_COUNT,PARTICLE_GROUP_SIZE,0,GL_RGBA,GL_FLOAT,velocities);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    ctx.glGenBuffers(1, &data.attractor_buffer);
    ctx.glBindBuffer(GL_UNIFORM_BUFFER, data.attractor_buffer);
    ctx.glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);



    GL_CHECK(ctx.glEnable(GL_CULL_FACE));   // 启用面剔除功能，即OpenGL将不会渲染被遮挡的面
    GL_CHECK(ctx.glEnable(GL_DEPTH_TEST));  // 启用深度测试功能，即OpenGL将根据深度值确定哪些像素应该被渲染


    SDL_GL_MakeCurrent(state->windows, NULL);

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;

    while (!done)
        loop();

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then)
        SDL_Log("%2.2f frames per second\n", ((double)frames * 1000) / (now - then));

    return 0;
}
