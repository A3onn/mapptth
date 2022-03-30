#include "logger.h"

// VERBOSE
bool _verbose = false;
void _verbose_print(const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(_verbose) {
        flockfile(stdout);
        printf("%s[i]%s thread #%lu %s: ", BLUE, RESET, pthread_self(), function_name);
        vprintf(format, args);
        funlockfile(stdout);
    }

    va_end(args);
}
