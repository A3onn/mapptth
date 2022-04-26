#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pcre.h>

#include "fetcher_thread.h"
#include "sitemaps_parser.h"
#include "stack_documents.h"
#include "stack_urls.h"
#include "trie_urls.h"
#include "utils.h"
#include "cli_parser.h"
#include "logger.h"

#if GRAPHVIZ_SUPPORT
#include <graphviz/gvc.h>
#endif

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "libcurl 7.62.0 or later is required"
#endif

struct FoundURLHandlerBundle {
    char* found_url; // url to handle
    struct Document* document; // only document->url will be used from this struct
    URLNode_t** urls_stack_todo;
    struct TrieNode** urls_done;
    pthread_cond_t* cv_url_added;
    bool allow_subdomains;
    char** allowed_domains;
    unsigned int count_allowed_domains;
    char** disallowed_domains;
    unsigned int count_disallowed_domains;
    char** allowed_extensions;
    unsigned int count_allowed_extensions;
    char** disallowed_extensions;
    unsigned int count_disallowed_extensions;
    pcre** disallowed_paths;
    unsigned int count_disallowed_paths;
    pcre** allowed_paths;
    unsigned int count_allowed_paths;
    unsigned short* allowed_ports;
    unsigned int count_allowed_ports;
    bool http_only;
    bool https_only;
    bool keep_query;
    unsigned int max_path_depth;
    char* base_tag_url;
#if GRAPHVIZ_SUPPORT
    bool generate_graph;
    Agraph_t* graph;
#endif
};

bool sigint_received = false; // if the user has CTRL-C
void sigint_handler(int signum) {
    (void) signum;
    sigint_received = true;
}

#define handle_found_url(X) _handle_found_url(X, false)
#define handle_found_url_from_sitemap(X) _handle_found_url(X, true)
static inline int _handle_found_url(struct FoundURLHandlerBundle* bundle, bool from_sitemap_parsing);

lexbor_action_t walk_cb(lxb_dom_node_t* node, void* ctx) {
    // this function will be called for every nodes

    if(node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return LEXBOR_ACTION_OK;
    }

    lxb_dom_element_t* element = lxb_dom_interface_element(node);
    // check if the element has a 'href' or a 'src' attribute
    if(!lxb_dom_element_has_attribute(element, (lxb_char_t*) "href", 4) && !lxb_dom_element_has_attribute(element, (lxb_char_t*) "src", 3)) {
        return LEXBOR_ACTION_OK;
    }

    // avoid <base> tags because they are treated before this function is called
    if(strcmp((char*) lxb_dom_element_tag_name(element, NULL), "BASE") == 0) {
        return LEXBOR_ACTION_OK;
    }

    struct FoundURLHandlerBundle* bundle = (struct FoundURLHandlerBundle*) ctx;

    // try to get the 'href' attribute if it has one, 'src' attribute if it
    // doesn't have an 'href' attribute instead
    // note that the element has to have either one as it has been checked just before
    char* found_url;
    found_url = (char*) lxb_dom_element_get_attribute(element, (lxb_char_t*) "href", 4, NULL);
    if(found_url == NULL) {  // if this element has a src attribute instead
        found_url = (char*) lxb_dom_element_get_attribute(element, (lxb_char_t*) "src", 3, NULL);
        if(found_url == NULL) {  // should not happen
            return LEXBOR_ACTION_OK;
        }
    }

    bundle->found_url = found_url;
    int result = handle_found_url(bundle);
    if(result) {
        LOG("Found in %s a <%s> containing: %s\n", bundle->document->url, lxb_dom_element_tag_name(element, NULL), found_url);
    }

    return LEXBOR_ACTION_OK;
}

static void lock_cb(CURL* handle, curl_lock_data data, curl_lock_access access, void* userptr) {
    (void)handle;
    (void)data;
    (void)access;
    pthread_mutex_lock((pthread_mutex_t*) userptr);
}

