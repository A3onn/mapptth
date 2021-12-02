#include <check.h>
#include <strings.h>
#include <pcre.h>
#include "trie_urls.h"
#include "utils.h"
#include "stack_urls.h"
#include "stack_documents.h"
#include "sitemaps_parser.h"


/* STACK OF URLS */
START_TEST(empty_stack_urls) {
    struct URLNode* urls = NULL;
    ck_assert_int_eq(stack_url_isempty(urls), 1);
}
END_TEST

START_TEST(zero_length_stack_urls) {
    struct URLNode* urls = NULL;
    ck_assert_int_eq(stack_url_length(urls), 0);
}
END_TEST

START_TEST(not_empty_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_int_eq(stack_url_isempty(urls), 0);
}
END_TEST

START_TEST(length_one_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_int_eq(stack_url_length(urls), 1);
}
END_TEST

START_TEST(length_five_stack_urls) {
    struct URLNode* urls = NULL;
    for(int i = 0; i < 5; i++) {
        stack_url_push(&urls, "test");
    }
    ck_assert_int_eq(stack_url_length(urls), 5);
}
END_TEST

START_TEST(pop_null_stack_urls) {
    struct URLNode* urls = NULL;
    ck_assert_ptr_eq(stack_url_pop(&urls), NULL);
}
END_TEST

START_TEST(get_correct_value_length_one_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_str_eq(stack_url_pop(&urls), "test");
}
END_TEST

START_TEST(false_contains_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_int_eq(stack_url_contains(urls, "nope"), 0);
}
END_TEST

START_TEST(false_substr_contains_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "tests");
    ck_assert_int_eq(stack_url_contains(urls, "tes"), 0);
}
END_TEST

START_TEST(true_contains_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_int_eq(stack_url_contains(urls, "test"), 1);
}
END_TEST

START_TEST(get_differents_correct_value_length_five_stack_urls) {
    struct URLNode* urls = NULL;
    for(int i = 0; i < 5; i++) {
        char* str = (char*) malloc(sizeof (char)*6);
        sprintf(str, "test%d", i);
        stack_url_push(&urls, str);
    }
    for(int i = 4; i >= 0; i--) {
        char* str = (char*) malloc(sizeof (char)*6);
        sprintf(str, "test%d", i);
        ck_assert_str_eq(stack_url_pop(&urls), str);
    }
}
END_TEST

START_TEST(get_same_correct_values_length_five_stack_urls) {
    struct URLNode* urls = NULL;
    for(int i = 0; i < 5; i++) {
        stack_url_push(&urls, "test");
    }
    for(int i = 0; i < 5; i++) {
        ck_assert_str_eq(stack_url_pop(&urls), "test");
    }
}
END_TEST


/* STACK OF DOCUMENTS */
START_TEST(zero_length_stack_documents) {
    struct DocumentNode* docs = NULL;
    ck_assert_int_eq(stack_document_length(docs), 0);
}
END_TEST

START_TEST(one_length_stack_documents) {
    struct DocumentNode* docs = NULL;
    stack_document_push(&docs, NULL, "test", 0, 0, "test", "test");
    ck_assert_int_eq(stack_document_length(docs), 1);
}
END_TEST

START_TEST(five_length_stack_documents) {
    struct DocumentNode* docs = NULL;
    for(int i = 0; i < 5; i++) {
        stack_document_push(&docs, NULL, "test", 0, 0, "test", "test");
    }
    ck_assert_int_eq(stack_document_length(docs), 5);
}
END_TEST

START_TEST(pop_null_stack_documents) {
    struct DocumentNode* docs = NULL;
    ck_assert_ptr_eq(stack_document_pop(&docs), NULL);
}
END_TEST

START_TEST(get_correct_value_length_one_stack_documents) {
    struct DocumentNode* docs = NULL;
    lxb_html_document_t* lxb_doc = lxb_html_document_create();
    struct Document* doc;

    stack_document_push(&docs, lxb_doc, "url", 1, 2, "text", "");

    doc = stack_document_pop(&docs);
    ck_assert_ptr_eq(doc->lexbor_document, lxb_doc);
    ck_assert_str_eq(doc->url, "url");
    ck_assert_int_eq(doc->status_code_http, 1);
    ck_assert_int_eq(doc->size, 2);
    ck_assert_str_eq(doc->content_type, "text");
    ck_assert_str_eq(doc->redirect_location, "");
}
END_TEST

START_TEST(get_differents_correct_value_length_five_stack_documents) {
    struct DocumentNode* docs = NULL;
    lxb_html_document_t* lxb_doc = lxb_html_document_create();
    struct Document* doc;
     char *url_buf, *content_type_buf, *redirect_location_buf;

    for(int i = 0; i < 5; i++) {
        url_buf = (char*) malloc(sizeof (char)*5);
        content_type_buf = (char*) malloc(sizeof (char)*14);
        redirect_location_buf = (char*) malloc(sizeof (char)*10);

        sprintf(url_buf, "url%d", i);
        sprintf(content_type_buf, "content_type%d", i);
        sprintf(redirect_location_buf, "redirect%d", i);

        stack_document_push(&docs, lxb_doc, url_buf, i, i*i, content_type_buf, redirect_location_buf);
    }

    for(int i = 4; i >= 0; i--) {
        url_buf = (char*) malloc(sizeof (char)*5);
        content_type_buf = (char*) malloc(sizeof (char)*14);
        redirect_location_buf = (char*) malloc(sizeof (char)*10);

        sprintf(url_buf, "url%d", i);
        sprintf(content_type_buf, "content_type%d", i);
        sprintf(redirect_location_buf, "redirect%d", i);

        doc = stack_document_pop(&docs);
        ck_assert_ptr_eq(doc->lexbor_document, lxb_doc);
        ck_assert_str_eq(doc->url, url_buf);
        ck_assert_int_eq(doc->status_code_http, i);
        ck_assert_int_eq(doc->size, i*i);
        ck_assert_str_eq(doc->content_type, content_type_buf);
        ck_assert_str_eq(doc->redirect_location, redirect_location_buf);
    }
}
END_TEST

START_TEST(correct_length_stack_documents) {
    struct DocumentNode* docs = NULL;

    for(int i = 0; i < 5; i++) {
        stack_document_push(&docs, NULL, "url", 1, 2, "text", "");
    }

    for(int i = 4; i >= 0; i--) {
        stack_document_pop(&docs);
        ck_assert_int_eq(stack_document_length(docs), i);
    }
}
END_TEST


