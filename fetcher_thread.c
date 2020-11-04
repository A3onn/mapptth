#include "fetcher_thread.h"

struct ProcessContentBundle {
    lxb_html_document_t* document;
    size_t size;  // size of the document without the headers
    // using this variable to have the size of the document
    // even if the server doesn't send it
};

static size_t __fetcher_content_callback(const char* content, size_t size, size_t nmemb, void* userp) {
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
    URLNode_t** urls_stack_todo = bundle->urls_stack_todo;
    URLNode_t** urls_stack_done = bundle->urls_stack_done;
    pthread_mutex_t* mutex = bundle->mutex;
    int* is_running = bundle->is_running;
    int* should_exit = bundle->should_exit;
    int max_retries = bundle->max_retries;
    int timeout = bundle->timeout;
    long max_file_size = bundle->max_file_size;

    CURLcode status_c;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __fetcher_content_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, bundle->resolve_ip_versions);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, max_file_size);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, bundle->user_agent);
    curl_easy_setopt(curl, CURLOPT_SHARE, bundle->curl_share);

    lxb_status_t status_l;
    struct ProcessContentBundle* current_document = (struct ProcessContentBundle*) malloc(sizeof(struct ProcessContentBundle));
    char* current_url;
    char* content_type;
    char* redirect_location;
    long status_code_http;

    while(*should_exit == 0) {
        pthread_mutex_lock(mutex);
        if(stack_url_isempty(*urls_stack_todo)) {  // no url to fetch
            *is_running = 0;  // change state
            pthread_mutex_unlock(mutex);
            continue;
        }
        *is_running = 1;
        current_url = stack_url_pop(urls_stack_todo);
        stack_url_push(urls_stack_done, current_url);
        pthread_mutex_unlock(mutex);

        current_document->document = lxb_html_document_create();
        current_document->size = 0L;
        if(current_document->document == NULL) {
            fprintf(stderr, "lxb_html_document_create failed for %s. Not doing it.\n", current_url);
            continue;
        }

        status_c = curl_easy_setopt(curl, CURLOPT_URL, current_url);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for URL failed for %s. Not doing it. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            lxb_html_document_destroy(current_document->document);
            continue;
        }
        status_c = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) current_document);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for setting the document to write in failed for %s. Not doing it. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            lxb_html_document_destroy(current_document->document);
            continue;
        }

        // fetch
        lxb_html_document_parse_chunk_begin(current_document->document);

        int count_retries = 0;
        do {
            status_c = curl_easy_perform(curl);
            count_retries += 1;
        } while(count_retries < max_retries && status_c != CURLE_FILESIZE_EXCEEDED && status_c != CURLE_OK);
        if(status_c != CURLE_OK && count_retries == max_retries) {
            if(!bundle->no_color) {
                fprintf(stderr, "%s%s : Max retries exceeded. Last error was: %s.%s\n", BRIGHT_RED, current_url, curl_easy_strerror(status_c), RESET);
            } else {
                fprintf(stderr, "%s : Max retries exceeded. Last error was: %s.\n", current_url, curl_easy_strerror(status_c));
            }
            lxb_html_document_parse_chunk_end(current_document->document);
            lxb_html_document_destroy(current_document->document);
            continue;
        } else if(status_c == CURLE_FILESIZE_EXCEEDED) {
            if(!bundle->no_color) {
                fprintf(stderr, "%s%s : File size limit exceeded.%s\n", BRIGHT_RED, current_url, RESET);
            } else {
                fprintf(stderr, "%s : File size limit exceeded.\n", current_url);
            }
        }

        status_l = lxb_html_document_parse_chunk_end(current_document->document);

        status_c = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code_http);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the HTTP status code failed for %s. Setting it to 0. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            status_code_http = 0L;
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the content type failed for %s. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            status_code_http = 0L;
        } else if(content_type != NULL) {
            // content_type is pointing to curl's private memory
            // and it's value will change when another curl_easy_perform will occur.
            content_type = strdup(content_type);
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_location);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the redirect URL failed for %s. Error: %s.\n", current_url, curl_easy_strerror(status_c));
        } else if(redirect_location != NULL) {  // if there is a location header
            redirect_location = strdup(redirect_location);
        }

        pthread_mutex_lock(mutex);
        stack_document_push(documents, current_document->document, current_url, status_code_http, current_document->size, content_type, redirect_location);
        pthread_mutex_unlock(mutex);
    }
    curl_easy_cleanup(curl);
    free(current_document);

    return NULL;
}