static void unlock_cb(CURL* handle, curl_lock_data data, void* userptr) {
    (void)handle;
    (void)data;
    (void)access;
    pthread_mutex_unlock((pthread_mutex_t*) userptr);
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffer
    fprintf(stderr, "\n _______             ______ _______ _______ _     _ \n"
           "(_______)           (_____ (_______|_______|_)   (_)\n"
           " _  _  _ _____ ____  _____) )  _       _    _______ \n"
           "| ||_|| (____ |  _ \\|  ____/  | |     | |  |  ___  |\n"
           "| |   | / ___ | |_| | |       | |     | |  | |   | |\n"
           "|_|   |_\\_____|  __/|_|       |_|     |_|  |_|   |_|\n"
           "              |_|                                   \n"
           "Version %s\n\n",
        MAPPTTH_VERSION);
    struct arguments* cli_arguments;
    if((cli_arguments = parse_cli_arguments(argc, argv)) == NULL) {
        return 1;
    }

    if(cli_arguments->verbose) {
        ACTIVATE_VERBOSE();
    }

    if((strncmp(cli_arguments->url, "http://", 7) != 0 && strncmp(cli_arguments->url, "https://", 8) != 0) || strchr(cli_arguments->url, ' ') != NULL) {
        fprintf(stderr, "%s: invalid URL: %s\n", argv[0], cli_arguments->url);
        return 1;
    }
    if(cli_arguments->threads <= 0) {
        fprintf(stderr, "%s: the number of threads should be positive\n", argv[0]);
        return 1;
    }
    if(cli_arguments->timeout <= 0) {
        fprintf(stderr, "%s: the timeout should be positive\n", argv[0]);
        return 1;
    }
    if(cli_arguments->max_depth_given && cli_arguments->max_depth <= 0) {
        fprintf(stderr, "%s: max-depth have to be positive\n", argv[0]);
        return 1;
    }
    for(unsigned int i = 0; i < cli_arguments->allowed_extensions_count; i++) {
        if(cli_arguments->allowed_extensions[i][0] != '.') {
            fprintf(stderr, "%s: extensions have to begin with a '.' (dot)\n", argv[0]);
            return 1;
        }
    }
    for(unsigned int i = 0; i < cli_arguments->disallowed_extensions_count; i++) {
        if(cli_arguments->disallowed_extensions[i][0] != '.') {
            fprintf(stderr, "%s: extensions have to begin with a '.' (dot)\n", argv[0]);
            return 1;
        }
    }

    // compile paths into regex
    int regex_error_offset;
    const char* regex_error_str;
    pcre** disallowed_paths = (pcre**) malloc(sizeof(pcre*) * cli_arguments->disallowed_paths_count);
    for(unsigned int i = 0; i < cli_arguments->disallowed_paths_count; i++) {
        disallowed_paths[i] = pcre_compile(cli_arguments->disallowed_paths[i], PCRE_NO_AUTO_CAPTURE, &regex_error_str, &regex_error_offset, 0);
        if(disallowed_paths[i] == NULL) {
            fprintf(stderr, "%s: Invalid regex: \"%s\" at %d (%s)\n", argv[0], cli_arguments->disallowed_paths[i], regex_error_offset, regex_error_str);
            return 1;
        }
    }
    pcre** allowed_paths = (pcre**) malloc(sizeof(pcre*) * cli_arguments->allowed_paths_count);
    for(unsigned int i = 0; i < cli_arguments->allowed_paths_count; i++) {
        allowed_paths[i] = pcre_compile(cli_arguments->allowed_paths[i], PCRE_NO_AUTO_CAPTURE, &regex_error_str, &regex_error_offset, 0);
        if(allowed_paths[i] == NULL) {
            fprintf(stderr, "%s: Invalid regex: \"%s\" at %d (%s)\n", argv[0], cli_arguments->allowed_paths[i], regex_error_offset, regex_error_str);
            return 1;
        }
    }

    // add the starting port to the list of allowed ports
    unsigned short starting_port = get_port_from_url(cli_arguments->url);
    if(errno == EINVAL) {
        fprintf(stderr, "%s: An error occured while processing url: '%s': %s\n", argv[0], cli_arguments->url, strerror(errno));
        return 1;
    }
    cli_arguments->allowed_ports_count++;
    cli_arguments->allowed_ports = (unsigned short*) reallocarray(cli_arguments->allowed_ports, cli_arguments->allowed_ports_count, sizeof (unsigned short));
    cli_arguments->allowed_ports[cli_arguments->allowed_ports_count-1] = starting_port;

    int resolve_ip_version = CURL_IPRESOLVE_WHATEVER;
    if(cli_arguments->only_ipv4_flag) {
        resolve_ip_version = CURL_IPRESOLVE_V4;
    } else if(cli_arguments->only_ipv6_flag) {
        resolve_ip_version = CURL_IPRESOLVE_V6;
    }

    // end checks

    // set sigint handler
    struct sigaction sighandler;
    sighandler.sa_handler = sigint_handler;
    sigaction(SIGINT, &sighandler, NULL);

    // set variables
    curl_global_init(CURL_GLOBAL_ALL);  // initialize libcurl

    pthread_t fetcher_threads[cli_arguments->threads]; // threads will be created after the sitemap parsing
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_conn = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cv_url_added = PTHREAD_COND_INITIALIZER;
    pthread_cond_t cv_fetcher_produced = PTHREAD_COND_INITIALIZER;

    DocumentNode_t* documents_stack = NULL;

    URLNode_t* urls_stack_todo = NULL;
    struct TrieNode* urls_done = trie_create();

    // create shared interface
    CURLSH* curl_share = curl_share_init();
    if(curl_share == NULL) {
        fprintf(stderr, "Could not create share interface. Quitting...");
        curl_global_cleanup();
        return 1;
    }

    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
    curl_share_setopt(curl_share, CURLSHOPT_LOCKFUNC, lock_cb);
    curl_share_setopt(curl_share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
    curl_share_setopt(curl_share, CURLSHOPT_USERDATA, (void*) &mutex_conn);

    stack_url_push(&urls_stack_todo, cli_arguments->url);  // add the URL given as parameter

    struct FoundURLHandlerBundle found_url_handler_bundle;  // in this bundle these elements never change
    found_url_handler_bundle.allowed_domains = cli_arguments->allowed_domains;
    found_url_handler_bundle.count_allowed_domains = cli_arguments->allowed_domains_count;
    found_url_handler_bundle.allowed_ports = cli_arguments->allowed_ports;
    found_url_handler_bundle.count_allowed_ports = cli_arguments->allowed_ports_count;
    found_url_handler_bundle.disallowed_domains = cli_arguments->disallowed_domains;
    found_url_handler_bundle.count_disallowed_domains = cli_arguments->disallowed_domains_count;
    found_url_handler_bundle.allowed_extensions = cli_arguments->allowed_extensions;
    found_url_handler_bundle.count_allowed_extensions = cli_arguments->allowed_extensions_count;
    found_url_handler_bundle.disallowed_extensions = cli_arguments->disallowed_extensions;
    found_url_handler_bundle.count_disallowed_extensions = cli_arguments->disallowed_extensions_count;
    found_url_handler_bundle.allow_subdomains = cli_arguments->allow_subdomains_flag;
    found_url_handler_bundle.http_only = cli_arguments->http_only_flag;
    found_url_handler_bundle.https_only = cli_arguments->https_only_flag;
    found_url_handler_bundle.disallowed_paths = disallowed_paths;
    found_url_handler_bundle.count_disallowed_paths = cli_arguments->disallowed_paths_count;
    found_url_handler_bundle.allowed_paths = allowed_paths;
    found_url_handler_bundle.count_allowed_paths = cli_arguments->allowed_paths_count;
    found_url_handler_bundle.keep_query = cli_arguments->keep_query_flag;
    found_url_handler_bundle.max_path_depth = cli_arguments->max_depth;
    found_url_handler_bundle.cv_url_added = &cv_url_added;
#if GRAPHVIZ_SUPPORT
    found_url_handler_bundle.generate_graph = cli_arguments->graph_flag;
    if(cli_arguments->graph_flag) {
        found_url_handler_bundle.graph = agopen(cli_arguments->url, Agstrictdirected, 0);
        Agnode_t* first_node = agnode(found_url_handler_bundle.graph, cli_arguments->url, 1); // add initial node

        // https://graphviz.org/doc/info/attrs.html#d:root
        agsafeset(first_node, "root", "true", "true"); // set the node as the root (used by circo and twopi)

        agsafeset(first_node, "URL", cli_arguments->url, cli_arguments->url); // used by svg
    }
#endif

    FILE* output_file = NULL;
    if(cli_arguments->output != NULL) {
        LOG("Opening %s...\n", cli_arguments->output);
        output_file = fopen(cli_arguments->output, "w");
        if(output_file == NULL) {
            fprintf(stderr, "%s: failed to open %s: %s.\n", argv[0], cli_arguments->output, strerror(errno));
            return 1;
        }
        // put some infos at the beginning of the file
        char time_buf[200];
        time_t curr_time = time(NULL);
        struct tm* tmp = localtime(&curr_time);
        strftime(time_buf, 200, "%c", tmp);

        fprintf(output_file, "# MapPTTH started at %s as:", time_buf);
        for(int i = 0; i < argc; i++) {
            fprintf(output_file, " %s", argv[i]);
        }
        fprintf(output_file, "\n");
        LOG("Wrote header in %s\n", cli_arguments->output);
    }

    if(cli_arguments->sitemap != NULL) {
        LOG("Fetching and parsing sitemaps\n");
        // get the content of the sitemap
        URLNode_t* url_stack_sitemap = NULL;
        get_sitemap_urls(cli_arguments->sitemap, cli_arguments->no_color_flag, &url_stack_sitemap);

        // validate the URLs found in the sitemap, same code as when finding an URL in a document
        CURLU* curl_url_handler = curl_url();
        char* initial_url_domain;
        curl_url_set(curl_url_handler, CURLUPART_URL, cli_arguments->url, 0);
        curl_url_get(curl_url_handler, CURLUPART_HOST, &initial_url_domain, 0);

        int count = 0;  // keep track of how many validated URLs were added
        while(!stack_url_isempty(url_stack_sitemap)) {  // loop over URLs
            char* url = stack_url_pop(&url_stack_sitemap);
            char* found_url_domain;

            curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);
            curl_url_get(curl_url_handler, CURLUPART_HOST, &found_url_domain, 0);
            if(!is_same_domain(found_url_domain, initial_url_domain, 0)) { // filter out urls not from the same domain
                continue;
            }

            // handle_found_url uses only the url in the document, and all other fields
            // in this struct are useless here (status code, size, content_type etc...)
            struct Document doc;
            doc.url = url;

            found_url_handler_bundle.found_url = url;
            found_url_handler_bundle.document = &doc;
            found_url_handler_bundle.base_tag_url = NULL;
            found_url_handler_bundle.urls_done = &urls_done;
            found_url_handler_bundle.urls_stack_todo = &urls_stack_todo;

            int result = handle_found_url_from_sitemap(&found_url_handler_bundle);
            if(result) {
                count++;
            }
        }
        curl_url_cleanup(curl_url_handler);
        printf("Added %i new URLs.\n", count);
    }


    LOG("Creating threads...\n");
    bool should_exit = false;  // if threads should exit, set to true when all threads have is_running == true
    bool* list_running_thread_status = (bool*) malloc(sizeof(bool) * cli_arguments->threads);  // list containing ints indicating if each thread is fetching
    struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof(struct BundleVarsThread) * cli_arguments->threads);
    for(unsigned int i = 0; i < cli_arguments->threads; i++) {
        list_running_thread_status[i] = true;

        bundles[i].documents = &documents_stack;
        bundles[i].urls_stack_todo = &urls_stack_todo;
        bundles[i].urls_done = &urls_done;
        bundles[i].mutex = &mutex;
        bundles[i].cv_url_added = &cv_url_added;
        bundles[i].cv_fetcher_produced = &cv_fetcher_produced;
        bundles[i].should_exit = &should_exit;
        bundles[i].is_running = &(list_running_thread_status[i]);
        bundles[i].timeout = cli_arguments->timeout;
        bundles[i].resolve_ip_versions = resolve_ip_version;
        bundles[i].no_color = cli_arguments->no_color_flag;
        bundles[i].curl_share = curl_share;
        bundles[i].cookies = cli_arguments->cookies;
        bundles[i].headers = cli_arguments->headers;
        bundles[i].proxy_url = cli_arguments->proxy_url;
        bundles[i].ignore_cert_validation = cli_arguments->ignore_cert_validation;
        if(cli_arguments->user_agent != NULL) {
            bundles[i].user_agent = cli_arguments->user_agent;
        } else {
            bundles[i].user_agent = "MapPTTH/" MAPPTTH_VERSION;
        }
        pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*) &(bundles[i]));
    }

    struct Document* current_document;
