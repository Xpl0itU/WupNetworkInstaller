cmake_minimum_required(VERSION 3.12)
add_definitions(-D__WIIU__ -D__wiiu__ -D__WUT__ -D__wut__)
project(wupnetworkinstaller LANGUAGES C CXX)

set(JSON_BuildTests OFF CACHE INTERNAL "")

include_directories(${DEVKITPRO}/portlibs/ppc/include)
link_directories(${DEVKITPRO}/portlibs/ppc/lib)
include_directories(${DEVKITPRO}/portlibs/wiiu/include)
link_directories(${DEVKITPRO}/portlibs/wiiu/lib)

add_subdirectory(nlohmann_json)
find_package(CURL REQUIRED)
find_library(LIBMOCHA mocha REQUIRED HINTS "${DEVKITPRO}/wut/usr/lib")

set(extusb_devoptab_sources
        extusb_devoptab.c
        extusb_fs_chdir.c
        extusb_fs_chmod.c
        extusb_fs_close.c
        extusb_fs_dirclose.c
        extusb_fs_dirnext.c
        extusb_fs_diropen.c
        extusb_fs_dirreset.c
        extusb_fs_fchmod.c
        extusb_fs_fstat.c
        extusb_fs_fsync.c
        extusb_fs_link.c
        extusb_fs_mkdir.c
        extusb_fs_open.c
        extusb_fs_read.c
        extusb_fs_rename.c
        extusb_fs_rmdir.c
        extusb_fs_seek.c
        extusb_fs_stat.c
        extusb_fs_statvfs.c
        extusb_fs_truncate.c
        extusb_fs_unlink.c
        extusb_fs_utils.c
        extusb_fs_utimes.c
        extusb_fs_write.c
        )
list(TRANSFORM extusb_devoptab_sources PREPEND "fatfs/extusb_devoptab/")

set(fatfs_sources
        diskio.c
        ff.c
        ffsystem.c
        ffunicode.c
        devices.c)
list(TRANSFORM fatfs_sources PREPEND "fatfs/")

add_executable(wupnetworkinstaller
        main.cpp
        fs.cpp
        ios_fs.c
        utils.cpp
        Installer.cpp
        net_receive.cpp
        ${fatfs_sources}
        ${extusb_devoptab_sources} Installer.cpp Installer.h)
set_target_properties(wupnetworkinstaller PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(wupnetworkinstaller PROPERTIES CXX_STANDARD_REQUIRED ON)
target_include_directories(wupnetworkinstaller PRIVATE "${DEVKITPRO}/wut/usr/include")
target_link_libraries(wupnetworkinstaller PRIVATE "${LIBMOCHA}" CURL::libcurl nlohmann_json::nlohmann_json)
target_compile_features(wupnetworkinstaller PRIVATE cxx_std_11)
target_compile_options(wupnetworkinstaller PRIVATE -Wall)
wut_create_rpx(wupnetworkinstaller)