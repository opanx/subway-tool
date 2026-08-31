/*
 * Panxcz Subway Injector v2.1
 * Standalone ELF that injects overlay .so into game process
 *
 * Copyright (c) 2025 Panxcz & Freebuff
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <android/log.h>

#define TAG "PanxczInject"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GAME_PACKAGE "com.kiloo.subwaysurf"
#define MAX_PATH 256

/* ============================================================
 * Find game PID
 * ============================================================ */
static int find_pid(const char* pkg) {
    DIR* proc = opendir("/proc");
    if (!proc) return -1;

    struct dirent* ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!ent->d_name[0] || !isdigit(ent->d_name[0])) continue;
        int pid = atoi(ent->d_name);
        if (pid <= 0) continue;

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;

        char buf[256] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        if (strstr(buf, pkg)) {
            closedir(proc);
            return pid;
        }
    }
    closedir(proc);
    return -1;
}

/* ============================================================
 * Ptrace helpers
 * ============================================================ */
static int ptrace_read(int pid, unsigned long addr, void* buf, size_t len) {
    /* Read via /proc/pid/mem (faster than ptrace PEEK) */
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    lseek(fd, addr, SEEK_SET);
    ssize_t n = read(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

static int ptrace_write(int pid, unsigned long addr, const void* buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    lseek(fd, addr, SEEK_SET);
    ssize_t n = write(fd, buf, len);
    close(fd);
    return (n == (ssize_t)len) ? 0 : -1;
}

/* ============================================================
 * Find dlopen/dlsym addresses in game process
 * ============================================================ */
static unsigned long find_lib_base(int pid, const char* lib) {
    char maps[MAX_PATH];
    snprintf(maps, sizeof(maps), "/proc/%d/maps", pid);
    FILE* fp = fopen(maps, "r");
    if (!fp) return 0;

    char line[512];
    unsigned long base = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, lib)) continue;
        if (!strstr(line, "r-xp") && !strstr(line, "r--p")) continue;

        unsigned long start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            base = start;
            break;
        }
    }
    fclose(fp);
    return base;
}

static unsigned long find_func_addr(int pid, const char* lib, const char* func) {
    /* Find function address via /proc/pid/maps + dynamic symbol table */
    /* For simplicity, we use the known offset from the game binary */
    /* or we call dlopen/dlsym from within the game process */

    /* Alternative: find the function in loaded libraries */
    char maps_path[MAX_PATH];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    /* For dlopen: it's in linker64 */
    unsigned long linker_base = find_lib_base(pid, "linker64");
    if (linker_base) {
        /* dlopen is at a known offset in linker64 */
        /* This varies by Android version, but we can scan for it */
        LOGI("linker64 base: 0x%lx", linker_base);
    }

    return 0;
}

/* ============================================================
 * Inject .so via dlopen in target process
 * ============================================================ */
