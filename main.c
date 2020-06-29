#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#define CHECK_LXB(X) if((X) != LXB_STATUS_OK) {fprintf(stderr,"An error occured line: %i", __LINE__);}

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp) {
	// called when getting data
	// passing the data received to the document using chunk parsing
	lxb_status_t status = lxb_html_document_parse_chunk((lxb_html_document_t*)userp, (lxb_char_t *)content, size*nmemb);
	if(status != LXB_STATUS_OK) {
		fprintf(stderr, "lxb_html_document_parse_chunk failed.");
	}
	
	return size*nmemb;
}

int main(int argc, char* argv[]) {
	curl_global_init(CURL_GLOBAL_ALL);
	CURL* curl;
	CURLcode res;

	lxb_status_t status;
	lxb_html_document_t* document;

	document = lxb_html_document_create(); // create document
	if(document == NULL) {
		fprintf(stderr, "lxb_html_document_create failed.");
		exit(EXIT_FAILURE);
	}

	curl = curl_easy_init();
	if(!curl) {
		fprintf(stderr, "curl_easy_init() failed: %s\n", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
	curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, processContent);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)document);

	// fetch

	status = lxb_html_document_parse_chunk_begin(document);
	CHECK_LXB(status);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}

	status = lxb_html_document_parse_chunk_end(document);
	CHECK_LXB(LXB_STATUS_STOP);

	// destroy
	lxb_html_document_destroy(document);
	curl_easy_cleanup(curl);
	return EXIT_SUCCESS;
}
