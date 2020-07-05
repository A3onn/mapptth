#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#include "linked_list_documents.h"
#include "linked_list_urls.h"
#include "fetcher_thread.h"

pthread_mutex_t mutexFetcher = PTHREAD_MUTEX_INITIALIZER;

void getLinks(lxb_html_document_t* document, CURLU* url, URLNode_t** urls, URLNode_t** urls_done) {
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

	char* path;
	curl_url_get(url, CURLUPART_PATH, &path, 0); // keep path
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        element = lxb_dom_collection_element(collection, i);

        lxb_dom_node_t* node = lxb_dom_interface_node(element);
		foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL); // try getting href
		if(foundURL == NULL) {
			foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "src", 3, NULL); // getting src otherwise
		}

		if(foundURL[0] != '#' && findURLList(*urls_done, (char*)foundURL) == 0 && findURLList(*urls, (char*)foundURL) == 0) {
			CURLU* newURL = curl_url();
			curl_url_set(url, CURLUPART_URL, (const char*)foundURL, 0);
			curl_url_get(url, CURLUPART_URL, (char**)(&foundURL), 0);
			pushURLList(urls, (const char*)foundURL);
			curl_url_set(url, CURLUPART_PATH, path, 0);
		}
    }

	lxb_dom_collection_destroy(collection, true);
}

int main(int argc, char* argv[]) {
	URLNode_t* urls = NULL;
	URLNode_t* urls_done = NULL;
	pushURLList(&urls, "http://127.0.0.1:8000");

	DocumentNode_t* documents = NULL;

	curl_global_init(CURL_GLOBAL_ALL);

	struct ListsThreads lists;
	lists.documents = &documents;
	lists.urls = &urls;


	pthread_t fetcher_thread;
	pthread_create(&fetcher_thread, NULL, fetcher_thread_func, (void*)&lists);
	pthread_join(fetcher_thread, NULL);

	struct Document* document;
	while(1) {
        pthread_mutex_lock(&mutexFetcher);
		if(getDocumentListLength(documents) == 0) {
			pthread_mutex_unlock(&mutexFetcher);
			break;
		}
		document = popDocumentList(&documents);
		if(document == NULL) {
			break;
		}
		getLinks(document->document, document->url, &urls, &urls_done);
        pthread_mutex_unlock(&mutexFetcher);

		//pushURLList(&urls_done, document->url);
		lxb_html_document_destroy(document->document);
		free(document);
	}

	printURLList(urls);

	curl_global_cleanup();
	return EXIT_SUCCESS;
}