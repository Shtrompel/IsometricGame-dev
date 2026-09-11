#version 130

uniform sampler2D newSample;
uniform sampler2D prevAccum;
uniform float blendFactor;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec3 a = texture2D(prevAccum, uv).rgb;
    vec3 b = texture2D(newSample, uv).rgb;
    gl_FragColor = vec4(mix(a, b, blendFactor), 1.0);
}
