#include "sitemaps_parser.h"
#include "stack_urls.h"
#include "utils.h"
#include <curl/curl.h>
#include <libxml/parser.h>
#include <string.h>
#include <stdlib.h>

static size_t sitemapXMLFetchCallback(const char *content, size_t size, size_t nmemb, void *userp) {
    xmlParseChunk(*((xmlParserCtxtPtr*) userp), content, size*nmemb, 0);
    return size * nmemb;
}


char* _GetLocationSitemap(xmlNode* sitemap_root) {
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

void _GetLocationURLs(xmlNode* sitemap_root, URLNode_t** urls_found) {
    xmlNode *cur_node = NULL;

    for (cur_node = sitemap_root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur_node->name, "loc") == 0) {
                if(!findURLStack(*urls_found, (char*)xmlNodeGetContent(cur_node))) {
                    pushURLStack(urls_found, (char*)xmlNodeGetContent(cur_node));
                }
            } else if(strcmp((const char*)cur_node->name, "link") == 0) {
                for(xmlAttrPtr attr = cur_node->properties; NULL != attr; attr = attr->next) {
                    if(strcmp((const char*)attr->name, "href") == 0) {
                        if(!findURLStack(*urls_found, (char*)attr->children->content)) {
                            pushURLStack(urls_found, (char*)attr->children->content);
                        }
                    }
                }
            }
        }
    }
}

void addURLsFromSitemap(xmlNode* root, URLNode_t** urls_sitemaps, URLNode_t** urls_found, int noColor) {
    xmlNode *cur_node = NULL;

    for (cur_node = root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((const char*)cur_node->name, "sitemap") == 0) {
                if(!findURLStack(*urls_found, (char*)xmlNodeGetContent(cur_node))) {
                    char* newSitemapURL = _GetLocationSitemap(cur_node->children);
                    pushURLStack(urls_sitemaps, newSitemapURL);
                    if(noColor) {
                        printf("Found new sitemap: %s\n", newSitemapURL);
                    } else {
                        printf("%sFound new sitemap: %s%s\n", GREEN, newSitemapURL, RESET);
                    }
                }
            } else if(strcmp((const char*)cur_node->name, "url") == 0) {
                _GetLocationURLs(cur_node->children, urls_found);
            }
        }

        addURLsFromSitemap(cur_node->children, urls_sitemaps, urls_found, noColor);
    }
}

void XMLParsingErrorCallback(void * ctx, const char * msg, ...) {
    // silent libXML errors
}

URLNode_t* getSitemap(char *url, int noColor) {

    LIBXML_TEST_VERSION // tests for libxml2

    xmlSetGenericErrorFunc(NULL, XMLParsingErrorCallback); // disable errors

    xmlParserCtxtPtr ctxt; // used for chunk parsing
    xmlDocPtr doc;

    URLNode_t* list_sitemaps = NULL;
    URLNode_t* list_urls_found = NULL;
    pushURLStack(&list_sitemaps, url);

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sitemapXMLFetchCallback);

    while(!isURLStackEmpty(list_sitemaps)) {
        char* currentSitemapURL = popURLStack(&list_sitemaps);
        if(noColor) {
            fprintf(stderr, "Fetching sitemap: %s...\n", currentSitemapURL);
        } else {
            fprintf(stderr, "%sFetching sitemap: %s...%s\n", BLUE, currentSitemapURL, RESET);
        }

        ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);
        if(!ctxt) {
            if(noColor) {
                fprintf(stderr, "Failed to create parser context while doing %s\n", url);
            } else {
                fprintf(stderr, "%sFailed to create parser context while doing %s%s\n", RED, url, RESET);
            }
            return NULL;
        }

        curl_easy_setopt(curl, CURLOPT_URL, currentSitemapURL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &ctxt);
        CURLcode status = curl_easy_perform(curl);
        if(status != CURLE_OK) {
            if(noColor) {
                fprintf(stderr, "Failed to fetch sitemap: %s\n", currentSitemapURL);
            } else {
                fprintf(stderr, "%sFailed to fetch sitemap: %s%s\n", RED, currentSitemapURL, RESET);
            }
            xmlFreeParserCtxt(ctxt);
            continue;
        }
        long status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        if(status_code != 200) {
            if(noColor) {
                fprintf(stderr, "%s doesn't exist.\n", currentSitemapURL);
            } else {
                fprintf(stderr, "%s%s doesn't exist.%s\n", RED, currentSitemapURL, RESET);
            }
            xmlFreeParserCtxt(ctxt);
            continue;
        }

        xmlParseChunk(ctxt, NULL, 0, 1); // indicate the end of chunk parsing

        doc = ctxt->myDoc; // get the final document
        int res = ctxt->wellFormed;
        xmlFreeParserCtxt(ctxt);

        if (!res) {
            if(noColor) {
                fprintf(stderr, "Failed to parse sitemap %s\n", url);
            } else {
                fprintf(stderr, "%sFailed to parse sitemap %s%s\n", RED, url, RESET);
            }
            continue;
        }

        xmlNode* root_element = xmlDocGetRootElement(doc);

        addURLsFromSitemap(root_element, &list_sitemaps, &list_urls_found, noColor);

        xmlFreeDoc(doc);
    }

    xmlCleanupParser();
    xmlMemoryDump();

    return list_urls_found;
}