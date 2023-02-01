#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <curl/curl.h>

// for versions strings
#include <curl/curlver.h>
#include <libxml/xmlversion.h>
#include <lexbor/core/base.h>
#include <pcre.h>

#if GRAPHVIZ_SUPPORT
#include <graphviz/graphviz_version.h> // PACKAGE_VERSION
#endif

struct arguments {
	char* url;
	unsigned int threads;
	unsigned int timeout;
	unsigned int max_depth;
	bool max_depth_given; // if max_depth has been set
	char* user_agent;
	char* sitemap;
	char* robots_txt;
	char* output;
	char* cookies;
	char* proxy_url;
	struct curl_slist* headers;
#if GRAPHVIZ_SUPPORT
	char* graph_output_format;
	char* graph_layout;
#endif

	// flags
	bool allow_subdomains_flag;
	bool keep_query_flag;
	bool no_color_flag;
	bool title_flag;
	bool http_only_flag;
	bool https_only_flag;
	bool only_body_flag;
	bool only_head_flag;
	bool only_ipv6_flag;
	bool only_ipv4_flag;
    bool print_as_dir;
	bool ignore_cert_validation;
    bool verbose;
#if GRAPHVIZ_SUPPORT
	bool graph_flag;
#endif

	// allowed
	char** allowed_extensions;
	unsigned int allowed_extensions_count;
	char** allowed_paths;
	unsigned int allowed_paths_count;
	char** allowed_domains;
	unsigned int allowed_domains_count;
	unsigned short* allowed_ports;
	unsigned int allowed_ports_count;

	// disallowed
	char** disallowed_extensions;
	unsigned int disallowed_extensions_count;
	char** disallowed_paths;
	unsigned int disallowed_paths_count;
	char** disallowed_domains;
	unsigned int disallowed_domains_count;
};

void cli_arguments_free();

bool parse_cli_arguments(int argc, char** argv);

extern struct arguments cli_arguments;

#endif
