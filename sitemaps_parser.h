#ifndef SITEMAPS_PARSER_H
#define SITEMAPS_PARSER_H

#include <string.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include "stack_urls.h"
#include "stack_urls.h"
#include "logger.h"
#include "utils.h"

URLNode_t* get_sitemap_urls(char* url, bool no_color);

// exported for testing reasons
char* __sitemap_get_location(xmlNode* sitemap_root);
void __sitemap_location_get_urls(xmlNode* sitemap_root, URLNode_t** urls_found);
void __sitemap_get_content(xmlNode* root, URLNode_t** urls_sitemaps, URLNode_t** urls_found, bool no_color);
#endif
