#include <check.h>
#include "stack_urls.h"
#include "stack_documents.h"
#include "utils.h"


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
        ck_assert_pstr_eq(stack_url_pop(&urls), str);
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


/* UTILS */
START_TEST(true_url_not_seen) {
    URLNode_t* urls_done = NULL, *urls_todo = NULL;
    stack_url_push(&urls_todo, "url1");
    stack_url_push(&urls_done, "url2");

    ck_assert_int_eq(url_not_seen("url3", urls_done, urls_todo), 1);
}
END_TEST

START_TEST(true_nothing_seen_url_not_seen) {
    URLNode_t* urls_done = NULL, *urls_todo = NULL;

    ck_assert_int_eq(url_not_seen("url1", urls_done, urls_todo), 1);
}
END_TEST

START_TEST(false_url_not_seen) {
    URLNode_t* urls_done = NULL, *urls_todo = NULL;

    stack_url_push(&urls_done, "url1");
    ck_assert_int_eq(url_not_seen("url1", urls_done, urls_todo), 0);
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
    char* dis_paths[] = {"/files"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 1);
}
END_TEST

START_TEST(false_one_disallowed_is_disallowed_path) {
    char* dis_paths[] = {"/images"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 0);
}
END_TEST

START_TEST(true_multiple_disallowed_is_disallowed_path) {
    char* dis_paths[] = {"/images", "/js", "/files"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 1);
}
END_TEST

START_TEST(false_multiple_disallowed_is_disallowed_path) {
    char* dis_paths[] = {"/images", "/js", "/css"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 3), 0);
}
END_TEST

START_TEST(empty_path_is_disallowed_path) {
    char* dis_paths[] = {"/images", "/js"};
    ck_assert_int_eq(is_disallowed_path("", dis_paths, 2), 0);
}
END_TEST

START_TEST(empty_list_is_disallowed_path) {
    char* dis_paths[] = {};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 0), 0);
}
END_TEST

START_TEST(checked_path_substr_disallowed_is_disallowed_path) {
    char* dis_paths[] = {"/files_images", "/images_files"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 2), 0);
}
END_TEST

START_TEST(disallowed_paths_substr_disallowed_is_disallowed_path) {
    char* dis_paths[] = {"/file"};
    ck_assert_int_eq(is_disallowed_path("/files/file", dis_paths, 1), 0);
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
    char* allowed_exts[] = {};
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
    char* domains[] = {};
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
    suite_add_tcase(s, tc);

    tc = tcase_create("normalize_path");
    tcase_add_test(tc, nothing_change_file_normalize_path);
    tcase_add_test(tc, missing_root_directory_slash_normalize_path);
    tcase_add_test(tc, missing_root_file_slash_normalize_path);
    tcase_add_test(tc, empty_path_normalize_path);
    tcase_add_test(tc, nothing_change_directory_normalize_path);
    tcase_add_test(tc, normalize_dot_dot_directory_normalize_path);
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
    tcase_add_test(tc, false_one_disallowed_is_disallowed_path);
    tcase_add_test(tc, true_multiple_disallowed_is_disallowed_path);
    tcase_add_test(tc, false_multiple_disallowed_is_disallowed_path);
    tcase_add_test(tc, empty_path_is_disallowed_path);
    tcase_add_test(tc, empty_list_is_disallowed_path);
    tcase_add_test(tc, checked_path_substr_disallowed_is_disallowed_path);
    tcase_add_test(tc, disallowed_paths_substr_disallowed_is_disallowed_path);
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


int main(void) {
    int number_failed = 0;
    SRunner* sr_stack_urls, * sr_stack_documents, *sr_utils;

    Suite* s_stack_urls = stack_urls_suite();
    Suite* s_stack_documents = stack_documents_suite();
    Suite* s_utils = utils_suite();

    sr_stack_urls = srunner_create(s_stack_urls);
    sr_stack_documents = srunner_create(s_stack_documents);
    sr_utils = srunner_create(s_utils);

    srunner_run_all(sr_stack_urls, CK_NORMAL);
    srunner_run_all(sr_stack_documents, CK_NORMAL);
    srunner_run_all(sr_utils, CK_NORMAL);

    number_failed += srunner_ntests_failed(sr_stack_urls);
    number_failed += srunner_ntests_failed(sr_stack_documents);
    number_failed += srunner_ntests_failed(sr_utils);

    srunner_free(sr_stack_urls);
    srunner_free(sr_stack_documents);
    srunner_free(sr_utils);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
