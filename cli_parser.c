#include "cli_parser.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

void _init_arguments(struct arguments* args) {
    memset(args, 0, sizeof (struct arguments));
    args->timeout = 3;
    args->threads = 5;
    args->disallowed_paths = (char**) malloc(sizeof (char*));
    args->disallowed_extensions = (char**) malloc(sizeof (char*));
    args->disallowed_domains = (char**) malloc(sizeof (char*));

    args->allowed_paths = (char**) malloc(sizeof (char*));
    args->allowed_extensions = (char**) malloc(sizeof (char*));
    args->allowed_domains = (char**) malloc(sizeof (char*));
#if GRAPHVIZ_SUPPORT
    args->graph_layout = "sfdp";
    args->graph_output_format = "png";
#endif
}

void cli_arguments_free(struct arguments* args) {
    free(args->disallowed_paths);
    free(args->disallowed_extensions);
    free(args->disallowed_domains);

    free(args->allowed_paths);
    free(args->allowed_extensions);
    free(args->allowed_domains);
    free(args);
}

void cli_arguments_print_help(char* prgm_name) {
        printf("Usage: %s <parameters>\n", prgm_name);
}

struct arguments* parse_cli_arguments(int argc, char** argv) {
    struct arguments* args = (struct arguments*) malloc(sizeof (struct arguments));
    _init_arguments(args);

#if GRAPHVIZ_SUPPORT
    char* args_str = "u:t:m:U:S:o:D:C:sqcTfFBH64qVhp:P:x:X:a:d:gG:L:";
#else
    char* args_str = "u:t:m:U:S:o:D:C:sqcTfFBH64qVhp:P:x:X:a:d:";
#endif

    // used when using strtoul
    char* endptr;

