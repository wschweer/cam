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

set (CMAKE_AUTOMOC TRUE)

add_library(mjpeg STATIC
      mjpeg.cpp
      mjpeg.h
      )

set_target_properties(mjpeg PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
target_link_libraries(mjpeg Qt5::Gui)

QT5_WRAP_UI (cam_ui camview.ui)

add_executable(cam
      ${cam_ui}
      main.cpp
      camera.cpp
      camview.cpp
      camview.h
      )

target_link_libraries(cam
      Qt5::Widgets
      mjpeg
      pthread
      -Wl,-rpath,/usr/local/lib
      -L/usr/local/lib
      avcodec
      avutil
      swscale
      )

