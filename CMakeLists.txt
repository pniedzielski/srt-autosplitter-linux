cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

project(srt-autosplitter-linux)
add_executable(srt-autosplitter-linux
  autosplitter.cpp
  fake_timer.hpp
  pointer_path.hpp
  settings.hpp)
