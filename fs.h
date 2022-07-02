#pragma once

#include <vector>
#include <string>
#include <coreinit/filesystem.h>

void flushVolume();
int mountWiiUDisk();
int mountExternalFat32Disk();
bool enumerateFatFsDirectory(const char *path, std::vector<std::string> *files, std::vector<std::string> *folders);
bool copyFolder(const char *src, const char *dst, void *buffer, size_t buf_size);
bool copyFile(const char *src, const char *dst, void *buffer, size_t buf_size);