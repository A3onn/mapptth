/** @file cmdline.h
 *  @brief The header file for the command line option parser
 *  generated by GNU Gengetopt version 2.23
 *  http://www.gnu.org/software/gengetopt.
 *  DO NOT modify this file, since it can be overwritten
 *  @author GNU Gengetopt */

#ifndef CMDLINE_H
#define CMDLINE_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h> /* for FILE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
/** @brief the program name (used for printing errors) */
#define CMDLINE_PARSER_PACKAGE "mapptth"
#endif

#ifndef CMDLINE_PARSER_PACKAGE_NAME
/** @brief the complete program name (used for help and version) */
#define CMDLINE_PARSER_PACKAGE_NAME "mapptth"
#endif

#ifndef CMDLINE_PARSER_VERSION
/** @brief the program version */
#define CMDLINE_PARSER_VERSION "0.1.0"
#endif

/** @brief Where the command line options are stored */
struct gengetopt_args_info
{
  const char *help_help; /**< @brief Print help and exit help description.  */
  const char *version_help; /**< @brief Print version and exit help description.  */
  int threads_arg;	/**< @brief Number of threads that will fetch URLs. (default='5').  */
  char * threads_orig;	/**< @brief Number of threads that will fetch URLs. original value given at command line.  */
  const char *threads_help; /**< @brief Number of threads that will fetch URLs. help description.  */
  char * url_arg;	/**< @brief URL where to start crawling..  */
  char * url_orig;	/**< @brief URL where to start crawling. original value given at command line.  */
  const char *url_help; /**< @brief URL where to start crawling. help description.  */
  int timeout_arg;	/**< @brief Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. (default='3').  */
  char * timeout_orig;	/**< @brief Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. original value given at command line.  */
  const char *timeout_help; /**< @brief Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. help description.  */
  int max_depth_arg;	/**< @brief Maximum depth of paths. If a path has a longer depth, it won't be fetched..  */
  char * max_depth_orig;	/**< @brief Maximum depth of paths. If a path has a longer depth, it won't be fetched. original value given at command line.  */
  const char *max_depth_help; /**< @brief Maximum depth of paths. If a path has a longer depth, it won't be fetched. help description.  */
  int allow_subdomains_flag;	/**< @brief Allow the crawler to go into subdomains of the initial URL and allowed domains. (default=off).  */
  const char *allow_subdomains_help; /**< @brief Allow the crawler to go into subdomains of the initial URL and allowed domains. help description.  */
  char ** allowed_domains_arg;	/**< @brief Allow the crawler to go to these domains..  */
  char ** allowed_domains_orig;	/**< @brief Allow the crawler to go to these domains. original value given at command line.  */
  unsigned int allowed_domains_min; /**< @brief Allow the crawler to go to these domains.'s minimum occurreces */
  unsigned int allowed_domains_max; /**< @brief Allow the crawler to go to these domains.'s maximum occurreces */
  const char *allowed_domains_help; /**< @brief Allow the crawler to go to these domains. help description.  */
  int keep_query_flag;	/**< @brief Keep the query part of the URL. Note that if two same URLs with a different query is found, both will be fetched. (default=off).  */
  const char *keep_query_help; /**< @brief Keep the query part of the URL. Note that if two same URLs with a different query is found, both will be fetched. help description.  */
  int no_color_flag;	/**< @brief Don't print with colors. (default=off).  */
  const char *no_color_help; /**< @brief Don't print with colors. help description.  */
  char * user_agent_arg;	/**< @brief String that will be used as user-agent. You can disable sending the user-agent header by giving an empty string..  */
  char * user_agent_orig;	/**< @brief String that will be used as user-agent. You can disable sending the user-agent header by giving an empty string. original value given at command line.  */
  const char *user_agent_help; /**< @brief String that will be used as user-agent. You can disable sending the user-agent header by giving an empty string. help description.  */
  int title_flag;	/**< @brief Print the title of the page if there is one when displaying an URL. (default=off).  */
  const char *title_help; /**< @brief Print the title of the page if there is one when displaying an URL. help description.  */
  char * sitemap_arg;	/**< @brief Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap..  */
  char * sitemap_orig;	/**< @brief Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap. original value given at command line.  */
  const char *sitemap_help; /**< @brief Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap. help description.  */
  char * output_arg;	/**< @brief File to write output into (without colors)..  */
  char * output_orig;	/**< @brief File to write output into (without colors). original value given at command line.  */
  const char *output_help; /**< @brief File to write output into (without colors). help description.  */
  int debug_flag;	/**< @brief Print debug information while running. Uses color when printing, --no-color doesn't have any effects on this. (default=off).  */
  const char *debug_help; /**< @brief Print debug information while running. Uses color when printing, --no-color doesn't have any effects on this. help description.  */
  const char *http_only_help; /**< @brief Only fetch URLs with HTTP as scheme. help description.  */
  const char *https_only_help; /**< @brief Only fetch URLs with HTTPS as scheme. help description.  */
  const char *only_body_help; /**< @brief Only parse the <body> part. help description.  */
  const char *only_head_help; /**< @brief Only parse the <head> part. help description.  */
  const char *IPv6_help; /**< @brief Only resolve to IPv6 addresses. help description.  */
  const char *IPv4_help; /**< @brief Only resolve to IPv4 addresses. help description.  */
  char ** disallowed_paths_arg;	/**< @brief Disallow the crawler to fetch URL starting with these paths..  */
  char ** disallowed_paths_orig;	/**< @brief Disallow the crawler to fetch URL starting with these paths. original value given at command line.  */
  unsigned int disallowed_paths_min; /**< @brief Disallow the crawler to fetch URL starting with these paths.'s minimum occurreces */
  unsigned int disallowed_paths_max; /**< @brief Disallow the crawler to fetch URL starting with these paths.'s maximum occurreces */
  const char *disallowed_paths_help; /**< @brief Disallow the crawler to fetch URL starting with these paths. help description.  */
  char ** allowed_paths_arg;	/**< @brief Allow the crawler to only fetch URL starting with these paths..  */
  char ** allowed_paths_orig;	/**< @brief Allow the crawler to only fetch URL starting with these paths. original value given at command line.  */
  unsigned int allowed_paths_min; /**< @brief Allow the crawler to only fetch URL starting with these paths.'s minimum occurreces */
  unsigned int allowed_paths_max; /**< @brief Allow the crawler to only fetch URL starting with these paths.'s maximum occurreces */
  const char *allowed_paths_help; /**< @brief Allow the crawler to only fetch URL starting with these paths. help description.  */
  char ** allowed_extensions_arg;	/**< @brief Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot)..  */
  char ** allowed_extensions_orig;	/**< @brief Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot). original value given at command line.  */
  unsigned int allowed_extensions_min; /**< @brief Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot).'s minimum occurreces */
  unsigned int allowed_extensions_max; /**< @brief Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot).'s maximum occurreces */
  const char *allowed_extensions_help; /**< @brief Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot). help description.  */
  char ** disallowed_extensions_arg;	/**< @brief Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot)..  */
  char ** disallowed_extensions_orig;	/**< @brief Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot). original value given at command line.  */
  unsigned int disallowed_extensions_min; /**< @brief Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot).'s minimum occurreces */
  unsigned int disallowed_extensions_max; /**< @brief Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot).'s maximum occurreces */
  const char *disallowed_extensions_help; /**< @brief Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. Extensions have to start with a '.' (dot). help description.  */
  
