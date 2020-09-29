#include "utils.h"
#include <string.h>

int canBeAdded(char* url, URLNode_t* urls_done, URLNode_t* urls_todo) {
    // Just check if a given url has already been seen.
    // Could just be:
    //  return !findURLStack(*urls_done, urlFinal) && !findURLStack(*urls_done, urlFinal)
    // but it would be longer as both findURLStack are called every time
    if(findURLStack(urls_done, url)) {
        return 0;
    }
    if(findURLStack(urls_todo, url)) {
        return 0;
    }
    return 1;
}

int isValidDomain(char* domainToCompare, char* domain, int canBeSubDomain) {
    int strlen_domainToCompare = strlen(domainToCompare), strlen_domain = strlen(domain);

    if(strlen_domain == strlen_domainToCompare) {
        return strcmp(domainToCompare, domain) == 0;
    } else if(strlen_domain < strlen_domainToCompare && canBeSubDomain == 1) {
        char* foundPos = strstr(domainToCompare, domain);
        if(foundPos != NULL) {
            // need to check if the char before is a '.' (dot) because for exemple:
            // domainToCompare = "xyzb.a" and domain = "b.a" would result in true because
            // "b.a" is in both strings
            return strcmp(foundPos, domain) == 0 && *(foundPos - 1) == '.';
        }
    }
    return 0;
}

int isInValidDomains(char* domain, char** allowedDomains, int countAllowedDomains, int canBeSubDomain) {
    for(int i = 0; i < countAllowedDomains; i++) {
        if(isValidDomain(domain, allowedDomains[i], canBeSubDomain)) {
            return 1;
        }
    }
    return 0;
}

int isValidLink(const char* url) {
    if(url == NULL) {
        return 0;
    }
    char* scheme;
    CURLU* curl_u = curl_url();
    curl_url_set(curl_u, CURLUPART_URL, url, 0);
    curl_url_get(curl_u, CURLUPART_SCHEME, &scheme, 0);
    int result = 0;
    if(scheme == NULL) {
        result = 1;
    } else {
        if(strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0) {
            result = 1;
        }
    }
    curl_url_cleanup(curl_u);
    return result;
}

char* normalizePath(char* path, int isDirectory) {
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

    if(isDirectory) {
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


int isDisallowedPath(char* path, char** disallowedPaths, int countDisallowedPaths) {
    if(countDisallowedPaths == 0) { // if no paths where specified, then it is allowed
        return 0;
    }

    for(int i = 0; i < countDisallowedPaths; i++) {
        if(strstr(path, disallowedPaths[i]) == path) {
            return 1;
        }
    }
    return 0;
}

int isAllowedExtension(char* path, char** allowedExtensions, int countAllowedExtensions) {
    if(countAllowedExtensions == 0) { // if no extensions where specified, then it is allowed
        return 1;
    }
    char* filename = strrchr(path, '/'); // find last '/', this will give the name of the file
    if(filename != NULL) {
        char* ext = strrchr(filename, '.'); // find extension
        if(ext != NULL) { // if there is an extension
            for(int i = 0; i < countAllowedExtensions; i++) {
                if(strstr(path, allowedExtensions[i]) == ext) {
                    return 1;
                }
            }
        } else { // if there isn't any extension, then it is valid
            return 1;
        }
    }
    return 0;
}