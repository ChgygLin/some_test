#version 310 es
precision mediump float;
out vec4 color;
//这个值来自顶点着色器中读取的粒子年龄值
in float intensity;
void main(void){
    //根据粒子的年龄，在热红色到冷蓝色之间的混合结果
    color=mix(vec4(0.0f,0.2f,1.0f,1.0f),vec4(0.2f,0.05f,0.0f,1.0f),intensity);
}
