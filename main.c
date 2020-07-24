#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fetcher_thread.h"
#include "linked_list_documents.h"
#include "linked_list_urls.h"
#include "utils.h"

#include "cmdline.h"

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "libcurl 7.62.0 or later is required"
#endif

struct WalkBundle {  // Used in walk_cb.
    struct Document* document;
    URLNode_t** urls_todo;
    URLNode_t** urls_done;
    int allowSubdomains;
    char** allowedDomains;
    int countAllowedDomains;
    int httpOnly;
    int httpsOnly;
};

lexbor_action_t walk_cb(lxb_dom_node_t* node, void* ctx) {
    if(node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return LEXBOR_ACTION_OK;
    }

    lxb_dom_element_t* element = lxb_dom_interface_element(node);
    if(!lxb_dom_element_has_attribute(element, (lxb_char_t*) "href", 4) && !lxb_dom_element_has_attribute(element, (lxb_char_t*) "src", 3)) {
        return LEXBOR_ACTION_OK;
    }

    struct WalkBundle* bundle = (struct WalkBundle*) ctx;

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

    // URL that will be added to the list of URLs to fetch
    char* finalURL;

    char* scheme;

    CURLU* curl_u = curl_url();
    curl_url_set(curl_u, CURLUPART_URL, bundle->document->url, 0);
    curl_url_get(curl_u, CURLUPART_HOST, &documentDomain, 0);
    if(isValidLink((char*) foundURL)) {
        curl_url_set(curl_u, CURLUPART_URL, (char*) foundURL, 0);  // curl will change the url by himself based on the document's URL
        curl_url_set(curl_u, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

        curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);
        if(bundle->httpOnly && strcmp("http", scheme) != 0) {
            free(scheme);
            free(documentDomain);
            return LEXBOR_ACTION_OK;
        } else if(bundle->httpsOnly && strcmp("https", scheme) != 0) {
            free(scheme);
            free(documentDomain);
            return LEXBOR_ACTION_OK;
        }

        curl_url_get(curl_u, CURLUPART_URL, &finalURL, 0);  // get final url
        curl_url_get(curl_u, CURLUPART_HOST, &foundURLDomain, 0);  // get the domain of the URL

        if(canBeAdded(finalURL, *(bundle->urls_done), *(bundle->urls_todo))) {

            if(isValidDomain(foundURLDomain, documentDomain, bundle->allowSubdomains)) {
                pushURLList(bundle->urls_todo, finalURL);
                hasBeenAdded = 1;
            } else {
                // check if it is an allowed domain
                for(int i = 0; i < bundle->countAllowedDomains; i++) {
                    if(isValidDomain(foundURLDomain, bundle->allowedDomains[i], bundle->allowSubdomains)) {
                        pushURLList(bundle->urls_todo, finalURL);
                        hasBeenAdded = 1;
                        break;
                    }
                }
            }
        }
        if(!hasBeenAdded) {
            free(finalURL);
        }
        free(foundURLDomain);
    }
    free(documentDomain);
    curl_url_cleanup(curl_u);
    return LEXBOR_ACTION_OK;
}

static void lock_cb(CURL* handle, curl_lock_data data, curl_lock_access access, void* userptr) {
    pthread_mutex_lock((pthread_mutex_t*) userptr);
}

static void unlock_cb(CURL* handle, curl_lock_data data, void* userptr) {
    pthread_mutex_unlock((pthread_mutex_t*) userptr);
}

