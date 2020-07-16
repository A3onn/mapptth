#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fetcher_thread.h"
#include "linked_list_documents.h"
#include "linked_list_urls.h"

#include "cmdline.h"

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "libcurl 7.62.0 or later is required"
#endif

int canBeAdded(char* url, URLNode_t* urls_done, URLNode_t* urls_todo) {
    // Just check if a given url has already been seen.
    // Could just be:
    //  return !findURLList(*urls_done, urlFinal) && !findURLList(*urls_done, urlFinal)
    // but it would be longer as both findURLList are called every time
    if(findURLList(urls_done, url)) {
        return 0;
    }
    if(findURLList(urls_todo, url)) {
        return 0;
    }
    return 1;
}

int isValidDomain(char* domainToCompare, char* domain, int canBeSubDomain) {
    int strlen_domainToCompare = strlen(domainToCompare), strlen_domain = strlen(domain);

    if(strlen_domain == strlen_domainToCompare) {
        return strcmp(domainToCompare, domain) == 0;
    } else if(strlen_domain < strlen_domainToCompare && canBeSubDomain == 1) {
        char* foundPos = strstr(domainToCompare, domain);
        if(foundPos != NULL) {
            // need to check if the char before is a '.' (dot) because for exemple:
            // domainToCompare = "xyzb.a" and domain = "b.a" would result in true because
            // "b.a" is in both strings
            return strcmp(foundPos, domain) == 0 && *(foundPos - 1) == '.';
        }
    }
    return 0;
}

