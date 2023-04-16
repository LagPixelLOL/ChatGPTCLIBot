cmake_minimum_required(VERSION 3.1.2)

project(tiktoken LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(PCRE2 CONFIG COMPONENTS 8BIT)
set(PCRE2_USE_STATIC_LIBS ON)

set(OPENAPI_SOURCES byte_pair_encoding.cc emdedded_resource_reader.cc modelparams.cc encoding.cc encoding_utils.cc pcre2_regex.cc)

add_library(tiktoken ${OPENAPI_SOURCES})
target_link_libraries(tiktoken pcre2-8)
target_include_directories(tiktoken PRIVATE ${CMAKE_CURRENT_LIST_DIR})

MESSAGE(STATUS "Copying tokenizers to '${CMAKE_BINARY_DIR}/tokenizers'.")
FILE(COPY cl100k_base.tiktoken p50k_base.tiktoken r50k_base.tiktoken DESTINATION "${CMAKE_BINARY_DIR}/tokenizers")
MESSAGE(STATUS "Tokenizers copied.")
