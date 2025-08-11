#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;

uniform bool  useTexture;
uniform sampler2D diffuseTex;
uniform vec3  baseColor;

uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform int numLanterns;
uniform PointLight lanterns[64];

uniform bool isLantern;
uniform vec3 lanternTint;
uniform float lanternEmissive;

uniform samplerCube pointShadowMap;
uniform float shadowFarPlane;
uniform vec3  pointLightPos;
uniform int   shadowedIndex;

float pointShadow(vec3 fragPos) {
    vec3  L = fragPos - pointLightPos;
    float current = length(L);
    float closest = texture(pointShadowMap, L).r * shadowFarPlane;

    float bias = 0.02;
    return (current - bias > closest) ? 1.0 : 0.0;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 albedo = useTexture ? texture(diffuseTex, TexCoords).rgb : baseColor;

    vec3 Ld = normalize(-dirLightDir);
    float ndotl = max(dot(N, Ld), 0.0);
    vec3 dirDiffuse = ndotl * dirLightColor * albedo;

    vec3 H = normalize(Ld + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    vec3 dirSpec = spec * dirLightColor * 0.25;

    vec3 pts = vec3(0.0);
    for (int i = 0; i < numLanterns; ++i) {
        vec3 L = lanterns[i].position - FragPos;
        float dist = length(L);
        L = L / max(dist, 1e-4);

        float atten = 1.0 / (lanterns[i].constant +
                            lanterns[i].linear * dist +
                            lanterns[i].quadratic * dist * dist);

        float d  = max(dot(N, L), 0.0);
        vec3  H2 = normalize(L + V);
        float s2 = pow(max(dot(N, H2), 0.0), 32.0);

        float sh = (i == shadowedIndex) ? pointShadow(FragPos) : 0.0;

        vec3 contrib = atten * (d * lanterns[i].color * albedo + 0.25 * s2 * lanterns[i].color);
        pts += (1.0 - sh) * contrib;
    }

    vec3 ambient = 0.12 * albedo;
    vec3 emissive = (isLantern ? lanternTint * lanternEmissive : vec3(0.0));
    vec3 color = ambient + dirDiffuse + dirSpec + pts + emissive;
    FragColor = vec4(color, 1.0);
}