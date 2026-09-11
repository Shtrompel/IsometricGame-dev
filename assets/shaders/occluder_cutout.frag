#version 130

uniform sampler2D u_texture;

void main()
{
    vec4 c = texture2D(u_texture, gl_TexCoord[0].xy);
    if (c.a < 0.5)
        discard;
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
