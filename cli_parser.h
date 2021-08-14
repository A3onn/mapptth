#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

struct arguments {
	char* url;
	unsigned int threads;
	unsigned int timeout;
	unsigned int max_depth;
    unsigned int max_depth_given; // if max_depth has been set
	char* user_agent;
	unsigned int user_agent_given;
	char* sitemap;
	unsigned int sitemap_given;
	char* output;
    unsigned int output_given;
	char* cookies;
    unsigned int cookies_given;
	char* proxy_url;
	unsigned int proxy_url_given;
	struct curl_slist* headers;
#if GRAPHVIZ_SUPPORT
	char* graph_output_format;
	char* graph_layout;
#endif

	// flags
	unsigned int allow_subdomains_flag;
	unsigned int keep_query_flag;
	unsigned int no_color_flag;
	unsigned int title_flag;
	unsigned int http_only_flag;
	unsigned int https_only_flag;
	unsigned int only_body_flag;
	unsigned int only_head_flag;
	unsigned int only_ipv6_flag;
	unsigned int only_ipv4_flag;
	unsigned int ignore_cert_validation;
    unsigned int verbose;
#if GRAPHVIZ_SUPPORT
	unsigned int graph_flag;
#endif

	// allowed
	char** allowed_extensions;
	unsigned int allowed_extensions_count;
	char** allowed_paths;
	unsigned int allowed_paths_count;
	char** allowed_domains;
	unsigned int allowed_domains_count;

	// disallowed
	char** disallowed_extensions;
	unsigned int disallowed_extensions_count;
	char** disallowed_paths;
	unsigned int disallowed_paths_count;
	char** disallowed_domains;
	unsigned int disallowed_domains_count;
};

void cli_arguments_free(struct arguments* args);

struct arguments* parse_cli_arguments(int argc, char** argv);

#endif
