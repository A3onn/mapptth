#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include "fetcher_thread.h"
#include "cli_parser.h"

#define RED "\033[0;31m"
#define BRIGHT_RED "\033[0;91m"
#define BLUE "\033[0;34m"
#define GREEN "\033[0;32m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

unsigned int _get_current_thread_number();
#define MAIN_THREAD_NUMBER 0

extern bool _verbose;
void _verbose_print(const char* function_name, const char* format, ...);

#define LOG(...) _verbose_print(__func__, __VA_ARGS__)

#define ACTIVATE_VERBOSE() _verbose = true;

#endif