#ifdef GRAPHVIZ_SUPPORT
    CURLU* curl_url_handler = curl_url();
#endif

    LOG("Starting...\n");
    while(!sigint_received) {
        pthread_mutex_lock(&mutex);
        if(stack_document_isempty(documents_stack)) {  // no documents to parse
            // if this thread has no document to parse and all threads are waiting for
            // urls, then it means that everything was discovered and they should quit
            bool should_quit = true;
            for(unsigned int i = 0; i < cli_arguments->threads; i++) {  // check if all threads are running
                if(list_running_thread_status[i] == 1) {  // if one is running
                    should_quit = false;  // should not quit
                    break;  // don't need to check other threads
                }
            }
            if(should_quit && stack_url_isempty(urls_stack_todo)) {  // if no threads are running and no urls to fetch left
                // quit
                should_exit = true;
                pthread_mutex_unlock(&mutex);
                break;  // quit parsing
            }

            LOG("Waiting for a document to parse...\n");

            // signaled when a fetcher adds something to the stack of documents and when
            // it waits for cv_url_added (meaning when it does not have anything to fetch)
            pthread_cond_wait(&cv_fetcher_produced, &mutex);

            pthread_mutex_unlock(&mutex); // no need to keep the mutex any more
            // need to recheck if the last pthread_cond_wait(&cv_fetcher) was not signaled
            // by a fetcher to indicate that it has nothing left to fetch and he is no longer running
            continue;
        }

        current_document = stack_document_pop(&documents_stack);
        pthread_mutex_unlock(&mutex);
        LOG("Got a document to parse.\n");

        int http_status_cat = current_document->status_code_http / 100;
        if(!cli_arguments->no_color_flag) {
            char* color;
            switch(http_status_cat) {
            case 5:  // 5xx
                color = RED;
                break;
            case 4:  // 4xx
                color = MAGENTA;
                break;
            case 3:  // 3xx
                color = YELLOW;
                break;
            case 2:  // 2xx
                color = GREEN;
                break;
            case 1:  // 1xx
                color = CYAN;
                break;
            }
            if(http_status_cat == 3) {
                printf("[%s%ld%s] %s -> %s [%s] [%zu]", color,
                    current_document->status_code_http, RESET, current_document->url,
                    current_document->redirect_location != NULL ? current_document->redirect_location : "(nowhere)",
                    current_document->content_type, current_document->size);
            } else {
                printf("[%s%ld%s] %s [%s] [%zu]", color,
                    current_document->status_code_http, RESET, current_document->url,
                    current_document->content_type, current_document->size);
            }
        } else {
            if(http_status_cat == 3) {
                printf("[%ld] %s -> %s [%s] [%zu]", current_document->status_code_http,
                    current_document->url, current_document->redirect_location,
                    current_document->content_type, current_document->size);
            } else {
                printf("[%ld] %s [%s] [%zu]", current_document->status_code_http,
                    current_document->url, current_document->content_type,
                    current_document->size);
            }
        }

        if(cli_arguments->title_flag) { // print title
            const char* title = (const char*) lxb_html_document_title(current_document->lexbor_document, NULL);
            if(title != NULL) {
                printf(" [%s]", title);
            }
        }
        printf("\n");
        if(cli_arguments->output != NULL) {
            LOG("Writing to %s...\n", cli_arguments->output);
            if(http_status_cat == 3) {
                if(cli_arguments->output != NULL) {
                    fprintf(output_file, "[%ld] %s -> %s [%s] [%zu]", current_document->status_code_http,
                            current_document->url, current_document->redirect_location,
                            current_document->content_type, current_document->size);
                }
            } else {
                if(cli_arguments->output != NULL) {
                    fprintf(output_file, "[%ld] %s [%s] [%zu]", current_document->status_code_http,
                            current_document->url, current_document->content_type,
                            current_document->size);
                }
            }
            if(cli_arguments->title_flag) {
                const char* title = (const char*) lxb_html_document_title(current_document->lexbor_document, NULL);
                if(title != NULL) {
                    fprintf(output_file, " [%s]", title);
                }
            }
            fprintf(output_file, "\n");
        }

#if GRAPHVIZ_SUPPORT
        // set node infos
        if(cli_arguments->graph_flag) {
            LOG("Adding label for the node: %s\n", current_document->url);
            // TODO: refactor code
            // get current node representing the current_document
            Agnode_t* node_current = agnode(found_url_handler_bundle.graph, current_document->url, 0);

            char label_curr_node[4096];
            if(current_document->redirect_location != NULL) {
                if(current_document->content_type != NULL) {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %lu\nRedirect location: %s\nContent type: %s\nContent length: %lu",
                        current_document->url, current_document->status_code_http, current_document->redirect_location,
                        current_document->content_type, current_document->size);
                } else {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %lu\nRedirect location: %s\nContent length: %lu",
                        current_document->url, current_document->status_code_http, current_document->redirect_location, current_document->size);
                }
            } else {
                if(current_document->content_type != NULL) {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %lu\nContent type: %s\nContent length: %lu",
                        current_document->url, current_document->status_code_http,
                        current_document->content_type, current_document->size);
                } else {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %lu\nContent length: %lu",
                        current_document->url, current_document->status_code_http, current_document->size);
                }
            }
            agsafeset(node_current, "label", label_curr_node, label_curr_node);
            agsafeset(node_current, "URL", current_document->url, current_document->url);

            // set rank for the node based on its path depth
            char* path;
            curl_url_set(curl_url_handler, CURLUPART_URL, current_document->url, 0);
            curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);

            char rank_str[4];
            snprintf(rank_str, 4, "%d", get_path_depth(path));
            agsafeset(node_current, "rank", rank_str, rank_str);

            char* color;
            switch(current_document->status_code_http / 100) {
            case 5:  // 5xx
                color = "#CC0000";
                break;
            case 4:  // 4xx
                color = "#CC00CC";
                break;
            case 3:  // 3xx
                color = "#CCCC00";
                break;
            case 2:  // 2xx
                color = "#00CC00";
                break;
            case 1:  // 1xx
                color = "#00CCCC";
                break;
            }
            agsafeset(node_current, "color", color, color);
        }
