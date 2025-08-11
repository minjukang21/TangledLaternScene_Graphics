#version 330 core
layout(location=0) in vec3 aPos;

uniform mat4 model, view, projection;

out vec3 vWorldPos;
out vec3 vViewDir;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;

    mat4 invV = inverse(view);
    vec3 camPos = invV[3].xyz;
    vViewDir = normalize(camPos - vWorldPos);

    gl_Position = projection * view * wp;
}