  unsigned int help_given ;	/**< @brief Whether help was given.  */
  unsigned int version_given ;	/**< @brief Whether version was given.  */
  unsigned int threads_given ;	/**< @brief Whether threads was given.  */
  unsigned int url_given ;	/**< @brief Whether url was given.  */
  unsigned int timeout_given ;	/**< @brief Whether timeout was given.  */
  unsigned int max_depth_given ;	/**< @brief Whether max-depth was given.  */
  unsigned int allow_subdomains_given ;	/**< @brief Whether allow-subdomains was given.  */
  unsigned int allowed_domains_given ;	/**< @brief Whether allowed-domains was given.  */
  unsigned int keep_query_given ;	/**< @brief Whether keep-query was given.  */
  unsigned int no_color_given ;	/**< @brief Whether no-color was given.  */
  unsigned int user_agent_given ;	/**< @brief Whether user-agent was given.  */
  unsigned int title_given ;	/**< @brief Whether title was given.  */
  unsigned int sitemap_given ;	/**< @brief Whether sitemap was given.  */
  unsigned int output_given ;	/**< @brief Whether output was given.  */
  unsigned int debug_given ;	/**< @brief Whether debug was given.  */
  unsigned int http_only_given ;	/**< @brief Whether http-only was given.  */
  unsigned int https_only_given ;	/**< @brief Whether https-only was given.  */
  unsigned int only_body_given ;	/**< @brief Whether only-body was given.  */
  unsigned int only_head_given ;	/**< @brief Whether only-head was given.  */
  unsigned int IPv6_given ;	/**< @brief Whether IPv6 was given.  */
  unsigned int IPv4_given ;	/**< @brief Whether IPv4 was given.  */
  unsigned int disallowed_paths_given ;	/**< @brief Whether disallowed-paths was given.  */
  int disallowed_paths_group ; /**< @brief Whether disallowed-paths's was updated.  */
  unsigned int allowed_paths_given ;	/**< @brief Whether allowed-paths was given.  */
  int allowed_paths_group ; /**< @brief Whether allowed-paths's was updated.  */
  unsigned int allowed_extensions_given ;	/**< @brief Whether allowed-extensions was given.  */
  int allowed_extensions_group ; /**< @brief Whether allowed-extensions's was updated.  */
  unsigned int disallowed_extensions_given ;	/**< @brief Whether disallowed-extensions was given.  */
  int disallowed_extensions_group ; /**< @brief Whether disallowed-extensions's was updated.  */

