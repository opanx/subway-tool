/*
 * Panxcz Subway Overlay v2.1
 * Shared library injected into game process
 * Hooks eglSwapBuffers to render ImGui overlay
 *
 * Copyright (c) 2025 Panxcz & Freebuff
 */

#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <android/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* ImGui */
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#define TAG "PanxczOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

/* ============================================================
 * EGL function pointers
 * ============================================================ */
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
typedef EGLDisplay (*eglGetDisplay_t)(EGLNativeDisplayType);
typedef EGLBoolean (*eglInitialize_t)(EGLDisplay, int*, int*);
typedef EGLBoolean (*eglMakeCurrent_t)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);

static eglSwapBuffers_t real_eglSwapBuffers = NULL;
static eglGetDisplay_t real_eglGetDisplay = NULL;
static eglInitialize_t real_eglInitialize = NULL;
static eglMakeCurrent_t real_eglMakeCurrent = NULL;

/* ============================================================
 * State
 * ============================================================ */
static bool g_Initialized = false;
static bool g_MenuVisible = true;
static pthread_mutex_t g_Lock = PTHREAD_MUTEX_INITIALIZER;

/* Cheat settings */
struct CheatData {
    int score_mult;
    int coin_mult;
    float speed_hack;
    float jump_hack;
    float gravity_hack;
    int no_collision;
    int infinite_coins;
    int double_coins;
    int magnet_range;
    int infinite_hoverboard;
    int jetpack_always;
    int invincible;
    int shield;
    int god_mode;
    int score_protect;
    int double_jump;
    int fast_landing;
    int no_ads;
};
static CheatData g_Cheat = {1, 1, 1.0f, 1.0f, 1.0f, 0,0,0,0,0,0,0,0,0,0,0,0,0};

/* ============================================================
 * OpenGL3 backend (inline, no separate files needed)
 * ============================================================ */
static GLuint g_Shader = 0;
static GLint g_uTex = 0, g_uProj = 0;
static GLuint g_Vao = 0, g_Vbo = 0, g_Ebo = 0;

static const char* vsh =
    "#version 300 es\n"
    "precision mediump float;\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec2 aUV;\n"
    "layout(location=2) in vec4 aCol;\n"
    "uniform mat4 uProj;\n"
    "out vec2 vUV;\n"
    "out vec4 vCol;\n"
    "void main(){\n"
    "  vUV=aUV; vCol=aCol;\n"
    "  gl_Position=uProj*vec4(aPos,0,1);\n"
    "}\n";

static const char* fsh =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 vUV;\n"
    "in vec4 vCol;\n"
    "uniform sampler2D uTex;\n"
    "out vec4 fragColor;\n"
    "void main(){\n"
    "  fragColor=vCol*texture(uTex,vUV);\n"
    "}\n";

static void InitGL() {
    if (g_Shader) return;

    g_Shader = glCreateProgram();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &vsh, NULL);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fsh, NULL);
    glCompileShader(fs);
    glAttachShader(g_Shader, vs);
    glAttachShader(g_Shader, fs);
    glLinkProgram(g_Shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_uTex = glGetUniformLocation(g_Shader, "uTex");
    g_uProj = glGetUniformLocation(g_Shader, "uProj");

    glGenVertexArrays(1, &g_Vao);
    glGenBuffers(1, &g_Vbo);
    glGenBuffers(1, &g_Ebo);

    glBindVertexArray(g_Vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_Vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Ebo);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, uv));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, col));

    glBindVertexArray(0);
    LOGI("GL init done");
}

