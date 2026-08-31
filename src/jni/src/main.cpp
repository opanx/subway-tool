/*
 * Panxcz Subway Surfers Tool v2.1
 * Standalone ELF with ImGui overlay
 * Copyright (c) 2025 Panxcz & Freebuff
 */

#include "main.h"
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/system_properties.h>
#include <sstream>

#include "Memory/Memory.h"
#include "Memory/PatternScanner.h"

using namespace Memory;

/* ============================================================
 * Global State
 * ============================================================ */
bool main_thread_flag = true;
int abs_ScreenX = 0;
int abs_ScreenY = 0;
long libbase = 0;

/* ============================================================
 * Subway Surfers Offsets (from dump.cs)
 * ============================================================ */
#define OFF_CRM_IS_ACTIVE       0x10
#define OFF_CRM_SESSION_DATA    0x18
#define OFF_CRM_RUN_MULTIPLIER  0x20

#define OFF_RSD_DISTANCE        0x18
#define OFF_RSD_KEYS            0x20
#define OFF_RSD_COINS           0x28
#define OFF_RSD_TOTAL_TIME      0x30
#define OFF_RSD_BONUS_COINS     0x48
#define OFF_RSD_MULTIPLIER_USED 0x4c

#define OFF_CRMUL_BOOSTER       0x10
#define OFF_CRMUL_MYSTERY       0x14
#define OFF_CRMUL_EVENT         0x18
#define OFF_CRMUL_DOUBLE_SCORE  0x1c
#define OFF_CRMUL_TOTAL         0x20

#define OFF_CMC_GRAVITY         0x18
#define OFF_CMC_JUMP_HEIGHT     0x4c
#define OFF_CMC_COLLIDER_H      0x64

/* ============================================================
 * Cheat Variables
 * ============================================================ */
static uintptr_t g_CRM = 0;

static int scoreMult = 1;
static int coinMult = 1;
static float speedHack = 1.0f;
static float jumpHack = 1.0f;
static float gravityHack = 1.0f;
static bool noCollision = false;
static bool infiniteCoins = false;
static bool doubleCoins = false;
static bool magnetRange = false;
static bool infiniteHoverboard = false;
static bool jetpackAlways = false;
static bool invincible = false;
static bool shield = false;
static bool godMode = false;
static bool scoreProtect = false;
static bool doubleJump = false;
static bool fastLanding = false;
static bool noAds = false;

/* ============================================================
 * Game Data
 * ============================================================ */
struct GameData {
    int coins;
    int keys;
    float distance;
    int totalMult;
    bool isActive;
};
static GameData g_GameData = {};

/* ============================================================
 * Find CoreRunnerManager
 * ============================================================ */
static uintptr_t FindCRM() {
    char map[128];
    snprintf(map, sizeof(map), "/proc/%d/maps", g_pid);
    FILE *f = fopen(map, "rt");
    if (!f) return 0;

    printf("[+] Scanning for CoreRunnerManager...\n");
    uintptr_t result = 0;
    char line[512];

    struct Region { uintptr_t start; uintptr_t end; };
    std::vector<Region> rwRegions;

    while (fgets(line, sizeof(line), f)) {
        uintptr_t start, end;
        char perms[5];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) < 3) continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if (end - start < 0x10000) continue;
        rwRegions.push_back({start, end});
    }
    fclose(f);

    printf("[+] Found %zu rw regions\n", rwRegions.size());

    for (auto &region : rwRegions) {
        for (uintptr_t addr = region.start; addr < region.end - 0x100; addr += 8) {
            uintptr_t ptr = 0;
            if (!ProcessRead((void*)addr, &ptr, 8)) continue;
            if (ptr < 0x10000 || ptr > 0x7FFFFFFFFFFF) continue;

            uintptr_t rsd = 0;
            if (!ProcessRead((void*)(ptr + OFF_CRM_SESSION_DATA), &rsd, 8)) continue;
            if (rsd < 0x10000 || rsd > 0x7FFFFFFFFFFF) continue;

            uintptr_t crmul = 0;
            if (!ProcessRead((void*)(ptr + OFF_CRM_RUN_MULTIPLIER), &crmul, 8)) continue;
            if (crmul < 0x10000 || crmul > 0x7FFFFFFFFFFF) continue;

            float tt = 0;
            if (!ProcessRead((void*)(rsd + OFF_RSD_TOTAL_TIME), &tt, 4)) continue;
            if (tt <= 0.0f || tt > 100000.0f) continue;

            int tm = 0;
            if (!ProcessRead((void*)(crmul + OFF_CRMUL_TOTAL), &tm, 4)) continue;
            if (tm < 1 || tm > 100) continue;

            printf("[+] FOUND CRM @ 0x%lx (Time=%.1f, Mult=%d)\n", ptr, tt, tm);
            result = ptr;
            break;
        }
        if (result) break;
    }
    return result;
}

