#include "fetcher_thread.h"

struct ProcessContentBundle {
    lxb_html_document_t* document;
    size_t size;  // size of the document without the headers
    // using this variable to have the size of the document
    // even if the server doesn't send it
};

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp) {
    // called when getting data
    // passing the data received to the document using chunk parsing
    struct ProcessContentBundle* doc = (struct ProcessContentBundle*) userp;
    lxb_status_t status = lxb_html_document_parse_chunk(doc->document, (lxb_char_t*) content, size * nmemb);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "An error occured while parsing...\n");
        return 0; // indicate libcurl that we didn't parse anything, meaning an error occurred
    }
    doc->size += size * nmemb;

    return size * nmemb;
}

void* fetcher_thread_func(void* bundle_arg) {
    struct BundleVarsThread* bundle = (struct BundleVarsThread*) bundle_arg;
    DocumentNode_t** documents = bundle->documents;
    URLNode_t** urls_todo = bundle->urls_todo;
    URLNode_t** urls_done = bundle->urls_done;
    pthread_mutex_t* mutex = bundle->mutex;
    int* isRunning = bundle->isRunning;
    int* shouldExit = bundle->shouldExit;
    int maxRetries = bundle->maxRetries;
    int timeout = bundle->timeout;
    long maxFileSize = bundle->maxFileSize;

    CURLcode status_c;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, processContent);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, bundle->resolve_ip_versions);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, maxFileSize);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, bundle->userAgent);
    curl_easy_setopt(curl, CURLOPT_SHARE, bundle->curl_share);

    lxb_status_t status_l;
    struct ProcessContentBundle* currentDocument = (struct ProcessContentBundle*) malloc(sizeof(struct ProcessContentBundle));
    char* currentURL;
    char* content_type;
    char* redirect_location;
    long status_code_http;

    while(*shouldExit == 0) {
        pthread_mutex_lock(mutex);
        if(isURLStackEmpty(*urls_todo)) {  // no url to fetch
            *isRunning = 0;  // change state
            pthread_mutex_unlock(mutex);
            continue;
        }
        *isRunning = 1;
        currentURL = popURLStack(urls_todo);
        pushURLStack(urls_done, currentURL);
        pthread_mutex_unlock(mutex);

        currentDocument->document = lxb_html_document_create();
        currentDocument->size = 0L;
        if(currentDocument->document == NULL) {
            fprintf(stderr, "lxb_html_document_create failed for %s. Not doing it.\n", currentURL);
            continue;
        }

        status_c = curl_easy_setopt(curl, CURLOPT_URL, currentURL);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for URL failed for %s. Not doing it. Error: %s.\n", currentURL, curl_easy_strerror(status_c));
            lxb_html_document_destroy(currentDocument->document);
            continue;
        }
        status_c = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) currentDocument);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for setting the document to write in failed for %s. Not doing it. Error: %s.\n", currentURL, curl_easy_strerror(status_c));
            lxb_html_document_destroy(currentDocument->document);
            continue;
        }

        // fetch
        lxb_html_document_parse_chunk_begin(currentDocument->document);

        int countRetries = 0;
        do {
            status_c = curl_easy_perform(curl);
            countRetries += 1;
        } while(countRetries < maxRetries && status_c != CURLE_FILESIZE_EXCEEDED && status_c != CURLE_OK);
        if(status_c != CURLE_OK && countRetries == maxRetries) {
            if(!bundle->noColor) {
                fprintf(stderr, "%s%s : Max retries exceeded. Last error was: %s.%s\n", BRIGHT_RED, currentURL, curl_easy_strerror(status_c), RESET);
            } else {
                fprintf(stderr, "%s : Max retries exceeded. Last error was: %s.\n", currentURL, curl_easy_strerror(status_c));
            }
            lxb_html_document_parse_chunk_end(currentDocument->document);
            lxb_html_document_destroy(currentDocument->document);
            continue;
        } else if(status_c == CURLE_FILESIZE_EXCEEDED) {
            if(!bundle->noColor) {
                fprintf(stderr, "%s%s : File size limit exceeded.%s\n", BRIGHT_RED, currentURL, RESET);
            } else {
                fprintf(stderr, "%s : File size limit exceeded.\n", currentURL);
            }
        }

        status_l = lxb_html_document_parse_chunk_end(currentDocument->document);

        status_c = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code_http);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the HTTP status code failed for %s. Setting it to 0. Error: %s.\n", currentURL, curl_easy_strerror(status_c));
            status_code_http = 0L;
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the content type failed for %s. Error: %s.\n", currentURL, curl_easy_strerror(status_c));
            status_code_http = 0L;
        } else if(content_type != NULL) {
            // content_type is pointing to curl's private memory
            // and it's value will change when another curl_easy_perform will occur.
            content_type = strdup(content_type);
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_location);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the redirect URL failed for %s. Error: %s.\n", currentURL, curl_easy_strerror(status_c));
        } else if(redirect_location != NULL) {  // if there is a location header
            redirect_location = strdup(redirect_location);
        }

        pthread_mutex_lock(mutex);
        pushDocumentStack(documents, currentDocument->document, currentURL, status_code_http, currentDocument->size, content_type, redirect_location);
        pthread_mutex_unlock(mutex);
    }
    curl_easy_cleanup(curl);
    free(currentDocument);

    return NULL;
}