#ifndef SITEMAPS_PARSER_H
#define SITEMAPS_PARSER_H

#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include "stack_urls.h"


static size_t sitemapXMLFetchCallback(const char* content, size_t size, size_t nmemb, void* userp);

URLNode_t* getSitemap(char* url);

#endif