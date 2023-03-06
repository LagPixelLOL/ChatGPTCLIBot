cmake_minimum_required(VERSION 3.24)
project(GPT3Bot)

set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_EXE_LINKER_FLAGS "-static")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif ()

find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(libproxy REQUIRED)
find_package(libmodman REQUIRED)

add_executable(${PROJECT_NAME} main/main.cpp main/GPTMain.cpp main/GPTMain.h main/util/PromptUtils.cpp main/util/PromptUtils.h main/util/SystemUtils.cpp main/util/SystemUtils.h main/model/Embedding.cpp main/model/Embedding.h main/chat/Exchange.cpp main/chat/Exchange.h)

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl nlohmann_json::nlohmann_json ${LIBPROXY_LIBRARIES} ${LIBMODMAN_LIBRARIES})
if (NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lpthread -Wl,-Bdynamic,--no-whole-archive")
endif()