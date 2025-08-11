#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <limits.h>
#include <random>
#include <fstream>
#include <string>
#include <array>
#include <algorithm>


void processInput(GLFWwindow* window);
unsigned int loadCubemap(const std::vector<std::string>& faces);
void renderSkybox(unsigned int skyboxVAO,
                  Shader& skyboxShader,
                  unsigned int cubemapTexture,
                  const glm::mat4& view,
                  const glm::mat4& projection);

unsigned int loadTexture2D(const std::string& path) {
    int w=0, h=0, n=0;
    std::printf("[TEX] loading '%s'\n", path.c_str());
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, STBI_rgb_alpha);
    if (!data) {
        std::printf("[TEX] stbi_load FAILED for '%s'\n", path.c_str());
        return 0;
    }
    std::printf("[TEX] loaded '%s' w=%d h=%d orig_ch=%d (now RGBA)\n", path.c_str(), w, h, n);

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    if (!tex) { std::puts("[TEX] glGenTextures returned 0"); stbi_image_free(data); return 0; }

    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::printf("[TEX] glTexImage2D error=0x%X\n", err);
        stbi_image_free(data);
        return 0;
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    std::printf("[TEX] OK '%s' -> id=%u\n", path.c_str(), tex);
    return tex;
}

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
Camera camera(glm::vec3(0.0f, 1.5f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 boatPosition(10.0f, 0.0f, 70.0f);
float boatRotation = 0.0f;
std::vector<glm::vec3> lanternPositions;
bool perspectiveBoat = true; //true = in-boat, false = aerial
glm::vec3 castleLanternOrigin = glm::vec3(65.0f, 12.0f, -18.0f);

bool gSpawnOverride = false;
glm::vec3 gSpawnOverridePos(0.0f);


static float cockpitYawOff   = 0.0f; //left/right peek
static float cockpitPitchOff = 0.0f; //up/down peek

//limits in the boat for mouse controlled perspective
static const float COCKPIT_YAW_LIMIT = glm::radians(5.0f); //looking around
static const float COCKPIT_PITCH_MIN = glm::radians(-8.0f);
static const float COCKPIT_PITCH_MAX = glm::radians(35.0f);

//mouse sensitivity in the boat
static const float COCKPIT_SENS_X = 0.00125f; //yaw
static const float COCKPIT_SENS_Y = 0.0020f; //pitch

float gFlowerOrbitRadius = 3.0f; //distance from boat
float gFlowerOrbitSpeed = 0.40f; //rad/s (orbit around boat)
float gFlowerSpinSpeed = 1.00f; //rad/s (self spin)
float gFlowerTilt = glm::radians(0.0f);
float gFlowerScale = 0.2f; 
glm::vec3 gFlowerTint = glm::vec3(1.0f, 0.92f, 0.55f);
float gFlowerBobAmp = 0.15f;
float gFlowerBobSpeed = 0.7f;
float gFlowerPulseAmp = 0.35f;
float gFlowerPulseSpeed = 0.8f;

inline glm::vec3 BoatForward() {
    return glm::vec3(sin(boatRotation), 0.0f, -cos(boatRotation));
}

struct Lantern {
    glm::vec3 pos;
    glm::vec3 vel;
    float t;
    float phase;
};
std::vector<Lantern> lanterns;
bool keyCPressed = false;

float frand(float a, float b) {
    return a + (b - a) * (float)rand() / (float)RAND_MAX;
}

inline void SpawnLantern() {
    glm::vec3 fwd = glm::vec3(sin(boatRotation), 0.0f, -cos(boatRotation));

    static std::mt19937 rng{12345};
    std::uniform_real_distribution<float> r01(0.f,1.f);

    glm::vec3 spawnPos = gSpawnOverride ? gSpawnOverridePos : castleLanternOrigin;

    Lantern L;
    L.pos = boatPosition + glm::vec3(0.0f, 1.25f, 0.2f);
    L.vel = glm::vec3(0.0f, 0.7f, 0.0f) + fwd * 0.10f + glm::vec3(r01(rng)-0.5f, 0.0f, r01(rng)-0.5f) * 0.05f; // tiny sideways drift
    L.t = 0.0f;
    L.phase = r01(rng) * 100.0f; //phase for each
    lanterns.push_back(L);
}

void SpawnLanternAtPos(const glm::vec3& pos) {
    Lantern L;
    L.pos = pos;

    glm::vec3 baseV(0.0f, 0.01f, 0.0f);

    //horizontal movement i.e. spreading out
    glm::vec3 delta = pos - castleLanternOrigin;
    glm::vec2 d2(delta.x, delta.z);
    float r = glm::length(d2);
    glm::vec3 dir = (r > 1e-4f) ? glm::normalize(glm::vec3(d2.x, 0.0f, d2.y))
                                : glm::vec3(1,0,0); // fallback

    //tune k with spawn radius r
    float k = 0.7f; // try 0.4–1.0
    glm::vec3 radialKick = dir * (k * r);

    //small random swirl
    glm::vec3 swirl(frand(-0.1f,0.1f), 0.0f, frand(-0.1f,0.1f));

    L.vel = baseV + radialKick + swirl;
    L.t = 0.0f;
    L.phase = frand(0.0f, 100.0f);
    lanterns.push_back(L);
}

inline void SpawnLanternBurstFromCastle(int n = 50) {
    for (int i = 0; i < n; ++i) {
        glm::vec3 jitter(frand(-0.5f, 0.5f), frand(0.0f, 0.4f), frand(-0.5f, 0.5f));
        SpawnLanternAtPos(castleLanternOrigin + jitter);
    }
}


struct PointShadow {
    GLuint fbo = 0;
    GLuint cube = 0;
    int size = 1024;
    float nearP = 0.1f;
    float farP  = 150.0f;
    glm::vec3 lightPos{0,5,0};

    void init() {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &cube);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
        for (int i=0;i<6;++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_DEPTH_COMPONENT,
                        size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cube, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};


std::array<glm::mat4,6> ShadowMatrices(const PointShadow& s) {
    float aspect = 1.0f;
    glm::mat4 P = glm::perspective(glm::radians(90.0f), aspect, s.nearP, s.farP);
    glm::vec3 L = s.lightPos;
    return {
        P * glm::lookAt(L, L + glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        P * glm::lookAt(L, L + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        P * glm::lookAt(L, L + glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        P * glm::lookAt(L, L + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        P * glm::lookAt(L, L + glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        P * glm::lookAt(L, L + glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
    };
}

static int pickShadowLight(const std::vector<Lantern>& Ls, const glm::vec3& target) {
    int best = -1;
    float bestD2 = std::numeric_limits<float>::infinity();
    for (int i = 0; i < (int)Ls.size(); ++i) {
        float d2 = glm::dot(Ls[i].pos - target, Ls[i].pos - target); // no sqrt
        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }
    return best;
}

int main() {
    std::puts("ENTER MAIN"); std::fflush(stdout);
    std::cout << "== Boat-only debug build ==\n";

    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::puts("S1 before create window");

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Boat Debug", nullptr, nullptr);
    if (!window) { 
        std::cerr << "Window create failed\n"; glfwTerminate(); return -1; 
    }
    

    std::puts("S2 before make context");
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, [](GLFWwindow*, double xpos, double ypos){
        if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

        float xoffset = float(xpos - lastX);
        float yoffset = float(lastY - ypos);
        lastX = xpos; lastY = ypos;

        if (perspectiveBoat) {
            cockpitYawOff += xoffset * COCKPIT_SENS_X;
            cockpitPitchOff += yoffset * COCKPIT_SENS_Y;

            cockpitYawOff   = glm::clamp(cockpitYawOff,  -COCKPIT_YAW_LIMIT, COCKPIT_YAW_LIMIT);
            cockpitPitchOff = glm::clamp(cockpitPitchOff, COCKPIT_PITCH_MIN, COCKPIT_PITCH_MAX);
        } else {
            camera.ProcessMouseMovement(xoffset, yoffset);
        }
    });

    static const int MAX_SHADOW_CASTERS = 5;
    PointShadow gPointShadows[MAX_SHADOW_CASTERS];

    std::puts("S3 before glewInit");
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n"; return -1; }

    std::puts("S4 after GL enables");
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);

    //loading shaders
    std::puts("S5 before shaders");
    Shader lit("shaders/lighting.vert", "shaders/lighting.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader water("shaders/water.vert", "shaders/water.frag");
    for (int i=0;i<MAX_SHADOW_CASTERS;++i) 
        gPointShadows[i].init();
    auto& sh = gPointShadows[0]; 
    Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");


    std::puts("S6 before textures");
    unsigned int boatTex = loadTexture2D("assets/textures/boat_diffuse.png");
    unsigned int lanternTex = loadTexture2D("assets/textures/emblem.jpg");
    unsigned int dirtTex = loadTexture2D("assets/textures/dirtTex.jpg");
    unsigned int grassTex = loadTexture2D("assets/textures/grassTex.jpg");
    unsigned int flowerTex = loadTexture2D("assets/textures/flowerTex.png");
    
    std::printf("S6a textures boat=%u lantern=%u\n", boatTex, lanternTex);

    std::puts("S7 before models");
    std::cout << "Loading model: assets/models/boat.obj\n";
    Model boat("assets/models/boat.obj");
    std::cout << "Loading model: assets/models/lantern.obj\n";
    Model lantern("assets/models/lantern.obj");
    std::cout << "Loading model: assets/models/castle.obj\n";
    Model castle("assets/models/castle.obj");
    std::cout << "Loading model: assets/models/island.obj\n";
    Model island("assets/models/island.obj");
    std::cout << "Loading model: assets/models/flower.obj\n";
    Model flower("assets/models/flower.obj");
    std::puts("S7a after models");

    glm::vec3 islandPos = glm::vec3(65.0f, 0.0f, -30.0f);
    glm::vec3 castlePos = glm::vec3(65.0f, 1.4f, -19.0f);
    float castleScale = 0.32f;
    float islandScale = 2.0f;

    //loading camera
    float boatYaw = 0.0f;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                      (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                      0.05f, 200.0f);

    //water mesh
    unsigned int waterVAO=0, waterVBO=0, waterEBO=0;
    {
        float verts[] = {
            -1.0f, 0.0f, -1.0f,
            1.0f, 0.0f, -1.0f,
            1.0f, 0.0f,  1.0f,
            -1.0f, 0.0f,  1.0f
        };
        unsigned int idx[] = { 0,1,2,  0,2,3 };

        glGenVertexArrays(1, &waterVAO);
        glGenBuffers(1, &waterVBO);
        glGenBuffers(1, &waterEBO);

        glBindVertexArray(waterVAO);
        glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    //skybox VAO
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    std::puts("S8 before skybox VAO");
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    std::puts("S8a after skybox VAO");

    std::vector<std::string> faces = {
        "assets/skybox/right.png",
        "assets/skybox/left.png",
        "assets/skybox/top.png",
        "assets/skybox/bottom.png",
        "assets/skybox/front.png",
        "assets/skybox/back.png"
    };

    std::puts("S9 before loadCubemap");
    unsigned int cubemapTexture = loadCubemap(faces);
    std::puts("S10 before skybox uniform");
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    GLint linked = 0;
    glGetProgramiv(skyboxShader.ID, GL_LINK_STATUS, &linked);


    //check timer for dt
    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 fwd = glm::vec3(sin(boatYaw), 0.0f, -cos(boatYaw));
        glm::vec3 eye = boatPosition + glm::vec3(0, 2.0f, 0) - fwd * 6.0f;
        glm::mat4 view = glm::lookAt(eye, boatPosition + glm::vec3(0,1,0), glm::vec3(0,1,0));

        float now = glfwGetTime();
        deltaTime = now - lastTime; //dt
        lastTime = now;
        processInput(window);

        if (perspectiveBoat) {
            //eye anchored in the boat
            glm::vec3 baseFwd = glm::vec3(sin(boatRotation), 0.0f, -cos(boatRotation));
            float camUp = 1.25f;
            float camBack = 0.35f;
            eye = boatPosition + glm::vec3(0, camUp, 0) - baseFwd * camBack;

            //direction from boat yaw, offset yaw, offset pitch
            float yaw = boatRotation + cockpitYawOff;
            float pitch = cockpitPitchOff;

            float cy = cosf(yaw),  sy = sinf(yaw);
            float cp = cosf(pitch), sp = sinf(pitch);

            glm::vec3 dir(cp * sy, sp, -cp * cy ); //(x=cp*sin, y=sp, z=-cp*cos)
            glm::vec3 center = eye + glm::normalize(dir);

            view = glm::lookAt(eye, center, glm::vec3(0,1,0));
        } else {
            eye  = camera.Position;
            view = camera.GetViewMatrix();
        }

        for (auto& L : lanterns) {
            L.t += deltaTime;
            L.vel.y += 0.01f * deltaTime;
            float w = L.phase + L.t;
            glm::vec3 wind(
                0.15f * sin(0.6f*w) + 0.08f * cos(1.1f*w),
                0.0f,
                0.15f * cos(0.5f*w) + 0.06f * sin(1.3f*w)
            );
            L.vel += wind * 0.15f * deltaTime;
            L.vel *= 0.9985f;
            L.pos += L.vel * deltaTime;
        }

        lanterns.erase(std::remove_if(lanterns.begin(), lanterns.end(),
            [&](const Lantern& L){
                if (L.t > 40.0f) return true;
                if (L.pos.y > 60.0f) return true;
                if (glm::length(L.pos - boatPosition) > 200.0f) return true;
                return false;
            }), lanterns.end());

        //declaring the models
        glm::mat4 I = glm::translate(glm::mat4(1), islandPos);
        I = glm::scale(I, glm::vec3(islandScale * 2, 0.7 * islandScale, islandScale * 2));

        glm::mat4 C = glm::translate(glm::mat4(1.f), castlePos);
        C = glm::scale(C, glm::vec3(castleScale));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), boatPosition);
        model = glm::rotate(model, boatRotation, glm::vec3(0,1,0));
        model = glm::scale(model, glm::vec3(0.3f));

        glm::mat4 Boat = glm::translate(glm::mat4(1.f), boatPosition) * glm::rotate(glm::mat4(1.f), 
                         boatRotation, 
                         glm::vec3(0,1,0));

        now = static_cast<float>(glfwGetTime());

        //angles for the flower
        float orbitAngle = now * gFlowerOrbitSpeed;
        float selfSpin = now * gFlowerSpinSpeed;

        //orbit position + gentle vertical bob
        float yBob = gFlowerBobAmp * std::sin(now * gFlowerBobSpeed);
        glm::vec3 orbitPos(
            gFlowerOrbitRadius * std::cos(orbitAngle), yBob, gFlowerOrbitRadius * std::sin(orbitAngle));

        //hierarchy: Boat • R(tilt) • T(orbit) • R(self) • S
        glm::mat4 M = Boat
            * glm::rotate(glm::mat4(1.f), gFlowerTilt, glm::vec3(1,0,0))
            * glm::translate(glm::mat4(1.f), orbitPos)
            * glm::rotate(glm::mat4(1.f), selfSpin, glm::vec3(0,1,0))
            * glm::scale(glm::mat4(1.f), glm::vec3(gFlowerScale));

        std::vector<glm::mat4> Lmats;
        Lmats.reserve(lanterns.size());
        for (const auto& L : lanterns) {
            Lmats.push_back(glm::translate(glm::mat4(1.f), L.pos)
                        * glm::scale(glm::mat4(1.f), glm::vec3(0.06f)));
        }

        //shadow
        int idx = pickShadowLight(lanterns, boatPosition);
        if (idx >= 0) {
            sh.lightPos = lanterns[idx].pos + glm::vec3(0.0f, 0.2f, 0.0f);
        } else {
            sh.lightPos = glm::vec3(65.0f, 12.0f, -19.0f);
        }

        glViewport(0, 0, sh.size, sh.size);
        glBindFramebuffer(GL_FRAMEBUFFER, sh.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        shadowShader.use();
        auto mats = ShadowMatrices(sh);
        for (int i = 0; i < 6; ++i)
            shadowShader.setMat4(("shadowMatrices[" + std::to_string(i) + "]").c_str(), mats[i]);
        shadowShader.setVec3("lightPos", sh.lightPos);
        shadowShader.setFloat("farPlane", sh.farP);

        shadowShader.setMat4("model", C);       
        castle.Draw(shadowShader);
        shadowShader.setMat4("model", I);       
        island.Draw(shadowShader);
        shadowShader.setMat4("model", model);  
        boat.Draw(shadowShader);
        for (int i = 0; i < (int)Lmats.size(); ++i) {
            if (i == idx) continue;
            shadowShader.setMat4("model", Lmats[i]);
            lantern.Draw(shadowShader);
        }
        shadowShader.setMat4("model", M);
        flower.Draw(shadowShader);

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        int w=0,h=0; glfwGetFramebufferSize(window,&w,&h);
        glViewport(0,0,w,h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE); 

        //bind depth cube for lighting
        glActiveTexture(GL_TEXTURE0 + 5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sh.cube);
        
        water.use();
        water.setInt("pointShadowMap", 5);
        water.setFloat("shadowFarPlane", sh.farP);
        water.setVec3("pointLightPos",  sh.lightPos);
        
        lit.use();
        lit.setMat4("projection", proj);  
        lit.setMat4("view",      view);    
        lit.setVec3("viewPos",   eye); 

        lit.setInt("pointShadowMap", 5);
        lit.setFloat("shadowFarPlane", sh.farP);
        lit.setInt("shadowedIndex", idx);  
        lit.setVec3("pointLightPos", sh.lightPos);

        //dirlight
        lit.setVec3("dirLightDir", glm::normalize(glm::vec3(-0.2f, -1.0f, -0.1f)));
        lit.setVec3("dirLightColor", glm::vec3(0.55f, 0.50f, 0.65f));
        lit.setFloat("iTime", (float)glfwGetTime());

        //lantern - lights, working wiht shadowing
        lit.use();
        
        const int MAX_GPU_LIGHTS = 64;
        std::vector<int> ids(lanterns.size());
        std::iota(ids.begin(), ids.end(), 0);

        //sort by distance to the boat
        const int take = std::min((int)ids.size(), MAX_GPU_LIGHTS);
        std::partial_sort(ids.begin(), ids.begin()+take, ids.end(),
            [&](int a, int b){
                float da = glm::dot(lanterns[a].pos - boatPosition, lanterns[a].pos - boatPosition);
                float db = glm::dot(lanterns[b].pos - boatPosition, lanterns[b].pos - boatPosition);
                return da < db;
            });

        int n = take;
        lit.setInt("numLanterns", n);

        int shadowGlobal = pickShadowLight(lanterns, boatPosition);
        int shadowLocal  = -1;
        for (int i = 0; i < n; ++i) if (ids[i] == shadowGlobal) { shadowLocal = i; break; }
        lit.setInt("shadowedIndex", shadowLocal);

        for (int i = 0; i < n; ++i) {
            const auto& L = lanterns[ids[i]];
            std::string b = "lanterns[" + std::to_string(i) + "]";
            lit.setVec3(b + ".position", lanterns[i].pos);
            lit.setVec3(b + ".color", glm::vec3(1.0f, 0.62f, 0.28f));
            lit.setFloat(b + ".constant", 1.0f);
            lit.setFloat(b + ".linear", 0.14f);
            lit.setFloat(b + ".quadratic", 0.07f);
        }

        //<Drawing the Models :)>
        //island
        lit.setBool("useTexture", false);
        lit.setMat4("model", I);

        glActiveTexture(GL_TEXTURE0);
        lit.setVec3("baseColor", glm::vec3(31.0f / 255.0f, 94.0f / 255.0f, 31.0f / 255.0f)); 
        island.Draw(lit);

        //castle
        lit.use();
        lit.setBool("useTexture", false);
        lit.setMat4("model", C);
        
        for (size_t i = 0; i < castle.getMeshes().size(); i++) {
            lit.use();
            lit.setBool("useTexture", false);

            if (i % 3 == 0) lit.setVec3("baseColor", glm::vec3(1.0f, 0.819f, 0.863f)); //pink
            if (i % 3 == 1) lit.setVec3("baseColor", glm::vec3(1.0f)); //white
            if (i % 3 == 2) lit.setVec3("baseColor", glm::vec3(1.0f, 0.992f, 0.921f)); //off-white

            castle.getMeshes()[i].Draw(lit);
        }

        //boat
        lit.use();
        lit.setBool("useTexture", true);
        lit.setVec3("baseColor", glm::vec3(0.8f, 0.6f, 0.4f));
        lit.setBool("isLantern", false);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, boatTex);

        lit.setMat4("model", model);
        boat.Draw(lit);

        //flower
        float pulse = 0.625f + 0.175f * std::sin(now * gFlowerPulseSpeed);
        pulse = glm::clamp(pulse, 0.0f, 1.0f);

        lit.use();
        lit.setMat4("model", M);
        lit.setBool("useTexture", true); 
        lit.setVec3("baseColor", glm::vec3(1.0f)); 
        lit.setVec3("emissiveColor", gFlowerTint);
        lit.setFloat("emissiveStrength", pulse); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flowerTex);

        flower.Draw(lit);

        //lanterns
        lit.setBool("useTexture", true);
        lit.setVec3("baseColor", glm::vec3(1.0f));
        lit.setBool("isLantern", true);
        lit.setVec3("lanternTint", glm::vec3(1.0f, 0.85f, 0.45f));
        lit.setFloat("lanternEmissive", 0.7f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lanternTex);

        for (const auto& L : lanterns) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), L.pos);
            m = glm::scale(m, glm::vec3(0.06f));
            lit.setMat4("model", m);
            lantern.Draw(lit);
        }
        lit.setBool("isLantern", false);

        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        renderSkybox(skyboxVAO, skyboxShader, cubemapTexture, skyView, proj);

        //water
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        water.use();
        water.setMat4("projection", proj);
        water.setMat4("view", view);

        glm::mat4 waterModel = glm::mat4(1.0f);
        waterModel = glm::translate(waterModel, glm::vec3(0.0f, 0.0f, 0.0f));
        waterModel = glm::scale(waterModel, glm::vec3(200.0f, 1.0f, 200.0f)); // tweak size
        water.setMat4("model", waterModel);

        water.setVec3("waterColor", glm::vec3(0.06f, 0.10f, 0.15f));
        water.setFloat("alphaBase", 0.55f);
        water.setFloat("eta", 1.33f);
        water.setFloat("reflectBoost", 0.45f);
        water.setFloat("absorb", 1.2f);
        water.setFloat("time", glfwGetTime());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glBindVertexArray(waterVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 10.0f * deltaTime;
    glm::vec3 forward = BoatForward();

    //boat movement (W/S) and rotation (A/D)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        boatPosition += forward * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        boatPosition -= forward * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        boatRotation += glm::radians(60.0f) * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        boatRotation -= glm::radians(60.0f) * deltaTime;

    //toggle perspectives with P
    static bool pWasDown = false;
    bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pDown && !pWasDown) {
        perspectiveBoat = !perspectiveBoat;

        if (perspectiveBoat) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // keep mouse captured in-boat
            
            cockpitYawOff = 0.0f;
            cockpitPitchOff = 0.0f;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // free-look also uses captured cursor
            camera.Position = boatPosition + glm::vec3(0.0f, 10.0f, 10.0f);
            camera.Yaw = -135.0f; 
            camera.Pitch = -20.0f;
            camera.ProcessMouseMovement(0, 0);
        }
    }
    pWasDown = pDown;

    static bool spaceWasDown = false;
    bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (perspectiveBoat && spaceDown && !spaceWasDown) {
        SpawnLantern();
    }
    spaceWasDown = spaceDown;

    static bool cWasDown = false;
    bool cDown = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    if (cDown && !cWasDown) {
        std::cout << "[C] Castle burst!\n";
        SpawnLanternBurstFromCastle(20);
    }
    cWasDown = cDown;
}

unsigned int loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false); // cubemap faces should not be flipped

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


void renderSkybox(unsigned int skyboxVAO,
                  Shader& skyboxShader,
                  unsigned int cubemapTexture,
                  const glm::mat4& view,
                  const glm::mat4& projection)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader.use();
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}