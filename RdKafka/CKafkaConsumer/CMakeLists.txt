cmake_minimum_required(VERSION 2.8)

project(CKafkaConsumer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER "g++")
#set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Kafka头文件路径
include_directories(/usr/local/include/librdkafka)
# Kafka库路径
link_directories(/usr/local/lib)

aux_source_directory(. SOURCE)

add_executable(${PROJECT_NAME} ${SOURCE})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} rdkafka)

