
#version 130


#define UV_COORD gl_TexCoord[0].xy

// You must still rename this because "texture" is a reserved keyword in GLSL 1.30+
uniform sampler2D u_texture; 
uniform vec2 texOffset;

#define MAX_LIGHTS 16
uniform int numLights;
uniform vec2 lightPositions[MAX_LIGHTS]; 
uniform vec3 lightColors[MAX_LIGHTS];    
uniform float lightRadii[MAX_LIGHTS];    

const vec3 ambientColor = vec3(0.2, 0.2, 0.3);
const vec3 luma = vec3(0.299, 0.587, 0.114);



void main()
{
    // Uses the macro to select the correct UV source based on the platform
    vec2 uv = UV_COORD;
    vec4 baseColor = texture2D(u_texture, uv);
    
    if (baseColor.a < 0.1) 
        discard;

    float center = dot(baseColor.rgb, luma);
    float right  = dot(texture2D(
        u_texture, 
        uv + vec2(texOffset.x, 0.0)).rgb, luma);
    float top    = dot(texture2D(
        u_texture, 
        uv + vec2(0.0, texOffset.y)).rgb, luma);
    
    // Normal direction
    vec3 N = normalize(vec3(center - right, center - top, 0.5));

    vec3 V = vec3(0.0, 0.0, 1.0); 
    vec3 finalLight = ambientColor;

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (i >= numLights) 
            break;

        float d = distance(lightPositions[i], gl_FragCoord.xy);
        if (d > lightRadii[i]) 
            continue;
        
        float attenuation = pow(1.0 - (d / lightRadii[i]), 2.0);

        vec3 lightVec3D = vec3(lightPositions[i] - gl_FragCoord.xy, 50.0);
        vec3 L = normalize(lightVec3D);
        vec3 H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);
        float spec = pow(max(dot(N, H), 0.0), 16.0);

        finalLight += (diff + spec) * lightColors[i] * attenuation;
    }

    gl_FragColor = vec4(baseColor.rgb * finalLight, baseColor.a);
}