    opterr = 0; // disable getopt error printing
    int c;
    while((c = getopt(argc, argv, args_str)) != -1) {
        switch(c) {
            case 'u': // url
                args->url = optarg;
                break;
            case 't': // threads
                errno = 0;
                args->threads = strtoul(optarg, &endptr, 10);
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free(args);
                    return NULL;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid number of threads value: %s\n", argv[0], optarg);
                    cli_arguments_free(args);
                    return NULL;
                }
                break;
            case 'm': // timeout
                errno = 0;
                args->timeout = strtoul(optarg, &endptr, 10);
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free(args);
                    return NULL;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid timeout value: %s\n", argv[0], optarg);
                    cli_arguments_free(args);
                    return NULL;
                }
                break;
            case 'D': // max-depth
                errno = 0;
                args->max_depth = strtoul(optarg, &endptr, 10);
                args->max_depth_given = 1;
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free(args);
                    return NULL;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid max-depth value: %s\n", argv[0], optarg);
                    cli_arguments_free(args);
                    return NULL;
                }
                break;
            case 'p': // allowed paths
                args->allowed_paths_count++;
                args->allowed_paths = (char**) reallocarray(args->allowed_paths, args->allowed_paths_count, sizeof (char*));
                args->allowed_paths[args->allowed_paths_count-1] = optarg;
                break;
            case 'P': // disallowed paths
                args->disallowed_paths_count++;
                args->disallowed_paths = (char**) reallocarray(args->disallowed_paths, args->disallowed_paths_count, sizeof (char*));
                args->disallowed_paths[args->disallowed_paths_count-1] = optarg;
                break;
            case 'a': // allowed domains
                args->allowed_domains_count++;
                args->allowed_domains = (char**) reallocarray(args->allowed_domains, args->allowed_domains_count, sizeof (char*));
                args->allowed_domains[args->allowed_domains_count-1] = optarg;
                break;
            case 'd': // disallowed domains
                args->disallowed_domains_count++;
                args->disallowed_domains = (char**) reallocarray(args->disallowed_domains, args->disallowed_domains_count, sizeof (char*));
                args->disallowed_domains[args->disallowed_domains_count-1] = optarg;
                break;
            case 'x': // allowed extensions
                args->allowed_extensions_count++;
                args->allowed_extensions = (char**) reallocarray(args->allowed_extensions, args->allowed_extensions_count, sizeof (char*));
                args->allowed_extensions[args->allowed_extensions_count-1] = optarg;
                break;
            case 'X': // disallowed extensions
                args->disallowed_extensions_count++;
                args->disallowed_extensions = (char**) reallocarray(args->disallowed_extensions, args->disallowed_extensions_count, sizeof (char*));
                args->disallowed_extensions[args->disallowed_extensions_count-1] = optarg;
                break;
            case 'o': // output
                args->output = optarg;
                args->output_given = 1;
                break;
            case 'S': // sitemap
                args->sitemap = optarg;
                args->sitemap_given = 1;
                break;
            case 'U': // user-agent
                args->user_agent = optarg;
                args->user_agent_given = 1;
                break;
            case 'C': // cookies
                args->cookies = optarg;
                args->cookies_given = 1;
                break;
            case 's': // allow subdomains
                args->allow_subdomains_flag = 1;
                break;
            case 'q': // keep-query
                args->keep_query_flag = 1;
                break;
            case 'c': // no color
                args->no_color_flag = 1;
                break;
            case 'T': // title
                args->title_flag = 1;
                break;
            case 'f': // http-only
                if(args->https_only_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both http and https only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->http_only_flag = 1;
                break;
            case 'F': // https-only
                if(args->http_only_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both http and https only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->https_only_flag = 1;
                break;
            case 'B': // parse only the <body>
                if(args->only_head_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both head-only and body-only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->only_body_flag = 1;
                break;
            case 'H': // parse only the <head> 
                if(args->only_body_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both head-only and body-only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->only_head_flag = 1;
                break;
            case '4': // ipv4 only
                if(args->only_ipv6_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both IPv4 and IPv6 only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->only_ipv4_flag = 1;
                break;
            case '6': // ipv6 only
                if(args->only_ipv4_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both IPv4 and IPv6 only flags\n", argv[0]);
                        cli_arguments_free(args);
                        return NULL;
                }
                args->only_ipv6_flag = 1;
                break;
            case 'h': // print help
                cli_arguments_print_help(argv[0]);
                cli_arguments_free(args);
                return NULL;
            case 'V': // print version
                printf("Version %s\n", MAPPTTH_VERSION);
                cli_arguments_free(args);
                return NULL;
#if GRAPHVIZ_SUPPORT
            case 'g': // graph flag
                args->graph_flag = 1;
                break;
            case 'G': // graph output format
                args->graph_output_format = optarg;
                break;
            case 'L': // graph layout
                args->graph_layout = optarg;
                break;
#endif
            case '?':
#if GRAPHVIZ_SUPPORT
                if(optopt == 'u' || optopt == 'm' || optopt == 't' || optopt == 'D' || optopt == 'C'
                        || optopt == 'x' || optopt == 'X' || optopt == 'a' || optopt == 'd'
                        || optopt == 'p' || optopt == 'P' || optopt == 'G' || optopt == 'L'
                        || optopt == 'S' || optopt == 'o' || optopt == 'U') {
#else
                if(optopt == 'u' || optopt == 'm' || optopt == 't' || optopt == 'D' || optopt == 'C'
                        || optopt == 'x' || optopt == 'X' || optopt == 'a' ||
                        optopt == 'd' || optopt == 'p' || optopt == 'P' || optopt == 'S'
                        || optopt == 'o' || optopt == 'U') {
#endif
                        fprintf(stderr, "%s: -%c requires an argument\n", argv[0], optopt);
                        free(args);
                        return NULL;
                } else {
                        fprintf(stderr, "%s: unknown parameter: -%c. Ignoring it...\n", argv[0], optopt);
                }
                break;
        }
    }

    if(args->url == NULL) {
        fprintf(stderr, "%s: you need to specify an URL\n", argv[0]);
        free(args);
        return NULL;
    }

    return args;
}
