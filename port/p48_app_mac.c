// Small native executable for the in-tree macOS app bundle.
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size);
    char* path = malloc(size);
    if (!path || _NSGetExecutablePath(path, &size)) return 2;
    char* slash = strrchr(path, '/');
    if (!slash) return 2;
    *slash = 0;
    if (chdir(path) || chdir("../../../../../..")) return 2;
    free(path);
    FILE* log = fopen("port/run/P48/app.log", "a");
    if (log) {
        dup2(fileno(log), STDOUT_FILENO);
        dup2(fileno(log), STDERR_FILENO);
        fclose(log);
    }
    char** args = calloc((size_t)argc + 2, sizeof(char*));
    if (!args) return 2;
    args[0] = "/bin/bash";
    args[1] = "port/play_mac.sh";
    for (int i = 1; i < argc; ++i) args[i + 1] = argv[i];
    execv(args[0], args);
    perror("Die Hard Arcade launcher");
    return 2;
}