/* ============================================================
 * Cheat Functions
 * ============================================================ */
static void ApplyCheats() {
    if (!g_CRM) return;

    uintptr_t mp = 0;
    if (!ProcessRead((void*)(g_CRM + OFF_CRM_RUN_MULTIPLIER), &mp, 8) || mp < 0x10000) return;

    uintptr_t sp = 0;
    if (!ProcessRead((void*)(g_CRM + OFF_CRM_SESSION_DATA), &sp, 8) || sp < 0x10000) return;

    if (scoreMult > 1) {
        int v = scoreMult;
        ProcessWrite((void*)(mp + OFF_CRMUL_BOOSTER), &v, 4);
        ProcessWrite((void*)(mp + OFF_CRMUL_TOTAL), &v, 4);
        int one = 1;
        ProcessWrite((void*)(mp + OFF_CRMUL_DOUBLE_SCORE), &one, 4);
    }

    if (coinMult > 1) {
        int v = coinMult * 100;
        ProcessWrite((void*)(sp + OFF_RSD_BONUS_COINS), &v, 4);
        ProcessWrite((void*)(sp + OFF_RSD_MULTIPLIER_USED), &coinMult, 4);
    }

    if (infiniteCoins) {
        int max = 999999;
        ProcessWrite((void*)(sp + OFF_RSD_COINS), &max, 4);
        ProcessWrite((void*)(sp + OFF_RSD_BONUS_COINS), &max, 4);
    }

    if (doubleCoins) {
        int one = 1;
        ProcessWrite((void*)(mp + OFF_CRMUL_DOUBLE_SCORE), &one, 4);
    }

    if (speedHack > 1.0f) {
        int v = (int)(speedHack * 10);
        ProcessWrite((void*)(mp + OFF_CRMUL_EVENT), &v, 4);
    }

    /* Read game data */
    ProcessRead((void*)(sp + OFF_RSD_COINS), &g_GameData.coins, 4);
    ProcessRead((void*)(sp + OFF_RSD_KEYS), &g_GameData.keys, 4);
    ProcessRead((void*)(sp + OFF_RSD_DISTANCE), &g_GameData.distance, 4);
    ProcessRead((void*)(mp + OFF_CRMUL_TOTAL), &g_GameData.totalMult, 4);
    uint8_t active = 0;
    ProcessRead((void*)(g_CRM + OFF_CRM_IS_ACTIVE), &active, 1);
    g_GameData.isActive = active;
}

/* ============================================================
 * ImGui Menu
 * ============================================================ */
static bool menuVisible = true;

