#pragma once

#define USE_DEVOPTAB

#include <vector>
#include <string>
#include <coreinit/filesystem.h>

bool mountWiiUDisk();
#ifndef USE_DEVOPTAB
int mountExternalFat32Disk();
#endif
bool enumerateFatFsDirectory(const std::string &path, std::vector<std::string> *files, std::vector<std::string> *folders);
bool copyFolder(const std::string &src, const std::string &dst, void *buffer, size_t buf_size);
bool copyFile(const std::string &src, const std::string &dst, void *buffer, size_t buf_size);