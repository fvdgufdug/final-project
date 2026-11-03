#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LocalPos;
out vec4 FragColor;

uniform sampler2D objectTexture;
uniform vec3      objectColor;
uniform vec3      sunWorldPos;
uniform bool      useTexture;
uniform bool      useLighting;
uniform float     uvOffset;
uniform float     alpha;

const int MAX_IMPACTS = 32;
uniform int   impactCount;
uniform vec4  impacts[MAX_IMPACTS];
uniform float impactAges[MAX_IMPACTS];

uniform int   cloudMode;
uniform float cloudSeed;

float hash31(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash31(i + vec3(0.0, 0.0, 0.0)), hash31(i + vec3(1.0, 0.0, 0.0)), f.x),
                   mix(hash31(i + vec3(0.0, 1.0, 0.0)), hash31(i + vec3(1.0, 1.0, 0.0)), f.x), f.y),
               mix(mix(hash31(i + vec3(0.0, 0.0, 1.0)), hash31(i + vec3(1.0, 0.0, 1.0)), f.x),
                   mix(hash31(i + vec3(0.0, 1.0, 1.0)), hash31(i + vec3(1.0, 1.0, 1.0)), f.x), f.y), f.z);
}

float fbm(vec3 p)
{
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; ++i) { v += a * valueNoise(p); p *= 2.03; a *= 0.5; }
    return v;
}

void main()
{
    vec3  norm     = normalize(Normal);
    vec3  lightDir = normalize(sunWorldPos - FragPos);
    float diff     = max(dot(norm, lightDir), 0.0);

    if (cloudMode == 1)
    {
        float n     = fbm(normalize(LocalPos) * 3.2 + vec3(cloudSeed));
        float cover = smoothstep(0.50, 0.72, n);
        vec3  c     = vec3(1.0) * (0.12 + 1.15 * diff);
        FragColor   = vec4(c, cover * alpha * 0.9);
        return;
    }

    vec4  texSample = texture(objectTexture, vec2(TexCoord.x + uvOffset, TexCoord.y));
    vec3  color     = useTexture ? texSample.rgb : objectColor;
    float texA      = useTexture ? texSample.a : 1.0;

    vec3 glow = vec3(0.0);
    if (impactCount > 0)
    {
        vec3  p   = normalize(LocalPos);
        float wob = valueNoise(p * 14.0) - 0.5;
        for (int i = 0; i < impactCount; ++i)
        {
            float r     = impacts[i].w;
            float ang   = acos(clamp(dot(p, impacts[i].xyz), -1.0, 1.0)) + wob * r * 0.6;
            float age   = impactAges[i];
            float burn  = 1.0 - smoothstep(r * 0.55, r, ang);
            float rim   = smoothstep(r * 0.75, r, ang) * (1.0 - smoothstep(r, r * 1.25, ang));
            float lava  = (1.0 - smoothstep(0.0, r * 0.7, ang)) * exp(-age * 0.15);
            float flash = (1.0 - smoothstep(0.0, r * 1.3, ang)) * exp(-age * 5.0);
            color = mix(color, color * 0.10 + vec3(0.03, 0.015, 0.01), burn);
            color = mix(color, color * 0.55, rim * 0.6);
            glow += vec3(1.0, 0.35, 0.05) * lava * 1.4 + vec3(1.0, 0.9, 0.6) * flash * 1.5;
        }
    }

    if (!useLighting)
    {
        FragColor = vec4(color + glow, alpha * texA);
        return;
    }

    vec3 ambient  = 0.25 * color;
    vec3 diffuse  = 1.4 * diff * color;
    FragColor = vec4(ambient + diffuse + glow, alpha * texA);
}
