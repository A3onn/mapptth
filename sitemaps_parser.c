#include "sitemaps_parser.h"

static size_t __sitemap_fetch_callback(const char *content, size_t size, size_t nmemb, void *userp) {
    xmlParseChunk(*((xmlParserCtxtPtr*) userp), content, size*nmemb, 0);
    return size * nmemb;
}


char* __sitemap_get_location(xmlNode* sitemap_root) {
    xmlNode *cur_node = NULL;

    for (cur_node = sitemap_root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur_node->name, "loc") == 0) {
                return (char*)xmlNodeGetContent(cur_node);
            }
        }
    }
    return ""; // should not happen
}

void __sitemap_location_get_urls(xmlNode* sitemap_root, URLNode_t** urls_found) {
    xmlNode *cur_node = NULL;

    for (cur_node = sitemap_root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur_node->name, "loc") == 0) { // <location>
                char* node_content = (char*)xmlNodeGetContent(cur_node);
                if(!stack_url_contains(*urls_found, node_content)) {
                    LOG("Found <loc> node containing the URL %s, adding it to the list of URL.\n", node_content);
                    stack_url_push(urls_found, strdup(node_content)); // add the url
                }
            } else if(strcmp((const char*)cur_node->name, "link") == 0) { // <link>
                // find href attribute and add the value to the list
                for(xmlAttrPtr attr = cur_node->properties; NULL != attr; attr = attr->next) {
                    if(strcmp((const char*)attr->name, "href") == 0) {
                        if(!stack_url_contains(*urls_found, (char*)attr->children->content)) {
                            LOG("Found <link> node having an attribute \"href\" containing the URL %s, adding it to the list of URL.\n", attr->children->content);
                            stack_url_push(urls_found, strdup((char*)attr->children->content));
                        }
                    }
                }
            }
        }
    }
}

void __sitemap_get_content(xmlNode* root, URLNode_t** urls_sitemaps, URLNode_t** urls_found, bool no_color) {
    xmlNode *cur_node = NULL;

    for (cur_node = root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur_node->name, "sitemap") == 0) { // if node is <sitemap>
                if(!stack_url_contains(*urls_found, (char*)xmlNodeGetContent(cur_node))) {
                    char* found_url = __sitemap_get_location(cur_node->children); // get the location of the sitemap
                    stack_url_push(urls_sitemaps, strdup(found_url)); // add the sitemap to the list of sitemaps to fetch and parse
                    if(no_color) {
                        printf("Found new sitemap: %s\n", found_url);
                    } else {
                        printf("%sFound new sitemap: %s%s\n", GREEN, found_url, RESET);
                    }
                }
            } else if(strcmp((const char*)cur_node->name, "url") == 0) { // found an <url>
                __sitemap_location_get_urls(cur_node->children, urls_found);
            }
        }

        __sitemap_get_content(cur_node->children, urls_sitemaps, urls_found, no_color);
    }
}

void __sitemap_error_callback(void * ctx, const char * msg, ...) {
    // silent libXML errors
    (void)ctx;
    (void)msg;
}

void get_sitemap_urls(char *url, bool no_color, URLNode_t** list_urls_found) {

    LIBXML_TEST_VERSION // tests for libxml2

    xmlSetGenericErrorFunc(NULL, __sitemap_error_callback); // disable errors

    xmlParserCtxtPtr ctxt; // used for chunk parsing
    xmlDocPtr doc;

    URLNode_t* list_sitemaps = NULL;
    stack_url_push(&list_sitemaps, url);

    CURL* curl = curl_easy_init();
#if CURL_AT_LEAST_VERSION(7, 85, 0)
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
#endif
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __sitemap_fetch_callback);

    LOG("Finished initialisation.\n");

    while(!stack_url_isempty(list_sitemaps)) {
        char* current_sitemap_url = stack_url_pop(&list_sitemaps);
        if(no_color) {
            fprintf(stderr, "Fetching sitemap: %s...\n", current_sitemap_url);
        } else {
            fprintf(stderr, "%sFetching sitemap: %s...%s\n", BLUE, current_sitemap_url, RESET);
        }

        ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);
        if(!ctxt) {
            if(no_color) {
                fprintf(stderr, "Failed to create parser context while doing %s\n", url);
            } else {
                fprintf(stderr, "%sFailed to create parser context while doing %s%s\n", RED, url, RESET);
            }
            return ;
        }

        curl_easy_setopt(curl, CURLOPT_URL, current_sitemap_url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &ctxt);
        CURLcode status = curl_easy_perform(curl);
        if(status != CURLE_OK) {
            if(no_color) {
                fprintf(stderr, "Failed to fetch sitemap: %s\n", current_sitemap_url);
            } else {
                fprintf(stderr, "%sFailed to fetch sitemap: %s%s\n", RED, current_sitemap_url, RESET);
            }
            xmlFreeParserCtxt(ctxt);
            continue;
        }
        long status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        if(status_code != 200) {
            if(no_color) {
                fprintf(stderr, "%s doesn't exist.\n", current_sitemap_url);
            } else {
                fprintf(stderr, "%s%s doesn't exist.%s\n", RED, current_sitemap_url, RESET);
            }
            xmlFreeParserCtxt(ctxt);
            continue;
        }

        if(no_color) {
            fprintf(stderr, "Finished fetching %s.\n", current_sitemap_url);
        } else {
            fprintf(stderr, "%sFinished fetching %s.%s\n", GREEN, current_sitemap_url, RESET);
        }

        xmlParseChunk(ctxt, NULL, 0, 1); // indicate the end of chunk parsing

        LOG("Parsing %s...\n", current_sitemap_url);
        doc = ctxt->myDoc; // get the final document
        int res = ctxt->wellFormed;
        xmlFreeParserCtxt(ctxt);

        if (!res) {
            if(no_color) {
                fprintf(stderr, "Failed to parse sitemap %s\n", url);
            } else {
                fprintf(stderr, "%sFailed to parse sitemap %s%s\n", RED, url, RESET);
            }
            continue;
        }

        xmlNode* root_element = xmlDocGetRootElement(doc);

        __sitemap_get_content(root_element, &list_sitemaps, list_urls_found, no_color);

        xmlFreeDoc(doc);

        if(no_color) {
            fprintf(stderr, "Finished parsing %s.\n", current_sitemap_url);
        } else {
            fprintf(stderr, "%sFinished parsing %s.%s\n", BLUE, current_sitemap_url, RESET);
        }
    }

    xmlCleanupParser();
    xmlMemoryDump();
}
