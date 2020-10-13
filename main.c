#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fetcher_thread.h"
#include "sitemaps_parser.h"
#include "stack_documents.h"
#include "stack_urls.h"
#include "utils.h"

#include "cmdline.h"

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "libcurl 7.62.0 or later is required"
#endif

struct WalkBundle {  // used with walk_cb.
    struct Document* document;
    URLNode_t** urls_todo;
    URLNode_t** urls_done;
    int allowSubdomains;
    char** allowedDomains;
    int countAllowedDomains;
    char** allowedExtensions;
    int countAllowedExtensions;
    char** disallowedPaths;
    int countDisallowedPaths;
    int httpOnly;
    int httpsOnly;
    int keepQuery;
    int maxDepth;
};

lexbor_action_t walk_cb(lxb_dom_node_t* node, void* ctx) {
    if(node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return LEXBOR_ACTION_OK;
    }

    lxb_dom_element_t* element = lxb_dom_interface_element(node);
    // check if the element has a 'href' or a 'src' attribute
    if(!lxb_dom_element_has_attribute(element, (lxb_char_t*) "href", 4) && !lxb_dom_element_has_attribute(element, (lxb_char_t*) "src", 3)) {
        return LEXBOR_ACTION_OK;
    }

    struct WalkBundle* bundle = (struct WalkBundle*) ctx;

    // try to get the 'href' attribute if it has one, 'src' attribute if it
    // doesn't have an 'href' attribute instead note that the element has to have
    // either one as it has been checked just before
    char* foundURL;
    foundURL = (char*) lxb_dom_element_get_attribute(element, (lxb_char_t*) "href", 4, NULL);
    if(foundURL == NULL) {  // if this element has a src attribute instead
        foundURL = (char*) lxb_dom_element_get_attribute(element, (lxb_char_t*) "src", 3, NULL);
        if(foundURL == NULL) {  // should not happen
            return LEXBOR_ACTION_OK;
        }
    }

    char hasBeenAdded = 0;  // used to check if the URL has been added, if not it will be freed

    // used when checking for valid domain
    char* documentDomain;
    char* foundURLDomain;

    // URL that will be added to the stack of URLs to fetch
    char* finalURL;

    char* scheme;

    if(isValidLink((char*) foundURL)) {
        CURLU* curl_u = curl_url();
        curl_url_set(curl_u, CURLUPART_URL, bundle->document->url, 0);
        curl_url_get(curl_u, CURLUPART_HOST, &documentDomain, 0);

        curl_url_set(curl_u, CURLUPART_URL, (char*) foundURL, 0);  // curl will change the url by himself based on the document's URL
        curl_url_set(curl_u, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

        // check scheme
        curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);
        if((bundle->httpOnly && strcmp("http", scheme) != 0) || (bundle->httpsOnly && strcmp("https", scheme) != 0)) {
            free(scheme);
            return LEXBOR_ACTION_OK;
        }
        free(scheme);

        if(!bundle->keepQuery) {
            curl_url_set(curl_u, CURLUPART_QUERY, NULL, 0);
        }

        char* path;
        curl_url_get(curl_u, CURLUPART_PATH, &path, 0);

        // check disallowed paths
        if(isDisallowedPath(path, bundle->disallowedPaths, bundle->countDisallowedPaths)) {
            free(path);
            return LEXBOR_ACTION_OK;
        }
        // check allowed extensions
        if(!isAllowedExtension(path, bundle->allowedExtensions, bundle->countAllowedExtensions)) {
            free(path);
            return LEXBOR_ACTION_OK;
        }
        if(bundle->maxDepth > 0 && pathDepth(path) > bundle->maxDepth) {
            free(path);
            return LEXBOR_ACTION_OK;
        }
        free(path);

        curl_url_get(curl_u, CURLUPART_URL, &finalURL, 0);  // get final url
        if(canBeAdded(finalURL, *(bundle->urls_done), *(bundle->urls_todo))) {
            curl_url_get(curl_u, CURLUPART_HOST, &foundURLDomain, 0);  // get the domain of the URL found

            if(isValidDomain(foundURLDomain, documentDomain, bundle->allowSubdomains) || isInValidDomains(foundURLDomain, bundle->allowedDomains, bundle->countAllowedDomains, bundle->allowSubdomains)) {
                pushURLStack(bundle->urls_todo, finalURL);
                hasBeenAdded = 1;
            }
            free(foundURLDomain);
        }
        if(!hasBeenAdded) {
            free(finalURL);
        }
        free(documentDomain);
        curl_url_cleanup(curl_u);
    }
    return LEXBOR_ACTION_OK;
}

