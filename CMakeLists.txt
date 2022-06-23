cmake_minimum_required(VERSION 3.12)
add_definitions(-D__WIIU__ -D__wiiu__ -D__wut__)
project(wupnetworkinstaller LANGUAGES C CXX)

include_directories(${DEVKITPRO}/portlibs/ppc/include)
link_directories(${DEVKITPRO}/portlibs/ppc/lib)
include_directories(${DEVKITPRO}/portlibs/wiiu/include)
link_directories(${DEVKITPRO}/portlibs/wiiu/lib)
include_directories(${DEVKITPRO}/wut/include)
link_directories(${DEVKITPRO}/wut/lib)
include_directories(external/include)
link_directories(external/lib)

add_executable(wupnetworkinstaller
main.cpp
fatfs/diskio.c
fatfs/ff.c
fatfs/ffsystem.c
fatfs/ffunicode.c
)
set_target_properties(wupnetworkinstaller PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(wupnetworkinstaller PRIVATE "${CMAKE_SOURCE_DIR}/external/lib/libiosuhax.a")
target_compile_options(wupnetworkinstaller PRIVATE -Wall)
wut_create_rpx(wupnetworkinstaller)