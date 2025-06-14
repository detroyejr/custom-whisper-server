cmake_minimum_required(VERSION 3.30)
project (custom-whisper)

find_package(ggml)

add_executable(custom-whisper-server src/main.cc)
target_include_directories(custom-whisper-server PRIVATE src)
include_directories(src)
add_library(
  common-whisper-library
  src/common-whisper.cpp
  src/common-whisper.h
  src/common.cpp
  src/common.h
)

target_link_libraries(
  custom-whisper-server 
  PRIVATE whisper
  PRIVATE ggml::ggml
  PRIVATE common-whisper-library
)


# TODO: What cmake command will copy over every libggml-cpu-*.so file in
# <whisper/bin> path?
