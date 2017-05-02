project(capturer)
cmake_minimum_required(VERSION 2.8)


############compiler settings############
set(CMAKE_C_COMPILER "g++")
add_definitions(-g -W -Wall -std=c++11 )
#add_definitions(-DDEBUG)


############source files############
aux_source_directory(. SRC_LIST)

#add_subdirectory(subsrc)
include_directories(./)    #head file path

############generate binary files############
set(LIBRARY_OUTPUT_PATH ./)
add_executable(${PROJECT_NAME} ${SRC_LIST})
#add_library(sharedecoder SHARED decoder.cpp)
#add_library(staticdecoder STATIC decoder.cpp)


############link in############
#link extern library directories, like -L in gcc
link_directories(/usr/local/lib)

#set extern libraries
set(LIBBOOST boost_thread boost_system boost_chrono boost_regex)
set(LIBTHIREPARTY pthread protobuf sqlite3 crypto)
set(LIBNDNCPP ndn-cpp)
set(LIBFFMPEG avcodec avutil avformat avfilter avdevice postproc swscale)
set(LIBGLOG glog)

#link libraries, like -l in gcc
target_link_libraries(${PROJECT_NAME} ${LIBBOOST})
target_link_libraries(${PROJECT_NAME} ${LIBTHIREPARTY})
target_link_libraries(${PROJECT_NAME} ${LIBNDNCPP})
target_link_libraries(${PROJECT_NAME} ${LIBFFMPEG})
target_link_libraries(${PROJECT_NAME} ${LIBGLOG})
#target_link_libraries(${PROJECT_NAME} sharedecoder)
#target_link_libraries(${PROJECT_NAME} dl)
