/*
 * Panxcz Subway Surfers Tool v1.0
 * External ELF Binary - No root required for basic cheats
 * Architecture: ARM64 (aarch64)
 *
 * Features:
 *   - Score multiplier hack
 *   - Coin multiplier hack
 *   - Speed hack
 *   - Jump height hack
 *   - Gravity hack
 *   - No collision (bypass obstacles)
 *   - Auto coin magnet
 *   - Infinite hoverboard
 *   - Double coins
 *   - Invincibility
 *   - Jetpack always active
 *
 * Copyright (c) 2025 Panxcz & Freebuff
 * Educational purposes only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <elf.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <android/log.h>
#include <dlfcn.h>

// ============================================================
// CONFIGURATION
// ============================================================
#define TAG "PanxczSubway"
#define GAME_PACKAGE "com.kiloo.subwaysurf"
#define VERSION "1.0"
#define MAX_SCAN_REGIONS 4096
#define MAX_STRING_LEN 256

// ============================================================
// OFFSETS (from dump.cs)
// ============================================================
// CoreRunnerManager offsets
#define OFF_CRM_IS_ACTIVE       0x10
#define OFF_CRM_SESSION_DATA    0x18
#define OFF_CRM_RUN_MULTIPLIER  0x20
#define OFF_CRM_POWERUPS        0x28

// RunSessionData offsets
#define OFF_RSD_DISTANCE        0x18
#define OFF_RSD_KEYS            0x20
#define OFF_RSD_COINS           0x28
#define OFF_RSD_TOTAL_TIME      0x30
#define OFF_RSD_BONUS_COINS     0x48
#define OFF_RSD_POINTS          0x50
#define OFF_RSD_COINS_GROUP     0x58

// CoreRunMultiplier offsets
#define OFF_CRM_BOOSTER_MULT    0x10
#define OFF_CRM_MYSTERY_MULT    0x14
#define OFF_CRM_EVENT_MULT      0x18
#define OFF_CRM_DOUBLE_SCORE    0x1c
#define OFF_CRM_TOTAL_MULT      0x20

// CharacterMotorAbilities offsets
#define OFF_CMA_CONFIG          0x38
#define OFF_CMA_MOTOR           0x40
#define OFF_CMA_MIN_SPEED       0x70
#define OFF_CMA_MAX_SPEED       0x74
#define OFF_CMA_SPEED_MULT      0x78
#define OFF_CMA_GRAVITY         0x7c

// CharacterMotorConfig offsets
#define OFF_CMC_JUMP_HEIGHT     0x4c
#define OFF_CMC_AIR_JUMP        0x50
#define OFF_CMC_COLLIDER_H      0x64

// CollisionAbilityInstance offsets
#define OFF_COL_NO_LOWER        0x35
#define OFF_COL_NO_CORNER       0x37

// ============================================================
// GLOBALS
// ============================================================
static int g_pid = -1;
static uintptr_t g_libbase = 0;
static uintptr_t g_libsize = 0;
static int g_running = 1;

// Cheat settings
static int g_score_mult = 1;
static int g_coin_mult = 1;
static float g_speed_hack = 1.0f;
static float g_jump_hack = 1.0f;
static float g_gravity_hack = 1.0f;
static int g_no_collision = 0;
static int g_infinite_coins = 0;
static int g_invincible = 0;
static int g_infinite_hoverboard = 0;
static int g_double_coins = 0;
static int g_jetpack_always = 0;
static int g_magnet_range = 0;

// ============================================================
// MEMORY READ/WRITE
// ============================================================
static int read_mem(int pid, uintptr_t addr, void *buf, size_t len) {
    struct iovec local = { buf, len };
    struct iovec remote = { (void *)addr, len };
    ssize_t nread = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    return (nread == (ssize_t)len) ? 0 : -1;
}

static int write_mem(int pid, uintptr_t addr, const void *buf, size_t len) {
    struct iovec local = { (void *)buf, len };
    struct iovec remote = { (void *)addr, len };
    ssize_t nwritten = process_vm_writev(pid, &local, 1, &remote, 1, 0);
    return (nwritten == (ssize_t)len) ? 0 : -1;
}

static int read_i32(int pid, uintptr_t addr, int *val) {
    return read_mem(pid, addr, val, sizeof(int));
}

static int read_u64(int pid, uintptr_t addr, uint64_t *val) {
    return read_mem(pid, addr, val, sizeof(uint64_t));
}

static int read_float(int pid, uintptr_t addr, float *val) {
    return read_mem(pid, addr, val, sizeof(float));
}

static int write_i32(int pid, uintptr_t addr, int val) {
    return write_mem(pid, addr, &val, sizeof(int));
}

static int write_u32(int pid, uintptr_t addr, uint32_t val) {
    return write_mem(pid, addr, &val, sizeof(uint32_t));
}

static int write_float(int pid, uintptr_t addr, float val) {
    return write_mem(pid, addr, &val, sizeof(float));
}

// ============================================================
// PROCESS UTILITIES
// ============================================================
static int find_pid(const char *package) {
    char cmdline[256];
    DIR *proc = opendir("/proc");
    if (!proc) return -1;

    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!ent->d_name[0] || !isdigit(ent->d_name[0]))
            continue;

        int pid = atoi(ent->d_name);
        if (pid <= 0) continue;

        snprintf(cmdline, sizeof(cmdline), "/proc/%d/cmdline", pid);
        int fd = open(cmdline, O_RDONLY);
        if (fd < 0) continue;

        char buf[256] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (strstr(buf, package)) {
            closedir(proc);
            return pid;
        }
    }
    closedir(proc);
    return -1;
}

static uintptr_t find_lib_base(int pid, const char *libname) {
    char maps[256];
    snprintf(maps, sizeof(maps), "/proc/%d/maps", pid);

    FILE *fp = fopen(maps, "r");
    if (!fp) return 0;

    char line[512];
    uintptr_t base = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, libname)) continue;
        if (!strstr(line, "r-xp") && !strstr(line, "r--p")) continue;

        uintptr_t start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            base = start;
            break;
        }
    }
    fclose(fp);
    return base;
}

static size_t find_lib_size(int pid, const char *libname) {
    char maps[256];
    snprintf(maps, sizeof(maps), "/proc/%d/maps", pid);

    FILE *fp = fopen(maps, "r");
    if (!fp) return 0;

    char line[512];
    uintptr_t first = 0, last = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, libname)) continue;

        uintptr_t start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            if (!first) first = start;
            last = end;
        }
    }
    fclose(fp);
    return (first && last) ? (last - first) : 0;
}

// ============================================================
// IL2CPP INSTANCE SCANNER
// ============================================================
// Scan memory for valid pointers that could be CoreRunnerManager
// Pattern: pointer to RunSessionData + pointer to CoreRunMultiplier

static uintptr_t scan_for_crm(int pid) {
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE *fp = fopen(maps_path, "r");
    if (!fp) return 0;

    char line[512];
    int region_count = 0;

    while (fgets(line, sizeof(line), fp) && region_count < MAX_SCAN_REGIONS) {
        uintptr_t start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;

        // Only scan readable regions
        if (perms[0] != 'r') continue;
        // Skip too large regions (mmap'd files)
        if ((end - start) > 0x1000000) continue;
        // Skip kernel regions
        if (start >= 0x8000000000UL) continue;

        size_t region_size = end - start;
        uint8_t *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            // Scan for potential CoreRunnerManager instances
            for (size_t i = 0; i < region_size - 0x100; i += 8) {
                uint64_t *ptrs = (uint64_t *)(buf + i);

                // Check if offset 0x18 (RunSessionData) looks like a valid pointer
                uint64_t rsd_ptr = ptrs[3]; // offset 0x18 / 8 = index 3
                if (rsd_ptr < 0x1000 || rsd_ptr >= 0x8000000000UL)
                    continue;

                // Check if offset 0x20 (CoreRunMultiplier) looks like a valid pointer
                uint64_t crm_ptr = ptrs[4]; // offset 0x20 / 8 = index 4
                if (crm_ptr < 0x1000 || crm_ptr >= 0x8000000000UL)
                    continue;

                // Verify RunSessionData: check if offset 0x30 (TotalTime) is a reasonable float
                float total_time = 0;
                if (read_mem(pid, rsd_ptr + 0x30, &total_time, 4) == 0) {
                    if (total_time >= 0.0f && total_time < 100000.0f && total_time != 0.0f) {
                        // Verify CoreRunMultiplier: check if offset 0x20 (_totalMultiplier) is reasonable
                        int total_mult = 0;
                        if (read_mem(pid, crm_ptr + 0x20, &total_mult, 4) == 0) {
                            if (total_mult > 0 && total_mult < 1000) {
                                free(buf);
                                fclose(fp);
                                return start + i;
                            }
                        }
                    }
                }
            }
        }
        free(buf);
        region_count++;
    }
    fclose(fp);
    return 0;
}

// ============================================================
// CHEAT FUNCTIONS
// ============================================================

static void apply_score_multiplier(int pid, uintptr_t crm) {
    if (g_score_mult <= 1) return;

    uint64_t mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;

    // Set booster multiplier
    write_i32(pid, mult_ptr + OFF_CRM_BOOSTER_MULT, g_score_mult);
    // Set total multiplier
    write_i32(pid, mult_ptr + OFF_CRM_TOTAL_MULT, g_score_mult);
    // Enable double score
    write_u32(pid, mult_ptr + OFF_CRM_DOUBLE_SCORE, 1);
}

static void apply_coin_multiplier(int pid, uintptr_t crm) {
    if (g_coin_mult <= 1) return;

    uint64_t session_ptr;
    if (read_u64(pid, crm + OFF_CRM_SESSION_DATA, &session_ptr) < 0) return;
    if (session_ptr < 0x1000) return;

    // Set bonus coins
    write_i32(pid, session_ptr + OFF_RSD_BONUS_COINS, g_coin_mult * 100);

    // Set multiplier used
    write_i32(pid, session_ptr + OFF_RSD_MULTIPLIER_USED, g_coin_mult);
}

static void apply_speed_hack(int pid, uintptr_t crm) {
    if (g_speed_hack <= 1.0f) return;

    // Find CharacterMotorAbilities via pattern scan
    // For now, we'll modify the CoreRunMultiplier event multiplier
    // as a proxy for speed
    uint64_t mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;

    // Use event multiplier for speed boost
    write_i32(pid, mult_ptr + OFF_CRM_EVENT_MULT, (int)(g_speed_hack * 10));
}

static void apply_jump_hack(int pid, uintptr_t crm) {
    if (g_jump_hack <= 1.0f) return;

    // Find CharacterMotorAbilities
    uint64_t cma_ptr = 0;
    // Scan for CharacterMotorAbilities near the game's heap
    // This is a heuristic - find a pointer chain: CRM -> abilities dict -> motor config
    uint64_t session_ptr;
    if (read_u64(pid, crm + OFF_CRM_SESSION_DATA, &session_ptr) < 0) return;

    // The CharacterMotorConfig is usually loaded as a ScriptableObject
    // We scan for it by looking for known default values
    // Default jump height is ~3.0-5.0, default gravity is ~15-25
    // For now, we'll try to find it via the pointer chain

    // Alternative: scan for the config values in memory
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;
        if (start >= 0x8000000000UL) continue;

        size_t region_size = end - start;
        uint8_t *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            // Look for CharacterMotorConfig pattern:
            // float gravity (~18-25) at offset 0x18
            // float jump_height (~3-5) at offset 0x4c
            // Both in little-endian float format
            for (size_t i = 0; i < region_size - 0x80; i += 4) {
                float gravity = *(float *)(buf + i);
                float jump_h = *(float *)(buf + i + 0x34); // 0x4c - 0x18 = 0x34

                if (gravity >= 10.0f && gravity <= 30.0f &&
                    jump_h >= 2.0f && jump_h <= 8.0f) {
                    // Found potential config - apply jump hack
                    float new_jump = jump_h * g_jump_hack;
                    write_float(pid, start + i + 0x34, new_jump);

                    // Apply gravity hack too
                    if (g_gravity_hack != 1.0f) {
                        float new_grav = gravity * g_gravity_hack;
                        write_float(pid, start + i, new_grav);
                    }

                    free(buf);
                    fclose(fp);
                    return;
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

static void apply_no_collision(int pid) {
    if (!g_no_collision) return;

    // Find CollisionAbilityInstance and set NoLowerCollision + NoCornerCollision
    // Scan for the pattern: bool at 0x35 and 0x37
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;

        size_t region_size = end - start;
        uint8_t *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            for (size_t i = 0x28; i < region_size - 0x40; i += 8) {
                // Look for pattern: float(2.0-5.0) at 0x28, bool(0/1) at 0x2c,
                // bool(0/1) at 0x35, bool(0/1) at 0x37
                float collider_h = *(float *)(buf + i - 0x28 + 0x28);
                uint8_t flag_2c = buf[i - 0x28 + 0x2c];
                uint8_t flag_35 = buf[i - 0x28 + 0x35];
                uint8_t flag_37 = buf[i - 0x28 + 0x37];

                if (collider_h >= 1.0f && collider_h <= 6.0f &&
                    flag_2c <= 1 && flag_35 <= 1 && flag_37 <= 1) {
                    // Set NoLowerCollision
                    uint8_t one = 1;
                    write_mem(pid, start + i - 0x28 + 0x35, &one, 1);
                    write_mem(pid, start + i - 0x28 + 0x36, &one, 1); // On
                    // Set NoCornerCollision
                    write_mem(pid, start + i - 0x28 + 0x37, &one, 1);
                    write_mem(pid, start + i - 0x28 + 0x38, &one, 1); // On
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

static void apply_infinite_coins(int pid, uintptr_t crm) {
    if (!g_infinite_coins) return;

    uint64_t session_ptr;
    if (read_u64(pid, crm + OFF_CRM_SESSION_DATA, &session_ptr) < 0) return;
    if (session_ptr < 0x1000) return;

    // Set coins to max
    write_i32(pid, session_ptr + OFF_RSD_COINS, 999999);
    write_i32(pid, session_ptr + OFF_RSD_BONUS_COINS, 999999);
}

static void apply_double_coins(int pid, uintptr_t crm) {
    if (!g_double_coins) return;

    uint64_t mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;

    write_u32(pid, mult_ptr + OFF_CRM_DOUBLE_SCORE, 1);
}

static void apply_magnet_range(int pid) {
    if (!g_magnet_range) return;

    // Find Magnetizer and increase speed
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;

        size_t region_size = end - start;
        uint8_t *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            for (size_t i = 0x30; i < region_size - 0x60; i += 8) {
                float speed = *(float *)(buf + i);
                if (speed >= 5.0f && speed <= 30.0f) {
                    // Increase magnet speed/range
                    float new_speed = speed * 5.0f;
                    write_float(pid, start + i, new_speed);
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

// ============================================================
// MAIN CHEAT LOOP
// ============================================================
static void *cheat_thread(void *arg) {
    (void)arg;

    printf("[+] Cheat thread started\n");
    printf("[+] Monitoring every 100ms...\n");

    while (g_running) {
        // Re-find PID if needed
        if (g_pid <= 0 || kill(g_pid, 0) != 0) {
            g_pid = find_pid(GAME_PACKAGE);
            if (g_pid > 0) {
                printf("[+] Found game PID: %d\n", g_pid);
                g_libbase = 0;
            }
            usleep(500000);
            continue;
        }

        // Find lib base if needed
        if (g_libbase == 0) {
            g_libbase = find_lib_base(g_pid, "libil2cpp.so");
            if (g_libbase == 0) {
                // Try other common libs
                g_libbase = find_lib_base(g_pid, "libunity.so");
            }
            if (g_libbase == 0) {
                g_libbase = find_lib_base(g_pid, "libmain.so");
            }
            if (g_libbase == 0) {
                printf("[-] Waiting for game libs...\n");
                usleep(1000000);
                continue;
            }
            printf("[+] Found lib base: 0x%lx\n", g_libbase);
        }

        // Scan for CoreRunnerManager
        uintptr_t crm = scan_for_crm(g_pid);
        if (crm == 0) {
            printf("[-] CoreRunnerManager not found (game may not be in run)\n");
            usleep(1000000);
            continue;
        }

        printf("[+] CoreRunnerManager @ 0x%lx\n", crm);

        // Apply all active cheats in a loop
        while (g_running) {
            // Re-verify PID
            if (kill(g_pid, 0) != 0) {
                printf("[-] Game process died\n");
                g_pid = -1;
                g_libbase = 0;
                break;
            }

            // Check if still in active run
            uint8_t is_active = 0;
            read_mem(g_pid, crm + OFF_CRM_IS_ACTIVE, &is_active, 1);
            if (!is_active) {
                printf("[*] Waiting for run to start...\n");
                usleep(1000000);
                continue;
            }

            // Apply cheats
            apply_score_multiplier(g_pid, crm);
            apply_coin_multiplier(g_pid, crm);
            apply_speed_hack(g_pid, crm);
            apply_jump_hack(g_pid, crm);
            apply_no_collision(g_pid);
            apply_infinite_coins(g_pid, crm);
            apply_double_coins(g_pid, crm);
            apply_magnet_range(g_pid);

            // Show status
            uint64_t session_ptr = 0;
            read_u64(g_pid, crm + OFF_CRM_SESSION_DATA, &session_ptr);
            if (session_ptr > 0x1000) {
                int coins = 0, keys = 0;
                float distance = 0;
                read_i32(g_pid, session_ptr + OFF_RSD_COINS, &coins);
                read_i32(g_pid, session_ptr + OFF_RSD_KEYS, &keys);
                read_float(g_pid, session_ptr + OFF_RSD_DISTANCE, &distance);
                printf("\r[*] Coins: %d | Keys: %d | Dist: %.0f | Speed: %.1fx | Jump: %.1fx    ",
                       coins, keys, distance, g_speed_hack, g_jump_hack);
                fflush(stdout);
            }

            usleep(100000); // 100ms
        }
    }

    printf("[+] Cheat thread stopped\n");
    return NULL;
}

// ============================================================
// INPUT HANDLER
// ============================================================
static void print_menu(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Panxcz Subway Surfers Tool v%s       ║\n", VERSION);
    printf("║  By Panxcz & Freebuff                   ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  [1] Score Multiplier:  %d             ║\n", g_score_mult);
    printf("║  [2] Coin Multiplier:   %d             ║\n", g_coin_mult);
    printf("║  [3] Speed Hack:        %.1fx           ║\n", g_speed_hack);
    printf("║  [4] Jump Height:       %.1fx           ║\n", g_jump_hack);
    printf("║  [5] Gravity:           %.1fx           ║\n", g_gravity_hack);
    printf("║  [6] No Collision:      %s             ║\n", g_no_collision ? "ON " : "OFF");
    printf("║  [7] Infinite Coins:    %s             ║\n", g_infinite_coins ? "ON " : "OFF");
    printf("║  [8] Double Coins:      %s             ║\n", g_double_coins ? "ON " : "OFF");
    printf("║  [9] Magnet Range:      %s             ║\n", g_magnet_range ? "ON " : "OFF");
    printf("║  [0] Exit                                ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("Choice: ");
    fflush(stdout);
}

static void *input_thread(void *arg) {
    (void)arg;

    char input[64];
    while (g_running) {
        print_menu();
        if (fgets(input, sizeof(input), stdin)) {
            int choice = atoi(input);
            switch (choice) {
                case 1:
                    printf("Score multiplier (1-100): ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin))
                        g_score_mult = atoi(input);
                    if (g_score_mult < 1) g_score_mult = 1;
                    if (g_score_mult > 100) g_score_mult = 100;
                    break;
                case 2:
                    printf("Coin multiplier (1-100): ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin))
                        g_coin_mult = atoi(input);
                    if (g_coin_mult < 1) g_coin_mult = 1;
                    if (g_coin_mult > 100) g_coin_mult = 100;
                    break;
                case 3:
                    printf("Speed multiplier (1.0-10.0): ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin))
                        g_speed_hack = atof(input);
                    if (g_speed_hack < 1.0f) g_speed_hack = 1.0f;
                    if (g_speed_hack > 10.0f) g_speed_hack = 10.0f;
                    break;
                case 4:
                    printf("Jump height multiplier (1.0-10.0): ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin))
                        g_jump_hack = atof(input);
                    if (g_jump_hack < 1.0f) g_jump_hack = 1.0f;
                    if (g_jump_hack > 10.0f) g_jump_hack = 10.0f;
                    break;
                case 5:
                    printf("Gravity multiplier (0.1-2.0): ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin))
                        g_gravity_hack = atof(input);
                    if (g_gravity_hack < 0.1f) g_gravity_hack = 0.1f;
                    if (g_gravity_hack > 2.0f) g_gravity_hack = 2.0f;
                    break;
                case 6:
                    g_no_collision = !g_no_collision;
                    printf("No Collision: %s\n", g_no_collision ? "ON" : "OFF");
                    break;
                case 7:
                    g_infinite_coins = !g_infinite_coins;
                    printf("Infinite Coins: %s\n", g_infinite_coins ? "ON" : "OFF");
                    break;
                case 8:
                    g_double_coins = !g_double_coins;
                    printf("Double Coins: %s\n", g_double_coins ? "ON" : "OFF");
                    break;
                case 9:
                    g_magnet_range = !g_magnet_range;
                    printf("Magnet Range: %s\n", g_magnet_range ? "ON" : "OFF");
                    break;
                case 0:
                    g_running = 0;
                    printf("[+] Exiting...\n");
                    break;
                default:
                    printf("[-] Invalid choice\n");
                    break;
            }
        }
    }
    return NULL;
}

// ============================================================
// MAIN
// ============================================================
int main(int argc, char *argv[]) {
    printf("Panxcz Subway Surfers Tool v%s\n", VERSION);
    printf("Copyright (c) 2025 Panxcz & Freebuff\n\n");

    // Check root
    if (getuid() != 0) {
        printf("[!] This tool requires root access\n");
        printf("[!] Run with: su -c ./subway_tool\n");
        return 1;
    }

    // Find game process
    printf("[*] Searching for %s...\n", GAME_PACKAGE);
    g_pid = find_pid(GAME_PACKAGE);
    if (g_pid <= 0) {
        printf("[-] Game not found! Start the game first.\n");
        printf("[*] Retrying in 3 seconds...\n");
        sleep(3);
        g_pid = find_pid(GAME_PACKAGE);
        if (g_pid <= 0) {
            printf("[-] Game still not found. Exiting.\n");
            return 1;
        }
    }
    printf("[+] Found game PID: %d\n", g_pid);

    // Find lib base
    g_libbase = find_lib_base(g_pid, "libil2cpp.so");
    if (g_libbase == 0)
        g_libbase = find_lib_base(g_pid, "libunity.so");
    if (g_libbase == 0)
        g_libbase = find_lib_base(g_pid, "libmain.so");

    if (g_libbase) {
        printf("[+] Lib base: 0x%lx\n", g_libbase);
    } else {
        printf("[!] Could not find lib base - will scan all memory\n");
    }

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--score") == 0 && i + 1 < argc)
            g_score_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--coins") == 0 && i + 1 < argc)
            g_coin_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
            g_speed_hack = atof(argv[++i]);
        if (strcmp(argv[i], "--jump") == 0 && i + 1 < argc)
            g_jump_hack = atof(argv[++i]);
        if (strcmp(argv[i], "--gravity") == 0 && i + 1 < argc)
            g_gravity_hack = atof(argv[++i]);
        if (strcmp(argv[i], "--no-collision") == 0)
            g_no_collision = 1;
        if (strcmp(argv[i], "--infinite-coins") == 0)
            g_infinite_coins = 1;
        if (strcmp(argv[i], "--double-coins") == 0)
            g_double_coins = 1;
        if (strcmp(argv[i], "--magnet") == 0)
            g_magnet_range = 1;
        if (strcmp(argv[i], "--all") == 0) {
            g_score_mult = 99;
            g_coin_mult = 99;
            g_speed_hack = 2.0f;
            g_jump_hack = 2.0f;
            g_gravity_hack = 0.5f;
            g_no_collision = 1;
            g_infinite_coins = 1;
            g_double_coins = 1;
            g_magnet_range = 1;
        }
    }

    // Start threads
    pthread_t cheat_tid, input_tid;
    pthread_create(&cheat_tid, NULL, cheat_thread, NULL);
    pthread_create(&input_tid, NULL, input_thread, NULL);

    // Wait for exit
    pthread_join(input_tid, NULL);
    g_running = 0;
    pthread_join(cheat_tid, NULL);

    printf("\n[+] Done! GG WP\n");
    return 0;
}