#endif

        if(current_document->content_type != NULL) {  // sometimes, the server doesn't send a content-type header
            LOG("Handling content type for %s\n", current_document->url);
            // parse only html and xhtml files
            if(strstr(current_document->content_type, "text/html") != NULL || strstr(current_document->content_type, "application/xhtml+xml") != NULL) {
                found_url_handler_bundle.base_tag_url = get_base_tag_value(current_document->lexbor_document);
                found_url_handler_bundle.document = current_document;
                found_url_handler_bundle.base_tag_url = NULL;
                pthread_mutex_lock(&mutex);
                found_url_handler_bundle.urls_done = &urls_done;
                found_url_handler_bundle.urls_stack_todo = &urls_stack_todo;
                if(current_document->lexbor_document->head && !cli_arguments->only_body_flag) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(current_document->lexbor_document->head), walk_cb, &found_url_handler_bundle);
                }
                if(current_document->lexbor_document->body && !cli_arguments->only_head_flag) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(current_document->lexbor_document->body), walk_cb, &found_url_handler_bundle);
                }
                pthread_mutex_unlock(&mutex);
                free(found_url_handler_bundle.base_tag_url);
            }
            free(current_document->content_type);  // allocated by strdup
        }  // maybe check using libmagick if this is a html file if the server didn't specified it

        if(current_document->redirect_location != NULL) {
            LOG("Handling redirect URL for %s\n", current_document->url);
            found_url_handler_bundle.document = current_document;
            pthread_mutex_lock(&mutex);
            found_url_handler_bundle.urls_done = &urls_done;
            found_url_handler_bundle.urls_stack_todo = &urls_stack_todo;
            found_url_handler_bundle.found_url = current_document->redirect_location;
            int result = handle_found_url(&found_url_handler_bundle);
            if(result) {
                LOG("Added redirect URL: %s\n", current_document->redirect_location);
            }
	     free(current_document->redirect_location);
            pthread_mutex_unlock(&mutex);
        }

        lxb_html_document_destroy(current_document->lexbor_document);
        free(current_document);
    }

    LOG("Quitting...\n");
    for(unsigned int i = 0; i < cli_arguments->threads; i++) {
        LOG("Waiting for the thread #%lu to quit...\n", fetcher_threads[i]);
        *(bundles[i].should_exit) = true;
        pthread_cond_broadcast(&cv_url_added);
        pthread_join(fetcher_threads[i], NULL);
    }

    if(cli_arguments->print_as_dir) {
        if(cli_arguments->no_color_flag) {
            printf("\n\n[o] Summary:\n\n");
        } else {
            printf("\n\n%s[o]%s Summary:\n\n", CYAN, RESET);
        }
        trie_beautiful_print(urls_done, cli_arguments->no_color_flag, stdout);
        if(cli_arguments->output != NULL) {
            trie_beautiful_print(urls_done, true, output_file);
        }
    }

    if(cli_arguments->output != NULL) {
        fclose(output_file); // closing output file as nothing more will be added
    }

