#version 310 es
precision mediump image2D;//OpenGL ES要求显示定义image2D的精度
layout (binding = 1,rgba32f) restrict uniform image2D position_buffer;
uniform mat4 mvp;
out float intensity;//粒子年龄值
void main() {
    ivec2 size=imageSize(position_buffer);
    vec4 pos=imageLoad(position_buffer,ivec2(gl_VertexID%size.y,gl_VertexID/size.y));//将一维坐标转换为图像二维坐标
    gl_Position=mvp*vec4(pos.xyz,1.0f);
    intensity=pos.w;
}
