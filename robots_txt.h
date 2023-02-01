#ifndef ROBOTS_TXT_H
#define ROBOTS_TXT_H

#include "stack_urls.h"
#include "logger.h"
#include "utils.h"
#include <curl/curl.h>

void get_robots_txt_urls(char* url, bool no_color, URLNode_t** urls_todo);

#endif