  int extensions_group_counter; /**< @brief Counter for group extensions */
  int parsing_part_group_counter; /**< @brief Counter for group parsing_part */
  int paths_group_counter; /**< @brief Counter for group paths */
  int resolving_ip_version_group_counter; /**< @brief Counter for group resolving_ip_version */
  int scheme_group_counter; /**< @brief Counter for group scheme */
} ;

/** @brief The additional parameters to pass to parser functions */
struct cmdline_parser_params
{
  int override; /**< @brief whether to override possibly already present options (default 0) */
  int initialize; /**< @brief whether to initialize the option structure gengetopt_args_info (default 1) */
  int check_required; /**< @brief whether to check that all required options were provided (default 1) */
  int check_ambiguity; /**< @brief whether to check for options already specified in the option structure gengetopt_args_info (default 0) */
  int print_errors; /**< @brief whether getopt_long should print an error message for a bad option (default 1) */
} ;

/** @brief the purpose string of the program */
extern const char *gengetopt_args_info_purpose;
/** @brief the usage string of the program */
extern const char *gengetopt_args_info_usage;
/** @brief the description string of the program */
extern const char *gengetopt_args_info_description;
/** @brief all the lines making the help output */
extern const char *gengetopt_args_info_help[];

/**
 * The command line parser
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser (int argc, char **argv,
  struct gengetopt_args_info *args_info);

/**
 * The command line parser (version with additional parameters - deprecated)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param override whether to override possibly already present options
 * @param initialize whether to initialize the option structure my_args_info
 * @param check_required whether to check that all required options were provided
 * @return 0 if everything went fine, NON 0 if an error took place
 * @deprecated use cmdline_parser_ext() instead
 */
int cmdline_parser2 (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);

/**
 * The command line parser (version with additional parameters)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param params additional parameters for the parser
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_ext (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  struct cmdline_parser_params *params);

/**
 * Save the contents of the option struct into an already open FILE stream.
 * @param outfile the stream where to dump options
 * @param args_info the option struct to dump
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_dump(FILE *outfile,
  struct gengetopt_args_info *args_info);

/**
 * Save the contents of the option struct into a (text) file.
 * This file can be read by the config file parser (if generated by gengetopt)
 * @param filename the file where to save
 * @param args_info the option struct to save
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_file_save(const char *filename,
  struct gengetopt_args_info *args_info);

/**
 * Print the help
 */
void cmdline_parser_print_help(void);
/**
 * Print the version
 */
void cmdline_parser_print_version(void);

/**
 * Initializes all the fields a cmdline_parser_params structure 
 * to their default values
 * @param params the structure to initialize
 */
void cmdline_parser_params_init(struct cmdline_parser_params *params);

/**
 * Allocates dynamically a cmdline_parser_params structure and initializes
 * all its fields to their default values
 * @return the created and initialized cmdline_parser_params structure
 */
struct cmdline_parser_params *cmdline_parser_params_create(void);

/**
 * Initializes the passed gengetopt_args_info structure's fields
 * (also set default values for options that have a default)
 * @param args_info the structure to initialize
 */
void cmdline_parser_init (struct gengetopt_args_info *args_info);
/**
 * Deallocates the string fields of the gengetopt_args_info structure
 * (but does not deallocate the structure itself)
 * @param args_info the structure to deallocate
 */
void cmdline_parser_free (struct gengetopt_args_info *args_info);

/**
 * Checks that all the required options were specified
 * @param args_info the structure to check
 * @param prog_name the name of the program that will be used to print
 *   possible errors
 * @return
 */
int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CMDLINE_H */
