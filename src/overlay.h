#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <android/log.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define TAG "PanxczOverlay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

/* ============================================================
 * EGL Function Pointers
 * ============================================================ */
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
typedef EGLDisplay (*eglGetDisplay_t)(EGLNativeDisplayType);
typedef EGLBoolean (*eglInitialize_t)(EGLDisplay, int*, int*);
typedef EGLContext (*eglCreateContext_t)(EGLDisplay, EGLConfig, EGLContext, const int*);
typedef EGLBoolean (*eglMakeCurrent_t)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);
typedef EGLSurface (*eglCreateWindowSurface_t)(EGLDisplay, EGLConfig, EGLNativeWindowType, const int*);

static eglSwapBuffers_t real_eglSwapBuffers = nullptr;
static eglGetDisplay_t real_eglGetDisplay = nullptr;
static eglInitialize_t real_eglInitialize = nullptr;
static eglCreateContext_t real_eglCreateContext = nullptr;
static eglMakeCurrent_t real_eglMakeCurrent = nullptr;
static eglCreateWindowSurface_t real_eglCreateWindowSurface = nullptr;

/* ============================================================
 * State
 * ============================================================ */
static bool g_ImGuiInitialized = false;
static bool g_MenuVisible = true;
static int g_ScreenWidth = 0;
static int g_ScreenHeight = 0;
static EGLDisplay g_Display = EGL_NO_DISPLAY;
static EGLContext g_Context = EGL_NO_CONTEXT;
static pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

/* Touch toggle: 3-finger tap shows/hides menu */
static int g_TouchCount = 0;
static bool g_TogglePending = false;

/* ============================================================
 * Font loading from embedded data
 * ============================================================ */
static void LoadCustomFont() {
    ImGuiIO& io = ImGui::GetIO();

    // Use default ImGui font (Roboto-like)
    // For custom font, embed ttf data here
    // io.Fonts->AddFontFromFileTTF(...)

    // Default font is fine for now
    io.Fonts->AddFontDefault();
}

/* ============================================================
 * ImGui Menu Rendering
 * ============================================================ */

/* Forward declarations of cheat settings from main */
struct CheatSettings {
    int score_mult;
    int coin_mult;
    float speed_hack;
    float jump_hack;
    float gravity_hack;
    bool no_collision;
    bool infinite_coins;
    bool double_coins;
    bool magnet_range;
    bool infinite_hoverboard;
    bool jetpack_always;
    bool invincible;
    bool shield;
    bool god_mode;
    bool score_protect;
    bool double_jump;
    bool fast_landing;
    bool no_ads;
    bool bypass_anti_root;
    bool bypass_frida;
    bool bypass_ptrace;
};

static CheatSettings g_CheatSettings = {};