void Layout_tick_UI() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowSizeConstraints(ImVec2(350, 0), ImVec2(400, FLT_MAX));

    ImGui::Begin("Panxcz Subway Tool v2.1", nullptr, flags);

    ImGui::TextColored(ImVec4(0, 0.9f, 1, 1), "By Panxcz & Freebuff");
    ImGui::Separator();

    if (g_GameData.isActive) {
        ImGui::TextColored(ImVec4(0, 1, 0.5f, 1), "IN RUN");
        ImGui::Text("Coins: %d | Keys: %d", g_GameData.coins, g_GameData.keys);
        ImGui::Text("Distance: %.0fm | Mult: %dx", g_GameData.distance, g_GameData.totalMult);
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "NOT IN RUN");
        ImGui::Text("Start a run in-game...");
    }
    ImGui::Separator();

    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Score")) {
            ImGui::SliderInt("Score x", &scoreMult, 1, 100);
            ImGui::SliderInt("Coins x", &coinMult, 1, 100);
            ImGui::Checkbox("Infinite Coins", &infiniteCoins);
            ImGui::Checkbox("Double Coins", &doubleCoins);
            ImGui::Checkbox("Score Protect", &scoreProtect);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Movement")) {
            ImGui::SliderFloat("Speed", &speedHack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Jump", &jumpHack, 1.0f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Gravity", &gravityHack, 0.1f, 2.0f, "%.1fx");
            ImGui::Checkbox("Double Jump", &doubleJump);
            ImGui::Checkbox("Fast Landing", &fastLanding);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Powers")) {
            ImGui::Checkbox("No Collision", &noCollision);
            ImGui::Checkbox("God Mode", &godMode);
            ImGui::Checkbox("Hoverboard", &infiniteHoverboard);
            ImGui::Checkbox("Jetpack", &jetpackAlways);
            ImGui::Checkbox("Invincible", &invincible);
            ImGui::Checkbox("Shield", &shield);
            ImGui::Checkbox("Magnet 5x", &magnetRange);
            ImGui::Checkbox("No Ads", &noAds);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings")) {
            static int theme = 0;
            const char* themes[] = {"Dark", "Light", "Classic"};
            if (ImGui::Combo("Theme", &theme, themes, 3)) {
                if (theme == 0) ImGui::StyleColorsDark();
                if (theme == 1) ImGui::StyleColorsLight();
                if (theme == 2) ImGui::StyleColorsClassic();
            }
            static float opacity = 1.0f;
            ImGui::SliderFloat("Opacity", &opacity, 0.1f, 1.0f);
            ImGui::GetStyle().Alpha = opacity;
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            if (ImGui::Button("Enable ALL")) {
                scoreMult = 99; coinMult = 99;
                speedHack = 2; jumpHack = 2.5; gravityHack = 0.4;
                noCollision = true; infiniteCoins = true;
                doubleCoins = true; magnetRange = true;
                infiniteHoverboard = true; jetpackAlways = true;
                invincible = true; shield = true; godMode = true;
                scoreProtect = true; doubleJump = true;
                fastLanding = true; noAds = true;
            }
            if (ImGui::Button("Disable ALL")) {
                scoreMult = 1; coinMult = 1;
                speedHack = 1; jumpHack = 1; gravityHack = 1;
                noCollision = false; infiniteCoins = false;
                doubleCoins = false; magnetRange = false;
                infiniteHoverboard = false; jetpackAlways = false;
                invincible = false; shield = false; godMode = false;
                scoreProtect = false; doubleJump = false;
                fastLanding = false; noAds = false;
            }
            if (ImGui::Button("Exit")) main_thread_flag = false;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    /* CRM re-scan handled by background thread */
}

/* ============================================================
 * Main
 * ============================================================ */
int main(int argc, char *argv[]) {
    printf("\n");
    printf("  ========================================\n");
    printf("  Panxcz Subway Surfers Tool v2.1\n");
    printf("  By Panxcz & Freebuff\n");
    printf("  ========================================\n\n");

    printf("[*] Searching for com.kiloo.subwaysurf...\n");
    g_pid = FindPid("com.kiloo.subwaysurf");
    if (g_pid <= 0) {
        printf("[-] Game not found!\n");
        return 1;
    }
    Memory::g_pid = g_pid;
    printf("[+] PID: %d\n", g_pid);

    libbase = GetBase("libil2cpp.so");
    if (libbase == 0) {
        printf("[-] libil2cpp.so not found!\n");
        return 1;
    }
    printf("[+] libil2cpp.so: 0x%lx\n", libbase);

    screen_config();
    abs_ScreenX = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    abs_ScreenY = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);
    printf("[+] Screen: %dx%d\n", abs_ScreenX, abs_ScreenY);

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        printf("[-] Failed to init overlay!\n");
        return 1;
    }
    printf("[+] Overlay initialized\n");

    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, false);
    printf("[+] Touch initialized\n");

    ImGui::GetStyle().WindowRounding = 25.0f;

    /* CRM scan runs in background thread */
    pthread_t crmThread;
    pthread_create(&crmThread, NULL, [](void*)->void* {
        printf("[*] CRM scanner started\n");
        while (main_thread_flag) {
            if (!g_CRM) {
                uintptr_t c = FindCRM();
                if (c) {
                    g_CRM = c;
                    printf("[+] CRM found @ 0x%lx\n", c);
                } else {
                    printf("[!] CRM not found - start a run\n");
                }
            }
            sleep(3);
        }
        return NULL;
    }, NULL);

    printf("[+] Main loop started\n\n");

    while (main_thread_flag) {
        if (libbase == 0) {
            libbase = GetBase("libil2cpp.so");
            if (libbase == 0) { usleep(500000); continue; }
        }

        ApplyCheats();

        drawBegin();
        Layout_tick_UI();
        drawEnd();

        usleep(1000);
    }

    shutdown();
    Touch_Close();
    return 0;
}
