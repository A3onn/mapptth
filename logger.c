#include "logger.h"

// extern variables used here
pthread_t* fetcher_threads;
struct arguments cli_arguments;

// VERBOSE
bool _verbose = false;
void _verbose_print(const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(_verbose) {
        unsigned int curr_thread_number = _get_current_thread_number();
        if(curr_thread_number == MAIN_THREAD_NUMBER) {
            flockfile(stdout);
            fprintf(stderr, "%s[i]%s main thread %s: ", BLUE, RESET, function_name);
            vprintf(format, args);
            funlockfile(stdout);
        } else {
            flockfile(stdout);
            fprintf(stderr, "%s[i]%s thread #%u %s: ", BLUE, RESET, curr_thread_number, function_name);
            vprintf(format, args);
            funlockfile(stdout);
        }
    }

    va_end(args);
}

unsigned int _get_current_thread_number() {
    pthread_t curr = pthread_self();
    for(size_t i = 0; i < cli_arguments.threads; i++) {
        if(pthread_equal(curr, fetcher_threads[i]) != 0) {
            return 1 + i; // avoid having the first thread being confused with the main thread
        }
    }
    return MAIN_THREAD_NUMBER;
}
