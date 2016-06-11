##=============================================================================
##  Cam
##  Linux Webcam App
##
##  Copyright (C) 2016 Werner Schweer
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License version 2
##  as published by the Free Software Foundation and appearing in
##  the file LICENCE.GPL
##=============================================================================

project(cam)
# set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_definitions(-Wall -Wextra -std=c++11 -g)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${PROJECT_BINARY_DIR}/momo)
include_directories(${PROJECT_SOURCE_DIR}/momo)

find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5WebSockets)

set (CMAKE_AUTOMOC TRUE)

add_library(mjpeg STATIC
      mjpeg.cpp
      mjpeg.h
      )

 set_target_properties(mjpeg PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
target_link_libraries(mjpeg Qt5::Gui)

add_executable(cam
      main.cpp
      cam.cpp
      )

target_link_libraries(cam Qt5::Widgets Qt5::Network Qt5::WebSockets mjpeg pthread -Wl,-rpath,/usr/local/lib -L/usr/local/lib avcodec avutil swscale)

