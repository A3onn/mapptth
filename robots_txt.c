#include "robots_txt.h"

size_t write_callback(char* new_content, size_t size, size_t nmemb, void* userdata) {
    char** content = (char**)userdata;
    size_t length_content = strlen(*content);

    *content = realloc(*content, length_content + size * nmemb);
    memcpy(&((*content)[length_content]), new_content, size * nmemb);

    return size * nmemb;
}

void get_robots_txt_urls(char* url, bool no_color, URLNode_t** list_urls_found) {
    (void) list_urls_found;
    CURL *curl;
    CURLcode res;

    char* content = malloc(1);
    content[0] = 0;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &content);
    if(no_color) {
        fprintf(stderr, "Fetching robots.txt: %s...\n", url);
    } else {
        fprintf(stderr, "%sFetching robots.txt: %s...%s\n", BLUE, url, RESET);
    }
    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        if(no_color) {
            fprintf(stderr, "Failed to fetch robots.txt: %s\n", url);
        } else {
            fprintf(stderr, "%sFailed to fetch robots.txt: %s%s\n", RED, url, RESET);
        }
        return ;
    }

    long status_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    if(status_code != 200) {
        if(no_color) {
            fprintf(stderr, "%s doesn't exist.\n", url);
        } else {
            fprintf(stderr, "%s%s doesn't exist.%s\n", RED, url, RESET);
        }
        return ;
    }

    curl_easy_cleanup(curl);

    if(no_color) {
        fprintf(stderr, "Finished fetching %s.\n", url);
    } else {
        fprintf(stderr, "%sFinished fetching %s.%s\n", GREEN, url, RESET);
    }

    char *line = strtok(content, "\n");
    while(line != NULL) {
        // remove comments
        char* next_comment = strchr(line, '#');
        if(next_comment != NULL) {
            line[next_comment-line] = '\0';
        }
        // cleanup spaces before at the beginning of the line
        while(*line == ' ') {
            line = line+1;
        }

        CURLU* curl_url_handler = curl_url();
        curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);

        if(strncmp(line, "Disallow:", 9) == 0) {
            char* trimed_path = trim_spaces(line + 9);
            if(strlen(trimed_path) == 0) {
                free(trimed_path);
                continue;
            }
            curl_url_set(curl_url_handler, CURLUPART_URL, trimed_path, 0);
        } else if(strncmp(line, "Allow:", 6) == 0) {
            char* trimed_path = trim_spaces(line + 6);
            if(strlen(trimed_path) == 0) {
                free(trimed_path);
                continue;
            }
            curl_url_set(curl_url_handler, CURLUPART_URL, trimed_path, 0);
        } else {
            curl_url_cleanup(curl_url_handler);
            line = strtok(NULL, "\n");
            continue;
        }
        // '*' in path is a wildcard, but doesn't indicate anything,
        // so we will have to cut the path before this wildcard
        char* path;
        curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);
        char* wildcard_char = strchr(path, '*');
        if(wildcard_char) {
            char* tmp = wildcard_char;
            // find past '/'
            while(*tmp != '/' && tmp > path) {
                tmp--;
            }
            // cut the path here
            *tmp = 0;
            curl_url_set(curl_url_handler, CURLUPART_PATH, path, 0);
            free(path);
        }


        char* url_res;
        curl_url_get(curl_url_handler, CURLUPART_URL, &url_res, 0);
        stack_url_push(list_urls_found, url_res);
        curl_url_cleanup(curl_url_handler);

        line = strtok(NULL, "\n");
    }

    if(no_color) {
        fprintf(stderr, "Finished parsing %s.\n", url);
    } else {
        fprintf(stderr, "%sFinished parsing %s.%s\n", BLUE, url, RESET);
    }
}