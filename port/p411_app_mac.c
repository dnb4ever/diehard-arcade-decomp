// P4.11 v1.01: native launcher for the standalone "Die Hard Arcade.app".
// ROMs, settings, quick saves, screenshots and the log live in
// ~/Library/Application Support/Die Hard Arcade/. No ROM data is bundled.
// Missing ROMs: a native dialog explains what to put where and offers a
// folder picker. A MAME "diehard" set (zip or loose files) is converted once
// into cart.bin + diehard_main.bin, the same layout build_cart.py produces.
#include "p413_roms.h"
#include <errno.h>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <spawn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

static char g_dir[1024];
static char g_dir_quoted[2048];  // g_dir escaped for an AppleScript string literal

// Runs argv, captures up to cap bytes of stdout; returns bytes read or -1.
static long run_capture(char* const argv[], unsigned char* buf, size_t cap) {
    int fd[2];
    if (pipe(fd)) return -1;
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, fd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&fa, fd[0]);
    pid_t pid;
    int err = posix_spawn(&pid, argv[0], &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);
    close(fd[1]);
    if (err) { close(fd[0]); return -1; }
    size_t n = 0;
    ssize_t r;
    unsigned char sink[4096];
    while ((r = read(fd[0], n < cap ? buf + n : sink, n < cap ? cap - n : sizeof sink)) > 0)
        if (n < cap) n += (size_t)r;
    close(fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? (long)n : -1;
}

// Runs an AppleScript (dialog); stores its trimmed result line ("" on cancel/error).
static void osa(const char* script, char* out, size_t cap) {
    char* argv[] = {"/usr/bin/osascript", "-e", (char*)script, NULL};
    long n = run_capture(argv, (unsigned char*)out, cap - 1);
    if (n < 0) n = 0;
    out[n] = 0;
    while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r')) out[--n] = 0;
    fprintf(stderr, "dialog result: %s\n", out);
}

static void alert(const char* msg) {
    char script[2048], out[256];
    fprintf(stderr, "alert: %s\n", msg);
    snprintf(script, sizeof script,
             "display alert \"Die Hard Arcade\" message \"%s\" as critical buttons {\"OK\"}", msg);
    osa(script, out, sizeof out);
}

// Shared bounded ZIP inflater, mapping, SHA checks and atomic output.
static int prepare_from(const char* src) {
    char error[512];
    int result=dha_prepare_roms(src,g_dir,error,sizeof error);
    fprintf(stderr,"ROM preparation from %s: %s\n",src,error);
    if(result<0)alert(error);
    return result;
}
static int roms_ready(void) {return dha_roms_ready(g_dir);}

// Dialog loop until ROMs are ready (1) or the user quits (0).
static int ask_for_roms(void) {
    for (;;) {
        char script[3072], out[1100];
        snprintf(script, sizeof script,
            "display dialog \"Die Hard Arcade needs the MAME ROM set 'diehard' (ST-V, USA). "
            "No ROMs are included with this app.\\n\\n"
            "Put diehard.zip into this folder and open the app again:\\n%s\\n\\n"
            "Or click 'Choose ROM folder...' and select a folder that contains diehard.zip "
            "(or its unzipped files: fpr19119.13, mpr19115.2 ... mpr19118.5).\" "
            "with title \"Die Hard Arcade: ROMs needed\" with icon caution "
            "buttons {\"Quit\", \"Show Folder\", \"Choose ROM folder...\"} "
            "default button \"Choose ROM folder...\" cancel button \"Quit\"", g_dir_quoted);
        osa(script, out, sizeof out);
        if (strstr(out, "Show Folder")) {
            char* argv[] = {"/usr/bin/open", g_dir, NULL};
            unsigned char sink[64];
            run_capture(argv, sink, sizeof sink);
            return 0;
        }
        if (!strstr(out, "Choose ROM folder")) return 0;
        osa("POSIX path of (choose folder with prompt \"Select the folder containing diehard.zip\")",
            out, sizeof out);
        if (!out[0]) continue;
        size_t n = strlen(out);
        if (n > 1 && out[n - 1] == '/') out[n - 1] = 0;
        int r = prepare_from(out);
        if (r == 1) return 1;
        if (r == 0)
            alert("That folder does not contain diehard.zip or the diehard ROM files "
                  "(fpr19119.13, mpr19115.2, mpr19116.3, mpr19117.4, mpr19118.5).");
    }
}

static void mkdirs(const char* path) {
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char* p = tmp + 1; *p; ++p)
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    mkdir(tmp, 0755);
}

int main(int argc, char** argv) {
    const char* home = getenv("HOME");
    if (!home || !*home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : NULL;
    }
    if (!home) return 2;
    snprintf(g_dir, sizeof g_dir, "%s/Library/Application Support/Die Hard Arcade", home);
    char* q = g_dir_quoted;
    for (const char* p = g_dir; *p; ++p) {
        if (*p == '"' || *p == '\\') *q++ = '\\';
        *q++ = *p;
    }
    *q = 0;
    char shots[1100];
    snprintf(shots, sizeof shots, "%s/shots", g_dir);
    mkdirs(shots);
    if (chdir(g_dir)) return 2;
    FILE* log = fopen("app.log", "a");
    if (log) {
        dup2(fileno(log), STDOUT_FILENO);
        dup2(fileno(log), STDERR_FILENO);
        fclose(log);
    }
    setvbuf(stderr, NULL, _IONBF, 0);
    fprintf(stderr, "--- Die Hard Arcade 1.06, data folder %s\n", g_dir);

    // diehard.zip or the loose MAME files dropped into the data folder are prepared once.
    if (!roms_ready() && prepare_from(g_dir) != 1) {
        fprintf(stderr, "ROMs missing: asking for diehard.zip / ROM folder\n");
        if (!ask_for_roms()) return 1;
    }

    // The game binary sits beside this launcher; drop inherited test-harness variables.
    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size);
    char* exe = malloc(size + 16);
    if (!exe || _NSGetExecutablePath(exe, &size)) return 2;
    char* slash = strrchr(exe, '/');
    if (!slash) return 2;
    strcpy(slash + 1, "saturn_stv");
    for (int again = 1; again;) {
        again = 0;
        for (char** e = environ; *e; ++e)
            if (strncmp(*e, "STV_", 4) == 0) {
                char name[256];
                size_t n = strcspn(*e, "=");
                snprintf(name, sizeof name, "%.*s", (int)(n < 255 ? n : 255), *e);
                unsetenv(name);
                again = 1;
                break;
            }
    }
    char** args = calloc((size_t)argc + 12, sizeof(char*));
    if (!args) return 2;
    int k = 0;
    args[k++] = exe;
    args[k++] = "--cart"; args[k++] = "diehard_main.bin";
    args[k++] = "--cartrom"; args[k++] = "cart.bin";
    args[k++] = "--window"; args[k++] = "--launcher";
    args[k++] = "--settings"; args[k++] = "stv.ini";
    args[k++] = "--out"; args[k++] = "shots";
    for (int i = 1; i < argc; ++i)
        if (strncmp(argv[i], "-psn_", 5) != 0) args[k++] = argv[i];
    execv(exe, args);
    fprintf(stderr, "cannot start %s: %s\n", exe, strerror(errno));
    alert("The game executable inside the app is missing or damaged. Please reinstall the app.");
    return 2;
}
