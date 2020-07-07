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
    URLNode_t** urls = bundle->urls;
	pthread_mutex_t* mutex = bundle->mutex;

	int* isRunning = bundle->isRunning;


	CURLcode status_c;
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, processContent);

	lxb_status_t status_l;
	lxb_html_document_t* document;
    
	while(1) {
        pthread_mutex_lock(mutex);
		if(getURLListLength(*urls) == 0) { // no url to fetch
			*isRunning = 0; // change state
			pthread_mutex_unlock(mutex);
			continue;
		}
		*isRunning = 1;
		char* url = popURLList(urls);
        pthread_mutex_unlock(mutex);

		printf("Doing: %s\n", url);

		document = lxb_html_document_create();
		if(document == NULL) {
			fprintf(stderr, "lxb_html_document_create failed.");
		}

		status_c = curl_easy_setopt(curl, CURLOPT_URL, url);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_setopt failed: %s\n", curl_easy_strerror(status_c));
		}
		status_c = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)document);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_setopt failed: %s\n", curl_easy_strerror(status_c));
		}

		// fetch
		status_l = lxb_html_document_parse_chunk_begin(document);
		CHECK_LXB(status_l)

		status_c = curl_easy_perform(curl);
		if(status_c != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(status_c));
		}

		status_l = lxb_html_document_parse_chunk_end(document);
		CHECK_LXB(status_l)

        pthread_mutex_lock(mutex);
		pushDocumentList(documents, document, url);

        pthread_mutex_unlock(mutex);
	}
	curl_easy_cleanup(curl);

    return NULL;
}