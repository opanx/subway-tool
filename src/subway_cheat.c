/*
 * Panxcz Subway Surfers Tool v1.0
 * External ELF Binary - Root required
 * Architecture: ARM64 (aarch64)
 *
 * Uses /proc/pid/mem for memory read/write (Android compatible)
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
#include <ctype.h>
#include <sys/wait.h>
#include <elf.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

/* ============================================================
 * CONFIGURATION
 * ============================================================ */
#define GAME_PACKAGE "com.kiloo.subwaysurf"
#define VERSION "1.0"
#define MAX_SCAN_REGIONS 4096

/* ============================================================
 * OFFSETS (from com.kiloo.subwaysurf_64bit.cs dump)
 * ============================================================ */

/* CoreRunnerManager */
#define OFF_CRM_IS_ACTIVE       0x10
#define OFF_CRM_SESSION_DATA    0x18
#define OFF_CRM_RUN_MULTIPLIER  0x20

/* RunSessionData */
#define OFF_RSD_DISTANCE        0x18
#define OFF_RSD_KEYS            0x20
#define OFF_RSD_COINS           0x28
#define OFF_RSD_TOTAL_TIME      0x30
#define OFF_RSD_BONUS_COINS     0x48
#define OFF_RSD_MULTIPLIER_USED 0x4c
#define OFF_RSD_POINTS          0x50

/* CoreRunMultiplier */
#define OFF_CRMUL_BOOSTER       0x10
#define OFF_CRMUL_MYSTERY       0x14
#define OFF_CRMUL_EVENT         0x18
#define OFF_CRMUL_DOUBLE_SCORE  0x1c
#define OFF_CRMUL_TOTAL         0x20

/* CharacterMotorConfig */
#define OFF_CMC_GRAVITY         0x18
#define OFF_CMC_JUMP_HEIGHT     0x4c
#define OFF_CMC_COLLIDER_H      0x64

/* CollisionAbilityInstance */
#define OFF_COL_NO_LOWER        0x35
#define OFF_COL_NO_CORNER       0x37

/* ============================================================
 * GLOBALS
 * ============================================================ */
static int g_pid = -1;
static int g_running = 1;

/* Cheat settings */
static int g_score_mult = 1;
static int g_coin_mult = 1;
static float g_speed_hack = 1.0f;
static float g_jump_hack = 1.0f;
static float g_gravity_hack = 1.0f;
static int g_no_collision = 0;
static int g_infinite_coins = 0;
static int g_double_coins = 0;
static int g_magnet_range = 0;

/* ============================================================
 * MEMORY READ/WRITE via /proc/pid/mem
 * ============================================================ */
