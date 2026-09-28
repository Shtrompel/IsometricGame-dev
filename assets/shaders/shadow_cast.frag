#version 130

uniform sampler2D u_texture;

void main()
{
    float a = texture2D(u_texture, gl_TexCoord[0].xy).a;
    gl_FragColor = vec4(0.0, 0.0, 0.0, a * gl_Color.a);
}