static void RenderImGui(ImDrawData* draw) {
    if (!draw || draw->CmdListsCount == 0) return;

    ImGuiIO& io = ImGui::GetIO();
    int fb_w = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_h = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_w <= 0 || fb_h <= 0) return;

    draw->ScaleClipRects(io.DisplayFramebufferScale);

    /* Save GL state */
    GLint lastprogram; glGetIntegerv(GL_CURRENT_PROGRAM, &lastprogram);
    GLint lastarray; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastarray);
    GLint lasttex; glGetIntegerv(GL_TEXTURE_BINDING_2D, &lasttex);
    GLint lastsctype[4]; glGetIntegerv(GL_SCISSOR_BOX, lastsctype);
    GLboolean lastblend = glIsEnabled(GL_BLEND);
    GLboolean lastcull = glIsEnabled(GL_CULL_FACE);
    GLboolean lastdepth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lastscissor = glIsEnabled(GL_SCISSOR_TEST);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    glUseProgram(g_Shader);
    float L=0, R=(float)fb_w, T=0, B=(float)fb_h;
    const float proj[16] = {
        2/(R-L), 0, 0, 0,
        0, 2/(T-B), 0, 0,
        0, 0, -1, 0,
        (R+L)/(L-R), (T+B)/(B-T), 0, 1
    };
    glUniformMatrix4fv(g_uProj, 1, GL_FALSE, proj);
    glUniform1i(g_uTex, 0);

    glBindVertexArray(g_Vao);

    for (int n = 0; n < draw->CmdListsCount; n++) {
        const ImDrawList* cmd = draw->CmdLists[n];
        glBindBuffer(GL_ARRAY_BUFFER, g_Vbo);
        glBufferData(GL_ARRAY_BUFFER, cmd->VtxBuffer.Size * sizeof(ImDrawVert), cmd->VtxBuffer.Data, GL_STREAM_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_Ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd->IdxBuffer.Size * sizeof(ImDrawIdx), cmd->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int i = 0; i < cmd->CmdBuffer.Size; i++) {
            const ImDrawCmd& pcmd = cmd->CmdBuffer[i];
            if (pcmd.UserCallback) { pcmd.UserCallback(cmd, &pcmd); continue; }
            ImVec4 cr = pcmd.ClipRect;
            glScissor((int)cr.x, (int)(fb_h - cr.w), (int)(cr.z - cr.x), (int)(cr.w - cr.y));
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd.TextureId);
            glDrawElements(GL_TRIANGLES, (GLsizei)pcmd.ElemCount, sizeof(ImDrawIdx)==2?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd.IdxOffset*sizeof(ImDrawIdx)));
        }
    }

    /* Restore */
    glUseProgram(lastprogram);
    glBindVertexArray(lastarray);
    glBindTexture(GL_TEXTURE_2D, lasttex);
    if (lastblend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (lastcull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (lastdepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (lastscissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}

/* ============================================================
 * ImGui Menu
 * ============================================================ */
static void DrawMenu() {
    if (!g_MenuVisible) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12,12));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8,5));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f,0.04f,0.08f,0.93f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f,0,0.25f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f,0,0.5f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f,0.1f,0.16f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f,0.08f,0.35f,1));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f,0,0.35f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0,0.55f,1));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0,0.85f,1,1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f,0.9f,1,1));

    ImGui::SetNextWindowPos(ImVec2(20,80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360,520), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("##Panxcz", &g_MenuVisible, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(ImVec4(0,0.9f,1,1), "Panxcz Subway Tool v2.1");
        ImGui::TextColored(ImVec4(0.5f,0.5f,0.6f,1), "By Panxcz & Freebuff");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Score & Coins", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Score x", &g_Cheat.score_mult, 1, 100);
            ImGui::SliderInt("Coins x", &g_Cheat.coin_mult, 1, 100);
            ImGui::Checkbox("Infinite Coins", (bool*)&g_Cheat.infinite_coins);
            ImGui::Checkbox("Double Coins", (bool*)&g_Cheat.double_coins);
            ImGui::Checkbox("Score Protect", (bool*)&g_Cheat.score_protect);
        }
        if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Speed", &g_Cheat.speed_hack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Jump", &g_Cheat.jump_hack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Gravity", &g_Cheat.gravity_hack, 0.1f, 2.0f, "%.1fx");
            ImGui::Checkbox("Double Jump", (bool*)&g_Cheat.double_jump);
            ImGui::Checkbox("Fast Land", (bool*)&g_Cheat.fast_landing);
        }
        if (ImGui::CollapsingHeader("Powers")) {
            ImGui::Checkbox("No Collision", (bool*)&g_Cheat.no_collision);
            ImGui::Checkbox("God Mode", (bool*)&g_Cheat.god_mode);
            ImGui::Checkbox("Hoverboard", (bool*)&g_Cheat.infinite_hoverboard);
            ImGui::Checkbox("Jetpack", (bool*)&g_Cheat.jetpack_always);
            ImGui::Checkbox("Invincible", (bool*)&g_Cheat.invincible);
            ImGui::Checkbox("Shield", (bool*)&g_Cheat.shield);
            ImGui::Checkbox("Magnet 5x", (bool*)&g_Cheat.magnet_range);
            ImGui::Checkbox("No Ads", (bool*)&g_Cheat.no_ads);
        }

        ImGui::Separator();
        if (ImGui::Button("Enable ALL", ImVec2(-1,32))) {
            g_Cheat.score_mult=99; g_Cheat.coin_mult=99;
            g_Cheat.speed_hack=2; g_Cheat.jump_hack=2.5; g_Cheat.gravity_hack=0.4;
            g_Cheat.no_collision=1; g_Cheat.infinite_coins=1; g_Cheat.double_coins=1;
            g_Cheat.magnet_range=1; g_Cheat.infinite_hoverboard=1; g_Cheat.jetpack_always=1;
            g_Cheat.invincible=1; g_Cheat.shield=1; g_Cheat.god_mode=1;
            g_Cheat.score_protect=1; g_Cheat.double_jump=1; g_Cheat.fast_landing=1;
        }
        if (ImGui::Button("Disable ALL", ImVec2(-1,32))) {
            memset(&g_Cheat, 0, sizeof(g_Cheat));
            g_Cheat.speed_hack=1; g_Cheat.jump_hack=1; g_Cheat.gravity_hack=1;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(9);
    ImGui::PopStyleVar(3);
}

