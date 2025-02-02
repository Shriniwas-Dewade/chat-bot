cmake_minimum_required(VERSION 3.22)

project(chatbot)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include)

include(FetchContent)

FetchContent_Declare(
    llama_cpp
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG master
)

FetchContent_MakeAvailable(llama_cpp)

add_executable(chatbot src/main.cpp src/bot.cpp)
target_link_libraries(chatbot PRIVATE llama)
