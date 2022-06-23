cmake_minimum_required(VERSION 3.10)
project(wupnetworkinstaller LANGUAGES CXX)

include_directories(${DEVKITPRO}/portlibs/ppc/include)
link_directories(${DEVKITPRO}/portlibs/ppc/lib)
find_library(ZLIB NAMES z HINTS "${DEVKITPRO}/portlibs/ppc/lib")

add_executable(wupnetworkinstaller main.cpp)
set_target_properties(wupnetworkinstaller PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(wupnetworkinstaller PRIVATE ${ZLIB})
target_compile_options(wupnetworkinstaller PRIVATE -Wall)
wut_create_rpx(wupnetworkinstaller)