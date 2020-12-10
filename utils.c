#include "utils.h"
#include <string.h>

int url_not_seen(char* url, URLNode_t* urls_done, URLNode_t* urls_todo) {
    // Just check if a given url has already been seen.
    // Could just be:
    //  return !stack_url_contains(*urls_done, url) && !stack_url_contains(*urls_done, url)
    // but it would be longer as both stack_url_contains are called every time
    if(stack_url_contains(urls_done, url)) {
        return 0;
    }
    if(stack_url_contains(urls_todo, url)) {
        return 0;
    }
    return 1;
}

int is_same_domain(char* domain_to_compare, char* domain, int allow_subdomain) {
    int len_domain_to_compare = strlen(domain_to_compare), len_domain = strlen(domain);

    if(len_domain == len_domain_to_compare) {
        return strcmp(domain_to_compare, domain) == 0;
    } else if(len_domain < len_domain_to_compare && allow_subdomain == 1) {
        char* found_position = strstr(domain_to_compare, domain);
        if(found_position != NULL) {
            // need to check if the char before is a '.' (dot) because for exemple:
            // domain_to_compare = "xyzb.a" and domain = "b.a" would result in true because
            // "b.a" is in both strings
            return strcmp(found_position, domain) == 0 && *(found_position - 1) == '.';
        }
    }
    return 0;
}

int is_in_valid_domains(char* domain, char** allowed_domains, int count_allowed_domains, int allow_subdomain) {
    for(int i = 0; i < count_allowed_domains; i++) {
        if(is_same_domain(domain, allowed_domains[i], allow_subdomain)) {
            return 1;
        }
    }
    return 0;
}

int is_valid_link(const char* url) {
    if(url == NULL) {
        return 0;
    }

    if(strncmp("mailto:", url, 7) == 0 || strncmp("javascript:", url, 11) == 0 || strncmp("tel:", url, 4) == 0) {
        return 0;
    }

    // check scheme
    char* scheme;
    CURLU* curl_u = curl_url();
    curl_url_set(curl_u, CURLUPART_URL, url, 0);
    curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);

    if(scheme == NULL) {
        free(scheme);
        curl_url_cleanup(curl_u);
        return 1;
    } else {
        if(strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0) {
            free(scheme);
            curl_url_cleanup(curl_u);
            return 1;
        }
    }
    free(scheme);
    curl_url_cleanup(curl_u);
    return 0;
}

char* normalize_path(char* path, int is_directory) {
    // remove './' and '../' from a given path
    if(path == NULL) {
        return NULL;
    }
    char* copy = strdup(path);

    char* elements[25] = { 0 };
    int index = 0;
    char* token;
    while((token = strsep(&copy, "/")) != NULL) {
        if(*token != '\0') {
            if(strcmp(token, ".") == 0) {
            } else if(strcmp(token, "..") == 0) {
                if(index >= 1) {
                    --index;
                    free(elements[index]);
                    elements[index] = NULL;
                }
            } else {
                elements[index] = strdup(token);
                index++;
                if(index >= 25) {
                    break;
                }
            }
        }
    }
    free(copy);

    char* result = calloc(1, 1);
    for(int i = 0; i < index; i++) {
        if(elements[i] == NULL) {
            continue;
        }
        char* tmp = strdup(result);
        int len = strlen(result) + strlen(elements[i]) + 2;
        result = (char*) realloc(result, len);
        strcpy(result, tmp);
        strcat(result, "/");
        strcat(result, elements[i]);
        free(tmp);
    }

    if(is_directory) {
        // add '/' at the end
        char* tmp = strdup(result);
        result = (char*) realloc(result, strlen(tmp)+1);
        strcpy(result, tmp);
        strcat(result, "/");
        free(tmp);
    }

    if(strlen(result) == 0) {
        result = realloc(result, 2);
        result[0] = '/';
    }
    return result;
}


int is_disallowed_path(char* path, char** disallowed_paths, int count_disallowed_paths) {
    if(count_disallowed_paths == 0) { // if no paths where specified, then it is allowed
        return 0;
    }

    for(int i = 0; i < count_disallowed_paths; i++) {
        if(strstr(path, disallowed_paths[i]) == path) {
            return 1;
        }
    }
    return 0;
}

int is_allowed_extension(char* path, char** allowed_extensions, int count_allowed_extensions) {
    if(count_allowed_extensions == 0) { // if no extensions where specified, then it is allowed
        return 1;
    }
    char* filename = strrchr(path, '/'); // find last '/', this will give the name of the file
    if(filename != NULL) {
        char* ext = strrchr(filename, '.'); // find extension
        if(ext != NULL) { // if there is an extension
            for(int i = 0; i < count_allowed_extensions; i++) {
                if(strstr(path, allowed_extensions[i]) == ext) {
                    return 1;
                }
            }
        } else { // if there isn't any extension, then it is valid
            return 1;
        }
    }
    return 0;
}

int get_path_depth(char* path) {
    if(path == NULL) {
        return 0;
    }

    char* copy = normalize_path(path, 0);

    if(strcmp(copy, "/") == 0) {
        free(copy);
        return 1;
    }

    int count = 0;
    char* token;
    while((token = strsep(&copy, "/")) != NULL) {
        if(*token != '\0') {
            count++;
        }
    }
    free(copy);
    return count;
}