static int read_mem(int pid, unsigned long addr, void *buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    if (lseek(fd, addr, SEEK_SET) < 0) { close(fd); return -1; }
    ssize_t n = read(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int write_mem(int pid, unsigned long addr, const void *buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    if (lseek(fd, addr, SEEK_SET) < 0) { close(fd); return -1; }
    ssize_t n = write(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int read_i32(int pid, unsigned long addr, int *val) {
    return read_mem(pid, addr, val, sizeof(int));
}

static int read_u64(int pid, unsigned long addr, unsigned long *val) {
    return read_mem(pid, addr, val, sizeof(unsigned long));
}

static int read_float(int pid, unsigned long addr, float *val) {
    return read_mem(pid, addr, val, sizeof(float));
}

static int write_i32(int pid, unsigned long addr, int val) {
    return write_mem(pid, addr, &val, sizeof(int));
}

static int write_u32(int pid, unsigned long addr, unsigned int val) {
    return write_mem(pid, addr, &val, sizeof(unsigned int));
}

static int write_float(int pid, unsigned long addr, float val) {
    return write_mem(pid, addr, &val, sizeof(float));
}

/* ============================================================
 * PROCESS UTILITIES
 * ============================================================ */
static int find_pid(const char *package) {
    char cmdline[256];
    DIR *proc = opendir("/proc");
    if (!proc) return -1;

    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!ent->d_name[0] || !isdigit((unsigned char)ent->d_name[0]))
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

/* ============================================================
 * SCANNER - Find CoreRunnerManager instance
 * ============================================================ */
static unsigned long scan_for_crm(int pid) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE *fp = fopen(maps_path, "r");
    if (!fp) return 0;

    char line[512];
    int region_count = 0;

    while (fgets(line, sizeof(line), fp) && region_count < MAX_SCAN_REGIONS) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;

        if (perms[0] != 'r') continue;
        if ((end - start) > 0x1000000) continue;

        size_t region_size = end - start;
        unsigned char *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            /* Scan for CoreRunnerManager pattern:
             * +0x18 = RunSessionData* (valid heap ptr)
             * +0x20 = CoreRunMultiplier* (valid heap ptr)
             * +0x30 = TotalTime (reasonable float) */
            for (size_t i = 0; i < region_size - 0x100; i += 8) {
                unsigned long *ptrs = (unsigned long *)(buf + i);

                unsigned long rsd_ptr = ptrs[3]; /* offset 0x18 / 8 */
                if (rsd_ptr < 0x1000 || rsd_ptr > 0x8000000000UL)
                    continue;

                unsigned long crmul_ptr = ptrs[4]; /* offset 0x20 / 8 */
                if (crmul_ptr < 0x1000 || crmul_ptr > 0x8000000000UL)
                    continue;

                /* Verify RunSessionData: TotalTime at +0x30 */
                float total_time = 0;
                if (read_mem(pid, rsd_ptr + 0x30, &total_time, 4) == 0) {
                    if (total_time > 0.0f && total_time < 100000.0f) {
                        /* Verify CoreRunMultiplier: totalMultiplier at +0x20 */
                        int total_mult = 0;
                        if (read_mem(pid, crmul_ptr + 0x20, &total_mult, 4) == 0) {
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

/* ============================================================
 * CHEAT FUNCTIONS
 * ============================================================ */

static void apply_score_multiplier(int pid, unsigned long crm) {
    if (g_score_mult <= 1) return;
    unsigned long mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;
    write_i32(pid, mult_ptr + OFF_CRMUL_BOOSTER, g_score_mult);
    write_i32(pid, mult_ptr + OFF_CRMUL_TOTAL, g_score_mult);
    write_u32(pid, mult_ptr + OFF_CRMUL_DOUBLE_SCORE, 1);
}

static void apply_coin_multiplier(int pid, unsigned long crm) {
    if (g_coin_mult <= 1) return;
    unsigned long session_ptr;
    if (read_u64(pid, crm + OFF_CRM_SESSION_DATA, &session_ptr) < 0) return;
    if (session_ptr < 0x1000) return;
    write_i32(pid, session_ptr + OFF_RSD_BONUS_COINS, g_coin_mult * 100);
    write_i32(pid, session_ptr + OFF_RSD_MULTIPLIER_USED, g_coin_mult);
}

static void apply_speed_hack(int pid, unsigned long crm) {
    if (g_speed_hack <= 1.0f) return;
    unsigned long mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;
    write_i32(pid, mult_ptr + OFF_CRMUL_EVENT, (int)(g_speed_hack * 10));
}

static void apply_jump_hack(int pid) {
    if (g_jump_hack <= 1.0f && g_gravity_hack >= 1.0f) return;

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;

        size_t region_size = end - start;
        unsigned char *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            for (size_t i = 0; i + 0x80 < region_size; i += 4) {
                float gravity = *(float *)(buf + i);
                float jump_h = *(float *)(buf + i + 0x34); /* 0x4c - 0x18 = 0x34 */

                if (gravity >= 10.0f && gravity <= 30.0f &&
                    jump_h >= 2.0f && jump_h <= 8.0f) {
                    if (g_jump_hack > 1.0f)
                        write_float(pid, start + i + 0x34, jump_h * g_jump_hack);
                    if (g_gravity_hack != 1.0f)
                        write_float(pid, start + i, gravity * g_gravity_hack);
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

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *fp = fopen(maps_path, "r");
    if (!fp) return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3)
            continue;
        if (perms[0] != 'r' || perms[1] != 'w') continue;
        if ((end - start) > 0x100000) continue;

        size_t region_size = end - start;
        unsigned char *buf = malloc(region_size);
        if (!buf) continue;

        if (read_mem(pid, start, buf, region_size) == 0) {
            for (size_t i = 0x28; i + 0x40 < region_size; i += 8) {
                float collider_h = *(float *)(buf + i);
                if (collider_h >= 1.0f && collider_h <= 6.0f) {
                    unsigned char one = 1;
                    write_mem(pid, start + i + 0xd, &one, 1); /* NoLowerCollision */
                    write_mem(pid, start + i + 0xe, &one, 1); /* On */
                    write_mem(pid, start + i + 0xf, &one, 1); /* NoCornerCollision */
                    write_mem(pid, start + i + 0x10, &one, 1); /* On */
                }
            }
        }
        free(buf);
    }
    fclose(fp);
}

static void apply_infinite_coins(int pid, unsigned long crm) {
    if (!g_infinite_coins) return;
    unsigned long session_ptr;
    if (read_u64(pid, crm + OFF_CRM_SESSION_DATA, &session_ptr) < 0) return;
    if (session_ptr < 0x1000) return;
    write_i32(pid, session_ptr + OFF_RSD_COINS, 999999);
    write_i32(pid, session_ptr + OFF_RSD_BONUS_COINS, 999999);
}

static void apply_double_coins(int pid, unsigned long crm) {
    if (!g_double_coins) return;
    unsigned long mult_ptr;
    if (read_u64(pid, crm + OFF_CRM_RUN_MULTIPLIER, &mult_ptr) < 0) return;
    if (mult_ptr < 0x1000) return;
    write_u32(pid, mult_ptr + OFF_CRMUL_DOUBLE_SCORE, 1);
}

/* ============================================================
 * CHEAT THREAD
 * ============================================================ */
static void *cheat_thread(void *arg) {
    (void)arg;
    printf("[+] Cheat thread started\n");

    while (g_running) {
        if (g_pid <= 0 || kill(g_pid, 0) != 0) {
            g_pid = find_pid(GAME_PACKAGE);
            if (g_pid > 0)
                printf("[+] Found game PID: %d\n", g_pid);
            usleep(500000);
            continue;
        }

        /* Scan for CoreRunnerManager */
        unsigned long crm = scan_for_crm(g_pid);
        if (crm == 0) {
            printf("[-] Waiting for game run...\n");
            usleep(1000000);
            continue;
        }

        printf("[+] CoreRunnerManager @ 0x%lx\n", crm);

        /* Apply cheats loop */
        while (g_running) {
            if (kill(g_pid, 0) != 0) {
                printf("[-] Game died\n");
                g_pid = -1;
                break;
            }

            uint8_t is_active = 0;
            read_mem(g_pid, crm + OFF_CRM_IS_ACTIVE, &is_active, 1);
            if (!is_active) {
                usleep(500000);
                continue;
            }

            apply_score_multiplier(g_pid, crm);
            apply_coin_multiplier(g_pid, crm);
            apply_speed_hack(g_pid, crm);
            apply_jump_hack(g_pid);
            apply_no_collision(g_pid);
            apply_infinite_coins(g_pid, crm);
            apply_double_coins(g_pid, crm);

            /* Status */
            unsigned long session_ptr = 0;
            read_u64(g_pid, crm + OFF_CRM_SESSION_DATA, &session_ptr);
            if (session_ptr > 0x1000) {
                int coins = 0, keys = 0;
                float dist = 0;
                read_i32(g_pid, session_ptr + OFF_RSD_COINS, &coins);
                read_i32(g_pid, session_ptr + OFF_RSD_KEYS, &keys);
                read_float(g_pid, session_ptr + OFF_RSD_DISTANCE, &dist);
                printf("\r[*] Coins:%d Keys:%d Dist:%.0f Spd:%.1fx Jmp:%.1fx   ",
                       coins, keys, dist, g_speed_hack, g_jump_hack);
                fflush(stdout);
            }

            usleep(100000);
        }
    }
    return NULL;
}

/* ============================================================
 * INPUT MENU
 * ============================================================ */
static void print_menu(void) {
    printf("\n");
    printf("+--------------------------------------+\n");
    printf("|  Panxcz Subway Surfers Tool v%s     |\n", VERSION);
    printf("|  By Panxcz & Freebuff               |\n");
    printf("+--------------------------------------+\n");
    printf("|  [1] Score Multiplier: %-3d          |\n", g_score_mult);
    printf("|  [2] Coin Multiplier:  %-3d          |\n", g_coin_mult);
    printf("|  [3] Speed Hack:       %.1fx          |\n", g_speed_hack);
    printf("|  [4] Jump Height:      %.1fx          |\n", g_jump_hack);
    printf("|  [5] Gravity:          %.1fx          |\n", g_gravity_hack);
    printf("|  [6] No Collision:     %-3s           |\n", g_no_collision ? "ON" : "OFF");
    printf("|  [7] Infinite Coins:   %-3s           |\n", g_infinite_coins ? "ON" : "OFF");
    printf("|  [8] Double Coins:     %-3s           |\n", g_double_coins ? "ON" : "OFF");
    printf("|  [9] Magnet Range:     %-3s           |\n", g_magnet_range ? "ON" : "OFF");
    printf("|  [0] Exit                            |\n");
    printf("+--------------------------------------+\n");
    printf("Choice: ");
    fflush(stdout);
}

static void *input_thread(void *arg) {
    (void)arg;
    char input[64];

    while (g_running) {
        print_menu();
        if (fgets(input, sizeof(input), stdin)) {
            int c = atoi(input);
            switch (c) {
            case 1:
                printf("Score mult (1-100): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g_score_mult = atoi(input);
                if (g_score_mult < 1) g_score_mult = 1;
                if (g_score_mult > 100) g_score_mult = 100;
                break;
            case 2:
                printf("Coin mult (1-100): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g_coin_mult = atoi(input);
                if (g_coin_mult < 1) g_coin_mult = 1;
                if (g_coin_mult > 100) g_coin_mult = 100;
                break;
            case 3:
                printf("Speed (1.0-10.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g_speed_hack = (float)atof(input);
                if (g_speed_hack < 1.0f) g_speed_hack = 1.0f;
                if (g_speed_hack > 10.0f) g_speed_hack = 10.0f;
                break;
            case 4:
                printf("Jump (1.0-10.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g_jump_hack = (float)atof(input);
                if (g_jump_hack < 1.0f) g_jump_hack = 1.0f;
                if (g_jump_hack > 10.0f) g_jump_hack = 10.0f;
                break;
            case 5:
                printf("Gravity (0.1-2.0): "); fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) g_gravity_hack = (float)atof(input);
                if (g_gravity_hack < 0.1f) g_gravity_hack = 0.1f;
                if (g_gravity_hack > 2.0f) g_gravity_hack = 2.0f;
                break;
            case 6: g_no_collision = !g_no_collision; break;
            case 7: g_infinite_coins = !g_infinite_coins; break;
            case 8: g_double_coins = !g_double_coins; break;
            case 9: g_magnet_range = !g_magnet_range; break;
            case 0: g_running = 0; break;
            default: printf("Invalid\n"); break;
            }
        }
    }
    return NULL;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char *argv[]) {
    printf("Panxcz Subway Surfers Tool v%s\n", VERSION);
    printf("Copyright (c) 2025 Panxcz & Freebuff\n\n");

    if (getuid() != 0) {
        printf("[!] Need root: su -c ./subway_tool\n");
        return 1;
    }

    /* Parse args */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--score") == 0 && i + 1 < argc) g_score_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--coins") == 0 && i + 1 < argc) g_coin_mult = atoi(argv[++i]);
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) g_speed_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--jump") == 0 && i + 1 < argc) g_jump_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--gravity") == 0 && i + 1 < argc) g_gravity_hack = (float)atof(argv[++i]);
        if (strcmp(argv[i], "--no-collision") == 0) g_no_collision = 1;
        if (strcmp(argv[i], "--infinite-coins") == 0) g_infinite_coins = 1;
        if (strcmp(argv[i], "--double-coins") == 0) g_double_coins = 1;
        if (strcmp(argv[i], "--all") == 0) {
            g_score_mult = 99; g_coin_mult = 99;
            g_speed_hack = 2.0f; g_jump_hack = 2.0f;
            g_gravity_hack = 0.5f;
            g_no_collision = 1; g_infinite_coins = 1;
            g_double_coins = 1; g_magnet_range = 1;
        }
    }

    /* Find game */
    printf("[*] Searching for %s...\n", GAME_PACKAGE);
    g_pid = find_pid(GAME_PACKAGE);
    if (g_pid <= 0) {
        printf("[-] Game not found. Start game first.\n");
        sleep(3);
        g_pid = find_pid(GAME_PACKAGE);
        if (g_pid <= 0) { printf("[-] Exiting.\n"); return 1; }
    }
    printf("[+] Game PID: %d\n", g_pid);

    /* Threads */
    pthread_t ctid, itid;
    pthread_create(&ctid, NULL, cheat_thread, NULL);
    pthread_create(&itid, NULL, input_thread, NULL);

    pthread_join(itid, NULL);
    g_running = 0;
    pthread_join(ctid, NULL);

    printf("\n[+] Done!\n");
    return 0;
}
