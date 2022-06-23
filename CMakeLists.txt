cmake_minimum_required(VERSION 3.2)
project(wupnetworkinstaller LANGUAGES CXX)

find_library(ZLIB zlib)
find_library(UTILS utils)
find_library(DYNLIBS dynamiclibs)

add_executable(wupnetworkinstaller entry.c)
target_link_libraries(wupnetworkinstaller PRIVATE "${ZLIB}" "${UTILS}" "${DYNLIBS}")
target_compile_options(wupnetworkinstaller PRIVATE -Wall)
wut_create_rpx(wupnetworkinstaller)