static void RenderMenu() {
    if (!g_MenuVisible) return;

    ImGuiIO& io = ImGui::GetIO();

    // Set next window position and size
    ImGui::SetNextWindowPos(ImVec2(20, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

    // Dark theme with cyan/purple accents
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.1f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f, 0.0f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.0f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.1f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.1f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.0f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.0f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.0f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.0f, 0.4f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.0f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.0f, 0.8f, 0.5f));

    if (ImGui::Begin("##PanxczMenu", &g_MenuVisible, flags)) {
        // Header
        ImGui::PushFont(ImGui::GetDefaultFont());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        ImGui::Text("🎮 Panxcz Subway Tool v2.1");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
        ImGui::Text("By Panxcz & Freebuff | 3-finger tap = toggle menu");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Score & Coins
        if (ImGui::CollapsingHeader("💰 Score & Coins", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            ImGui::SliderInt("Score Multiplier", &g_CheatSettings.score_mult, 1, 100);
            ImGui::SliderInt("Coin Multiplier", &g_CheatSettings.coin_mult, 1, 100);
            ImGui::Checkbox("Infinite Coins", &g_CheatSettings.infinite_coins);
            ImGui::Checkbox("Double Coins", &g_CheatSettings.double_coins);
            ImGui::Checkbox("Score Protector", &g_CheatSettings.score_protect);
            ImGui::PopItemWidth();
        }

        // Movement
        if (ImGui::CollapsingHeader("🏃 Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("Speed", &g_CheatSettings.speed_hack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Jump Height", &g_CheatSettings.jump_hack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Gravity", &g_CheatSettings.gravity_hack, 0.1f, 2.0f, "%.1fx");
            ImGui::Checkbox("Double Jump", &g_CheatSettings.double_jump);
            ImGui::Checkbox("Fast Landing", &g_CheatSettings.fast_landing);
            ImGui::PopItemWidth();
        }

        // Powers
        if (ImGui::CollapsingHeader("⚡ Powers")) {
            ImGui::PushItemWidth(-1);
            ImGui::Checkbox("No Collision", &g_CheatSettings.no_collision);
            ImGui::Checkbox("God Mode", &g_CheatSettings.god_mode);
            ImGui::Checkbox("Infinite Hoverboard", &g_CheatSettings.infinite_hoverboard);
            ImGui::Checkbox("Jetpack Always", &g_CheatSettings.jetpack_always);
            ImGui::Checkbox("Invincibility", &g_CheatSettings.invincible);
            ImGui::Checkbox("Shield", &g_CheatSettings.shield);
            ImGui::Checkbox("Magnet Range 5x", &g_CheatSettings.magnet_range);
            ImGui::PopItemWidth();
        }

        // Bypass
        if (ImGui::CollapsingHeader("🛡️ Bypass")) {
            ImGui::PushItemWidth(-1);
            ImGui::Checkbox("Anti-Root Bypass", &g_CheatSettings.bypass_anti_root);
            ImGui::Checkbox("Frida Bypass", &g_CheatSettings.bypass_frida);
            ImGui::Checkbox("Ptrace Bypass", &g_CheatSettings.bypass_ptrace);
            ImGui::Checkbox("No Ads", &g_CheatSettings.no_ads);
            ImGui::PopItemWidth();
        }

        // Quick buttons
        ImGui::Separator();
        if (ImGui::Button("🔥 Enable ALL", ImVec2(-1, 35))) {
            g_CheatSettings.score_mult = 99;
            g_CheatSettings.coin_mult = 99;
            g_CheatSettings.speed_hack = 2.0f;
            g_CheatSettings.jump_hack = 2.5f;
            g_CheatSettings.gravity_hack = 0.4f;
            g_CheatSettings.no_collision = 1;
            g_CheatSettings.infinite_coins = 1;
            g_CheatSettings.double_coins = 1;
            g_CheatSettings.magnet_range = 1;
            g_CheatSettings.infinite_hoverboard = 1;
            g_CheatSettings.jetpack_always = 1;
            g_CheatSettings.invincible = 1;
            g_CheatSettings.shield = 1;
            g_CheatSettings.god_mode = 1;
            g_CheatSettings.score_protect = 1;
            g_CheatSettings.double_jump = 1;
            g_CheatSettings.fast_landing = 1;
            g_CheatSettings.no_ads = 1;
        }
        if (ImGui::Button("🚫 Disable ALL", ImVec2(-1, 35))) {
            g_CheatSettings = {};
            g_CheatSettings.speed_hack = 1.0f;
            g_CheatSettings.jump_hack = 1.0f;
            g_CheatSettings.gravity_hack = 1.0f;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(15);
    ImGui::PopStyleVar(3);
}

/* ============================================================
 * Overlay hook: eglSwapBuffers
 * ============================================================ */
static pthread_t g_CheatThread;
static bool g_CheatThreadRunning = false;

/* External cheat apply functions (defined in subway_cheat.c) */
extern void apply_all_cheats(CheatSettings* settings);

static void* CheatThread(void* arg) {
    (void)arg;
    LOGI("Cheat thread started");
    while (g_CheatThreadRunning) {
        apply_all_cheats(&g_CheatSettings);
        usleep(100000); // 100ms
    }
    return NULL;
}

static void InitOverlay() {
    if (g_ImGuiInitialized) return;

    LOGI("Initializing ImGui overlay...");

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Dark obsidian theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    ImGui_ImplAndroid_Init(nullptr);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    LoadCustomFont();

    g_ImGuiInitialized = true;

    // Start cheat thread
    g_CheatThreadRunning = true;
    pthread_create(&g_CheatThread, nullptr, CheatThread, nullptr);

    LOGI("Overlay initialized!");
}

static void ShutdownOverlay() {
    if (!g_ImGuiInitialized) return;
    g_CheatThreadRunning = false;
    pthread_join(g_CheatThread, nullptr);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    g_ImGuiInitialized = false;
    LOGI("Overlay shutdown");
}

/* ============================================================
 * Hooked eglSwapBuffers
 * ============================================================ */
static EGLBoolean hooked_eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (!real_eglSwapBuffers) return EGL_FALSE;

    // Init overlay on first call
    if (!g_ImGuiInitialized) {
        g_Display = display;
        InitOverlay();
    }

    // Start new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // Get screen size
    int w = 0, h = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    if (w > 0 && h > 0) {
        g_ScreenWidth = w;
        g_ScreenHeight = h;
        ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
    }

    // Render menu
    RenderMenu();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return real_eglSwapBuffers(display, surface);
}

/* ============================================================
 * EGL hook setup
 * ============================================================ */
static void HookEGL() {
    // Get real function pointers
    real_eglSwapBuffers = (eglSwapBuffers_t)dlsym(RTLD_NEXT, "eglSwapBuffers");
    real_eglGetDisplay = (eglGetDisplay_t)dlsym(RTLD_NEXT, "eglGetDisplay");
    real_eglInitialize = (eglInitialize_t)dlsym(RTLD_NEXT, "eglInitialize");
    real_eglCreateContext = (eglCreateContext_t)dlsym(RTLD_NEXT, "eglCreateContext");
    real_eglMakeCurrent = (eglMakeCurrent_t)dlsym(RTLD_NEXT, "eglMakeCurrent");
    real_eglCreateWindowSurface = (eglCreateWindowSurface_t)dlsym(RTLD_NEXT, "eglCreateWindowSurface");

    if (!real_eglSwapBuffers) {
        LOGE("Failed to get eglSwapBuffers");
        return;
    }

    // We need to patch the PLT/GOT to redirect eglSwapBuffers
    // Since this is a standalone binary, we need to find the game's
    // eglSwapBuffers call and redirect it

    LOGI("EGL functions loaded, overlay will activate on next frame");
}

/* ============================================================
 * Constructor: runs when .so is loaded
 * ============================================================ */
__attribute__((constructor))
static void lib_init() {
    LOGI("Panxcz Overlay v2.1 loaded!");
    HookEGL();
}

__attribute__((destructor))
static void lib_deinit() {
    ShutdownOverlay();
}
