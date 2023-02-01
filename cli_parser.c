#include "cli_parser.h"

void _init_arguments() {
    memset(&cli_arguments, 0, sizeof (struct arguments));
    cli_arguments.timeout = 3;
    cli_arguments.threads = 5;
    cli_arguments.disallowed_paths = (char**) malloc(sizeof (char*));
    cli_arguments.disallowed_extensions = (char**) malloc(sizeof (char*));
    cli_arguments.disallowed_domains = (char**) malloc(sizeof (char*));

    cli_arguments.allowed_paths = (char**) malloc(sizeof (char*));
    cli_arguments.allowed_extensions = (char**) malloc(sizeof (char*));
    cli_arguments.allowed_domains = (char**) malloc(sizeof (char*));
    cli_arguments.allowed_ports = (unsigned short*) malloc(sizeof (unsigned short));
#if GRAPHVIZ_SUPPORT
    cli_arguments.graph_layout = "dot"; // works best with ranks
    cli_arguments.graph_output_format = "png";
#endif
}

void cli_arguments_free() {
    if(cli_arguments.headers != NULL) {
        curl_slist_free_all(cli_arguments.headers);
    }
    free(cli_arguments.disallowed_paths);
    free(cli_arguments.disallowed_extensions);
    free(cli_arguments.disallowed_domains);

    free(cli_arguments.allowed_paths);
    free(cli_arguments.allowed_ports);
    free(cli_arguments.allowed_extensions);
    free(cli_arguments.allowed_domains);
}

void cli_arguments_print_help(char* prgm_name) {
    printf("Usage: %s <parameters>\n\n", prgm_name);
    puts("Connection:");
    puts("\t<url>: URL where to start crawling, the last specified will be used.");
    printf("\t-B <string>: string that will be used as user-agent. You can disable sending the user-agent header by giving an empty string. (default='MAPPTTH/%s')\n", MAPPTTH_VERSION);
    puts("\t-m <integer>: Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. (default=3)");
    puts("\t-4: Only resolve to IPv4 addresses.");
    puts("\t-6: Only resolve to IPv6 addresses.");
    puts("\t-Q <header>: Add headers in the HTTP request, they are like this: \"<key>:<value>;\", the ':' and the value are optionals and they have to end with a ';'.");
    puts("\t-C <cookies>: Add cookies in the HTTP request, they are like this: \"<key>:<value>;\", you can specify mulitple cookies at once by separating them by a ';'. Note that they won't be modified during the crawl.");
    puts("\t-i: Allow insecure connections when using SSL/TLS.");

    puts("\nControlling where the crawler goes:");
    puts("\t-s: Allow the crawler to go into subdomains of the initial URL and allowed domains. (default=false)");
    puts("\t-a <domain>: Allow the crawler to go to these domains.");
    puts("\t-d <domain>: Disallow the crawler to go to these domains.");
    puts("\t-p <path or regex>: Allow the crawler to only fetch URL starting with these paths. Can be a regex (extended and case-sensitive).");
    puts("\t-P <path or regex>: Disallow the crawler to fetch URL starting with these paths. Can be a regex (extended and case-sensitive).");
    puts("\t-D <integer>: Maximum depth of paths. If a path has a longer depth, it won't be fetched.");
    puts("\t-f: Only fetch URLs with HTTP as scheme (Don't forget to add '-r 80' if you start with an 'https://' URL).");
    puts("\t-F: Only fetch URLs with HTTPS as scheme (Don't forget to add '-r 443' if you start with an 'http://' URL).");
    puts("\t-x <extension>: Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply.");
    puts("\t-X <extension>: Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply.");
    puts("\t-r <port>: Allow the crawler to go to theses ports.");
    puts("\t-q: Keep the query part of the URL. Note that if two same URLs with a different query is found, both will be fetched.");

    puts("\nParsing:");
    puts("\t-H: Only parse the <head> part.");
    puts("\t-B: Only parse the <body> part.");

    puts("\nOutput:");
    puts("\t-c: Don't print with colors.");
    puts("\t-T: Print the title of the page if there is one when displaying an URL.");
    puts("\t-o <file name>: File to write output into (without colors).");
    puts("\t-O: Print a summary of what was found as a directory structure.");

#if GRAPHVIZ_SUPPORT
    puts("\nGraph:");
    puts("\t-g: Create a graph.");
    puts("\t-L <layout>: Change the layout of the graph. (default='sfdp')");
    puts("\t-G <format>: Change the output graph file format. (default='png')");
#endif

    puts("\nOther:");
    puts("\t-t <integer>: Number of threads that will fetch URLs. (default=5)");
    puts("\t-S <url>: Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap.");
    puts("\t-R <url>: Parse the robots.txt of the site, paths found in 'allowed' and 'disallowed' directives are added to the list of found URLs. Other directives are ignored.");
    puts("\t-z <url>: URL of the proxy to use.");
    puts("\t-v: Verbose mode.");
    puts("\t-h: Print the help.");
    puts("\t-V: Print the version.");


    puts("\nExemples:");
    printf("\t%s https://google.com/some/url/file.html\n", prgm_name);
    printf("\t%s http://google.com -s -a gitlab.com -a github.com -r 443\n", prgm_name);
    printf("\t%s https://google.com -P /path -P /some-path\n", prgm_name);
    printf("\t%s https://google.com -P /some-path -x .html -x .php\n", prgm_name);
    printf("\t%s https://google.com/mail -x .html -P /some-path -t 10 -m 5 -s -q -D 6 -T -o output.txt -H -S http://www.google.com/sitemap.xml\n", prgm_name);
}

