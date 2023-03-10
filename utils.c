#include "utils.h"

bool url_not_seen(char* url, struct TrieNode* urls_done, URLNode_t* urls_todo) {
    // Just check if a given url has already been seen.
    if(trie_contains(urls_done, url)) {
        return false;
    }
    if(stack_url_contains(urls_todo, url)) {
        return false;
    }
    return true;
}

bool is_same_domain(char* domain_to_compare, char* domain, bool allow_subdomain) {
    size_t len_domain_to_compare = strlen(domain_to_compare), len_domain = strlen(domain);

    if(len_domain == len_domain_to_compare) {
        return strcmp(domain_to_compare, domain) == 0;
    } else if(len_domain < len_domain_to_compare && allow_subdomain == true) {
        char* found_position = strstr(domain_to_compare, domain);
        if(found_position != NULL) {
            // need to check if the char before is a '.' (dot) because for example:
            // domain_to_compare = "xyzb.a" and domain = "b.a" would result in true because
            // "b.a" is in both strings
            return strcmp(found_position, domain) == 0 && *(found_position - 1) == '.';
        }
    }
    return false;
}

bool is_in_valid_domains(char* domain, char** allowed_domains, int count_allowed_domains, bool allow_subdomain) {
    for(int i = 0; i < count_allowed_domains; i++) {
        if(is_same_domain(domain, allowed_domains[i], allow_subdomain)) {
            return true;
        }
    }
    return false;
}

bool is_in_disallowed_domains(char* domain, char** disallowed_domains, int count_disallowed_domains) {
    for(int i = 0; i < count_disallowed_domains; i++) {
        if(is_same_domain(domain, disallowed_domains[i], false)) {
            return true;
        }
    }
    return false;
}

bool is_valid_link(const char* url) {
    if(url == NULL) {
        return false;
    }

    // need to check these because curl doesn't set them as scheme, so next check will fail to filter them
    if(strncmp("javascript:", url, 11) == 0 || strncmp("data:", url, 5) == 0 || strncmp("mailto:", url, 7) == 0 || strncmp("tel:", url, 4) == 0) {
        return false;
    }

    // check scheme
    char* scheme;
    CURLU* curl_u = curl_url();
    curl_url_set(curl_u, CURLUPART_URL, url, 0);
    curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);

    if(scheme == NULL) {
        free(scheme);
        curl_url_cleanup(curl_u);
        return true;
    } else {
        if(strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0) {
            free(scheme);
            curl_url_cleanup(curl_u);
            return true;
        }
    }
    free(scheme);
    curl_url_cleanup(curl_u);
    return false;
}