/* TRIE OF URLS */
START_TEST(create_trie_urls) {
    struct TrieNode* root = trie_create();
    ck_assert_int_eq(root->children_count, 2);
    ck_assert_str_eq(root->children[HTTP_INDEX_T].data, "http");
    ck_assert_str_eq(root->children[HTTPS_INDEX_T].data, "https");
    ck_assert_int_eq(root->children[HTTP_INDEX_T].type, SCHEME_T);
    ck_assert_int_eq(root->children[HTTPS_INDEX_T].type, SCHEME_T);
}
END_TEST

START_TEST(contains_true_trie_urls) {
    struct TrieNode* root = trie_create();

    trie_add(root, "http://url1.com");
    ck_assert_int_eq(trie_contains(root, "http://url1.com"), 1);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/"), 1);

    trie_add(root, "http://url2.com");
    ck_assert_int_eq(trie_contains(root, "http://url1.com"), 1); // still contains "http://url1.com" ?
    ck_assert_int_eq(trie_contains(root, "http://url2.com"), 1);
    ck_assert_int_eq(trie_contains(root, "http://url2.com/"), 1);

    trie_add(root, "http://url3.com");
    trie_add(root, "https://url3.com");
    ck_assert_int_eq(trie_contains(root, "http://url3.com"), 1);
    ck_assert_int_eq(trie_contains(root, "https://url3.com"), 1);

    trie_add(root, "http://url4.com#test");
    trie_add(root, "http://url4.com?test");
    trie_add(root, "http://url4.com?test#test");
    ck_assert_int_eq(trie_contains(root, "http://url4.com#test"), 1);
    ck_assert_int_eq(trie_contains(root, "http://url4.com?test"), 1);
    ck_assert_int_eq(trie_contains(root, "http://url4.com?test#test"), 1);
}
END_TEST

START_TEST(trie_add_simple_trie_urls) {
    struct TrieNode* root = trie_create();
    trie_add(root, "http://url.com:8080/some/path?query#fragment");
    ck_assert_int_eq(root->is_end_url, 0);

    struct TrieNode scheme = root->children[0];
    ck_assert_str_eq(scheme.data, "http");
    ck_assert_int_eq(scheme.type, SCHEME_T);
    ck_assert_int_eq(scheme.is_end_url, 0);

    struct TrieNode host = scheme.children[0];
    ck_assert_str_eq(host.data, "url.com");
    ck_assert_int_eq(host.type, HOST_T);
    ck_assert_int_eq(host.is_end_url, 0);

    struct TrieNode port = host.children[0];
    ck_assert_str_eq(port.data, "8080");
    ck_assert_int_eq(port.type, PORT_T);
    ck_assert_int_eq(port.is_end_url, 0);

    struct TrieNode path1 = port.children[0];
    ck_assert_str_eq(path1.data, "some");
    ck_assert_int_eq(path1.type, PATH_T);
    ck_assert_int_eq(path1.is_end_url, 0);

    struct TrieNode path2 = path1.children[0];
    ck_assert_str_eq(path2.data, "path");
    ck_assert_int_eq(path2.type, PATH_T);
    ck_assert_int_eq(path2.is_end_url, 0);

    struct TrieNode query = path2.children[0];
    ck_assert_str_eq(query.data, "query");
    ck_assert_int_eq(query.type, QUERY_T);
    ck_assert_int_eq(query.is_end_url, 0);

    struct TrieNode fragment = query.children[0];
    ck_assert_str_eq(fragment.data, "fragment");
    ck_assert_int_eq(fragment.type, FRAGMENT_T);
    ck_assert_int_eq(fragment.is_end_url, 1);
}
END_TEST

START_TEST(trie_add_multiple_trie_urls) {
    struct TrieNode* root = trie_create();
    trie_add(root, "http://url.com:8080/some/path?query#fragment");
    trie_add(root, "https://url.com:8080/some/path?query#fragment");
    trie_add(root, "http://another-url.com:8080/some/path?query#fragment");
    trie_add(root, "http://url.com:4444/some/path?query#fragment");
    trie_add(root, "http://url.com:8080/another/path?query#fragment");
    trie_add(root, "http://url.com:8080/some/test?query#fragment");
    trie_add(root, "http://url.com:8080/some/path?another-query#fragment");
    trie_add(root, "http://url.com:8080/some/path?query#another-fragment");
    ck_assert_int_eq(root->is_end_url, 0);

    struct TrieNode scheme = root->children[0];
    ck_assert_str_eq(scheme.data, "http");
    ck_assert_int_eq(scheme.type, SCHEME_T);
    ck_assert_int_eq(scheme.is_end_url, 0);

    struct TrieNode host = scheme.children[0];
    ck_assert_str_eq(host.data, "url.com");
    ck_assert_int_eq(host.type, HOST_T);
    ck_assert_int_eq(host.is_end_url, 0);

    struct TrieNode port = host.children[0];
    ck_assert_str_eq(port.data, "8080");
    ck_assert_int_eq(port.type, PORT_T);
    ck_assert_int_eq(port.is_end_url, 0);

    struct TrieNode path1 = port.children[0];
    ck_assert_str_eq(path1.data, "some");
    ck_assert_int_eq(path1.type, PATH_T);
    ck_assert_int_eq(path1.is_end_url, 0);

    struct TrieNode path2 = path1.children[0];
    ck_assert_str_eq(path2.data, "path");
    ck_assert_int_eq(path2.type, PATH_T);
    ck_assert_int_eq(path2.is_end_url, 0);

    struct TrieNode query = path2.children[0];
    ck_assert_str_eq(query.data, "query");
    ck_assert_int_eq(query.type, QUERY_T);
    ck_assert_int_eq(query.is_end_url, 0);

    struct TrieNode fragment = query.children[0];
    ck_assert_str_eq(fragment.data, "fragment");
    ck_assert_int_eq(fragment.type, FRAGMENT_T);
    ck_assert_int_eq(fragment.is_end_url, 1);

    ck_assert_str_eq(root->children[1].data, "https");
    ck_assert_int_eq(root->children[1].type, SCHEME_T);
    ck_assert_int_eq(root->children[1].is_end_url, 0);

    ck_assert_str_eq(scheme.children[1].data, "another-url.com");
    ck_assert_int_eq(scheme.children[1].type, HOST_T);
    ck_assert_int_eq(scheme.children[1].is_end_url, 0);

    ck_assert_str_eq(host.children[1].data, "4444");
    ck_assert_int_eq(host.children[1].type, PORT_T);
    ck_assert_int_eq(host.children[1].is_end_url, 0);

    ck_assert_str_eq(port.children[1].data, "another");
    ck_assert_int_eq(port.children[1].type, PATH_T);
    ck_assert_int_eq(port.children[1].is_end_url, 0);

    ck_assert_str_eq(path1.children[1].data, "test");
    ck_assert_int_eq(path1.children[1].type, PATH_T);
    ck_assert_int_eq(path1.children[1].is_end_url, 0);

    ck_assert_str_eq(path2.children[1].data, "another-query");
    ck_assert_int_eq(path2.children[1].type, QUERY_T);
    ck_assert_int_eq(path2.children[1].is_end_url, 0);

    ck_assert_str_eq(query.children[1].data, "another-fragment");
    ck_assert_int_eq(query.children[1].type, FRAGMENT_T);
    ck_assert_int_eq(query.children[1].is_end_url, 1);
}
END_TEST

