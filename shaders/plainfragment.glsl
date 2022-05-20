#version 400
uniform sampler2D texture;
varying vec4 texc;
uniform int texturePresent = 1;

void main(void)
{
    if (texturePresent==0) {gl_FragColor=vec4(0.2f,0.2f,0.2f,1); return;}
    int p=int(texc.x*32*10);
    if (p%10==0) {gl_FragColor=vec4(0.2f,0.2f,0.2f,1); return;}
   // p=int(texc.y*32*10);
   // if (p%10==0) {gl_FragColor=vec4(0.2f,0.2f,0.2f,1); return;}
    gl_FragColor = texture2D(texture, vec2(texc.x,texc.y));
}
