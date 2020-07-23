#include "utils.h"
#include <string.h>

int canBeAdded(char* url, URLNode_t* urls_done, URLNode_t* urls_todo) {
    // Just check if a given url has already been seen.
    // Could just be:
    //  return !findURLList(*urls_done, urlFinal) && !findURLList(*urls_done, urlFinal)
    // but it would be longer as both findURLList are called every time
    if(findURLList(urls_done, url)) {
        return 0;
    }
    if(findURLList(urls_todo, url)) {
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

int isValidLink(const char* url) {
    if(url == NULL) {
        return 0;
    }
    return url[0] != '#' && strstr(url, "mailto:") != url && strstr(url, "tel:") != url && strstr(url, "data:") != url;
}