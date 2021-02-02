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

## How to use

The only required argument is ```-u <URL>```. This specifies where the crawler starts to crawl.

Here is the list of available parameters grouped by category:

### Connection

| Name | Argument |
| --- | --- |
| URL where to start crawling. __(REQUIRED)__ | -u \<URL> |
| String that will be used as user-agent. You can disable sending the user-agent header by giving an empty string. | -U \<user-agent> |
| Timeout in seconds for each connection. If a connection timeout, an error will be printed to standard error but no informations about the URL. | -m \<timeout> |
| Only resolve to IPv4 addresses. | -4 |
| Only resolve to IPv6 addresses. | -6 |


### Controlling where the crawler goes

| Name | Argument |
| --- | --- |
| Allow the crawler to go into subdomains of the initial URL and allowed domains. | -s |
| Allow the crawler to go to these domains. | -a \<domain> |
| Disallow the crawler to fetch URL starting with these paths. | -d \<path> |
| Allow the crawler to only fetch URL starting with these paths. | -A \<path> |
| Maximum depth of paths. If a path has a longer depth, it won't be fetched. | -D \<depth> |
| Only fetch URLs with HTTP as scheme. | -p |
| Only fetch URLs with HTTPS as scheme. | -P |
| Allow the crawler to only fetch files with these extensions. If no extension is found then this filter won't apply. | -x .\<extension> |
| Disallow the crawler to fetch files with these extensions. If no extension is found then this filter won't apply. | -X .\<extension> |
| Keep the query part of the URL. Note that if two same URLs with a different query is found, both will be fetched. | -k |


### Parsing

_Only works for HTML files._

| Name | Argument |
| --- | --- |
| Only parse the \<head> part. | -H |
| Only parse the \<body> part. | -B |


### Output

| Name | Argument |
| --- | --- |
| Don't print with colors. | -c |
| Print the title of the page if there is one when displaying an URL. | -T |
| File to write output into (without colors). | -o \<file name> |


### Other

| Name | Argument |
| --- | --- |
| Number of threads that will fetch URLs. | -t \<number of threads> |
| Parse the sitemap of the site, this should speeds up the crawler and will maybe provide URLs that couldn't be found without the sitemap. | -S \<URL of the sitemap> |
| Print the help. | -h |
| Print the version. | -V |


## TODO

ASAP:

- [ ] Handling the <base> tag


Without any priority :

- [ ] More unit tests

- [ ] GraphViz support to generate graphs

- [ ] Add a parameter to control the connection rate

- [ ] Add parameters to control: disallowed domains, only allowed paths and disallowed extensions

- [ ] Add exemples in readme

- [ ] Add robots.txt parser, can choose to follow rules or add URLs specified into starting list of URLs

- [ ] Print when encountering mailto: or tel:

- [ ] Create logo (maybe)

- [X] Output to file