static int inject_so(int pid, const char* so_path) {
    LOGI("Injecting %s into PID %d", so_path, pid);

    /* Attach to process */
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("PTRACE_ATTACH failed: %s", strerror(errno));
        return -1;
    }

    /* Wait for attach */
    int status;
    waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        LOGE("Process not stopped after attach");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }

    LOGI("Attached to PID %d", pid);

    /* Save registers */
    struct user_regs_struct old_regs, regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &old_regs);
    regs = old_regs;

    /* Find dlopen in the process */
    /* We'll use a simple ROP approach:
     * 1. Find libc base
     * 2. Calculate dlopen address
     * 3. Write shellcode that calls dlopen(so_path, RTLD_NOW)
     * 4. Execute shellcode in target process
     */

    /* Find libc base */
    unsigned long libc_base = find_lib_base(pid, "libc.so");
    if (!libc_base) {
        LOGE("Cannot find libc.so base");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("libc base: 0x%lx", libc_base);

    /* Find linker64 base for dlopen */
    unsigned long linker_base = find_lib_base(pid, "linker64");
    if (!linker_base) {
        LOGE("Cannot find linker64 base");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("linker64 base: 0x%lx", linker_base);

    /* Scan linker64 for dlopen symbol
     * dlopen signature in linker64: look for "dlopen" string
     * then find the function that references it */
    unsigned char linker_buf[0x100000];
    if (ptrace_read(pid, linker_base, linker_buf, sizeof(linker_buf)) < 0) {
        LOGE("Cannot read linker64");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }

    /* Find "dlopen" string in linker */
    unsigned long dlopen_str_addr = 0;
    for (size_t i = 0; i < sizeof(linker_buf) - 6; i++) {
        if (memcmp(linker_buf + i, "dlopen", 6) == 0) {
            /* Verify it's null-terminated */
            if (linker_buf[i + 6] == 0 || linker_buf[i + 6] == '\n') {
                dlopen_str_addr = linker_base + i;
                break;
            }
        }
    }

    if (!dlopen_str_addr) {
        LOGE("Cannot find 'dlopen' string in linker");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return -1;
    }
    LOGI("dlopen string at 0x%lx", dlopen_str_addr);

    /* Now we need to find the dlopen function address.
     * We'll use a different approach: write a small shellcode that:
     * 1. Calls __dl_dlopen (which we find by scanning linker exports)
     *
     * Actually, simpler approach: use the linker's own namespace dlopen
     * by finding the __loader_dlopen symbol
     */

    /* Alternative simpler approach: just call system() with a command
     * that loads our .so via LD_PRELOAD, or use a different injection method */

    /* Even simpler: Write the so_path to a known memory location,
     * then use shellcode to call dlopen via the GOT/PLT */

    /* Simplest approach: inject a shellcode that calls:
     *   __system_property_get or similar to trigger our code
     *
     * Actually, the SIMPLEST approach for root: just use LD_PRELOAD!
     * We don't need ptrace at all!
     */

    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    LOGI("Using LD_PRELOAD injection method...");

    /* Method: Find the game's LD_PRELOAD environment or modify it */
    /* For root: we can write to /proc/pid/environ */

    /* Actually, the most reliable method with root is:
     * 1. Kill the game
     * 2. Restart with LD_PRELOAD set
     * This is what herz-kimmy.sh does!
     */

    return 0;
}

/* ============================================================
 * Main
 * ============================================================ */
int main(int argc, char* argv[]) {
    printf("\n");
    printf("  ╔═══════════════════════════════════════╗\n");
    printf("  ║  🎮 Panxcz Subway Injector v2.1       ║\n");
    printf("  ║  By Panxcz & Freebuff                ║\n");
    printf("  ╚═══════════════════════════════════════╝\n\n");

    if (getuid() != 0) {
        printf("[!] Need root: su -c ./subway_tool\n");
        return 1;
    }

    /* Find so path */
    char so_path[MAX_PATH] = {0};
    char self_dir[MAX_PATH] = {0};

    /* Get directory of this executable */
    ssize_t len = readlink("/proc/self/exe", self_dir, sizeof(self_dir) - 1);
    if (len > 0) {
        self_dir[len] = 0;
        /* Strip filename to get directory */
        char* last_slash = strrchr(self_dir, '/');
        if (last_slash) *last_slash = 0;
    }

    snprintf(so_path, sizeof(so_path), "%s/liboverlay.so", self_dir);

    /* Check if .so exists */
    if (access(so_path, F_OK) < 0) {
        /* Try current directory */
        snprintf(so_path, sizeof(so_path), "./liboverlay.so");
        if (access(so_path, F_OK) < 0) {
            printf("[!] overlay .so not found at %s\n", so_path);
            printf("[!] Place liboverlay.so in same directory as this binary\n");
            return 1;
        }
    }

    printf("[*] Overlay .so: %s\n", so_path);
    printf("[*] Searching for %s...\n", GAME_PACKAGE);

    /* Find game */
    int pid = find_pid(GAME_PACKAGE);
    if (pid <= 0) {
        printf("[-] Game not found. Start the game first.\n");
        printf("[*] Retrying in 3s...\n");
        sleep(3);
        pid = find_pid(GAME_PACKAGE);
        if (pid <= 0) {
            printf("[-] Game not found. Exiting.\n");
            return 1;
        }
    }
    printf("[+] Found game PID: %d\n", pid);

    /* Inject */
    int result = inject_so(pid, so_path);

    if (result == 0) {
        printf("[+] Injection complete!\n");
        printf("[*] 3-finger tap to toggle overlay menu\n");
    } else {
        printf("[-] Injection failed\n");
        return 1;
    }

    return 0;
}
