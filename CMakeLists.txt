cmake_minimum_required(VERSION 3.1.0)
project(mapptth VERSION 0.0.8 LANGUAGES C)

include(CTest)
enable_testing()

set(SRCS main.c fetcher_thread.c linked_list_documents.c linked_list_urls.c cmdline.c utils.c)
set(HEADERS fetcher_thread.h linked_list_documents.h linked_list_urls.h cmdline.h utils.h)

add_executable(mapptth ${SRCS} ${HEADERS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(mapptth Threads::Threads)


# check for curl
set(CURL_FIND_COMPONENTS HTTP HTTPS HTTP2 HTTP-Proxy)
include(FindCURL)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "libcurl not found")
else()
    string(REPLACE "." ";" CURL_VERSION_LIST ${CURL_VERSION_STRING}) # str -> list
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
    message(FATAL_ERROR "liblexbor not found.")
else()
    message(STATUS "Found Lexbor ${LEXBOR_LIBRARY}")
    target_link_libraries(mapptth ${LEXBOR_LIBRARY})
endif()