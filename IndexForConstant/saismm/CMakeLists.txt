PROJECT(saismm-dev)

#cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -O3")

#build saism
ADD_SUBDIRECTORY(src)
