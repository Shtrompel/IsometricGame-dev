#version 130

uniform sampler2D occluderTex;
uniform vec2 resolution;
uniform float uSeed;

#define MAX_LIGHTS 64
uniform int numLights;
uniform vec2 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

#define STEPS 65

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233)) + uSeed) * 43758.5453);
}

void main()
{
    vec2 fragPos = gl_FragCoord.xy;

    // Code made by Claude Sonnet 5 - neutral floor so pixels outside every tracked light's radius stay near ambient level instead of going pure black
    vec3 result = vec3(0.35);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (i >= numLights)
            break;

        vec2 lightPos = lightPositions[i];
        float dist = distance(fragPos, lightPos);
        if (dist > lightRadii[i])
            continue;

        float attenuation = pow(1.0 - (dist / lightRadii[i]), 2.0);

        float jitter = hash(fragPos + float(i) * 17.0);
        vec2 dir = lightPos - fragPos;
        float visibility = 1.0;

        for (int s = 0; s < STEPS; s++)
        {
            float t = (float(s) + jitter) / float(STEPS);
            vec2 samplePos = fragPos + dir * t;
            vec2 sampleUv = samplePos / resolution;
            sampleUv.y = 1.0 - sampleUv.y;

            if (sampleUv.x < 0.0 || sampleUv.x > 1.0 ||
                sampleUv.y < 0.0 || sampleUv.y > 1.0)
                continue;

            float occlusion = texture2D(occluderTex, sampleUv).r;
            if (occlusion > 0.5)
            {
                visibility = 0.0;
                break;
            }
        }

        result += lightColors[i] * attenuation * visibility;
    }

    gl_FragColor = vec4(min(result, vec3(1.0)), 1.0);
}
