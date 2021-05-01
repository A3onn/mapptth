cmake_minimum_required(VERSION 3.1.0)

option(MAPPTTH_GENERATE_TESTS "Generate tests" OFF)
option(MAPPTTH_NO_GRAPHVIZ "GraphVizSupport" OFF)

project(mapptth VERSION 0.3.0 LANGUAGES C)

set(SRCS main.c fetcher_thread.c stack_documents.c stack_urls.c cli_parser.c utils.c sitemaps_parser.c)
set(HEADERS fetcher_thread.h stack_documents.h stack_urls.h cli_parser.h utils.h sitemaps_parser.h)

add_executable(mapptth ${SRCS} ${HEADERS})

target_compile_definitions(mapptth PRIVATE MAPPTTH_VERSION="${CMAKE_PROJECT_VERSION}")

# check for threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(mapptth Threads::Threads)


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
    if((CURL_VERSION_MAJOR GREATER_EQUAL 7) AND (CURL_VERSION_MINOR GREATER_EQUAL 62))
        link_directories(mapptth ${CURL_INCLUDE_DIRS})
        target_link_libraries(mapptth ${CURL_LIBRARY})
    else()
        message(FATAL_ERROR "libcurl's version should be at least 7.62.0")
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
        add_executable(tests_mapptth tests_mapptth.c stack_documents.c stack_urls.c utils.c sitemaps_parser.c stack_documents.h stack_urls.h utils.h sitemaps_parser.h)

        # libcheck
        target_link_libraries(tests_mapptth ${CHECK_LIBRARY})

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
    else()
        message(STATUS "Couldn't find Check library for testing, disable tests")
    endif()
endif()