static void lock_cb(CURL* handle, curl_lock_data data, curl_lock_access access, void* userptr) {
    pthread_mutex_lock((pthread_mutex_t*) userptr);
}

static void unlock_cb(CURL* handle, curl_lock_data data, void* userptr) {
    pthread_mutex_unlock((pthread_mutex_t*) userptr);
}

int main(int argc, char* argv[]) {
    printf("\n _______             ______ _______ _______ _     _ \n"
           "(_______)           (_____ (_______|_______|_)   (_)\n"
           " _  _  _ _____ ____  _____) )  _       _    _______ \n"
           "| ||_|| (____ |  _ \\|  ____/  | |     | |  |  ___  |\n"
           "| |   | / ___ | |_| | |       | |     | |  | |   | |\n"
           "|_|   |_\\_____|  __/|_|       |_|     |_|  |_|   |_|\n"
           "              |_|                                   \n"
           "Version %s\n\n",
        CMDLINE_PARSER_VERSION);
    struct gengetopt_args_info args_info;
    if(cmdline_parser(argc, argv, &args_info) != 0) {
        return 1;
    }
    if((strncmp(args_info.url_arg, "http://", 7) != 0 && strncmp(args_info.url_arg, "https://", 8) != 0) || strchr(args_info.url_arg, ' ') != NULL) {
        fprintf(stderr, "%s: invalid URL: %s\n", argv[0], args_info.url_arg);
        return 1;
    }
    if(args_info.threads_arg <= 0) {
        fprintf(stderr, "%s: the number of threads should be positive\n", argv[0]);
        return 1;
    }
    if(args_info.max_document_size_arg <= 0L) {
        fprintf(stderr, "%s: the max size of a document should be positive\n", argv[0]);
        return 1;
    }
    if(args_info.retries_arg <= 0) {
        fprintf(stderr, "%s: the maximum number of retries should be positive\n", argv[0]);
        return 1;
    }
    if(args_info.timeout_arg <= 0) {
        fprintf(stderr, "%s: the timeout should be positive\n", argv[0]);
        return 1;
    }
    if(args_info.max_depth_given && args_info.max_depth_arg <= 0) {
        fprintf(stderr, "%s: max-depth have to be positive\n", argv[0]);
        return 1;
    }
    for(int i = 0; i < args_info.allowed_extensions_given; i++) {
        if(args_info.allowed_extensions_arg[i][0] != '.') {
            fprintf(stderr, "%s: extensions have to begin with a '.' (dot)\n", argv[0]);
            return 1;
        }
    }
    // normalize paths given by the user
    char** disallowed_paths = (char**) malloc(sizeof(char*) * args_info.disallowed_paths_given);
    for(int i = 0; i < args_info.disallowed_paths_given; i++) {
        disallowed_paths[i] = normalizePath(args_info.disallowed_paths_arg[i], 1);
    }

    int resolve_ip_version = CURL_IPRESOLVE_WHATEVER;
    if(args_info.IPv4_given) {
        resolve_ip_version = CURL_IPRESOLVE_V4;
    } else if(args_info.IPv6_given) {
        resolve_ip_version = CURL_IPRESOLVE_V6;
    }

    pthread_t fetcher_threads[args_info.threads_arg];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    DocumentNode_t* documents = NULL;

    URLNode_t* urls_todo = NULL;
    URLNode_t* urls_done = NULL;

    curl_global_init(CURL_GLOBAL_ALL);  // initialize libcurl

    if(args_info.sitemap_given) {
        // get the content of the sitemap
        URLNode_t* urlsFromSitemap = getSitemap(args_info.sitemap_arg, args_info.no_color_flag);

        // validate the URLs found in the sitemap, same code as when finding an URL
        // in a document
        CURLU* curl_u = curl_url();
        int count = 0;  // keep track of how many validated URLs were added
        while(!isURLStackEmpty(urlsFromSitemap)) {  // loop over URLs
            char* url = popURLStack(&urlsFromSitemap);
            curl_url_set(curl_u, CURLUPART_URL, url, 0);

            if(isValidLink(url)) {
                if(!args_info.keep_query_flag) {
                    curl_url_set(curl_u, CURLUPART_QUERY, NULL, 0);
                }

                char* path;
                curl_url_get(curl_u, CURLUPART_PATH, &path, 0);
                // check disallowed paths
                if(isDisallowedPath(path, args_info.disallowed_paths_arg, args_info.disallowed_paths_given)) {
                    free(url);
                    free(path);
                    continue;
                }
                // check allowed extensions
                if(!isAllowedExtension(path, args_info.allowed_extensions_arg, args_info.allowed_extensions_given)) {
                    free(url);
                    free(path);
                    continue;
                }
                if(args_info.max_depth_given && pathDepth(path) > args_info.max_depth_given) {
                    free(url);
                    free(path);
                    continue;
                }
                free(path);

                if(canBeAdded(url, urls_done, urls_todo)) {
                    char* domainURLFound;
                    curl_url_get(curl_u, CURLUPART_HOST, &domainURLFound, 0);  // get the domain of the URL

                    char* domainInitialURL;  // URL specified by -u
                    curl_url_set(curl_u, CURLUPART_URL, args_info.url_arg, 0);
                    curl_url_get(curl_u, CURLUPART_HOST, &domainInitialURL, 0);

                    // check domains
                    if(isValidDomain(domainURLFound, domainInitialURL, args_info.allow_subdomains_flag) || isInValidDomains(domainURLFound, args_info.allowed_domains_arg, args_info.allowed_domains_given, args_info.allow_subdomains_flag)) {
                        pushURLStack(&urls_todo, url);
                        count++;
                    } else {
                        free(url);
                    }
                    free(domainURLFound);
                    free(domainInitialURL);
                }
            } else {
                free(url);
            }
        }
        curl_url_cleanup(curl_u);
        printf("Added %i new URLs.\n", count);
    }

    pushURLStack(&urls_todo, args_info.url_arg);  // add the URL specified by -u

    // create shared interface
    CURLSH* curl_share = curl_share_init();
    if(curl_share == NULL) {
        fprintf(stderr, "Could not create share interface. Quitting...");
        curl_global_cleanup();
        return 1;
    }

    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(curl_share, CURLSHOPT_LOCKFUNC, lock_cb);
    curl_share_setopt(curl_share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
    pthread_mutex_t mutex_conn = PTHREAD_MUTEX_INITIALIZER;
    curl_share_setopt(curl_share, CURLSHOPT_USERDATA, (void*) &mutex_conn);

    int shouldExit = 0;  // if threads should exit, set to 1 when all threads have isRunning == 1
    int* isRunningThreadsList = (int*) malloc(sizeof(int) * args_info.threads_arg);  // list containing ints indicating if each thread is fetching
    struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof(struct BundleVarsThread) * args_info.threads_arg);
    for(int i = 0; i < args_info.threads_arg; i++) {
        isRunningThreadsList[i] = 1;

        bundles[i].documents = &documents;
        bundles[i].urls_todo = &urls_todo;
        bundles[i].urls_done = &urls_done;
        bundles[i].mutex = &mutex;
        bundles[i].shouldExit = &shouldExit;
        bundles[i].isRunning = &(isRunningThreadsList[i]);
        bundles[i].maxRetries = args_info.retries_arg;
        bundles[i].timeout = args_info.timeout_arg;
        bundles[i].maxFileSize = args_info.max_document_size_arg;
        bundles[i].resolve_ip_versions = resolve_ip_version;
        bundles[i].noColor = args_info.no_color_given;
        bundles[i].curl_share = curl_share;
        if(args_info.user_agent_given) {
            bundles[i].userAgent = args_info.user_agent_arg;
        } else {
            bundles[i].userAgent = "MapPTTH/" CMDLINE_PARSER_VERSION;
        }
        pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*) &(bundles[i]));
    }

    struct Document* currentDocument;
    CURLU* curl_u = curl_url();  // used when handling redirections

    struct WalkBundle bundleWalk;  // in this bundle these elements never change
    bundleWalk.allowedDomains = args_info.allowed_domains_arg;
    bundleWalk.countAllowedDomains = args_info.allowed_domains_given;
    bundleWalk.allowedExtensions = args_info.allowed_extensions_arg;
    bundleWalk.countAllowedExtensions = args_info.allowed_extensions_given;
    bundleWalk.allowSubdomains = args_info.allow_subdomains_given;
    bundleWalk.httpOnly = args_info.http_only_given;
    bundleWalk.httpsOnly = args_info.https_only_given;
    bundleWalk.disallowedPaths = disallowed_paths;
    bundleWalk.countDisallowedPaths = args_info.disallowed_paths_given;
    bundleWalk.keepQuery = args_info.keep_query_given;
    bundleWalk.maxDepth = args_info.max_depth_arg;

    while(1) {
        pthread_mutex_lock(&mutex);
        if(getDocumentStackLength(documents) == 0) {  // no documents to parse
            // if this thread has no document to parse and all threads are waiting for
            // urls, then it means that everything was discovered and they should
            // quit, otherwise just continue to check for something to do
            int shouldQuit = 1;
            for(int i = 0; i < args_info.threads_arg; i++) {  // check if all threads are running
                if(isRunningThreadsList[i] == 1) {  // if one is running
                    shouldQuit = 0;  // should not quit
                    break;  // don't need to check other threads
                }
            }
            if(shouldQuit == 1 && isURLStackEmpty(urls_todo)) {  // if no threads are running and no urls to fetch left
                // quit
                shouldExit = 1;
                pthread_mutex_unlock(&mutex);
                break;  // quit parsing
            }
            // if some thread(s) are running, just continue to check for a document to come
            pthread_mutex_unlock(&mutex);
            continue;
        }

        currentDocument = popDocumentStack(&documents);
        pthread_mutex_unlock(&mutex);

        int httpStatusCat = currentDocument->status_code_http / 100;
        if(!args_info.no_color_given) {
            char* color;
            switch(httpStatusCat) {
            case 5:  // 5xx
                color = RED;
                break;
            case 4:  // 4xx
                color = MAGENTA;
                break;
            case 3:  // 3xx
                color = YELLOW;
                break;
            case 2:  // 2xx
                color = GREEN;
                break;
            case 1:  // 1xx
                color = CYAN;
                break;
            }
            if(httpStatusCat == 3) {
                printf("[%s%lu%s] %s -> %s [%s] [%zu]\n", color,
                    currentDocument->status_code_http, RESET, currentDocument->url,
                    currentDocument->redirect_location,
                    currentDocument->content_type, currentDocument->size);
            } else {
                printf("[%s%lu%s] %s [%s] [%zu]\n", color,
                    currentDocument->status_code_http, RESET, currentDocument->url,
                    currentDocument->content_type, currentDocument->size);
            }
        } else {
            if(httpStatusCat == 3) {
                printf("[%lu] %s -> %s [%s] [%zu]\n", currentDocument->status_code_http,
                    currentDocument->url, currentDocument->redirect_location,
                    currentDocument->content_type, currentDocument->size);
            } else {
                printf("[%lu] %s [%s] [%zu]\n", currentDocument->status_code_http,
                    currentDocument->url, currentDocument->content_type,
                    currentDocument->size);
            }
        }

        if(currentDocument->content_type != NULL) {  // sometimes, the server doesn't send a content-type header
            // parse only html and xhtml files
            if(strstr(currentDocument->content_type, "text/html") != NULL || strstr(currentDocument->content_type, "application/xhtml+xml") != NULL) {
                bundleWalk.document = currentDocument;
                pthread_mutex_lock(&mutex);
                bundleWalk.urls_done = &urls_done;
                bundleWalk.urls_todo = &urls_todo;
                if(currentDocument->document->head && !args_info.only_body_given) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(currentDocument->document->head), walk_cb, &bundleWalk);
                }
                if(currentDocument->document->body && !args_info.only_head_given) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(currentDocument->document->body), walk_cb, &bundleWalk);
                }
                pthread_mutex_unlock(&mutex);
            }
            free(currentDocument->content_type);  // allocated by strdup
        }  // maybe check using libmagick if this is a html file if the server didn't specified it

        if(currentDocument->redirect_location != NULL) {
            // get the domain of the current document and the domain of the redirect URL
            char* currentDocumentURLDomain;
            char* redirectLocationDomain;
            char* scheme;

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->redirect_location, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &redirectLocationDomain, 0);

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->url, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &currentDocumentURLDomain, 0);

            pthread_mutex_lock(&mutex);
            if(isValidLink((char*) currentDocument->redirect_location)) {
                curl_url_set(curl_u, CURLUPART_URL, currentDocument->redirect_location, 0);  // curl will change the url by himself based on the document's URL
                curl_url_set(curl_u, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

                curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);

                int isStillValid = 1;  // indicates if it is a valid URL through the checks, if not then it is useless to do checks anymore

                if((args_info.http_only_given && strcmp("http", scheme) != 0) || (args_info.https_only_given && strcmp("https", scheme) != 0)) {
                    isStillValid = 0;
                }
                free(scheme);

                char* path;
                curl_url_get(curl_u, CURLUPART_PATH, &path, 0);
                if(isStillValid) {
                    if(isDisallowedPath(path, args_info.disallowed_paths_arg, args_info.disallowed_paths_given)) {
                        isStillValid = 0;
                    }
                }
                if(isStillValid) {
                    if(!isAllowedExtension(path, args_info.allowed_extensions_arg, args_info.allowed_extensions_given)) {
                        isStillValid = 0;
                    }
                }
                if(isStillValid) {
                    if(args_info.max_depth_given && pathDepth(path) > args_info.max_depth_given) {
                        isStillValid = 0;
                    }
                }
                free(path);

                int hasBeenAdded = 0;  // used to check if the URL has been added, if not it will be freed
                if(isStillValid) {
                    if(canBeAdded(currentDocument->redirect_location, urls_done, urls_todo)) {
                        if(isValidDomain(redirectLocationDomain, currentDocumentURLDomain, args_info.allow_subdomains_flag) ||
                                isInValidDomains(redirectLocationDomain, args_info.allowed_domains_arg, args_info.allowed_domains_given, args_info.allow_subdomains_flag)) {
                            pushURLStack(&urls_todo, currentDocument->redirect_location);
                            hasBeenAdded = 1;
                        }
                    }
                }
                if(!hasBeenAdded) {
                    free(currentDocument->redirect_location);
                }
            }
            pthread_mutex_unlock(&mutex);

            free(currentDocumentURLDomain);
            free(redirectLocationDomain);
        }

        lxb_html_document_destroy(currentDocument->document);
        free(currentDocument);
    }
    for(int i = 0; i < args_info.threads_arg; i++) {
        pthread_join(fetcher_threads[i], NULL);
    }

    curl_url_cleanup(curl_u);

    // CLEANUP
    free(bundles);
    free(isRunningThreadsList);

    while(getURLStackLength(urls_done) != 1) {  // all urls are allocated by curl when parsing url, except the initial url
        char* url_done = popURLStack(&urls_done);
        free(url_done);
    }

    for(int i = 0; i < args_info.disallowed_paths_given; i++) {
        free(disallowed_paths[i]);
    }

    cmdline_parser_free(&args_info);
    curl_share_cleanup(curl_share);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}
