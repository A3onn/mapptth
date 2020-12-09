#include <check.h>
#include "stack_urls.h"

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

int main(void) {
    int number_failed;
    SRunner* sr_stack_urls;

    Suite* s_stack_urls = stack_urls_suite();

    sr_stack_urls = srunner_create(s_stack_urls);

    srunner_run_all(sr_stack_urls, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr_stack_urls);
    srunner_free(sr_stack_urls);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
