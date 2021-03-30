#include "utils.h"
#include "stack_urls.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


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
    size_t len_domain_to_compare = strlen(domain_to_compare), len_domain = strlen(domain);

    if(len_domain == len_domain_to_compare) {
        return strcmp(domain_to_compare, domain) == 0;
    } else if(len_domain < len_domain_to_compare && allow_subdomain == 1) {
        char* found_position = strstr(domain_to_compare, domain);
        if(found_position != NULL) {
            // need to check if the char before is a '.' (dot) because for example:
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

int is_in_disallowed_domains(char* domain, char** disallowed_domains, int count_disallowed_domains) {
    for(int i = 0; i < count_disallowed_domains; i++) {
        if(is_same_domain(domain, disallowed_domains[i], false)) {
            return 1;
        }
    }
    return 0;
}

int is_valid_link(const char* url) {
    if(url == NULL) {
        return 0;
    }

    // need to check these because curl doesn't set them as scheme, so next check will fail to filter them
    if(strncmp("mailto:", url, 7) == 0 || strncmp("javascript:", url, 11) == 0 || strncmp("tel:", url, 4) == 0 || strncmp("data:", url, 5) == 0) {
        return 0;
    }

    // check scheme
    char* scheme;
    CURLU* curl_u = curl_url();
    curl_url_set(curl_u, CURLUPART_URL, url, 0);
    curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);

    printf("scheme of %s: %s\n", url, scheme);

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
    char* copy, *copy_origin;
    copy = copy_origin = strdup(path);

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
    free(copy_origin);

    char* result = calloc(1, 1);
    for(int i = 0; i < index; i++) { // go through the path
        if(elements[i] == NULL) {
            continue;
        }
        char* tmp = strdup(result);
        size_t len = strlen(result) + strlen(elements[i]) + 2; // len current path + len element + '/' + '\0'
        result = (char*) realloc(result, len);
        strcpy(result, tmp); // copy path from before
        strcat(result, "/"); // add '/' at the end of the path
        strcat(result, elements[i]); // add directory/file name
        free(tmp);
        free(elements[i]);
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

    // will search through all paths and it will try to find one mathching with the path to check
    for(int i = 0; i < count_disallowed_paths; i++) {
	if(strncmp(disallowed_paths[i], "/", 2) == 0) {
		return 1;
	}
        size_t found_length = strlen(disallowed_paths[i]);
        if(strstr(path, disallowed_paths[i]) == path && (*(path+found_length) == '/' || *(path+found_length) == '\0')) {
            // check if the path found is at the beginning of the path, and need to check if the following char is '\' or the
            // end of the string to avoid wrong errors like:
            // is_disallowed_path("/path_of_file", {"/path"}, 1) would return 1
            return 1;
        }
    }
    return 0;
}

int is_allowed_path(char* path, char** allowed_paths, int count_allowed_paths) {
  // if no paths where specified, then it is allowed, this check is
  // needed here because in is_disallowed_path, it would return 0 instead
  // of 1
  if(count_allowed_paths == 0) {
      return 1;
  }
  // as is_disallowed_path will search for a matching path in the list of paths,
  // we only need to call it with the list of allowed paths instead
  return is_disallowed_path(path, allowed_paths, count_allowed_paths);
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
                char* found_ext = strstr(path, allowed_extensions[i]);
                if(found_ext == ext && strcmp(allowed_extensions[i], found_ext) == 0) {
                    return 1;
                }
            }
        } else { // if there isn't any extension, then it is valid
            return 1;
        }
    }
    return 0;
}

int is_disallowed_extension(char* path, char** disallowed_extensions, int count_disallowed_extensions) {
  if(count_disallowed_extensions == 0) { // if no extensions where specified, then it is allowed
      return 0;
  }
  char* filename = strrchr(path, '/'); // find last '/', this will give the name of the file
  if(filename != NULL) {
      char* ext = strrchr(filename, '.'); // find extension
      if(ext != NULL) { // if there is an extension
          for(int i = 0; i < count_disallowed_extensions; i++) {
              char* found_ext = strstr(path, disallowed_extensions[i]);
              if(found_ext == ext && strcmp(disallowed_extensions[i], found_ext) == 0) {
                  return 1;
              }
          }
      } else { // if there isn't any extension, then it is valid
          return 0;
      }
  }
  return 0;
}

int get_path_depth(char* path) {
    if(path == NULL) {
        return 0;
    }

    char* copy, *copy_origin;
    copy = copy_origin = normalize_path(path, 0);

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
    free(copy_origin);
    return count;
}


char* get_base_tag_value(lxb_html_document_t* document) {
    lxb_dom_collection_t* collection = 	lxb_dom_collection_create(lxb_html_document_original_ref(document));

    // init collection with 1 as len as only one base should be allowed
    lxb_dom_collection_init(collection, 1);

    char* url = NULL;

    lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->head), collection, (const lxb_char_t*) "base", 4);

    // should not happen (by the standard) by just in case
    lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body), collection, (const lxb_char_t*) "base", 4);

    // will loop through the bases and return the first base tag with a href attribute
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/base#multiple_%3Cbase%3E_elements
    for(int i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);
        if(lxb_dom_element_has_attribute(element, (lxb_char_t*) "href", 4)) {
            // if it has a href, get it and break in order to free the collection
            url = (char*) lxb_dom_element_get_attribute(element, (const lxb_char_t*) "href", 4, NULL);
            break;
        }
    }

    lxb_dom_collection_destroy(collection, 1);
    
    if(url == NULL) {
        return NULL;
    }

    if(is_valid_link(url)) {
        // return a copy
        char* result = (char*) malloc(sizeof (char) * strlen(url) + 1); // url + \0
        strcpy(result, url);
        return result;
    } else {
        return normalize_path(url, 1);
    }
}

// DEBUG
int _debug = 0;
void _debug_print(const char* function_name, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if(_debug) {
        printf("%s[debug]%s thread #%lu %s: ", BLUE, RESET, pthread_self(), function_name);
        vprintf(format, args);
    }

    va_end(args);
}

