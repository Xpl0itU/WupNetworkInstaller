cmake_minimum_required(VERSION 3.12)
add_definitions(-D__WIIU__ -D__wiiu__ -D__WUT__ -D__wut__)
project(wupnetworkinstaller LANGUAGES C CXX)

set(JSON_BuildTests OFF CACHE INTERNAL "")

include_directories(${DEVKITPRO}/portlibs/ppc/include)
link_directories(${DEVKITPRO}/portlibs/ppc/lib)
include_directories(${DEVKITPRO}/portlibs/wiiu/include)
link_directories(${DEVKITPRO}/portlibs/wiiu/lib)

add_subdirectory(nlohmann_json)
find_package( CURL REQUIRED )
find_library( LIBIOSUHAX iosuhax REQUIRED HINTS "${DEVKITPRO}/wut/usr/lib" )

add_executable(wupnetworkinstaller
main.cpp
fs.cpp
ios_fs.c
utils.cpp
fatfs/diskio.c
fatfs/ff.c
fatfs/ffsystem.c
fatfs/ffunicode.c
)
set_target_properties(wupnetworkinstaller PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(wupnetworkinstaller PROPERTIES CXX_STANDARD_REQUIRED ON)
target_include_directories(wupnetworkinstaller PRIVATE "${DEVKITPRO}/wut/usr/include")
target_link_libraries(wupnetworkinstaller PRIVATE "${LIBIOSUHAX}" CURL::libcurl nlohmann_json::nlohmann_json)
target_compile_features(wupnetworkinstaller PRIVATE cxx_std_11)
target_compile_options(wupnetworkinstaller PRIVATE -Wall)
wut_create_rpx(wupnetworkinstaller)