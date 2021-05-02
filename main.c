#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "fetcher_thread.h"
#include "sitemaps_parser.h"
#include "stack_documents.h"
#include "stack_urls.h"
#include "utils.h"
#include "cli_parser.h"

#if GRAPHVIZ_SUPPORT
#include <graphviz/gvc.h>
#endif

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "libcurl 7.62.0 or later is required"
#endif

struct WalkBundle {  // used with walk_cb.
    struct Document* document;
    URLNode_t** urls_stack_todo;
    URLNode_t** urls_stack_done;
    int allow_subdomains;
    char** allowed_domains;
    int count_allowed_domains;
    char** disallowed_domains;
    int count_disallowed_domains;
    char** allowed_extensions;
    int count_allowed_extensions;
    char** disallowed_extensions;
    int count_disallowed_extensions;
    char** disallowed_paths;
    int count_disallowed_paths;
    char** allowed_paths;
    int count_allowed_paths;
    int http_only;
    int https_only;
    int keep_query;
    int max_path_depth;
    char* base_tag_url;
#if GRAPHVIZ_SUPPORT
    int generate_graph;
    Agraph_t* graph;
#endif
};

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

    struct WalkBundle* bundle = (struct WalkBundle*) ctx;

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


    // used when checking for valid domain
    char* document_domain;
    char* found_url_domain;

    // URL that will be added to the stack of URLs to fetch
    char* final_url;

    char* url_scheme;

    if(is_valid_link((char*) found_url)) {
        CURLU* curl_url_handler = curl_url();
        curl_url_set(curl_url_handler, CURLUPART_URL, bundle->document->url, 0);
        curl_url_get(curl_url_handler, CURLUPART_HOST, &document_domain, 0);

        // set final URL
        // curl will change the url by himself based on the document's URL
        if(bundle->base_tag_url != NULL) {
            curl_url_set(curl_url_handler, CURLUPART_URL, bundle->base_tag_url, 0);
        }
        curl_url_set(curl_url_handler, CURLUPART_URL, (char*) found_url, 0);
        curl_url_set(curl_url_handler, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

        // check scheme
        curl_url_get(curl_url_handler, CURLUPART_SCHEME, &url_scheme, 0);
        if((bundle->http_only && strcmp("http", url_scheme) != 0) || (bundle->https_only && strcmp("https", url_scheme) != 0)) {
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
            free(document_domain);
            free(path);
            return LEXBOR_ACTION_OK;
        }
        if(!is_allowed_path(path, bundle->allowed_paths, bundle->count_allowed_paths)) {
            free(document_domain);
            free(path);
            return LEXBOR_ACTION_OK;
        }

        // check extensions
        if(is_disallowed_extension(path, bundle->disallowed_extensions, bundle->count_disallowed_extensions)) {
            free(document_domain);
            free(path);
            return LEXBOR_ACTION_OK;
        }
        if(!is_allowed_extension(path, bundle->allowed_extensions, bundle->count_allowed_extensions)) {
          free(document_domain);
          free(path);
          return LEXBOR_ACTION_OK;
        }

        if(bundle->max_path_depth > 0 && get_path_depth(path) > bundle->max_path_depth) {
            free(document_domain);
            free(path);
            return LEXBOR_ACTION_OK;
        }
        free(path);

        char has_been_added = 0;  // used to check if the URL has been added, if not it will be freed
        curl_url_get(curl_url_handler, CURLUPART_URL, &final_url, 0);  // get final url
        curl_url_get(curl_url_handler, CURLUPART_HOST, &found_url_domain, 0);  // get the domain of the URL found

        if((is_same_domain(found_url_domain, document_domain, bundle->allow_subdomains) ||
            is_in_valid_domains(found_url_domain, bundle->allowed_domains, bundle->count_allowed_domains, bundle->allow_subdomains)) &&
            !is_in_disallowed_domains(found_url_domain, bundle->disallowed_domains, bundle->count_disallowed_domains)) {

            if(url_not_seen(final_url, *(bundle->urls_stack_done), *(bundle->urls_stack_todo))) {
                stack_url_push(bundle->urls_stack_todo, final_url);
                has_been_added = 1;
            }

#if GRAPHVIZ_SUPPORT
            if(bundle->generate_graph) {
                Agnode_t* node_new = agnode(bundle->graph, final_url, 1);

                // should not create any node
                Agnode_t* node_current = agnode(bundle->graph, bundle->document->url, 0);

                Agedge_t* edge = agedge(bundle->graph, node_current, node_new, 0, 1);
                agsafeset(edge, "splines", "curved", "curved");
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
    return LEXBOR_ACTION_OK;
}

static void lock_cb(CURL* handle, curl_lock_data data, curl_lock_access access, void* userptr) {
    pthread_mutex_lock((pthread_mutex_t*) userptr);
}

static void unlock_cb(CURL* handle, curl_lock_data data, void* userptr) {
    pthread_mutex_unlock((pthread_mutex_t*) userptr);
}

int main(int argc, char* argv[]) {
    printf("\n _______             ______ _______ _______ _     _ \n"
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

    if(cli_arguments->debug_flag) {
        ACTIVATE_DEBUG();
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
    for(int i = 0; i < cli_arguments->allowed_extensions_count; i++) {
        if(cli_arguments->allowed_extensions[i][0] != '.') {
            fprintf(stderr, "%s: extensions have to begin with a '.' (dot)\n", argv[0]);
            return 1;
        }
    }
    for(int i = 0; i < cli_arguments->disallowed_extensions_count; i++) {
        if(cli_arguments->disallowed_extensions[i][0] != '.') {
            fprintf(stderr, "%s: extensions have to begin with a '.' (dot)\n", argv[0]);
            return 1;
        }
    }

    FILE* output_file = NULL;
    if(cli_arguments->output_given) {
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
    }

    // normalize paths given by the user
    char** disallowed_paths = (char**) malloc(sizeof(char*) * cli_arguments->disallowed_paths_count);
    for(int i = 0; i < cli_arguments->disallowed_paths_count; i++) {
        disallowed_paths[i] = normalize_path(cli_arguments->disallowed_paths[i], 0);
    }
    char** allowed_paths = (char**) malloc(sizeof(char*) * cli_arguments->allowed_paths_count);
    for(int i = 0; i < cli_arguments->allowed_paths_count; i++) {
        allowed_paths[i] = normalize_path(cli_arguments->allowed_paths[i], 0);
    }

    int resolve_ip_version = CURL_IPRESOLVE_WHATEVER;
    if(cli_arguments->only_ipv4_flag) {
        resolve_ip_version = CURL_IPRESOLVE_V4;
    } else if(cli_arguments->only_ipv6_flag) {
        resolve_ip_version = CURL_IPRESOLVE_V6;
    }

    pthread_t fetcher_threads[cli_arguments->threads];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    DocumentNode_t* documents_stack = NULL;

    URLNode_t* urls_stack_todo = NULL;
    URLNode_t* urls_stack_done = NULL;

    curl_global_init(CURL_GLOBAL_ALL);  // initialize libcurl

    if(cli_arguments->sitemap_given) {
        // get the content of the sitemap
        URLNode_t* url_stack_sitemap = get_sitemap_urls(cli_arguments->sitemap, cli_arguments->no_color_flag);

        // validate the URLs found in the sitemap, same code as when finding an URL in a document
        CURLU* curl_url_handler = curl_url();
        int count = 0;  // keep track of how many validated URLs were added
        while(!stack_url_isempty(url_stack_sitemap)) {  // loop over URLs
            char* url = stack_url_pop(&url_stack_sitemap);
            curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);

            if(is_valid_link(url)) {
                if(!cli_arguments->keep_query_flag) {
                    curl_url_set(curl_url_handler, CURLUPART_QUERY, NULL, 0);
                }

                char* path;
                curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);
                // check disallowed paths
                if(is_disallowed_path(path, disallowed_paths, cli_arguments->disallowed_paths_count)) {
                    free(url);
                    free(path);
                    continue;
                }
                if(!is_allowed_path(path, allowed_paths, cli_arguments->allowed_paths_count)) {
                    free(url);
                    free(path);
                    continue;
                }
                // check extensions
                if(is_disallowed_extension(path, cli_arguments->disallowed_extensions, cli_arguments->disallowed_extensions_count)) {
                    free(url);
                    free(path);
                    continue;
                }
                if(!is_allowed_extension(path, cli_arguments->allowed_extensions, cli_arguments->allowed_extensions_count)) {
                  free(url);
                  free(path);
                  continue;
                }
                if(cli_arguments->max_depth_given && get_path_depth(path) > cli_arguments->max_depth_given) {
                    free(url);
                    free(path);
                    continue;
                }
                free(path);

                if(url_not_seen(url, urls_stack_done, urls_stack_todo)) {
                    char* domain_url_found;
                    curl_url_get(curl_url_handler, CURLUPART_HOST, &domain_url_found, 0);  // get the domain of the URL

                    char* initial_url_domain;  // URL specified by -u
                    curl_url_set(curl_url_handler, CURLUPART_URL, cli_arguments->url, 0);
                    curl_url_get(curl_url_handler, CURLUPART_HOST, &initial_url_domain, 0);

                    // check domains
                    if((is_same_domain(domain_url_found, initial_url_domain, cli_arguments->allow_subdomains_flag) ||
			is_in_valid_domains(domain_url_found, cli_arguments->allowed_domains, cli_arguments->allowed_domains_count, cli_arguments->allow_subdomains_flag)) &&
			!is_in_disallowed_domains(domain_url_found, cli_arguments->disallowed_domains, cli_arguments->disallowed_domains_count)) {

                        stack_url_push(&urls_stack_todo, url);
                        count++;
                    } else {
                        free(url);
                    }
                    free(domain_url_found);
                    free(initial_url_domain);
                }
            } else {
                free(url);
            }
        }
        curl_url_cleanup(curl_url_handler);
        printf("Added %i new URLs.\n", count);
    }

    stack_url_push(&urls_stack_todo, cli_arguments->url);  // add the URL specified by -u

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
    pthread_mutex_t mutex_conn = PTHREAD_MUTEX_INITIALIZER;
    curl_share_setopt(curl_share, CURLSHOPT_USERDATA, (void*) &mutex_conn);

    int should_exit = 0;  // if threads should exit, set to 1 when all threads have is_running == 1
    int* list_running_thread_status = (int*) malloc(sizeof(int) * cli_arguments->threads);  // list containing ints indicating if each thread is fetching
    struct BundleVarsThread* bundles = (struct BundleVarsThread*) malloc(sizeof(struct BundleVarsThread) * cli_arguments->threads);
    for(int i = 0; i < cli_arguments->threads; i++) {
        list_running_thread_status[i] = 1;

        bundles[i].documents = &documents_stack;
        bundles[i].urls_stack_todo = &urls_stack_todo;
        bundles[i].urls_stack_done = &urls_stack_done;
        bundles[i].mutex = &mutex;
        bundles[i].should_exit = &should_exit;
        bundles[i].is_running = &(list_running_thread_status[i]);
        bundles[i].timeout = cli_arguments->timeout;
        bundles[i].resolve_ip_versions = resolve_ip_version;
        bundles[i].no_color = cli_arguments->no_color_flag;
        bundles[i].curl_share = curl_share;
        bundles[i].cookies = cli_arguments->cookies;
        if(cli_arguments->user_agent_given) {
            bundles[i].user_agent = cli_arguments->user_agent;
        } else {
            bundles[i].user_agent = "MapPTTH/" MAPPTTH_VERSION;
        }
        pthread_create(&fetcher_threads[i], NULL, fetcher_thread_func, (void*) &(bundles[i]));
    }

    struct Document* current_document ;
    CURLU* curl_url_handler = curl_url();  // used when handling redirections

    struct WalkBundle bundle_walk;  // in this bundle these elements never change
    bundle_walk.allowed_domains = cli_arguments->allowed_domains;
    bundle_walk.count_allowed_domains = cli_arguments->allowed_domains_count;
    bundle_walk.disallowed_domains = cli_arguments->disallowed_domains;
    bundle_walk.count_disallowed_domains = cli_arguments->disallowed_domains_count;
    bundle_walk.allowed_extensions = cli_arguments->allowed_extensions;
    bundle_walk.count_allowed_extensions = cli_arguments->allowed_extensions_count;
    bundle_walk.disallowed_extensions = cli_arguments->disallowed_extensions;
    bundle_walk.count_disallowed_extensions = cli_arguments->disallowed_extensions_count;
    bundle_walk.allow_subdomains = cli_arguments->allow_subdomains_flag;
    bundle_walk.http_only = cli_arguments->http_only_flag;
    bundle_walk.https_only = cli_arguments->https_only_flag;
    bundle_walk.disallowed_paths = disallowed_paths;
    bundle_walk.count_disallowed_paths = cli_arguments->disallowed_paths_count;
    bundle_walk.allowed_paths = allowed_paths;
    bundle_walk.count_allowed_paths = cli_arguments->allowed_paths_count;
    bundle_walk.keep_query = cli_arguments->keep_query_flag;
    bundle_walk.max_path_depth = cli_arguments->max_depth;
#if GRAPHVIZ_SUPPORT
    bundle_walk.generate_graph = cli_arguments->graph_flag;
    if(cli_arguments->graph_flag) {
        bundle_walk.graph = agopen(cli_arguments->url, Agstrictdirected, 0);
        Agnode_t* first_node = agnode(bundle_walk.graph, cli_arguments->url, 1); // add initial node

        // https://graphviz.org/doc/info/attrs.html#d:root
        agsafeset(first_node, "root", "true", "true"); // set the node as the root (used by circo and twopi)

        agsafeset(first_node, "URL", cli_arguments->url, cli_arguments->url); // used by svg
    }
#endif

    while(1) {
        pthread_mutex_lock(&mutex);
        if(stack_document_length(documents_stack) == 0) {  // no documents to parse
            // if this thread has no document to parse and all threads are waiting for
            // urls, then it means that everything was discovered and they should
            // quit, otherwise just continue to check for something to do
            int should_quit = 1;
            for(int i = 0; i < cli_arguments->threads; i++) {  // check if all threads are running
                if(list_running_thread_status[i] == 1) {  // if one is running
                    should_quit = 0;  // should not quit
                    break;  // don't need to check other threads
                }
            }
            if(should_quit == 1 && stack_url_isempty(urls_stack_todo)) {  // if no threads are running and no urls to fetch left
                // quit
                should_exit = 1;
                pthread_mutex_unlock(&mutex);
                break;  // quit parsing
            }
            // if some thread(s) are running, just continue to check for a document to come
            pthread_mutex_unlock(&mutex);
            continue;
        }

        current_document = stack_document_pop(&documents_stack);
        pthread_mutex_unlock(&mutex);

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
        if(cli_arguments->output_given) {
            if(http_status_cat == 3) {
                if(cli_arguments->output_given) {
                    fprintf(output_file, "[%ld] %s -> %s [%s] [%zu]", current_document->status_code_http,
                            current_document->url, current_document->redirect_location,
                            current_document->content_type, current_document->size);
                }
            } else {
                if(cli_arguments->output_given) {
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
            // TODO: refactor code
            // get currend node representing the current_document
            Agnode_t* node_current = agnode(bundle_walk.graph, current_document->url, 0);

            char label_curr_node[4096];
            if(current_document->redirect_location != NULL) {
                if(current_document->content_type != NULL) {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %d\nRedirect location: %s\nContent type: %s\nContent length: %d",
                        current_document->url, current_document->status_code_http, current_document->redirect_location,
                        current_document->content_type, current_document->size);
                } else {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %d\nRedirect location: %s\nContent length: %d",
                        current_document->url, current_document->status_code_http, current_document->redirect_location, current_document->size);
                }
            } else {
                if(current_document->content_type != NULL) {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %d\nContent type: %s\nContent length: %d",
                        current_document->url, current_document->status_code_http,
                        current_document->content_type, current_document->size);
                } else {
                    snprintf(label_curr_node, 4096, "%s\nStatus code: %d\nContent length: %d",
                        current_document->url, current_document->status_code_http, current_document->size);
                }
            }
            agsafeset(node_current, "label", label_curr_node, label_curr_node);
            agsafeset(node_current, "URL", current_document->url, current_document->url);

            // set rank for the node based on its path depth
            char* path;
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
            // parse only html and xhtml files
            if(strstr(current_document->content_type, "text/html") != NULL || strstr(current_document->content_type, "application/xhtml+xml") != NULL) {
                bundle_walk.base_tag_url = get_base_tag_value(current_document->lexbor_document);
                bundle_walk.document = current_document;
                pthread_mutex_lock(&mutex);
                bundle_walk.urls_stack_done = &urls_stack_done;
                bundle_walk.urls_stack_todo = &urls_stack_todo;
                if(current_document->lexbor_document->head && !cli_arguments->only_body_flag) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(current_document->lexbor_document->head), walk_cb, &bundle_walk);
                }
                if(current_document->lexbor_document->body && !cli_arguments->only_head_flag) {
                    lxb_dom_node_simple_walk(lxb_dom_interface_node(current_document->lexbor_document->body), walk_cb, &bundle_walk);
                }
                pthread_mutex_unlock(&mutex);
                free(bundle_walk.base_tag_url);
            }
            free(current_document->content_type);  // allocated by strdup
        }  // maybe check using libmagick if this is a html file if the server didn't specified it

        if(current_document->redirect_location != NULL) {
            // get the domain of the current document and the domain of the redirect URL
            char* cur_doc_url_domain;
            char* redirect_location_domain;
            char* url_scheme;

            curl_url_set(curl_url_handler, CURLUPART_URL, current_document->redirect_location, 0);
            curl_url_get(curl_url_handler, CURLUPART_HOST, &redirect_location_domain, 0);

            curl_url_set(curl_url_handler, CURLUPART_URL, current_document->url, 0);
            curl_url_get(curl_url_handler, CURLUPART_HOST, &cur_doc_url_domain, 0);

            pthread_mutex_lock(&mutex);
            if(is_valid_link((char*) current_document->redirect_location)) {
                curl_url_set(curl_url_handler, CURLUPART_URL, current_document->redirect_location, 0);  // curl will change the url by himself based on the document's URL
                curl_url_set(curl_url_handler, CURLUPART_FRAGMENT, NULL, 0);  // remove fragment

                curl_url_get(curl_url_handler, CURLUPART_SCHEME, &url_scheme, 0);

                int is_still_valid = 1;  // indicates if it is a valid URL through the checks, if not then it is useless to do checks anymore

                if((cli_arguments->http_only_flag && strcmp("http", url_scheme) != 0) || (cli_arguments->https_only_flag && strcmp("https", url_scheme) != 0)) {
                    is_still_valid = 0;
                }
                free(url_scheme);

                char* path;
                curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);
                if(is_still_valid) {
                    if(is_disallowed_path(path, disallowed_paths, cli_arguments->disallowed_paths_count)) {
                        is_still_valid = 0;
                    }
                }
                if(is_still_valid) {
                    if(!is_allowed_path(path, allowed_paths, cli_arguments->allowed_paths_count)) {
                        is_still_valid = 0;
                    }
                }
                if(is_still_valid) {
                    if(!is_allowed_extension(path, cli_arguments->allowed_extensions, cli_arguments->allowed_extensions_count)) {
                        is_still_valid = 0;
                    }
                }
                if(is_still_valid) {
                    if(is_disallowed_extension(path, cli_arguments->disallowed_extensions, cli_arguments->disallowed_extensions_count)) {
                        is_still_valid = 0;
                    }
                }
                if(is_still_valid) {
                    if(cli_arguments->max_depth_given && get_path_depth(path) > cli_arguments->max_depth_given) {
                        is_still_valid = 0;
                    }
                }
                free(path);

                int has_been_added = 0;  // used to check if the URL has been added, if not it will be freed
                if(is_still_valid) {
                    if((is_same_domain(redirect_location_domain, cur_doc_url_domain, cli_arguments->allow_subdomains_flag) ||
                        is_in_valid_domains(redirect_location_domain, cli_arguments->allowed_domains, cli_arguments->allowed_domains_count, cli_arguments->allow_subdomains_flag)) &&
                        !is_in_disallowed_domains(redirect_location_domain, cli_arguments->disallowed_domains, cli_arguments->disallowed_domains_count)) {

                        if(url_not_seen(current_document->redirect_location, urls_stack_done, urls_stack_todo)) {
                            stack_url_push(&urls_stack_todo, current_document->redirect_location);
                            has_been_added = 1;
                        }
#if GRAPHVIZ_SUPPORT
                        if(cli_arguments->graph_flag) {
                            Agnode_t* node_new = agnode(bundle_walk.graph, current_document->redirect_location, 1);

                            // should not create any node
                            Agnode_t* node_current = agnode(bundle_walk.graph, current_document->url, 0);

                            Agedge_t* edge = agedge(bundle_walk.graph, node_current, node_new, 0, 1);
                            agsafeset(edge, "splines", "curved", "curved");
			}
#endif
                    }
                }
                if(!has_been_added) {
                    free(current_document->redirect_location);
                }
            }
            pthread_mutex_unlock(&mutex);

            free(cur_doc_url_domain);
            free(redirect_location_domain);
        }

        lxb_html_document_destroy(current_document->lexbor_document);
        free(current_document);
    }
    for(int i = 0; i < cli_arguments->threads; i++) {
        pthread_join(fetcher_threads[i], NULL);
    }

    if(cli_arguments->output_given) {
        fclose(output_file);
    }

    curl_url_cleanup(curl_url_handler);

    // CLEANUP
    free(bundles);
    free(list_running_thread_status);

    while(stack_url_length(urls_stack_done) != 1) {  // all urls are allocated by curl when parsing url, except the initial url
        char* url_done = stack_url_pop(&urls_stack_done);
        free(url_done);
    }

    for(int i = 0; i < cli_arguments->disallowed_paths_count; i++) {
        free(disallowed_paths[i]);
    }
    for(int i = 0; i < cli_arguments->allowed_paths_count; i++) {
        free(allowed_paths[i]);
    }

    curl_share_cleanup(curl_share);
    curl_global_cleanup();

#if GRAPHVIZ_SUPPORT
    if(cli_arguments->graph_flag) {
        GVC_t* gvc = gvContext();

        char* graph_output_filename = (char*) malloc(sizeof (char) * (strlen("output.") + strlen(cli_arguments->graph_output_format) + 1));
        strcpy(graph_output_filename, "output.");
        strcat(graph_output_filename, cli_arguments->graph_output_format);

        gvLayout(gvc, bundle_walk.graph, cli_arguments->graph_layout);

        gvRenderFilename(gvc, bundle_walk.graph, cli_arguments->graph_output_format, graph_output_filename);

        free(graph_output_filename);

        gvFreeLayout(gvc, bundle_walk.graph);
        agclose(bundle_walk.graph);
        gvFreeContext(gvc);
    }
#endif

    cli_arguments_free(cli_arguments);

    return EXIT_SUCCESS;
}
