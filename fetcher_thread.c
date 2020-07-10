#include "fetcher_thread.h"

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp) {
	// called when getting data
	// passing the data received to the document using chunk parsing
	lxb_status_t status = lxb_html_document_parse_chunk((lxb_html_document_t*)userp, (lxb_char_t *)content, size*nmemb);
	CHECK_LXB(status)
	
	return size*nmemb;
}


void* fetcher_thread_func(void* bundle_arg) {
    struct BundleVarsThread* bundle = (struct BundleVarsThread*) bundle_arg;
    DocumentNode_t** documents = bundle->documents;
    URLNode_t** urls_todo = bundle->urls_todo;
    URLNode_t** urls_done = bundle->urls_done;
	pthread_mutex_t* mutex = bundle->mutex;
	int* isRunning = bundle->isRunning;
	int maxRetries = bundle->maxRetries;
	int timeout = bundle->timeout;


	CURLcode status_c;
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, processContent);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

	lxb_status_t status_l;
	lxb_html_document_t* currentDocument;
	char* currentURL;
	long status_code_http;
    
	while(1) {
        pthread_mutex_lock(mutex);
		if(getURLListLength(*urls_todo) == 0) { // no url to fetch
			*isRunning = 0; // change state
			pthread_mutex_unlock(mutex);
			continue;
		}
		*isRunning = 1;
		currentURL = popURLList(urls_todo);
		pushURLList(urls_done, currentURL);
        pthread_mutex_unlock(mutex);

		currentDocument = lxb_html_document_create();
		if(currentDocument == NULL) {
			fprintf(stderr, "lxb_html_document_create failed.");
		}

		status_c = curl_easy_setopt(curl, CURLOPT_URL, currentURL);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_setopt failed: %s\n", curl_easy_strerror(status_c));
		}
		status_c = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)currentDocument);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_setopt failed: %s\n", curl_easy_strerror(status_c));
		}

		// fetch
		status_l = lxb_html_document_parse_chunk_begin(currentDocument);
		CHECK_LXB(status_l)

		int countRetries = 0;
		do {
			status_c = curl_easy_perform(curl);
			countRetries += 1;
		} while(countRetries < maxRetries && status_c != CURLE_OK);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "Max retries exceeded for %s. %s.\n", currentURL, curl_easy_strerror(status_c));
			continue;
		}

		status_l = lxb_html_document_parse_chunk_end(currentDocument);
		CHECK_LXB(status_l)

		status_c = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code_http);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_getinfo failed: %s\n", curl_easy_strerror(status_c));
		}

        pthread_mutex_lock(mutex);
		pushDocumentList(documents, currentDocument, currentURL, status_code_http);
        pthread_mutex_unlock(mutex);
	}
	curl_easy_cleanup(curl);

    return NULL;
}