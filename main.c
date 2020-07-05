#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#include "linked_list_documents.h"
#include "linked_list_urls.h"
#include "fetcher_thread.h"

void getLinks(lxb_html_document_t* document, char* url, URLNode_t** urls, URLNode_t** urls_done) {
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
	const lxb_char_t* foundURL;

	CURLU* url_c = curl_url(); // will hold the final url, coupled with urlFinal
	curl_url_set(url_c, CURLUPART_URL, url, 0);

	char* urlFinal; // will hold the url to push into the list
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        element = lxb_dom_collection_element(collection, i);

        lxb_dom_node_t* node = lxb_dom_interface_node(element);
		foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL); // try getting href
		if(foundURL == NULL) {
			foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "src", 3, NULL); // getting src otherwise
		}

		if(foundURL[0] != '#') { // if this is not a fragment of the same page
			// set url
			curl_url_set(url_c, CURLUPART_URL, foundURL, 0);
			curl_url_get(url_c, CURLUPART_URL, &urlFinal, 0);
		}
		if(findURLList(*urls_done, urlFinal) == 0 && findURLList(*urls, urlFinal) == 0) {
			// add url to the list
			pushURLList(urls, urlFinal);
		}
		// reset CURLU instance by re-setting the url
		curl_url_set(url_c, CURLUPART_URL, url, 0);
    }

	lxb_dom_collection_destroy(collection, true);
}

int main(int argc, char* argv[]) {
	pthread_t fetcher_thread;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

	URLNode_t* urls = NULL;
	URLNode_t* urls_done = NULL;
	pushURLList(&urls, "http://127.0.0.1:8000");

	DocumentNode_t* documents = NULL;

	curl_global_init(CURL_GLOBAL_ALL);

	struct BundleVarsThread bundle;
	bundle.documents = &documents;
	bundle.urls = &urls;
	bundle.mutex = &mutex;
	bundle.cond = &cond;

	pthread_create(&fetcher_thread, NULL, fetcher_thread_func, (void*)&bundle);

	struct Document* document;
	while(1) {
        pthread_mutex_lock(&mutex);
		int docLength = getDocumentListLength(documents);
		if(docLength == 0) {
			pthread_cond_wait(&cond, &mutex);
		}
		document = popDocumentList(&documents);

		getLinks(document->document, document->url, &urls, &urls_done);

		pushURLList(&urls_done, document->url);
        pthread_mutex_unlock(&mutex);
		lxb_html_document_destroy(document->document);
		free(document);
	}

	printURLList(urls);

	curl_global_cleanup();
	return EXIT_SUCCESS;
}