#ifdef GRAPHVIZ_SUPPORT
    curl_url_cleanup(curl_url_handler);
#endif

    // CLEANUP
    free(bundles);
    free(list_running_thread_status);

    trie_free(urls_done);

    for(unsigned int i = 0; i < cli_arguments->disallowed_paths_count; i++) {
        free(disallowed_paths[i]);
    }
    for(unsigned int i = 0; i < cli_arguments->allowed_paths_count; i++) {
        free(allowed_paths[i]);
    }

    curl_share_cleanup(curl_share);
    curl_global_cleanup();
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_conn);
    pthread_cond_destroy(&cv_fetcher_produced);
    pthread_cond_destroy(&cv_url_added);

#if GRAPHVIZ_SUPPORT
    if(cli_arguments->graph_flag) {
        char* graph_output_filename = (char*) malloc(sizeof (char) * (strlen("output.") + strlen(cli_arguments->graph_output_format) + 1));
        strcpy(graph_output_filename, "output.");
        strcat(graph_output_filename, cli_arguments->graph_output_format);
        printf("[G] Creating graph: %s\n", graph_output_filename);

        GVC_t* gvc = gvContext();

        puts("[G] Generating layout...");
        gvLayout(gvc, found_url_handler_bundle.graph, cli_arguments->graph_layout);

        puts("[G] Rendering graph to file...");
        gvRenderFilename(gvc, found_url_handler_bundle.graph, cli_arguments->graph_output_format, graph_output_filename);

        free(graph_output_filename);

        gvFreeLayout(gvc, found_url_handler_bundle.graph);
        agclose(found_url_handler_bundle.graph);
        gvFreeContext(gvc);
    }