/* ============================================================
 * Touch input (3-finger toggle)
 * ============================================================ */
static int g_FingerCount = 0;

static void HandleTouch(float x, float y, int action) {
    ImGuiIO& io = ImGui::GetIO();

    switch (action) {
    case 0: /* down */
        io.MouseDown[0] = true;
        io.MousePos = ImVec2(x, y);
        g_FingerCount++;
        if (g_FingerCount >= 3) {
            g_MenuVisible = !g_MenuVisible;
            g_FingerCount = 0;
        }
        break;
    case 1: /* up */
        io.MouseDown[0] = false;
        g_FingerCount--;
        if (g_FingerCount < 0) g_FingerCount = 0;
        break;
    case 2: /* move */
        io.MousePos = ImVec2(x, y);
        break;
    }
}

/* ============================================================
 * Hooked eglSwapBuffers
 * ============================================================ */
static bool g_TouchRegistered = false;

static EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!real_eglSwapBuffers) return EGL_FALSE;

    pthread_mutex_lock(&g_Lock);

    /* Init on first call */
    if (!g_Initialized) {
        LOGI("First eglSwapBuffers - initializing overlay");

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = NULL; /* no settings file */

        /* Dark theme */
        ImGui::StyleColorsDark();
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding = 12;
        s.FrameRounding = 6;
        s.GrabRounding = 4;
        s.WindowBorderSize = 1;
        s.FrameBorderSize = 0;

        /* Init GL backend */
        InitGL();
        ImGui_ImplOpenGL3_Init("#version 300 es");

        /* Create font atlas */
        unsigned char* pixels;
        int w, h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)tex);

        g_Initialized = true;
        LOGI("Overlay initialized! Font: %dx%d", w, h);
    }

    /* Get screen size */
    int winW = 0, winH = 0;
    eglQuerySurface(dpy, surface, EGL_WIDTH, &winW);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &winH);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)winW, (float)winH);
    io.DeltaTime = 1.0f / 60.0f;

    /* New frame */
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    /* Draw menu */
    DrawMenu();

    /* Render */
    ImGui::Render();
    RenderImGui(ImGui::GetDrawData());

    pthread_mutex_unlock(&g_Lock);

    return real_eglSwapBuffers(dpy, surface);
}

/* ============================================================
 * Android input handling (for touch)
 * ============================================================ */
#include <android/input.h>
#include <poll.h>
#include <linux/input.h>

/* ============================================================
 * Constructor: hook EGL
 * ============================================================ */
__attribute__((constructor))
static void on_load() {
    LOGI("Panxcz Overlay v2.1 loaded into game process!");

    /* Get real EGL functions */
    real_eglSwapBuffers = (eglSwapBuffers_t)dlsym(RTLD_NEXT, "eglSwapBuffers");
    real_eglGetDisplay = (eglGetDisplay_t)dlsym(RTLD_NEXT, "eglGetDisplay");
    real_eglInitialize = (eglInitialize_t)dlsym(RTLD_NEXT, "eglInitialize");
    real_eglMakeCurrent = (eglMakeCurrent_t)dlsym(RTLD_NEXT, "eglMakeCurrent");

    if (!real_eglSwapBuffers) {
        LOGE("Failed to find eglSwapBuffers!");
        return;
    }

    LOGI("EGL hooks installed. Waiting for first frame...");

    /* Start a thread to handle input via /dev/input */
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, [](void*)->void* {
        /* Wait for init */
        while (!g_Initialized) usleep(100000);

        /* Open touch device */
        int fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
        if (fd < 0) fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            LOGE("Cannot open input device");
            return NULL;
        }

        /* Read touch events */
        struct pollfd pfd = {fd, POLLIN, 0};
        while (g_Initialized) {
            if (poll(&pfd, 1, 100) > 0 && (pfd.revents & POLLIN)) {
                struct input_event ev;
                if (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if (ev.type == 3) { /* ABS */
                        if (ev.code == 0) HandleTouch((float)ev.value, 0, 2); /* X move */
                        if (ev.code == 1) { /* Y */
                            /* Update last known Y */
                        }
                        if (ev.code == 53) HandleTouch(0, (float)ev.value, 2); /* MT_X */
                        if (ev.code == 54) HandleTouch(0, (float)ev.value, 2); /* MT_Y */
                    }
                    if (ev.type == 1 && ev.code == 330) { /* BTN_TOUCH */
                        HandleTouch(0, 0, ev.value ? 0 : 1);
                    }
                }
            }
        }
        close(fd);
        return NULL;
    }, NULL);
}

__attribute__((destructor))
static void on_unload() {
    LOGI("Panxcz Overlay unloaded");
    if (g_Initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
}
