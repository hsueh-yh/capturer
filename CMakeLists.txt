project(capturer)
cmake_minimum_required(VERSION 2.8)


############compiler settings############
set(CMAKE_C_COMPILER "g++")
add_definitions(-g -W -Wall -std=c++11 )
#add_definitions(-DDEBUG)


############source files############
set(SRC_LIST
capturer.cpp
encoder.cpp
face-wrapper.cpp
frame-buffer.cpp
frame-data.cpp
glogger.cpp
main.cpp
manager.cpp
mtndn-object.cpp
mtndn-utils.cpp
name-components.cpp
namespacer.cpp
publisher.cpp
utils.cpp
video-capturer.cpp
video-encoder.cpp
video-publisher.cpp
)

set(INCLUDE_LIST
capturer.h
common.h
encoder.h
face-wrapper.h
frame-buffer.h
frame-data.h
glogger.h
interfaces.h
manager.h
mtndn-defines.h
mtndn-object.h
mtndn-utils.h
name-components.h
namespacer.h
params.h
publisher.h
utils.h
video-capturer.h
video-encoder.h
video-publisher.h
)

#aux_source_directory(./ SRC_LIST)
#aux_source_directory(. SRC_LIST)
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