void getLinks(lxb_html_document_t* document, char* url, URLNode_t** urls_todo, URLNode_t** urls_done, pthread_mutex_t* mutex, int allowSubDomains) {
    lxb_status_t status;

    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 16);
    if(collection == NULL) {
        fprintf(stderr, "lxb_dom_collection_make(&document->dom_document, 16) failed.");
        return;
    }

    // HEAD
    lxb_dom_element_t* head = lxb_dom_interface_element(document->head);
    if(head == NULL) {
        fprintf(stderr, "lxb_dom_interface_element failed.");
        return;
    }

    status = lxb_dom_elements_by_attr_contain(head, collection, (const lxb_char_t*) "href", 4, NULL, 0, true);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
        return;
    }
    status = lxb_dom_elements_by_attr_contain(head, collection, (const lxb_char_t*) "src", 3, NULL, 0, true);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
        return;
    }

    // BODY
    lxb_dom_element_t* body = lxb_dom_interface_element(document->body);
    if(body == NULL) {
        fprintf(stderr, "lxb_dom_interface_element failed.");
        return;
    }

    status = lxb_dom_elements_by_attr_contain(body, collection, (const lxb_char_t*) "href", 4, NULL, 0, true);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
        return;
    }
    status = lxb_dom_elements_by_attr_contain(body, collection, (const lxb_char_t*) "src", 3, NULL, 0, true);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
        return;
    }

    CURLU* baseURL = curl_url();  // will hold the final url, coupled with urlFinal
    curl_url_set(baseURL, CURLUPART_URL, url, 0);

    char* documentDomain;  // domain of this document, used when checking domains
    curl_url_get(baseURL, CURLUPART_HOST, &documentDomain, 0);

    lxb_dom_element_t* element;
    const lxb_char_t* foundURL;
    char* urlFinal;  // will hold the url to push into the list
    char* foundURLDomain;  // domain of the url found
    for(size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        element = lxb_dom_collection_element(collection, i);

        lxb_dom_node_t* node = lxb_dom_interface_node(element);
        foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL);  // try getting href
        if(foundURL == NULL) {
            foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "src", 3, NULL);  // getting src otherwise
            if(foundURL == NULL) {  // should not happend
                continue;
            }
        }

        if(foundURL[0] != '#' && strstr((const char*) foundURL, "mailto:") != (char*) foundURL && strstr((const char*) foundURL, "tel:") != (char*) foundURL) {  // if this is not a fragment of the same page, a mailto: or a tel: url
            // set url
            curl_url_set(baseURL, CURLUPART_URL, (char*) foundURL, 0);  // curl will change the url accordingly
            curl_url_set(baseURL, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

            curl_url_get(baseURL, CURLUPART_URL, &urlFinal, 0);  // get final url

            curl_url_get(baseURL, CURLUPART_HOST, &foundURLDomain, 0);  // get the domain of the URL

            pthread_mutex_lock(mutex);
            if(canBeAdded(urlFinal, *urls_done, *urls_todo) && isValidDomain(foundURLDomain, documentDomain, allowSubDomains)) {
                // add url to the list
                pushURLList(urls_todo, urlFinal);
            }
            pthread_mutex_unlock(mutex);
        }

        // re-set base URL because it was modified
        curl_url_set(baseURL, CURLUPART_URL, url, 0);
    }
    curl_url_cleanup(baseURL);
    lxb_dom_collection_destroy(collection, true);
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
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);

    int* listRunningThreads = (int*) malloc(sizeof(int) * args_info.threads_arg);
    struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof(struct BundleVarsThread) * args_info.threads_arg);
    for(int i = 0; i < args_info.threads_arg; i++) {
        listRunningThreads[i] = 1;

        bundles[i].documents = &documents;
        bundles[i].urls_todo = &urls_todo;
        bundles[i].urls_done = &urls_done;
        bundles[i].mutex = &mutex;
        bundles[i].isRunning = &(listRunningThreads[i]);
        bundles[i].maxRetries = args_info.retries_arg;
        bundles[i].timeout = args_info.timeout_arg;
        bundles[i].maxFileSize = args_info.max_document_size_arg;
        bundles[i].curl_share = curl_share;
        pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*) &(bundles[i]));
    }

    struct Document* currentDocument;
    CURLU* curl_u = curl_url();  // used when handling redirections
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
            if(shouldQuit == 1 && getURLListLength(urls_todo) == 0) {  // if no threads are running and no urls to fetch left
                // quit
                for(int i = 0; i < args_info.threads_arg; i++) {  // stop all threads
                    pthread_cancel(fetcher_threads[i]);
                }
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
            if(strstr(currentDocument->content_type, "text/html")) {
                getLinks(currentDocument->document, currentDocument->url, &urls_todo, &urls_done, &mutex, args_info.allow_subdomains_flag);
            }
            free(currentDocument->content_type);  // allocated by strdup
        }  // maybe check using libmagick if this is a html file if the server didn't specified it

        if(currentDocument->redirect_location != NULL) {
            // get the domain of the current document and the domain of the redirect URL

            char* currentDocumentURLDomain;
            char* redirectLocationDomain;

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->redirect_location, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &redirectLocationDomain, 0);

            curl_url_set(curl_u, CURLUPART_URL, currentDocument->url, 0);
            curl_url_get(curl_u, CURLUPART_HOST, &currentDocumentURLDomain, 0);

            pthread_mutex_lock(&mutex);
            if(canBeAdded(currentDocument->redirect_location, urls_done, urls_todo) && isValidDomain(redirectLocationDomain, currentDocumentURLDomain, args_info.allow_subdomains_flag) == 1) {
                pushURLList(&urls_todo, currentDocument->redirect_location);
            }
            pthread_mutex_unlock(&mutex);

            free(currentDocumentURLDomain);
            free(redirectLocationDomain);
        }

        lxb_html_document_destroy(currentDocument->document);
        free(currentDocument);
    }

    curl_url_cleanup(curl_u);

    // CLEANUP
    free(bundles);
    free(listRunningThreads);

    while(getURLListLength(urls_done) != 1) {  // all urls are allocated by curl when parsing url, except the initial url
        char* url_done = popURLList(&urls_done);
        free(url_done);
    }

    curl_share_cleanup(curl_share);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}
