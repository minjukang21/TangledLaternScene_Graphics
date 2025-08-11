#version 330 core
in vec3 vWorldPos;
in vec3 vViewDir;
out vec4 FragColor;

uniform samplerCube skybox;
uniform samplerCube pointShadowMap;
uniform float shadowFarPlane;
uniform vec3 pointLightPos;

uniform vec3 waterColor;
uniform float alphaBase;
uniform float eta;
uniform float reflectBoost;
uniform float absorb;
uniform float time;

vec3 fakeNormal(vec3 p) {
    float n = 0.0;
    n += sin(p.x*0.6 + time*0.7) * 0.5;
    n += cos(p.z*0.8 + time*0.5) * 0.5;
    n *= 0.035;
    return normalize(vec3(-n, 1.0, -n));
}

float pointShadow(vec3 fragPos) {
    vec3  L = fragPos - pointLightPos;
    float current = length(L);
    float closest = texture(pointShadowMap, L).r * shadowFarPlane;
    float bias = 0.02; // tune 0.005–0.03
    return (current - bias > closest) ? 1.0 : 0.0; // 1=in shadow
}

void main() {
    vec3 N = fakeNormal(vWorldPos);
    vec3 V = normalize(vViewDir);

    float F0 = 0.02;
    float VoN = clamp(dot(V, N), 0.0, 1.0);
    float fres = F0 + (1.0 - F0)*pow(1.0 - VoN, 5.0);
    fres = clamp(fres * (1.0 + reflectBoost), 0.0, 1.0);

    vec3 R = reflect(-V, N);
    vec3 T = refract(-V, N, 1.0/eta);

    vec3 refl = texture(skybox, R).rgb;
    vec3 refr = texture(skybox, T).rgb * waterColor;

    refr *= exp(-absorb * 0.5);

    float sh = pointShadow(vWorldPos);     // 0 lit, 1 shadow
    refr *= (1.0 - 0.8 * sh);              // dim refracted (direct-lit-ish) path in shadow

    vec3 color = mix(refr, refl, fres);
    float alpha = alphaBase * (0.65 + 0.35 * fres);

    FragColor = vec4(color, alpha);
}