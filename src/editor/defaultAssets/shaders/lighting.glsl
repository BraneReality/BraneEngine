#ifndef LIGHTING_FRAG
#define LIGHTING_FRAG

struct PointLight{
    vec3 position;
    vec4 color;
};

layout(binding = 2) buffer PointLightBuffer{
    uint numLights;
    PointLight[] lights;
} plb;

vec3 gammaCorrect(vec3 color)
{
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0/2.2));
}

vec3 shadeDiffuse(vec3 color, vec3 position, vec3 normal)
{
    vec3 lightHue = vec3(0,0,0);
    for(uint i = 0; i < plb.numLights; ++i)
    {
        float sqrdist = distance(plb.lights[i].position, position);
        sqrdist *= sqrdist;
        float b = max(0, dot(normalize(plb.lights[i].position - position), normal) * max(0, plb.lights[i].color.a * (1 / sqrdist)));
        lightHue += plb.lights[i].color.xyz * b;
    }
    color *= lightHue;

    return color;
}

#ifndef PI
const float PI = 3.14159265359;
#endif

float DistributionGGX(vec3 normal, vec3 halfVec, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(normal, halfVec), 0.0);
    float NdotH2 = NdotH*NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float dotp, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    return dotp / (dotp * (1.0 - k) + k + 0.0001);
}
float GeometrySmith(vec3 normal, vec3 viewVec, vec3 lightVec, float roughness)
{
    float viewDot = max(dot(normal, viewVec), 0.0);
    float lightDot = max(dot(normal, lightVec), 0.0);
    float ggx2  = GeometrySchlickGGX(viewDot, roughness);
    float ggx1  = GeometrySchlickGGX(lightDot, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 shadePBR(vec3 albedo, vec3 position, vec3 cameraPos, vec3 normal, float roughness, float metallic)
{
    vec3 viewVec = normalize(cameraPos - position);
    const vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 lightHue = vec3(0);
    for(uint i = 0; i < plb.numLights; ++i)
    {
        // radiance
        vec3  lightVec    = normalize(plb.lights[i].position - position);
        vec3  halfVec     = normalize(viewVec + lightVec);
        float d   = length(plb.lights[i].position - position);
        float attenuation = 1.0 / (d * d);
        vec3  radiance    = plb.lights[i].color.xyz * max(plb.lights[i].color.a, 0) * attenuation;

        // brdf
        float NDF = DistributionGGX(normal, halfVec, roughness);
        float G   = GeometrySmith(normal, viewVec, lightVec, roughness);
        vec3  F   = fresnelSchlick(max(dot(halfVec, viewVec), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD     *= 1.0 - metallic;

        float viewDot  = max(dot(normal, viewVec), 0.0);
        float lightDot = max(dot(normal, lightVec), 0.0);

        vec3 specular = (NDF * G * F) / (4.0 * viewDot * lightDot + 0.00001);

        // add to hue
        lightHue += (kD * albedo / PI + specular) * radiance * lightDot;

    }

    vec3 ambient = vec3(0.03) * albedo * (1.0 - metallic);
    vec3 color = ambient + lightHue;

    return color;
}

#endif
