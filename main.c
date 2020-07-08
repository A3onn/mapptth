#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#include "linked_list_documents.h"
#include "linked_list_urls.h"
#include "fetcher_thread.h"

#define NBR_THREAD 2

void getLinks(lxb_html_document_t* document, char* url, URLNode_t** urls_todo, URLNode_t** urls_done, pthread_mutex_t* mutex) {
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

	CURLU* baseURL = curl_url(); // will hold the final url, coupled with urlFinal
	curl_url_set(baseURL, CURLUPART_URL, url, 0);

	char* documentDomain; // domain of this document, used when checking domains
	curl_url_get(baseURL, CURLUPART_HOST, &documentDomain, 0);

    lxb_dom_element_t *element;
	const lxb_char_t* foundURL;
	char* urlFinal; // will hold the url to push into the list
	char* foundURLDomain; // domain of the url found
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        element = lxb_dom_collection_element(collection, i);

        lxb_dom_node_t* node = lxb_dom_interface_node(element);
		foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL); // try getting href
		if(foundURL == NULL) {
			foundURL = lxb_dom_element_get_attribute(element, (const lxb_char_t*) "src", 3, NULL); // getting src otherwise
		}

		if(foundURL[0] != '#') { // if this is not a fragment of the same page
			// set url
			curl_url_set(baseURL, CURLUPART_URL, (char*)foundURL, 0); // curl will change the url accordingly
			curl_url_get(baseURL, CURLUPART_URL, &urlFinal, 0); // get final url

			curl_url_get(baseURL, CURLUPART_HOST, &foundURLDomain, 0); // get the domain of the URL

			pthread_mutex_lock(mutex);
			if(!findURLList(*urls_done, urlFinal) && !findURLList(*urls_todo, urlFinal) && !strcmp(foundURLDomain, documentDomain)) {
				// add url to the list
				pushURLList(urls_todo, urlFinal);
			}
			pthread_mutex_unlock(mutex);
		}

		// re-set base URL because it was modified
		curl_url_set(baseURL, CURLUPART_URL, url, 0);
    }

	lxb_dom_collection_destroy(collection, true);
}

int main(int argc, char* argv[]) {
	pthread_t fetcher_threads[NBR_THREAD];
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	DocumentNode_t* documents = NULL;

	URLNode_t* urls_todo = NULL;
	URLNode_t* urls_done = NULL;
	pushURLList(&urls_todo, argv[1]);

	curl_global_init(CURL_GLOBAL_ALL);


	int* listRunningThreads = (int*) malloc(sizeof (int) * NBR_THREAD);
	struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof (struct BundleVarsThread) * NBR_THREAD);
	for(int i = 0; i < NBR_THREAD; i++) {
		listRunningThreads[i] = 1;

		bundles[i].documents = &documents;
		bundles[i].urls_todo = &urls_todo;
		bundles[i].mutex = &mutex;
		bundles[i].isRunning = &(listRunningThreads[i]);
		pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*)&(bundles[i]));
	}

	struct Document* currentDocument;
	while(1) {
        pthread_mutex_lock(&mutex);
		if(getDocumentListLength(documents) == 0) { // no documents to parse
			// if this thread has no document to parse and all threads are waiting for urls,
			// then it means that everything was discovered and they should quit,
			// otherwise just continue to check for something to do
			int shouldQuit = 1;
			for(int i = 0; i < NBR_THREAD; i++) { // check if all threads are running
				if(listRunningThreads[i] == 1) { // if one is running
					shouldQuit = 0; // should not quit
					break; // don't need to check other threads
				}
			}
			if(shouldQuit == 1 && getURLListLength(urls_todo) == 0) { // if no threads are running and no urls to fetch left
				// quit
				for(int i = 0; i < NBR_THREAD; i++) { // stop all threads
					pthread_cancel(fetcher_threads[i]);
				}
				break; // quit parsing
			}
			// if some thread(s) are running, just continue to check for a document to come
			pthread_mutex_unlock(&mutex);
			continue;
		}

		currentDocument = popDocumentList(&documents);
		pthread_mutex_unlock(&mutex);

		getLinks(currentDocument->document, currentDocument->url, &urls_todo, &urls_done, &mutex);

		pthread_mutex_lock(&mutex);
		pushURLList(&urls_done, currentDocument->url);
		pthread_mutex_unlock(&mutex);
		lxb_html_document_destroy(currentDocument->document);
		free(currentDocument);
	}

	printf("Got: %i\n", getURLListLength(urls_done));

	free(bundles);
	free(listRunningThreads);

	while(getURLListLength(urls_done) != 1) { // all urls are allocated by curl when parsing url, except the initial url
		char* url_done = popURLList(&urls_done);
		printf("%s\n", url_done);
		free(url_done);
	}

	// last URL in urls_done is the initial URL
	char* url_done = popURLList(&urls_done);
	printf("Initial url: %s\n", url_done);

	curl_global_cleanup();
	return EXIT_SUCCESS;
}