START_TEST(trie_add_multiple_same_trie_urls) {
    // add same url and check if no parts was duplicated
    struct TrieNode* root = trie_create();
    trie_add(root, "http://url.com");

    struct TrieNode* scheme = &(root->children[0]);

    trie_add(root, "http://url.com");
    ck_assert_ptr_eq(scheme, &(root->children[0]));
}
END_TEST

START_TEST(contains_false_trie_urls) {
    struct TrieNode* root = trie_create();

    trie_add(root, "http://url1.com");
    ck_assert_int_eq(trie_contains(root, "http://url2.com"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/path"), 0);

    root = trie_create();
    trie_add(root, "http://url1.com?test");
    ck_assert_int_eq(trie_contains(root, "http://url1.com"), 0);
    ck_assert_int_eq(trie_contains(root, "https://url1.com"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com?"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com?not-test"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/test"), 0); // mind the '/'
    ck_assert_int_eq(trie_contains(root, "http://url1.com#test"), 0); // mind the '#'
    ck_assert_int_eq(trie_contains(root, "http://url1.com?test#not-test"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/not-test?test"), 0);

    root = trie_create();
    trie_add(root, "http://url1.com#test");
    ck_assert_int_eq(trie_contains(root, "http://url1.com"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com#"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com#not-test"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/test"), 0); // mind the '/'
    ck_assert_int_eq(trie_contains(root, "http://url1.com?test"), 0); // mind the '?'
    ck_assert_int_eq(trie_contains(root, "http://url1.com?not-test#test"), 0);
    ck_assert_int_eq(trie_contains(root, "http://url1.com/not-test#test"), 0);
}
END_TEST

START_TEST(_find_child_simple_trie_urls) {
    struct TrieNode* root = trie_create();

    trie_add(root, "http://url.com:8080/some/path?query#fragment");

    struct TrieNode* scheme = _find_child(root, "http", SCHEME_T);
    ck_assert_ptr_ne(scheme, NULL);
    ck_assert_int_eq(scheme->type, SCHEME_T);
    ck_assert_str_eq(scheme->data, "http");

    struct TrieNode* host = _find_child(scheme, "url.com", HOST_T);
    ck_assert_ptr_ne(host, NULL);
    ck_assert_int_eq(host->type, HOST_T);
    ck_assert_str_eq(host->data, "url.com");

    struct TrieNode* port = _find_child(host, "8080", PORT_T);
    ck_assert_ptr_ne(port, NULL);
    ck_assert_int_eq(port->type, PORT_T);
    ck_assert_str_eq(port->data, "8080");

    struct TrieNode* path1 = _find_child(port, "some", PATH_T);
    ck_assert_ptr_ne(path1, NULL);
    ck_assert_int_eq(path1->type, PATH_T);
    ck_assert_str_eq(path1->data, "some");

    struct TrieNode* path2 = _find_child(path1, "path", PATH_T);
    ck_assert_ptr_ne(path2, NULL);
    ck_assert_int_eq(path2->type, PATH_T);
    ck_assert_str_eq(path2->data, "path");

    struct TrieNode* query = _find_child(path2, "query", QUERY_T);
    ck_assert_ptr_ne(query, NULL);
    ck_assert_int_eq(query->type, QUERY_T);
    ck_assert_str_eq(query->data, "query");

    struct TrieNode* fragment = _find_child(query, "fragment", FRAGMENT_T);
    ck_assert_ptr_ne(fragment, NULL);
    ck_assert_int_eq(fragment->type, FRAGMENT_T);
    ck_assert_str_eq(fragment->data, "fragment");
    
    //ck_assert_ptr_eq(_find_child(root, "https", SCHEME_T), NULL); // should not be tested because 'https' is added when the trie is created
    ck_assert_ptr_eq(_find_child(root, "http", HOST_T), NULL); // check wrong type
    ck_assert_ptr_eq(_find_child(root, "", SCHEME_T), NULL);

    ck_assert_ptr_eq(_find_child(scheme, "url1.com", HOST_T), NULL); // check wrong type
    ck_assert_ptr_eq(_find_child(scheme, "url1.com", PORT_T), NULL);

    ck_assert_ptr_eq(_find_child(host, "8000", PORT_T), NULL);
    ck_assert_ptr_eq(_find_child(host, "8080", SCHEME_T), NULL);
    ck_assert_ptr_eq(_find_child(host, "", SCHEME_T), NULL);

    ck_assert_ptr_eq(_find_child(port, "path", PATH_T), NULL); // /some/path
    ck_assert_ptr_eq(_find_child(port, "some", SCHEME_T), NULL);
    ck_assert_ptr_eq(_find_child(port, "", PATH_T), NULL);

    ck_assert_ptr_eq(_find_child(path2, "query", HOST_T), NULL);
    ck_assert_ptr_eq(_find_child(path2, "query-not", QUERY_T), NULL);
    ck_assert_ptr_eq(_find_child(path2, "not-query", QUERY_T), NULL);
    ck_assert_ptr_eq(_find_child(path2, "", PATH_T), NULL);

    ck_assert_ptr_eq(_find_child(query, "fragment", QUERY_T), NULL);
    ck_assert_ptr_eq(_find_child(query, "fragment-not", FRAGMENT_T), NULL);
    ck_assert_ptr_eq(_find_child(query, "not-fragment", FRAGMENT_T), NULL);
    ck_assert_ptr_eq(_find_child(query, "", FRAGMENT_T), NULL);
}
END_TEST

START_TEST(_find_child_multiple_trie_urls) {
    struct TrieNode* root = trie_create();

    trie_add(root, "http://url.com:8080/some/path?query#fragment"); // will search for this one
    trie_add(root, "https://url.com:8080/some/path?query#fragment");
    trie_add(root, "http://url1.com:8080/some/path?query#fragment");
    trie_add(root, "http://url.com:4444/some/path?query#fragment");
    trie_add(root, "http://url.com:8080/some/?query#fragment");
    trie_add(root, "http://url.com:8080/path/some?query#fragment");
    trie_add(root, "http://url.com:8080/some/another?query#fragment");
    trie_add(root, "http://url.com:8080/some/path/another?query#fragment");
    trie_add(root, "http://url.com:8080/some/path?wrong-query#fragment");
    trie_add(root, "http://url.com:8080/some/path?query#wrong-fragment");
    trie_add(root, "http://url.com:8080/some/path?#fragment");
    trie_add(root, "http://url.com:8080/some/path?#");
    trie_add(root, "http://url.com:8080/some/path?");
    trie_add(root, "http://url.com:8080/some/path");

    struct TrieNode* scheme = _find_child(root, "http", SCHEME_T);
    ck_assert_ptr_ne(scheme, NULL);
    ck_assert_int_eq(scheme->type, SCHEME_T);
    ck_assert_str_eq(scheme->data, "http");

    struct TrieNode* host = _find_child(scheme, "url.com", HOST_T);
    ck_assert_ptr_ne(host, NULL);
    ck_assert_int_eq(host->type, HOST_T);
    ck_assert_str_eq(host->data, "url.com");

    struct TrieNode* port = _find_child(host, "8080", PORT_T);
    ck_assert_ptr_ne(port, NULL);
    ck_assert_int_eq(port->type, PORT_T);
    ck_assert_str_eq(port->data, "8080");

    struct TrieNode* path1 = _find_child(port, "some", PATH_T);
    ck_assert_ptr_ne(path1, NULL);
    ck_assert_int_eq(path1->type, PATH_T);
    ck_assert_str_eq(path1->data, "some");

    struct TrieNode* path2 = _find_child(path1, "path", PATH_T);
    ck_assert_ptr_ne(path2, NULL);
    ck_assert_int_eq(path2->type, PATH_T);
    ck_assert_str_eq(path2->data, "path");

    struct TrieNode* query = _find_child(path2, "query", QUERY_T);
    ck_assert_ptr_ne(query, NULL);
    ck_assert_int_eq(query->type, QUERY_T);
    ck_assert_str_eq(query->data, "query");

    struct TrieNode* fragment = _find_child(query, "fragment", FRAGMENT_T);
    ck_assert_ptr_ne(fragment, NULL);
    ck_assert_int_eq(fragment->type, FRAGMENT_T);
    ck_assert_str_eq(fragment->data, "fragment");
}
END_TEST


/* UTILS */
START_TEST(true_url_not_seen) {
    URLNode_t* urls_todo = NULL;
    struct TrieNode* urls_done = trie_create();

    stack_url_push(&urls_todo, "http://url1.com");
    trie_add(urls_done, "http://url2.com");

    ck_assert_int_eq(url_not_seen("http://url3.com", urls_done, urls_todo), 1);
}
END_TEST

START_TEST(true_nothing_seen_url_not_seen) {
    URLNode_t* urls_todo = NULL;
    struct TrieNode* urls_done = trie_create();

    ck_assert_int_eq(url_not_seen("http://url1.com", urls_done, urls_todo), 1);
}
END_TEST

START_TEST(false_url_not_seen) {
    URLNode_t* urls_todo = NULL;
    struct TrieNode* urls_done = trie_create();

    trie_add(urls_done, "http://url1.com");

    ck_assert_int_eq(url_not_seen("http://url1.com", urls_done, urls_todo), 0);
}
END_TEST



START_TEST(empty_to_compare_is_same_domain) {
    ck_assert_int_eq(is_same_domain("", "google.com", 0), 0);
}
END_TEST

START_TEST(empty_comparing_to_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "", 0), 0);
}
END_TEST

START_TEST(empty_can_be_subdomain_to_compare_is_same_domain) {
    ck_assert_int_eq(is_same_domain("", "google.com", 1), 0);
}
END_TEST

START_TEST(empty_can_be_subdomain_comparing_to_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "", 1), 0);
}
END_TEST

START_TEST(true_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "google.com", 0), 1);
}
END_TEST

START_TEST(true_can_be_subdomain_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "google.com", 1), 1);
}
END_TEST

