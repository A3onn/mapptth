cmake_minimum_required(VERSION 3.1.0)

option(MAPPTTH_GENERATE_TESTS "Generate tests" OFF)
option(MAPPTTH_NO_GRAPHVIZ "GraphVizSupport" OFF)

project(mapptth VERSION 1.0.0 LANGUAGES C)

set(SRCS main.c fetcher_thread.c stack_documents.c stack_urls.c trie_urls.c cli_parser.c utils.c sitemaps_parser.c logger.c robots_txt.c)
set(HEADERS fetcher_thread.h stack_documents.h stack_urls.h trie_urls.h cli_parser.h utils.h sitemaps_parser.h logger.h robots_txt.h)

add_executable(mapptth ${SRCS} ${HEADERS})

target_compile_definitions(mapptth PRIVATE MAPPTTH_VERSION="${CMAKE_PROJECT_VERSION}")

# compile settings
if(MSVC)
        target_compile_options(mapptth PUBLIC /W4 /WX)
else()
        target_compile_options(mapptth PUBLIC -Wall -Wextra -pedantic -Werror)
endif()
set_target_properties(mapptth PROPERTIES C_STANDARD 99) # use c99 like lexbor

# check for threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(mapptth Threads::Threads)

# check for pcre
include(FindPackageHandleStandardArgs)
find_package(PkgConfig QUIET)
pkg_check_modules(PC_PCRE QUIET pcre)

find_path(PCRE_INCLUDE_DIR NAMES pcre.h HINTS ${PCRE_ROOT_DIR}/include ${PCRE_ROOT_INCLUDE_DIRS} PATHS ${PC_PCRE_INCLUDE_DIRS})
find_library(PCRE_LIBRARY NAMES pcre HINTS ${PCRE_ROOT_DIR}/lib ${PCRE_ROOT_LIBRARY_DIRS} PATHS ${PC_PCRE_LIBRARY_DIRS})
find_package_handle_standard_args(PCRE FOUND_VAR PCRE_FOUND REQUIRED_VARS PCRE_LIBRARY PCRE_INCLUDE_DIR VERSION_VAR PC_PCRE_VERSION)

if(PCRE_FOUND AND NOT TARGET Pcre::Pcre)
    message(STATUS "Found PCRE library: " ${PCRE_INCLUDE_DIR})
    add_library(Pcre::Pcre UNKNOWN IMPORTED)
    set_target_properties(Pcre::Pcre PROPERTIES IMPORTED_LOCATION "${PCRE_LIBRARY}" INTERFACE_COMPILE_OPTIONS "${PC_PCRE_CFLAGS_OTHER}" INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
    target_link_libraries(mapptth Pcre::Pcre)
else()
    message(FATAL_ERROR "Could not find PCRE library.")
endif()


#check for graphviz
if(NOT MAPPTTH_NO_GRAPHVIZ)
	find_library(GRAPHVIZ_LIB_GVC NAMES gvc libgvc)
	find_library(GRAPHVIZ_LIB_CGRAPH NAMES cgraph libcgraph)
	if(NOT(GRAPHVIZ_LIB_GVC AND GRAPHVIZ_LIB_CGRAPH))
		message(STATUS "Could not find the GraphViz library. Disabling GraphViz support")
		target_compile_definitions(mapptth PRIVATE GRAPHVIZ_SUPPORT=0)
	else()
		message(STATUS "Found the GraphViz library: ${GRAPHVIZ_LIB_GVC} ${GRAPHVIZ_LIB_CGRAPH}")
		target_compile_definitions(mapptth PRIVATE GRAPHVIZ_SUPPORT=1)
		target_link_libraries(mapptth ${GRAPHVIZ_LIB_GVC} ${GRAPHVIZ_LIB_CGRAPH})
	endif()
endif()


# check for curl
set(CURL_FIND_COMPONENTS HTTP HTTPS HTTP2 HTTP-Proxy)
include(FindCURL)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "libcurl not found")
else()
    string(REPLACE "." ";" CURL_VERSION_LIST ${CURL_VERSION_STRING}) # str -> stack
    list(GET CURL_VERSION_LIST 0 CURL_VERSION_MAJOR)
    list(GET CURL_VERSION_LIST 1 CURL_VERSION_MINOR)
    if((CURL_VERSION_MAJOR LESS 7) AND (CURL_VERSION_MINOR LESS 62))
        message(FATAL_ERROR "libcurl's version should be at least 7.62.0")
    else()
        link_directories(mapptth ${CURL_INCLUDE_DIRS})
        target_link_libraries(mapptth ${CURL_LIBRARY})
    endif()
endif()

# check for lexbor
find_library(LEXBOR_LIBRARY NAMES lexbor required)
if(NOT LEXBOR_LIBRARY)
    message(STATUS "liblexbor not found on the system. Trying to find it in the ${PROJECT_SOURCE_DIR}/lexbor/ directory...")
    if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/lexbor/)
        message(STATUS "${PROJECT_SOURCE_DIR}/lexbor/ exists. Trying to use it to build liblexbor...")
        add_subdirectory(${PROJECT_SOURCE_DIR}/lexbor/)
        target_include_directories(mapptth PRIVATE ${LEXBOR_DIR_HEADER})
        target_link_libraries(mapptth lexbor)
    else()
        message(FATAL_ERROR "Could not find liblexbor...")
    endif()
else()
    message(STATUS "Found Lexbor ${LEXBOR_LIBRARY}")
    target_link_libraries(mapptth ${LEXBOR_LIBRARY})
endif()

#check for libxml2
include(FindLibXml2)
if(NOT LIBXML2_FOUND)
    message(FATAL_ERROR "libxml2 not found")
else()
    include_directories(mapptth ${LIBXML2_INCLUDE_DIRS})
    target_link_libraries(mapptth ${LIBXML2_LIBRARY})
    add_definitions(${LIBXML2_DEFINITIONS})
endif()


# tests
if(MAPPTTH_GENERATE_TESTS)
    message(STATUS "Trying to find libCheck...")
    find_library(CHECK_LIBRARY NAMES libcheck libCheck check Check)
    if(CHECK_LIBRARY)
        message(STATUS "Making tests...")
        add_executable(tests_mapptth tests_mapptth.c stack_documents.c stack_urls.c utils.c sitemaps_parser.c stack_documents.h stack_urls.h utils.h sitemaps_parser.h trie_urls.c logger.c)

        # libcheck
        target_link_libraries(tests_mapptth ${CHECK_LIBRARY} rt subunit)

        # libxml2
        include_directories(tests_mapptth ${LIBXML2_INCLUDE_DIRS})
        target_link_libraries(tests_mapptth ${LIBXML2_LIBRARY})

        # lexbor
        if(LEXBOR_LIBRARY)
            target_link_libraries(tests_mapptth ${LEXBOR_LIBRARY})
        else()
            target_include_directories(tests_mapptth PRIVATE ${LEXBOR_DIR_HEADER})
            target_link_libraries(tests_mapptth lexbor)
        endif()

        # libcurl
        link_directories(tests_mapptth ${CURL_INCLUDE_DIRS})
        target_link_libraries(tests_mapptth ${CURL_LIBRARY})

        # PRCE
        target_link_libraries(tests_mapptth Pcre::Pcre)
    else()
        message(STATUS "Couldn't find Check library for testing, disable tests")
    endif()
endif()
