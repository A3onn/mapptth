[MapPTTH](https://gitlab.com/Aeonn/mapptth/ "MapPTTH gitlab")
====

A simple to use multi-threaded web-crawler written in C with libcURL and Lexbor.
---

## Dependencies

MapPTTH uses:

- libcURL (>= 7.62.0)
- Lexbor (see [Installation](#installation) if you don't to install it)
- libxml2
- CMake (>= 3.1.0)

#### Optional

- GraphViz (_libgvc_ and _libcgraph_): generate graphs
- libcheck: unit tests

## Installation

If you have all dependencies installed on your system:

```
git clone https://gitlab.com/Aeonn/mapptth/
cd mapptth/
mkdir build/ && cd build/
cmake .. && make
```

If you don't have Lexbor installed and don't want to install it, you can clone Lexbor while cloning MapPTTH and compile without any installation:

```
git clone --recurse-submodules https://gitlab.com/Aeonn/mapptth/
cd mapptth/
mkdir build/ && cd build/
cmake .. && make
```

### GraphViz support

If GraphViz is found on the system when running CMake, you will be able to generate graphs.

If you want to disable it, you can run `cmake -DMAPPTTH_NO_GRAPHVIZ ..` instead of `cmake ..`.

## How to use

### Parameters

The only required argument is ```-u <URL>```. This parameter specifies where the crawler starts to crawl.

Here is the list of available parameters grouped by category:

#### Connection

| Name | Argument |
| --- | --- |
| URL where to start crawling. __(REQUIRED)__ | -u \<URL> |
| String that will be used as user-agent. You can disable sending the user-agent header by giving an empty string. (default='MAPPTTH/<version>') | -U \<user-agent> |
| Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. (default=3) | -m \<timeout> |
| Only resolve to IPv4 addresses. | -4 |
| Only resolve to IPv6 addresses. | -6 |
| \<header>: Add headers in the HTTP request, they are like this: "\<key>:;\<value>;", the ':' and the value are optionals and they have to end with a ';'. | -Q \<header> |


#### Controlling where the crawler goes

| Name | Argument |
| --- | --- |
| Allow the crawler to go into subdomains of the initial URL and allowed domains. (default=false) | -s |
| Allow the crawler to go to these domains. | -a \<domain> |
| Disallow the crawler to go to these domains. | -d \<domain> |
| Allow the crawler to only fetch URL starting with these paths. | -p \<path> |
| Disallow the crawler to fetch URL starting with these paths. | -P \<path> |
| Maximum depth of paths. If a path has a longer depth, it won't be fetched. | -D \<depth> |
| Only fetch URLs with HTTP as scheme. | -f |
| Only fetch URLs with HTTPS as scheme. | -F |
| Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. | -x .\<extension> |
| Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. | -X .\<extension> |
| Keep the query part of the URL. Note that if two same URLs with a different query is found, both will be fetched. | -q |


#### Parsing

_Only works for HTML files._

| Name | Argument |
| --- | --- |
| Only parse the \<head> part. | -H |
| Only parse the \<body> part. | -B |


#### Output

| Name | Argument |
| --- | --- |
| Don't print with colors. | -c |
| Print the title of the page if there is one when displaying an URL. | -T |
| File to write output into (without colors). | -o \<file name> |

#### Graph

_GraphViz support must be enabled to use these parameters._

| Name | Argument |
| --- | --- |
| Create a graph. | -g |
| Change the layout of the graph. (default='sfdp') | -L \<layout> |
| Change the output graph file format. (default='png') | -G \<format> |

#### Other

| Name | Argument |
| --- | --- |
| Number of threads that will fetch URLs. (default=5) | -t \<number of threads> |
| Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap. | -S \<URL of the sitemap> |
| Print the help. | -h |
| Print the version. | -V |

### Exemples

Simple crawl:

```
mapptth -u https://google.com
```

Start crawling at a certain URL:

```
mapptth -u https://google.com/some/url/file.html
```

More threads:

```
mapptth -u https://google.com -t 10
```

Allow to crawl into subdomains (ex: www.google.com, mail.google.com, ww.mail.google.com):

```
mapptth -u https://google.com -s
```

Allow to crawl certain domains and their subdomains (ex: www.google.com, mail.gitlab.com, www.mail.github.com):

```
mapptth -u https://google.com -s -a gitlab.com -a github.com
```

Disallow some paths:

```
mapptth -u https://google.com -P /path -P /some-path
```

Disallow a path and only fetch .html and .php files:

```
mapptth -u https://google.com -P /some-path -x .html -x .php
```

Only crawl in the /path directory:

```
mapptth -u https://google.com -p /path
```

A more complete and complicated one:

```
mapptth -u https://google.com/mail -x .html -P /some-path -t 10 -m 5 -s -q -D 6 -T -o output.txt -H -S http://www.google.com/sitemap.xml
```

## TODO

ASAP:

- [X] Handling the \<base\> tag


Without any priority :

- [ ] More unit tests

- [ ] Add a parameter to control the connection rate

- [X] Add exemples in readme

- [ ] Add robots.txt parser, can choose to follow rules or add URLs specified into starting list of URLs

- [ ] Print when encountering mailto: or tel:

- [ ] Create logo (maybe)

- [ ] Use regex in filters (disallowed paths, allowed paths, etc...)

- [X] Use only getopt to parse arguments

- [X] GraphViz support to generate graphs

- [X] Output to file

- [X] Add parameters to control: disallowed domains, only allowed paths and disallowed extensions