bool parse_cli_arguments(int argc, char** argv) {
    struct arguments* args = (struct arguments*) malloc(sizeof (struct arguments));
    _init_arguments(args);

#if GRAPHVIZ_SUPPORT
    char* args_str = "t:m:U:S:R:o:D:C:z:r:OvsqciTfFBH64qVhQ:p:P:x:X:a:d:gG:L:";
#else
    char* args_str = "t:m:U:S:R:o:D:C:z:r:OvsqciTfFBH64qVhQ:p:P:x:X:a:d:";
#endif

    // used when using strtoul
    char* endptr;

    opterr = 0; // disable getopt error printing
    int c;
    while((c = getopt(argc, argv, args_str)) != -1) {
        switch(c) {
            case 't': // threads
                errno = 0;
                cli_arguments.threads = strtoul(optarg, &endptr, 10);
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free();
                    return false;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid number of threads value: %s\n", argv[0], optarg);
                    cli_arguments_free();
                    return false;
                }
                break;
            case 'm': // timeout
                errno = 0;
                cli_arguments.timeout = strtoul(optarg, &endptr, 10);
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free();
                    return false;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid timeout value: %s\n", argv[0], optarg);
                    cli_arguments_free();
                    return false;
                }
                break;
            case 'D': // max-depth
                errno = 0;
                cli_arguments.max_depth = strtoul(optarg, &endptr, 10);
                cli_arguments.max_depth_given = 1;
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free();
                    return false;
                } else if(endptr == optarg) { // if starts of invalid number is the first chararcter of the arg
                    fprintf(stderr, "%s: invalid max-depth value: %s\n", argv[0], optarg);
                    cli_arguments_free();
                    return false;
                }
                break;
            case 'p': // allowed paths
                cli_arguments.allowed_paths_count++;
                cli_arguments.allowed_paths = (char**) reallocarray(cli_arguments.allowed_paths, cli_arguments.allowed_paths_count, sizeof (char*));
                cli_arguments.allowed_paths[cli_arguments.allowed_paths_count-1] = optarg;
                break;
            case 'P': // disallowed paths
                cli_arguments.disallowed_paths_count++;
                cli_arguments.disallowed_paths = (char**) reallocarray(cli_arguments.disallowed_paths, cli_arguments.disallowed_paths_count, sizeof (char*));
                cli_arguments.disallowed_paths[cli_arguments.disallowed_paths_count-1] = optarg;
                break;
            case 'a': // allowed domains
                cli_arguments.allowed_domains_count++;
                cli_arguments.allowed_domains = (char**) reallocarray(cli_arguments.allowed_domains, cli_arguments.allowed_domains_count, sizeof (char*));
                cli_arguments.allowed_domains[cli_arguments.allowed_domains_count-1] = optarg;
                break;
            case 'd': // disallowed domains
                cli_arguments.disallowed_domains_count++;
                cli_arguments.disallowed_domains = (char**) reallocarray(cli_arguments.disallowed_domains, cli_arguments.disallowed_domains_count, sizeof (char*));
                cli_arguments.disallowed_domains[cli_arguments.disallowed_domains_count-1] = optarg;
                break;
            case 'x': // allowed extensions
                cli_arguments.allowed_extensions_count++;
                cli_arguments.allowed_extensions = (char**) reallocarray(cli_arguments.allowed_extensions, cli_arguments.allowed_extensions_count, sizeof (char*));
                cli_arguments.allowed_extensions[cli_arguments.allowed_extensions_count-1] = optarg;
                break;
            case 'X': // disallowed extensions
                cli_arguments.disallowed_extensions_count++;
                cli_arguments.disallowed_extensions = (char**) reallocarray(cli_arguments.disallowed_extensions, cli_arguments.disallowed_extensions_count, sizeof (char*));
                cli_arguments.disallowed_extensions[cli_arguments.disallowed_extensions_count-1] = optarg;
                break;
            case 'r': // allowed ports
                errno = 0;
                unsigned short newPort = (unsigned short) strtoul(optarg, &endptr, 10);
                if(errno != 0) {
                    fprintf(stderr, "%s: %s", argv[0], strerror(errno));
                    cli_arguments_free();
                    return false;
                } else if(endptr == optarg) {
                    fprintf(stderr, "%s: invalid port value: %s\n", argv[0], optarg);
                    cli_arguments_free();
                    return false;
                }
                cli_arguments.allowed_ports_count++;
                cli_arguments.allowed_ports = (unsigned short*) reallocarray(cli_arguments.allowed_ports, cli_arguments.allowed_ports_count, sizeof (unsigned short));
                cli_arguments.allowed_ports[cli_arguments.allowed_ports_count-1] = newPort;
                break;
            case 'o': // output
                cli_arguments.output = optarg;
                break;
            case 'S': // sitemap
                cli_arguments.sitemap = optarg;
                break;
            case 'R': // robots.txt
                cli_arguments.robots_txt = optarg;
                break;
            case 'U': // user-agent
                cli_arguments.user_agent = optarg;
                break;
            case 'C': // cookies
                cli_arguments.cookies = optarg;
                break;
            case 'z': // proxy
                cli_arguments.proxy_url = optarg;
                break;
            case 'Q': // headers to send
                if(optarg[strlen(optarg) - 1] != ';') {
                    fprintf(stderr ,"Headers specified with -Q should end with a ';'. \"%s\" does not.\n", optarg);
                    cli_arguments_free();
                    return false;
                }
                cli_arguments.headers = curl_slist_append(cli_arguments.headers, optarg);
                if(cli_arguments.headers == NULL) {
                    fprintf(stderr, "An error occured while adding the header: %s, quitting...\n", optarg);
                    cli_arguments_free();
                }
                break;
            case 's': // allow subdomains
                cli_arguments.allow_subdomains_flag = true;
                break;
            case 'q': // keep-query
                cli_arguments.keep_query_flag = true;
                break;
            case 'c': // no color
                cli_arguments.no_color_flag = true;
                break;
            case 'T': // title
                cli_arguments.title_flag = true;
                break;
            case 'O': // output
                cli_arguments.print_as_dir = true;
                break;
            case 'f': // http-only
                if(cli_arguments.https_only_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both http and https only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.http_only_flag = true;
                break;
            case 'F': // https-only
                if(cli_arguments.http_only_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both http and https only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.https_only_flag = true;
                break;
            case 'B': // parse only the <body>
                if(cli_arguments.only_head_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both head-only and body-only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.only_body_flag = true;
                break;
            case 'H': // parse only the <head> 
                if(cli_arguments.only_body_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both head-only and body-only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.only_head_flag = true;
                break;
            case '4': // ipv4 only
                if(cli_arguments.only_ipv6_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both IPv4 and IPv6 only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.only_ipv4_flag = true;
                break;
            case '6': // ipv6 only
                if(cli_arguments.only_ipv4_flag) { // cannot have both flag set
                        fprintf(stderr, "%s: you cannot set both IPv4 and IPv6 only flags\n", argv[0]);
                        cli_arguments_free();
                        return false;
                }
                cli_arguments.only_ipv6_flag = true;
                break;
            case 'h': // print help
                cli_arguments_print_help(argv[0]);
                cli_arguments_free();
                return false;
            case 'i': // don't verify certificate
                cli_arguments.ignore_cert_validation = true;
                break;
            case 'v': // verbose mode
                cli_arguments.verbose = true;
                break;
            case 'V': // print version
                printf("MapPPTH: %s\n", MAPPTTH_VERSION);
                printf("libcurl: %s\n", LIBCURL_VERSION);
                printf("lexbor: %s\n", LEXBOR_VERSION_STRING);
                printf("libxml2: %s\n", LIBXML_DOTTED_VERSION);

#ifdef PCRE_CONFIG_JIT
                int pcre_has_jit;
                pcre_config(PCRE_CONFIG_JIT, &pcre_has_jit);
                if(pcre_has_jit) {
                    const char* pcre_jit_arch;
                    pcre_config(PCRE_CONFIG_JITTARGET, &pcre_jit_arch);
                    printf("PCRE: %s (JIT support: %s)\n", pcre_version(), pcre_jit_arch);
                } else {
                    printf("PCRE: %s (no JIT)\n", pcre_version());
                }
#else
                printf("PCRE: %s (no JIT)\n", pcre_version());
#endif
#if GRAPHVIZ_SUPPORT
                printf("GraphViz: %s\n", PACKAGE_VERSION);
#endif
                cli_arguments_free();
                return false;
#if GRAPHVIZ_SUPPORT
            case 'g': // graph flag
                cli_arguments.graph_flag = true;
                break;
            case 'G': // graph output format
                cli_arguments.graph_output_format = optarg;
                break;
            case 'L': // graph layout
                cli_arguments.graph_layout = optarg;
                break;
#endif
            case '?':
#if GRAPHVIZ_SUPPORT
                if(optopt == 'm' || optopt == 't' || optopt == 'D' || optopt == 'C'
                        || optopt == 'x' || optopt == 'X' || optopt == 'a' || optopt == 'd'
                        || optopt == 'p' || optopt == 'P' || optopt == 'G' || optopt == 'L'
                        || optopt == 'S' || optopt == 'o' || optopt == 'U' || optopt == 'Q'
                        || optopt == 'R') {
#else
                if(optopt == 'm' || optopt == 't' || optopt == 'D' || optopt == 'C'
                        || optopt == 'x' || optopt == 'X' || optopt == 'a' || optopt == 'd'
                        || optopt == 'p' || optopt == 'P' || optopt == 'S' || optopt == 'o'
                        || optopt == 'U' || optopt == 'Q' || optopt == 'R') {
#endif
                        fprintf(stderr, "%s: -%c requires an argument\n", argv[0], optopt);
                        free(args);
                        return false;
                } else {
                        fprintf(stderr, "%s: unknown parameter: -%c. Ignoring it...\n", argv[0], optopt);
                }
                break;
        }
    }

    for(; optind < argc; optind++) {
        if(cli_arguments.url != NULL) { // free last URL given, so it uses the last one specified
            free(cli_arguments.url);
        }
        cli_arguments.url = malloc(strlen(argv[optind]) * sizeof (char) + sizeof (char)); // <url> + '\0'
        strcpy(cli_arguments.url, argv[optind]);
    }

    if(cli_arguments.url == NULL) {
        fprintf(stderr, "%s: you need to specify an URL\n", argv[0]);
        cli_arguments_free();
        return false;
    }

    return true;
}