START_TEST(false_is_same_domain) {
    ck_assert_int_eq(is_same_domain("amazon.com", "google.com", 0), 0);
}
END_TEST

START_TEST(false_can_be_subdomain_is_same_domain) {
    ck_assert_int_eq(is_same_domain("amazon.com", "google.com", 1), 0);
}
END_TEST

START_TEST(true_subdomain_is_same_domain) {
    ck_assert_int_eq(is_same_domain("mail.google.com", "google.com", 1), 1);
}
END_TEST

START_TEST(true_depth_subdomain_is_same_domain) {
    ck_assert_int_eq(is_same_domain("test.mapptth.mail.google.com", "mail.google.com", 1), 1);
}
END_TEST

START_TEST(domain_to_compare_is_main_domain_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "mail.google.com", 1), 0);
}
END_TEST

START_TEST(almost_same_domains_is_same_domain) {
    ck_assert_int_eq(is_same_domain("testgoogle.com", "google.com", 0), 0);
}
END_TEST

START_TEST(almost_same_domains_2_is_same_domain) {
    ck_assert_int_eq(is_same_domain("google.com", "testgoogle.com", 0), 0);
}
END_TEST

START_TEST(almost_same_subdomains_is_same_domain) {
    ck_assert_int_eq(is_same_domain("testmail.google.com", "mail.google.com", 0), 0);
}
END_TEST



START_TEST(empty_is_valid_link) {
    ck_assert_int_eq(is_valid_link(""), 1);
}
END_TEST

START_TEST(true_is_valid_link) {
    ck_assert_int_eq(is_valid_link("https://google.com"), 1);
}
END_TEST

START_TEST(true_2_is_valid_link) {
    ck_assert_int_eq(is_valid_link("//google.com"), 1);
}
END_TEST

START_TEST(true_3_is_valid_link) {
    ck_assert_int_eq(is_valid_link("https://google.com/index.php"), 1);
}
END_TEST

