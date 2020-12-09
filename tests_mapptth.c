#include <check.h>
#include "stack_urls.h"
#include "stack_documents.h"


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
    ck_assert_ptr_null(stack_url_pop(&urls));
}
END_TEST

START_TEST(get_correct_value_length_one_stack_urls) {
    struct URLNode* urls = NULL;
    stack_url_push(&urls, "test");
    ck_assert_str_eq(stack_url_pop(&urls), "test");
}
END_TEST

START_TEST(get_differents_correct_value_length_five_stack_urls) {
    struct URLNode* urls = NULL;
    char str[35];
    for(int i = 0; i < 5; i++) {
        sprintf(str, "test%d", i*i);
        stack_url_push(&urls, str);
    }
    for(int i = 0; i < 5; i++) {
        sprintf(str, "test%d", i*i);
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
    ck_assert_ptr_null(stack_document_pop(&docs));
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

    char url_buf[50];
    char content_type_buf[50];
    char redirect_location_buf[50];

    for(int i = 0; i < 5; i++) {
        sprintf(url_buf, "url%d", i);
        sprintf(content_type_buf, "content_type%d", i);
        sprintf(redirect_location_buf, "redirect%d", i);
        stack_document_push(&docs, lxb_doc, url_buf, i, i*i, content_type_buf, redirect_location_buf);
    }

    for(int i = 4; i >= 0; i--) {
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

int main(void) {
    int number_failed = 0;
    SRunner* sr_stack_urls, * sr_stack_documents;

    Suite* s_stack_urls = stack_urls_suite();
    Suite* s_stack_documents = stack_documents_suite();

    sr_stack_urls = srunner_create(s_stack_urls);
    sr_stack_documents = srunner_create(s_stack_documents);

    srunner_run_all(sr_stack_urls, CK_NORMAL);
    srunner_run_all(sr_stack_documents, CK_NORMAL);

    number_failed += srunner_ntests_failed(sr_stack_urls);
    number_failed += srunner_ntests_failed(sr_stack_documents);

    srunner_free(sr_stack_urls);
    srunner_free(sr_stack_documents);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