#endif

    cli_arguments_free(cli_arguments);

    return EXIT_SUCCESS;
}

static inline int _handle_found_url(struct FoundURLHandlerBundle* bundle, bool from_sitemap_parsing) {
    // this function is called when finding an URL, not after fetching it
    // NOTE:
    // only document->url will be used in the bundle->document, this is important when parsing the sitemap
    // as nothing else in the document will be set (no redirection, no status code, etc... as it it useless for the sitemap)


    // used when checking for valid domain
    char* document_domain;
    char* found_url_domain;

    // URL that will be added to the stack of URLs to fetch
    char* final_url;

    char* url_scheme;

    if(is_valid_link(bundle->found_url)) {
        CURLU* curl_url_handler = curl_url();
        curl_url_set(curl_url_handler, CURLUPART_URL, bundle->document->url, 0);
        curl_url_get(curl_url_handler, CURLUPART_HOST, &document_domain, 0);

        // set final URL
        // curl will change the url by himself based on the document's URL
        if(bundle->base_tag_url != NULL) {
            curl_url_set(curl_url_handler, CURLUPART_URL, bundle->base_tag_url, 0);
        }
        curl_url_set(curl_url_handler, CURLUPART_URL, bundle->found_url, 0);
        curl_url_set(curl_url_handler, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

        // check scheme
        curl_url_get(curl_url_handler, CURLUPART_SCHEME, &url_scheme, 0);
        if((bundle->http_only && strcmp("http", url_scheme) != 0) || (bundle->https_only && strcmp("https", url_scheme) != 0)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(url_scheme);
            return LEXBOR_ACTION_OK;
        }
        free(url_scheme);

        if(!bundle->keep_query) {
            curl_url_set(curl_url_handler, CURLUPART_QUERY, NULL, 0);
        }

        char* path;
        curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);

        // check path
        if(is_disallowed_path(path, bundle->disallowed_paths, bundle->count_disallowed_paths)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(path);
            return 0;
        }
        if(!is_allowed_path(path, bundle->allowed_paths, bundle->count_allowed_paths)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(path);
            return 0;
        }

        // check extensions
        if(is_disallowed_extension(path, bundle->disallowed_extensions, bundle->count_disallowed_extensions)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(path);
            return 0;
        }
        if(!is_allowed_extension(path, bundle->allowed_extensions, bundle->count_allowed_extensions)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(path);
            return 0;
        }

        if(bundle->max_path_depth > 0 && get_path_depth(path) > bundle->max_path_depth) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(path);
            return 0;
        }
        free(path);

        curl_url_get(curl_url_handler, CURLUPART_URL, &final_url, 0);  // get final url

        // check ports
        if(!is_allowed_port(get_port_from_url(final_url), bundle->allowed_ports, bundle->count_allowed_ports)) {
            curl_url_cleanup(curl_url_handler);
            free(document_domain);
            free(final_url);
            return LEXBOR_ACTION_OK;
        }

        bool has_been_added = false;  // used to check if the URL has been added, if not it will be freed
        curl_url_get(curl_url_handler, CURLUPART_HOST, &found_url_domain, 0);  // get the domain of the URL found

        if((is_same_domain(found_url_domain, document_domain, bundle->allow_subdomains) ||
            is_in_valid_domains(found_url_domain, bundle->allowed_domains, bundle->count_allowed_domains, bundle->allow_subdomains)) &&
            !is_in_disallowed_domains(found_url_domain, bundle->disallowed_domains, bundle->count_disallowed_domains)) {

            if(url_not_seen(final_url, *(bundle->urls_done), *(bundle->urls_stack_todo))) {
                stack_url_push(bundle->urls_stack_todo, final_url);
                // note that is this func is called from the sitemap parsing, no one listen for this signal,
                // thus it will be useless in that particular case to call it
                if(!from_sitemap_parsing) {
                    pthread_cond_signal(bundle->cv_url_added);
                }
                has_been_added = true;
            }

#if GRAPHVIZ_SUPPORT
            if(bundle->generate_graph) {
                LOG("Adding node and edge for: %s\n", final_url);
                Agnode_t* node_new = agnode(bundle->graph, final_url, 1);

                if(!from_sitemap_parsing)  {
                        // current link does not exists when parsing the sitemap as
                        // all urls are found "randomly" in the sitemap without any link between them
                        Agnode_t* node_current = agnode(bundle->graph, bundle->document->url, 0);
                        Agedge_t* edge = agedge(bundle->graph, node_current, node_new, 0, 1);
                        agsafeset(edge, "splines", "curved", "curved");
                }
            }
#endif
        }

        if(!has_been_added) {
            free(final_url);
        }
        free(found_url_domain);
        free(document_domain);
        curl_url_cleanup(curl_url_handler);
    }

    return 1;
}