START_TEST(false_not_http_scheme_is_valid_link) {
    ck_assert_int_eq(is_valid_link("gopher://google.com/"), 0);
    ck_assert_int_eq(is_valid_link("ftp://google.com/"), 0);
    ck_assert_int_eq(is_valid_link("ftps://google.com/"), 0);
    ck_assert_int_eq(is_valid_link("smtp://google.com/"), 0);
}
END_TEST

START_TEST(false_mailto_is_valid_link) {
    ck_assert_int_eq(is_valid_link("mailto:"), 0);
    ck_assert_int_eq(is_valid_link("mailto:test@gmail.com"), 0);
}
END_TEST

START_TEST(false_javascript_is_valid_link) {
    ck_assert_int_eq(is_valid_link("javascript:"), 0);
    ck_assert_int_eq(is_valid_link("javascript:void(0);"), 0);
}
END_TEST

START_TEST(false_tel_is_valid_link) {
    ck_assert_int_eq(is_valid_link("tel:"), 0);
    ck_assert_int_eq(is_valid_link("tel://636-48018"), 0);
}
END_TEST

START_TEST(false_data_is_valid_link) {
    ck_assert_int_eq(is_valid_link("data:"), 0);
    ck_assert_int_eq(is_valid_link("data:image"), 0);
    ck_assert_int_eq(is_valid_link("data:,"), 0);
}
END_TEST



START_TEST(nothing_change_file_normalize_path) {
    ck_assert_str_eq(normalize_path("/root/flag.txt", 0), "/root/flag.txt");
}
END_TEST

START_TEST(empty_path_normalize_path) {
    ck_assert_str_eq(normalize_path("", 1), "/");
}
END_TEST

START_TEST(missing_root_directory_slash_normalize_path) {
    ck_assert_str_eq(normalize_path("etc/", 1), "/etc/");
}
END_TEST

START_TEST(missing_root_file_slash_normalize_path) {
    ck_assert_str_eq(normalize_path("etc/passwd", 0), "/etc/passwd");
}
END_TEST

START_TEST(nothing_change_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("/root/", 1), "/root/");
}
END_TEST

START_TEST(normalize_dot_dot_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("/root/..", 1), "/");
}
END_TEST

START_TEST(current_dir_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("/././././etc/./", 1), "/etc/");
}
END_TEST

START_TEST(current_dir_file_normalize_path) {
    ck_assert_str_eq(normalize_path("/././././etc/./passwd", 0), "/etc/passwd");
}
END_TEST

START_TEST(stop_root_dir_normalize_path) {
    ck_assert_str_eq(normalize_path("/../../../../../..", 1), "/");
}
END_TEST

START_TEST(normalize_multiple_slash_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("/////", 1), "/");
}
END_TEST

START_TEST(normalize_2_multiple_slash_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("/root//////", 1), "/root/");
}
END_TEST

START_TEST(normalize_dot_dot_file_normalize_path) {
    ck_assert_str_eq(normalize_path("/root/../etc/passwd", 0), "/etc/passwd");
}
END_TEST

START_TEST(normalize_dot_dot_dir_normalize_path) {
    ck_assert_str_eq(normalize_path("/root/../etc", 1), "/etc/");
}
END_TEST

START_TEST(complex_directory_normalize_path) {
    ck_assert_str_eq(normalize_path("///etc/passwd/../cron.d///../cron.d/.././../../../root/../home//", 1), "/home/");
}
END_TEST

START_TEST(complex_file_normalize_path) {
    ck_assert_str_eq(normalize_path("///etc////passwd", 0), "/etc/passwd");
}
END_TEST



START_TEST(true_one_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/files", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 1);
}
END_TEST

START_TEST(true_root_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 1);
}
END_TEST

START_TEST(false_one_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 0);
}
END_TEST

START_TEST(true_multiple_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/js", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/files", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 1);
}
END_TEST

START_TEST(true_multiple_disallowed_ends_with_slash_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/js/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/files/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 1);
}
END_TEST

START_TEST(false_multiple_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/js", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/css", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 0);
}
END_TEST

START_TEST(false_multiple_disallowed_ends_with_slash_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/js/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/css/", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 0);
}
END_TEST

START_TEST(empty_path_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/images", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/js", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("", dis_paths, 2), 0);
}
END_TEST

START_TEST(empty_list_is_disallowed_path) {
    pcre* dis_paths[] = { 0 };
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 0), 0);
}
END_TEST

START_TEST(checked_path_substr_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/files_images", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0), pcre_compile("/images_files", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 2), 0);
}
END_TEST

START_TEST(disallowed_paths_substr_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/file", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 0);
}
END_TEST

START_TEST(disallowed_paths_regex_disallowed_is_disallowed_path) {
    const char* err;
    int err_offset = 0;
    pcre* dis_paths[] = {pcre_compile("/.{5}/[lief]*[abcde]{0}", PCRE_NO_AUTO_CAPTURE, &err, &err_offset, 0)};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 1);
}
END_TEST


START_TEST(true_one_allowed_is_allowed_extension) {
    char* allowed_exts[] = {".ext"};
    ck_assert_int_eq(is_allowed_extension("/file.ext", allowed_exts, 1), 1);
}
END_TEST

START_TEST(false_one_allowed_is_allowed_extension) {
    char* allowed_exts[] = {".ext"};
    ck_assert_int_eq(is_allowed_extension("/file.png", allowed_exts, 1), 0);
}
END_TEST

START_TEST(true_multiple_allowed_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4"};
    ck_assert_int_eq(is_allowed_extension("/file.mp4", allowed_exts, 3), 1);
}
END_TEST

START_TEST(false_multiple_allowed_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4"};
    ck_assert_int_eq(is_allowed_extension("/file.cpp", allowed_exts, 3), 0);
}
END_TEST

START_TEST(empty_path_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4"};
    ck_assert_int_eq(is_allowed_extension("", allowed_exts, 3), 0);
}
END_TEST

START_TEST(no_extension_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4"};
    ck_assert_int_eq(is_allowed_extension("/file", allowed_exts, 3), 1);
}
END_TEST

START_TEST(no_file_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4"};
    ck_assert_int_eq(is_allowed_extension("/", allowed_exts, 3), 1);
}
END_TEST

START_TEST(empty_list_extensions_is_allowed_extension) {
    char* allowed_exts[] = { 0 };
    // SPECIAL CASE: if no extensions, then everything is allowed
    ck_assert_int_eq(is_allowed_extension("/file.ext", allowed_exts, 0), 1);
}
END_TEST

