#version 310 es
precision mediump image2D;//OpenGL ES要求显示定义image2D的精度
//uniform块中包含引力器的位置和质量
layout (std140,binding=0) uniform attractor_block{
    vec4 attractor[64];//xyz=position,w=mass
};
//每块中粒子的数量为128
layout (local_size_x=128) in;
//使用两个缓冲来包含粒子的位置和速度信息
layout (binding = 0,rgba32f) restrict uniform image2D velocity_buffer;
layout (binding = 1,rgba32f) restrict uniform image2D position_buffer;
//时间间隔
uniform float dt;
void main(void){
    //从缓存中读取当前的位置和速度
    ivec2 size=imageSize(velocity_buffer);
    ivec2 p=ivec2(int(gl_GlobalInvocationID.x)/size.y,int(gl_GlobalInvocationID.x)%size.y);
    vec4 vel=imageLoad(velocity_buffer,p);
    vec4 pos=imageLoad(position_buffer,p);
    int i;
    //使用当前速度x时间来更新位置
    pos.xyz += vel.xyz * dt;
    pos.w -= 0.0001 * dt;
    //对于每个引力器
    for (i = 0; i < 4; i++)
    {
        //计算受力并更新速度
        vec3 dist = (attractor[i].xyz - pos.xyz);
        vel.xyz += dt * dt * attractor[i].w * normalize(dist) / (dot(dist, dist) + 10.0);
    }
    //如果粒子已经过期，那么重置它
    if (pos.w <= 0.0)
    {
        pos.xyz = vec3(0.0f);
        vel.xyz *= 0.01;
        pos.w += 1.0f;
    }

    //将新的位置和速度信息重新保存在缓存中
    imageStore(position_buffer,p,pos);
    imageStore(velocity_buffer,p,vel);
}
