attribute vec4 av_Position; //定义顶点坐标 向量vec4  代表x(横坐标) y(纵坐标) z(z坐标) w(焦距)
attribute vec2 af_Position; //定义纹理坐标 向量vec2
varying vec2 v_texPosition;

void main() 
{
    v_texPosition = af_Position;
    gl_Position = av_Position;  //gl_Position是OpenGL中提供的变量，装载顶点坐标
}
//attribute 只能在Vertex Shader(顶点着色器)中使用, attribute表示一些顶点的数据，这些顶点数据包含了顶点坐标，法线，纹理坐标，顶点颜色等.
//attribute 变量来表示一些顶点的数据，如：顶点坐标，法线，纹理坐标，顶点颜色等。
//varying[易变量] 用于Vertex Shader(顶点着色器)和Fragment Shader(纹理着色器)之间传递值