#version 130

uniform sampler2D maskTex;
uniform vec2 resolution;
uniform vec2 lightPos;
uniform vec3 lightColor;
uniform float lightRadius;

void main()
{
    vec2 fragPos = gl_FragCoord.xy;
    float dist = distance(fragPos, lightPos);
    if (dist > lightRadius)
        discard;

    float attenuation = pow(1.0 - (dist / lightRadius), 2.0);
    float visibility = texture2D(maskTex, fragPos / resolution).r;

    gl_FragColor = vec4(lightColor * attenuation * visibility, 1.0);
}