int main(int argc, char* argv[]) {
    struct gengetopt_args_info args_info;
    if(cmdline_parser(argc, argv, &args_info) != 0) {
        exit(1);
    }
    if((strncmp(args_info.url_arg, "http://", 7) != 0 && strncmp(args_info.url_arg, "https://", 8) != 0) || strchr(args_info.url_arg, ' ') != NULL) {
        fprintf(stderr, "%s: invalid URL: %s\n", argv[0], args_info.url_arg);
        exit(1);
    }
    if(args_info.threads_arg <= 0) {
        fprintf(stderr, "%s: the number of threads should be positive\n", argv[0]);
        exit(1);
    }
    if(args_info.max_document_size_arg <= 0L) {
        fprintf(stderr, "%s: the max size of a document should be positive\n", argv[0]);
        exit(1);
    }
    if(args_info.retries_arg <= 0) {
        fprintf(stderr, "%s: the maximum number of retries should be positive\n", argv[0]);
        exit(1);
    }
    if(args_info.timeout_arg <= 0) {
        fprintf(stderr, "%s: the timeout should be positive\n", argv[0]);
        exit(1);
    }
    pthread_t fetcher_threads[args_info.threads_arg];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    DocumentNode_t* documents = NULL;

    URLNode_t* urls_todo = NULL;
    URLNode_t* urls_done = NULL;
    pushURLList(&urls_todo, args_info.url_arg);

    curl_global_init(CURL_GLOBAL_ALL);

    CURLSH* curl_share = curl_share_init();
    if(curl_share == NULL) {
        fprintf(stderr, "curl_share_init() failed. Quitting...");
        curl_global_cleanup();
        return 1;
    }

    pthread_mutex_t mutex_conn = PTHREAD_MUTEX_INITIALIZER;
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(curl_share, CURLSHOPT_LOCKFUNC, lock_cb);
    curl_share_setopt(curl_share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
    curl_share_setopt(curl_share, CURLSHOPT_USERDATA, (void*) &mutex_conn);

    int shouldExit = 0;  // if threads should exit, set to 1 when all threads have isRunning == 1
    int* listRunningThreads = (int*) malloc(sizeof(int) * args_info.threads_arg);
    struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof(struct BundleVarsThread) * args_info.threads_arg);
    for(int i = 0; i < args_info.threads_arg; i++) {
        listRunningThreads[i] = 1;

        bundles[i].documents = &documents;
        bundles[i].urls_todo = &urls_todo;
        bundles[i].urls_done = &urls_done;
        bundles[i].mutex = &mutex;
        bundles[i].shouldExit = &shouldExit;
        bundles[i].isRunning = &(listRunningThreads[i]);
        bundles[i].maxRetries = args_info.retries_arg;
        bundles[i].timeout = args_info.timeout_arg;
        bundles[i].maxFileSize = args_info.max_document_size_arg;
        bundles[i].curl_share = curl_share;
        pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*) &(bundles[i]));
    }

    struct Document* currentDocument;
    CURLU* curl_u = curl_url();  // used when handling redirections

    struct WalkBundle bundleWalk;  // in this bundle these elements never change
    bundleWalk.allowedDomains = args_info.allowed_domains_arg;
    bundleWalk.countAllowedDomains = args_info.allowed_domains_given;
    bundleWalk.allowSubdomains = args_info.allow_subdomains_given;
    bundleWalk.httpOnly = args_info.http_only_given;
    bundleWalk.httpsOnly = args_info.https_only_given;
    while(1) {
        pthread_mutex_lock(&mutex);
        if(getDocumentListLength(documents) == 0) {  // no documents to parse
            // if this thread has no document to parse and all threads are waiting for urls,
            // then it means that everything was discovered and they should quit,
            // otherwise just continue to check for something to do
            int shouldQuit = 1;
            for(int i = 0; i < args_info.threads_arg; i++) {  // check if all threads are running
                if(listRunningThreads[i] == 1) {  // if one is running
                    shouldQuit = 0;  // should not quit
                    break;  // don't need to check other threads
                }
            }
            if(shouldQuit == 1 && isURLListEmpty(urls_todo)) {  // if no threads are running and no urls to fetch left
                // quit
                shouldExit = 1;
                pthread_mutex_unlock(&mutex);
                break;  // quit parsing
            }
            // if some thread(s) are running, just continue to check for a document to come
            pthread_mutex_unlock(&mutex);
            continue;
        }

        currentDocument = popDocumentList(&documents);
        pthread_mutex_unlock(&mutex);

        printf("%s (%lu) %s\n", currentDocument->url, currentDocument->status_code_http, currentDocument->content_type);

        if(currentDocument->content_type != NULL) {  // sometimes, the server doesn't send a content-type header
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
            int hasBeenAdded = 0;

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->redirect_location, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &redirectLocationDomain, 0);

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->url, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &currentDocumentURLDomain, 0);

            pthread_mutex_lock(&mutex);
            if(isValidLink((char*) currentDocument->redirect_location)) {
                curl_url_set(curl_u, CURLUPART_URL, currentDocument->redirect_location, 0);  // curl will change the url by himself based on the document's URL
                curl_url_set(curl_u, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

                curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);
                if(args_info.http_only_given && strcmp("http", scheme) != 0) {
                    free(scheme);
                    free(redirectLocationDomain);
                    free(currentDocumentURLDomain);
                    return LEXBOR_ACTION_OK;
                } else if(args_info.https_only_given && strcmp("https", scheme) != 0) {
                    free(scheme);
                    free(redirectLocationDomain);
                    free(currentDocumentURLDomain);
                    return LEXBOR_ACTION_OK;
                }

                if(canBeAdded(currentDocument->redirect_location, urls_done, urls_todo)) {
                    if(isValidDomain(redirectLocationDomain, currentDocumentURLDomain, args_info.allow_subdomains_flag)) {
                        pushURLList(&urls_todo, currentDocument->redirect_location);
                        hasBeenAdded = 1;
                    } else {
                        // check if it is an allowed domain
                        for(int i = 0; i < args_info.allowed_domains_given; i++) {
                            if(isValidDomain(redirectLocationDomain, args_info.allowed_domains_arg[i], args_info.allow_subdomains_flag)) {
                                pushURLList(&urls_todo, currentDocument->redirect_location);
                                hasBeenAdded = 1;
                                break;
                            }
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
    free(listRunningThreads);

    while(getURLListLength(urls_done) != 1) {  // all urls are allocated by curl when parsing url, except the initial url
        char* url_done = popURLList(&urls_done);
        free(url_done);
    }

    cmdline_parser_free(&args_info);
    curl_share_cleanup(curl_share);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}
