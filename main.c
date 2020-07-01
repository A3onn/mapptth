#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#include "linked_list_documents.h"
#include "linked_list_urls.h"

#define CHECK_LXB(X) if((X) != LXB_STATUS_OK) {fprintf(stderr,"An error occured line %i: %i\n", __LINE__, (lxb_status_t)(X));}

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp) {
	// called when getting data
	// passing the data received to the document using chunk parsing
	lxb_status_t status = lxb_html_document_parse_chunk((lxb_html_document_t*)userp, (lxb_char_t *)content, size*nmemb);
	CHECK_LXB(status)
	
	return size*nmemb;
}

void getLinks(lxb_html_document_t* document) {
	lxb_status_t status;
	lxb_dom_element_t *body = lxb_dom_interface_element(document->body);
	if(body == NULL) {
		fprintf(stderr, "lxb_dom_interface_element failed.");
		return ;
	}

	lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 16);
	if (collection == NULL) {
        fprintf(stderr, "lxb_dom_collection_make(&document->dom_document, 16) failed.");
		return ;
    }

	status = lxb_dom_elements_by_attr_contain(body, collection, (const lxb_char_t *) "href", 4, NULL, 0, true);
    if (status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
		return ;
    }
	status = lxb_dom_elements_by_attr_contain(body, collection, (const lxb_char_t *) "src", 3, NULL, 0, true);
    if (status != LXB_STATUS_OK) {
        fprintf(stderr, "lxb_dom_elements_by_attr_contain failed.");
		return ;
    }

    lxb_dom_element_t *element;
	const lxb_char_t* val;
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        element = lxb_dom_collection_element(collection, i);

        lxb_dom_node_t* node = lxb_dom_interface_node(element);
		val = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL); // try getting href
		if(val == NULL) {
			val = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "src", 3, NULL); // getting src otherwise
		}
		if(val[0] != '#') {
			printf("Found:%s\n", val);
		}
    }

	lxb_dom_collection_destroy(collection, true);
}

int main(int argc, char* argv[]) {
	URLNode_t* urls = NULL;
	pushURLList(&urls, "https://www.google.com/");
	pushURLList(&urls, "https://www.amazon.com/");
	pushURLList(&urls, "https://www.wikipedia.org/");

	DocumentNode_t* documents = NULL;

	curl_global_init(CURL_GLOBAL_ALL);
	CURL* curl;
	CURLcode res;

	lxb_status_t status;
	lxb_html_document_t* document;

	curl = curl_easy_init();
	if(!curl) {
		fprintf(stderr, "curl_easy_init() failed: %s\n", curl_easy_strerror(res));
		exit(EXIT_FAILURE);
	}
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, processContent);

	while(getURLListLength(urls) != 0) {
		char* url = popURLList(&urls);
		printf("Doing: %s\n", url);

		document = lxb_html_document_create();
		if(document == NULL) {
			fprintf(stderr, "lxb_html_document_create failed.");
		}
		status = lxb_html_document_parse_chunk_begin(document);
		CHECK_LXB(status)

		curl_easy_setopt(curl, CURLOPT_URL, url);
		free(url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)document);

		// fetch
		CHECK_LXB(status)

		res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
			exit(EXIT_FAILURE);
		}

		status = lxb_html_document_parse_chunk_end(document);
		CHECK_LXB(status)
		pushDocumentList(&documents, document);
	}

	printf("Got %i documents\n", getDocumentListLength(documents));
	for(int i = getDocumentListLength(documents); i > 0; i--) {
		document = popDocumentList(&documents);
		printf("Title: %s\n", lxb_html_document_title_raw(document, NULL));
		getLinks(document);
		lxb_html_document_destroy(document);
	}

	// destroy
	curl_easy_cleanup(curl);
	return EXIT_SUCCESS;
}