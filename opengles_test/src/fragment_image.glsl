precision mediump float;
varying vec2 v_texPosition;
uniform sampler2D sTexture;
void main() {
    gl_FragColor=texture2D(sTexture, v_texPosition);
}
//uniform 变量一般用来表示：变换矩阵，材质，光照参数和颜色等信息。它可以在vertex和fragment共享使用。（相当于一个被vertex和fragment shader共享的全局变量）