START_TEST(extension_substr_allowed_is_allowed_extension) {
    char* allowed_exts[] = {".ext", ".png", ".mp4", ".gz"};
    ck_assert_int_eq(is_allowed_extension("/file.anext", allowed_exts, 4), 0);
    ck_assert_int_eq(is_allowed_extension("/file.exta", allowed_exts, 4), 0);
    ck_assert_int_eq(is_allowed_extension("/file.tar.gz", allowed_exts, 4), 1);
}
END_TEST



START_TEST(true_is_in_valid_domains) {
    char* domains[] = {"google.com"};
    ck_assert_int_eq(is_in_valid_domains("google.com", domains, 1, 0), 1);
    ck_assert_int_eq(is_in_valid_domains("mail.google.com", domains, 1, 1), 1);

    ck_assert_int_eq(is_in_valid_domains("mail.google.com", domains, 1, 1), 1);
}
END_TEST

START_TEST(false_is_in_valid_domains) {
    char* domains[] = {"google.com"};
    ck_assert_int_eq(is_in_valid_domains("amazon.com", domains, 1, 0), 0);
    ck_assert_int_eq(is_in_valid_domains("aws.amazon.com", domains, 1, 1), 0);
}
END_TEST

START_TEST(empty_domain_is_in_valid_domains) {
    char* domains[] = {"google.com"};
    ck_assert_int_eq(is_in_valid_domains("", domains, 1, 1), 0);
}
END_TEST

START_TEST(empty_allowed_domains_domain_is_in_valid_domains) {
    char* domains[] = { 0 };
    ck_assert_int_eq(is_in_valid_domains("google.com", domains, 0, 1), 0);
}
END_TEST



START_TEST(empty_get_path_depth) {
    ck_assert_int_eq(get_path_depth(""), 1);
}
END_TEST

START_TEST(one_get_path_depth) {
    ck_assert_int_eq(get_path_depth("/"), 1);
}
END_TEST

START_TEST(three_file_get_path_depth) {
    ck_assert_int_eq(get_path_depth("/one/two/three"), 3);
}
END_TEST

START_TEST(three_directory_get_path_depth) {
    ck_assert_int_eq(get_path_depth("/one/two/three/"), 3);
}
END_TEST

START_TEST(need_normalize_three_get_path_depth) {
    ck_assert_int_eq(get_path_depth("/one/two/../two/three"), 3);
}
END_TEST


/* SITEMAPS PARSER */
#define getXMLDoc(content) xmlReadMemory(content, strlen(content), "test.xml", NULL, 0) 
#define getRoot(doc) xmlDocGetRootElement(doc)