char* normalize_path(char* path, bool is_directory) {
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

char* trim_spaces(char* str) {
    if(str == NULL) {
        return NULL;
    }
    size_t len_str = strlen(str);
    if(len_str == 0) {
        return calloc(1, sizeof (char));
    }
    char* tmp_res = malloc(len_str+1);
    strcpy(tmp_res, str);
    tmp_res[len_str] = 0;

    size_t start = 0, end = len_str;
    while(tmp_res[start] == ' ' && end != start) {
        start++;
    }
    while(tmp_res[end-1] == ' ' && end != start) {
        end--;
    }
    if(end-start == 0) {
        free(tmp_res);
        return calloc(1, sizeof (char));
    }
    char* res = calloc(end-start, sizeof (char));
    strncpy(res, tmp_res + start, end-start);
    free(tmp_res);
    return res;

}

bool is_disallowed_path(char* path, pcre** disallowed_paths, int count_disallowed_paths) {
    if(count_disallowed_paths == 0) { // if no paths where specified, then it is allowed
        return false;
    }

    int path_len = strlen(path);

    // will search through all paths and it will try to find one mathching with the path to check
    int ovectors[100];
    for(int i = 0; i < count_disallowed_paths; i++) {
        int result = pcre_exec(disallowed_paths[i], NULL, path, path_len, 0, PCRE_ANCHORED|PCRE_PARTIAL_SOFT, ovectors, sizeof(ovectors));
        // checks if there is a match, if that match ends on a '/' or on a '\0' or the char before is a '/'
        if(result >= 0 && (path[ovectors[1] - ovectors[0]] == '/' || path[ovectors[1] - ovectors[0]-1] == '/' || path[ovectors[1] - ovectors[0]] == '\0')) {
            return true;
        }
    }
    return false;
}

bool is_allowed_path(char* path, pcre** allowed_paths, int count_allowed_paths) {
  // if no paths where specified, then it is allowed, this check is
  // needed here because in is_disallowed_path, it would return 0 instead
  // of 1
  if(count_allowed_paths == 0) {
      return true;
  }
  // as is_disallowed_path will search for a matching path in the list of paths,
  // we only need to call it with the list of allowed paths instead
  return is_disallowed_path(path, allowed_paths, count_allowed_paths);
}

bool is_allowed_extension(char* path, char** allowed_extensions, int count_allowed_extensions) {
    if(count_allowed_extensions == 0) { // if no extensions where specified, then it is allowed
        return true;
    }
    char* filename = strrchr(path, '/'); // find last '/', this will give the name of the file
    if(filename != NULL) {
        char* ext = strrchr(filename, '.'); // find extension
        if(ext != NULL) { // if there is an extension
            for(int i = 0; i < count_allowed_extensions; i++) {
                char* found_ext = strstr(path, allowed_extensions[i]);
                if(found_ext == ext && strcmp(allowed_extensions[i], found_ext) == 0) {
                    return true;
                }
            }
        } else { // if there isn't any extension, then it is valid
            return true;
        }
    }
    return false;
}

bool is_disallowed_extension(char* path, char** disallowed_extensions, int count_disallowed_extensions) {
  if(count_disallowed_extensions == 0) { // if no extensions where specified, then it is allowed
      return false;
  }
  char* filename = strrchr(path, '/'); // find last '/', this will give the name of the file
  if(filename != NULL) {
      char* ext = strrchr(filename, '.'); // find extension
      if(ext != NULL) { // if there is an extension
          for(int i = 0; i < count_disallowed_extensions; i++) {
              char* found_ext = strstr(path, disallowed_extensions[i]);
              if(found_ext == ext && strcmp(disallowed_extensions[i], found_ext) == 0) {
                  return true;
              }
          }
      } else { // if there isn't any extension, then it is valid
          return false;
      }
  }
  return false;
}

bool is_allowed_port(unsigned short port, unsigned short* allowed_ports, int count_allowed_short) {
    for(int i = 0; i < count_allowed_short; i++) {
        if(port == allowed_ports[i]) {
            return true;
        }
    }
    return false;
}

unsigned int get_path_depth(char* path) {
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

unsigned short get_port_from_url(char* url) {
    errno = 0;
    CURLU* curl_url_handler = curl_url();
    char* scheme, *port;
    curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);
    curl_url_get(curl_url_handler, CURLUPART_PORT, &port, 0);
    if(port) {
        char* endptr;
        unsigned short result_port = (unsigned short) strtoul(port, &endptr, 10);
        if(errno != 0 || endptr == port) { // should not happen
            errno = EINVAL;
            curl_url_cleanup(curl_url_handler);
            free(port);
            return 0;
        }
        free(port);
        curl_url_cleanup(curl_url_handler);
        return result_port;
    }
    free(port);

    curl_url_get(curl_url_handler, CURLUPART_SCHEME, &scheme, 0);
    if(scheme) {
        if(strcmp(scheme, "http") == 0) {
	    free(scheme);
	    curl_url_cleanup(curl_url_handler);
            return 80;
        } else if(strcmp(scheme, "https") == 0) {
	    free(scheme);
	    curl_url_cleanup(curl_url_handler);
            return 443;
        }
    }
    free(scheme);
    curl_url_cleanup(curl_url_handler);
    errno = EINVAL;
    return 0;
}

char* get_base_tag_value(lxb_html_document_t* document) {
    lxb_dom_collection_t* collection = 	lxb_dom_collection_create(lxb_html_document_original_ref(document));

    // init collection with 1 as len as only one base should be allowed
    lxb_dom_collection_init(collection, 1);

    char* url = NULL;

    if(document->head != NULL) {
        lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->head), collection, (const lxb_char_t*) "base", 4);
    }

    // should not happen (by the standard) by just in case
    if(document->body != NULL) {
        lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body), collection, (const lxb_char_t*) "base", 4);
    }

    // will loop through the bases and return the first base tag with a href attribute
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/base#multiple_%3Cbase%3E_elements
    for(size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
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
