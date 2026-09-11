uniform sampler2D texture;

void main()
{
    // lookup the pixel in the texture
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

    pixel.rgb = vec3(1.0) - pixel.rgb;

    // multiply it by the color
    gl_FragColor = gl_Color * pixel;
}