START_TEST(correct__sitemap_get_location) {
	xmlDocPtr doc;

	doc = getXMLDoc("<loc>http://localhost</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "http://localhost");
	xmlFreeDoc(doc);

	doc = getXMLDoc("<loc>http://localhost/</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "http://localhost/");
	xmlFreeDoc(doc);

	doc = getXMLDoc("<loc>http://localhost:8080/</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "http://localhost:8080/");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(correct_not_valid_url__sitemap_get_location) {
	xmlDocPtr doc;

	doc = getXMLDoc("<loc>not a valid url</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "not a valid url");
	xmlFreeDoc(doc);

	doc = getXMLDoc("<loc>test</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "test");
	xmlFreeDoc(doc);

	doc = getXMLDoc("<loc>some string</loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "some string");
	xmlFreeDoc(doc);
}
END_TEST


START_TEST(empty_value__sitemap_get_location) {
	xmlDocPtr doc;
	doc = getXMLDoc("<loc></loc>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(empty_node__sitemap_get_location) {
	xmlDocPtr doc;
	doc = getXMLDoc("");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(null_root__sitemap_get_location) {
	ck_assert_str_eq(__sitemap_get_location(NULL), "");
}
END_TEST

START_TEST(multiple_values__sitemap_get_location) {
	// check if returns only 1 value and the first one
	xmlDocPtr doc;
	doc = getXMLDoc("<test><loc>http://localhost:8000</loc><loc>http://localhost:4444</loc></test>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)->children), "http://localhost:8000"); // getRoot(doc)->children refers to the childrens of <test>, getRoot refers to <test>
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(no_loc_tag__sitemap_get_location) {
	xmlDocPtr doc;
	doc = getXMLDoc("<test></test>");
	ck_assert_str_eq(__sitemap_get_location(getRoot(doc)), "");
	xmlFreeDoc(doc);
}
END_TEST



START_TEST(correct_loc__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><loc>http://127.0.0.1</loc></url>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_str_eq(stack_url_pop(&urls), "http://127.0.0.1");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(correct_link_href__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><link href='http://127.0.0.1'/></url>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_str_eq(stack_url_pop(&urls), "http://127.0.0.1");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(correct_loc_and_link_href__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><loc>http://localhost</loc><link href='http://127.0.0.1'/></url>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_str_eq(stack_url_pop(&urls), "http://127.0.0.1");
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(null_root__sitemap_location_get_urls) {
	URLNode_t* urls = NULL;
	__sitemap_location_get_urls(NULL, &urls);
	ck_assert_ptr_eq(urls, NULL);
}
END_TEST

START_TEST(empty_root__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("");
	__sitemap_location_get_urls(getRoot(doc), &urls);
	ck_assert_ptr_eq(urls, NULL);
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(no_loc__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><loc></loc></url>");
	__sitemap_location_get_urls(getRoot(doc), &urls);
	ck_assert_ptr_eq(urls, NULL);
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(link_no_href__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><link /></url>");
	__sitemap_location_get_urls(getRoot(doc), &urls);
	ck_assert_ptr_eq(urls, NULL);
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(link_empty_href__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<url><link href=''/></url>");
	__sitemap_location_get_urls(getRoot(doc), &urls);
	ck_assert_ptr_eq(urls, NULL);
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(link_href_in_urls__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	stack_url_push(&urls, "http://127.0.0.1");
	doc = getXMLDoc("<url><link href='http://127.0.0.1'/></url>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_int_eq(stack_url_length(urls), 1);
	ck_assert_str_eq(stack_url_pop(&urls), "http://127.0.0.1");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(multiple_link_href_differents__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<urls><link href='http://localhost:8000'/><link href='http://localhost:4444'/></urls>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_int_eq(stack_url_length(urls), 2);
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost:4444");
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost:8000");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(multiple_link_href_same__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<urls><link href='http://localhost'/><link href='http://localhost'/></urls>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_int_eq(stack_url_length(urls), 1);
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(multiple_loc_differents__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<urls><loc>http://localhost:8000</loc><loc>http://localhost:4444</loc></urls>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_int_eq(stack_url_length(urls), 2);
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost:4444");
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost:8000");
	xmlFreeDoc(doc);
}
END_TEST

START_TEST(multiple_loc_same__sitemap_location_get_urls) {
	xmlDocPtr doc;
	URLNode_t* urls = NULL;
	doc = getXMLDoc("<urls><loc>http://localhost</loc><loc>http://localhost</loc></urls>");
	__sitemap_location_get_urls(getRoot(doc)->children, &urls);
	ck_assert_int_eq(stack_url_length(urls), 1);
	ck_assert_str_eq(stack_url_pop(&urls), "http://localhost");
	xmlFreeDoc(doc);
}
END_TEST


Suite* stack_urls_suite(void) {
    Suite* s;
    TCase* tc;
    s = suite_create("stack urls suite");
    tc = tcase_create("stack urls");
    tcase_add_test(tc, empty_stack_urls);
    tcase_add_test(tc, zero_length_stack_urls);
    tcase_add_test(tc, not_empty_stack_urls);
    tcase_add_test(tc, length_one_stack_urls);
    tcase_add_test(tc, length_five_stack_urls);
    tcase_add_test(tc, pop_null_stack_urls);
    tcase_add_test(tc, get_correct_value_length_one_stack_urls);
    tcase_add_test(tc, false_contains_stack_urls);
    tcase_add_test(tc, false_substr_contains_stack_urls);
    tcase_add_test(tc, true_contains_stack_urls);
    tcase_add_test(tc, get_differents_correct_value_length_five_stack_urls);
    tcase_add_test(tc, get_same_correct_values_length_five_stack_urls);

    suite_add_tcase(s, tc);
    return s;
}

Suite* stack_documents_suite(void) {
    Suite* s;
    TCase* tc;
    s = suite_create("stack documents suite");
    tc = tcase_create("stack documents");
    tcase_add_test(tc, zero_length_stack_documents);
    tcase_add_test(tc, one_length_stack_documents);
    tcase_add_test(tc, five_length_stack_documents);
    tcase_add_test(tc, pop_null_stack_documents);
    tcase_add_test(tc, get_correct_value_length_one_stack_documents);
    tcase_add_test(tc, get_differents_correct_value_length_five_stack_documents);
    tcase_add_test(tc, correct_length_stack_documents);

    suite_add_tcase(s, tc);
    return s;
}

Suite* trie_urls_suite(void) {
    Suite* s;
    TCase* tc;
    s = suite_create("trie urls suite");
    tc = tcase_create("trie urls");
    tcase_add_test(tc, create_trie_urls);
    tcase_add_test(tc, trie_add_simple_trie_urls);
    tcase_add_test(tc, trie_add_multiple_trie_urls);
    tcase_add_test(tc, trie_add_multiple_same_trie_urls);
    tcase_add_test(tc, contains_true_trie_urls);
    tcase_add_test(tc, contains_false_trie_urls);
    tcase_add_test(tc, _find_child_simple_trie_urls);
    tcase_add_test(tc, _find_child_multiple_trie_urls);

    suite_add_tcase(s, tc);
    return s;
}

Suite* utils_suite(void) {
    Suite* s;
    TCase* tc;
    s = suite_create("utils suite");
    tc = tcase_create("url_not_seen");
    tcase_add_test(tc, true_url_not_seen);
    tcase_add_test(tc, true_nothing_seen_url_not_seen);
    tcase_add_test(tc, false_url_not_seen);
    suite_add_tcase(s, tc);

    tc = tcase_create("is_same_domain");
    tcase_add_test(tc, empty_to_compare_is_same_domain);
    tcase_add_test(tc, empty_comparing_to_is_same_domain);
    tcase_add_test(tc, empty_can_be_subdomain_to_compare_is_same_domain);
    tcase_add_test(tc, empty_can_be_subdomain_comparing_to_is_same_domain);
    tcase_add_test(tc, true_is_same_domain);
    tcase_add_test(tc, true_can_be_subdomain_is_same_domain);
    tcase_add_test(tc, false_is_same_domain);
    tcase_add_test(tc, false_can_be_subdomain_is_same_domain);
    tcase_add_test(tc, true_subdomain_is_same_domain);
    tcase_add_test(tc, true_depth_subdomain_is_same_domain);
    tcase_add_test(tc, domain_to_compare_is_main_domain_is_same_domain);
    tcase_add_test(tc, almost_same_domains_is_same_domain);
    tcase_add_test(tc, almost_same_domains_2_is_same_domain);
    tcase_add_test(tc, almost_same_subdomains_is_same_domain);
    tcase_add_test(tc, true_subdomain_is_same_domain);
    suite_add_tcase(s, tc);

    tc = tcase_create("is_valid_link");
    tcase_add_test(tc, empty_is_valid_link);
    tcase_add_test(tc, true_is_valid_link);
    tcase_add_test(tc, true_2_is_valid_link);
    tcase_add_test(tc, true_3_is_valid_link);
    tcase_add_test(tc, false_not_http_scheme_is_valid_link);
    tcase_add_test(tc, false_mailto_is_valid_link);
    tcase_add_test(tc, false_javascript_is_valid_link);
    tcase_add_test(tc, false_tel_is_valid_link);
    tcase_add_test(tc, false_data_is_valid_link);
    suite_add_tcase(s, tc);

    tc = tcase_create("normalize_path");
    tcase_add_test(tc, nothing_change_file_normalize_path);
    tcase_add_test(tc, missing_root_directory_slash_normalize_path);
    tcase_add_test(tc, missing_root_file_slash_normalize_path);
    tcase_add_test(tc, empty_path_normalize_path);
    tcase_add_test(tc, nothing_change_directory_normalize_path);
    tcase_add_test(tc, normalize_dot_dot_directory_normalize_path);
    tcase_add_test(tc, normalize_dot_dot_dir_normalize_path);
    tcase_add_test(tc, current_dir_directory_normalize_path);
    tcase_add_test(tc, current_dir_file_normalize_path);
    tcase_add_test(tc, stop_root_dir_normalize_path);
    tcase_add_test(tc, normalize_multiple_slash_directory_normalize_path);
    tcase_add_test(tc, normalize_2_multiple_slash_directory_normalize_path);
    tcase_add_test(tc, normalize_dot_dot_file_normalize_path);
    tcase_add_test(tc, complex_directory_normalize_path);
    tcase_add_test(tc, complex_file_normalize_path);
    suite_add_tcase(s, tc);

    tc = tcase_create("is_disallowed_path");
    tcase_add_test(tc, true_one_disallowed_is_disallowed_path);
    tcase_add_test(tc, true_root_disallowed_is_disallowed_path);
    tcase_add_test(tc, false_one_disallowed_is_disallowed_path);
    tcase_add_test(tc, true_multiple_disallowed_is_disallowed_path);
    tcase_add_test(tc, true_multiple_disallowed_ends_with_slash_is_disallowed_path);
    tcase_add_test(tc, false_multiple_disallowed_is_disallowed_path);
    tcase_add_test(tc, false_multiple_disallowed_ends_with_slash_is_disallowed_path);
    tcase_add_test(tc, empty_path_is_disallowed_path);
    tcase_add_test(tc, empty_list_is_disallowed_path);
    tcase_add_test(tc, checked_path_substr_disallowed_is_disallowed_path);
    tcase_add_test(tc, disallowed_paths_substr_disallowed_is_disallowed_path);
    tcase_add_test(tc, disallowed_paths_regex_disallowed_is_disallowed_path);
    suite_add_tcase(s, tc);


    tc = tcase_create("is_allowed_extension");
    tcase_add_test(tc, true_one_allowed_is_allowed_extension);
    tcase_add_test(tc, false_one_allowed_is_allowed_extension);
    tcase_add_test(tc, true_multiple_allowed_is_allowed_extension);
    tcase_add_test(tc, false_multiple_allowed_is_allowed_extension);
    tcase_add_test(tc, empty_path_is_allowed_extension);
    tcase_add_test(tc, no_extension_is_allowed_extension);
    tcase_add_test(tc, no_file_is_allowed_extension);
    tcase_add_test(tc, empty_list_extensions_is_allowed_extension);
    tcase_add_test(tc, extension_substr_allowed_is_allowed_extension);
    suite_add_tcase(s, tc);

    tc = tcase_create("is_in_valid_domains");
    tcase_add_test(tc, true_is_in_valid_domains);
    tcase_add_test(tc, false_is_in_valid_domains);
    tcase_add_test(tc, empty_domain_is_in_valid_domains);
    tcase_add_test(tc, empty_allowed_domains_domain_is_in_valid_domains);
    suite_add_tcase(s, tc);

    tc = tcase_create("get_path_depth");
    tcase_add_test(tc, empty_get_path_depth);
    tcase_add_test(tc, one_get_path_depth);
    tcase_add_test(tc, three_file_get_path_depth);
    tcase_add_test(tc, three_directory_get_path_depth);
    tcase_add_test(tc, need_normalize_three_get_path_depth);
    suite_add_tcase(s, tc);
    return s;
}


Suite* sitemaps_parser_suite(void) {
    Suite* s;
    TCase* tc;
    s = suite_create("sitemaps parser suite");

    tc = tcase_create("__sitemap_get_location");
    tcase_add_test(tc, correct__sitemap_get_location);
    tcase_add_test(tc, correct_not_valid_url__sitemap_get_location);
    tcase_add_test(tc, empty_node__sitemap_get_location);
    tcase_add_test(tc, empty_value__sitemap_get_location);
    tcase_add_test(tc, null_root__sitemap_get_location);
    tcase_add_test(tc, multiple_values__sitemap_get_location);
    tcase_add_test(tc, no_loc_tag__sitemap_get_location);
    suite_add_tcase(s, tc);

    tc = tcase_create("__sitemap_location_get_urls");
    tcase_add_test(tc, correct_loc__sitemap_location_get_urls);
    tcase_add_test(tc, correct_link_href__sitemap_location_get_urls);
    tcase_add_test(tc, correct_loc_and_link_href__sitemap_location_get_urls);
    tcase_add_test(tc, null_root__sitemap_location_get_urls);
    tcase_add_test(tc, empty_root__sitemap_location_get_urls);
    tcase_add_test(tc, no_loc__sitemap_location_get_urls);
    tcase_add_test(tc, link_no_href__sitemap_location_get_urls);
    tcase_add_test(tc, link_empty_href__sitemap_location_get_urls);
    tcase_add_test(tc, link_href_in_urls__sitemap_location_get_urls);
    tcase_add_test(tc, multiple_link_href_differents__sitemap_location_get_urls);
    tcase_add_test(tc, multiple_link_href_same__sitemap_location_get_urls);
    tcase_add_test(tc, multiple_loc_differents__sitemap_location_get_urls);
    tcase_add_test(tc, multiple_loc_same__sitemap_location_get_urls);
    suite_add_tcase(s, tc);
    return s;
}


int main(void) {
    int number_failed = 0;
    SRunner* sr_stack_urls, *sr_stack_documents, *sr_trie_urls, *sr_utils, *sr_sitemaps_parser;

    Suite* s_stack_urls = stack_urls_suite();
    Suite* s_stack_documents = stack_documents_suite();
    Suite* s_trie_urls = trie_urls_suite();
    Suite* s_utils = utils_suite();
    Suite* s_sitemaps_parser = sitemaps_parser_suite();

    sr_stack_urls = srunner_create(s_stack_urls);
    sr_stack_documents = srunner_create(s_stack_documents);
    sr_trie_urls = srunner_create(s_trie_urls);
    sr_utils = srunner_create(s_utils);
    sr_sitemaps_parser = srunner_create(s_sitemaps_parser);

    srunner_run_all(sr_stack_urls, CK_NORMAL);
    srunner_run_all(sr_stack_documents, CK_NORMAL);
    srunner_run_all(sr_trie_urls, CK_NORMAL);
    srunner_run_all(sr_utils, CK_NORMAL);

    LIBXML_TEST_VERSION; // initialize libXML2
    srunner_run_all(sr_sitemaps_parser, CK_NORMAL);

    number_failed += srunner_ntests_failed(sr_stack_urls);
    number_failed += srunner_ntests_failed(sr_stack_documents);
    number_failed += srunner_ntests_failed(sr_trie_urls);
    number_failed += srunner_ntests_failed(sr_utils);
    number_failed += srunner_ntests_failed(sr_sitemaps_parser);

    srunner_free(sr_stack_urls);
    srunner_free(sr_stack_documents);
    srunner_free(sr_trie_urls);
    srunner_free(sr_utils);
    srunner_free(sr_sitemaps_parser);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
