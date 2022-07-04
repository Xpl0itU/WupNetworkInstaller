#include <vector>
#include <string>
#include <cstring>
#include <malloc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <coreinit/filesystem.h>
#include <stdlib.h>
#include "fatfs/ff.h"
#include "fs.h"
#include "utils.h"
#include "ios_fs.h"
#include "mocha/fsa.h"


static FATFS fs;


static int translate_error(FSStatus error)
{
   switch ((int32_t)error) {
   case FS_STATUS_END:
      return ENOENT;
   case FS_STATUS_CANCELLED:
      return EINVAL;
   case FS_STATUS_EXISTS:
      return EEXIST;
   case FS_STATUS_MEDIA_ERROR:
      return EIO;
   case FS_STATUS_NOT_FOUND:
      return ENOENT;
   case FS_STATUS_PERMISSION_ERROR:
      return EPERM;
   case FS_STATUS_STORAGE_FULL:
      return ENOSPC;
   case FS_ERROR_ALREADY_EXISTS:
      return EEXIST;
   case FS_ERROR_BUSY:
      return EBUSY;
   case FS_ERROR_CANCELLED:
      return ECANCELED;
   case FS_STATUS_FILE_TOO_BIG:
      return EFBIG;
   case FS_ERROR_INVALID_PATH:
      return ENAMETOOLONG;
   case FS_ERROR_NOT_DIR:
      return ENOTDIR;
   case FS_ERROR_NOT_FILE:
      return EISDIR;
   case FS_ERROR_OUT_OF_RANGE:
      return ESPIPE;
   case FS_ERROR_UNSUPPORTED_COMMAND:
      return ENOTSUP;
   case FS_ERROR_WRITE_PROTECTED:
      return EROFS;
   default:
      return (int)error;
   }
}


int mountExternalFat32Disk() {
    // This actually mounts SD for testing, change "0" to "1" to mount usb
    int res = f_mount(&fs, "0", 1);
    return res;
}


bool mountWiiUDisk() {
    // TODO: Use FSA_Mount() etc to mount, since mount_fs doesn't exist in libmocha anymore
    /*
    if (mount_fs("storage_usb", fsaFdWiiU, NULL, "/vol/storage_usb01") < 0) {
        WHBLogPrint("Mount failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }
    if (mount_fs("sd", fsaFdSd, NULL, "/vol/storage_external01") < 0) {
        WHBLogPrint("Mount failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }
    */
    return true;
}


bool enumerateFatFsDirectory(const char *path, std::vector<std::string> *files, std::vector<std::string> *folders) {
    if (files == nullptr || folders == nullptr) {
        WHBLogPrint("vectors null");
        return false;
    }

    DIR dir;
    if (f_opendir(&dir, path) != FR_OK) {
        WHBLogPrint("fopendir fail");
        return false;
    } else {
        std::string base_path = path;
        FILINFO fi;
        while (true) {
            if (f_readdir(&dir, &fi) != FR_OK) {
                WHBLogPrint("freaddir fail");
                return false;
            }
            if (fi.fname[0] == 0) break;

            std::string name = fi.fname;
            if (fi.fattrib & AM_DIR) {
                folders->push_back(name);
            } else {
                files->push_back(name);
            }
        }
        return true;
    }
}

bool copyFolder(const char *src, const char *dst, void *buffer, size_t buf_size) {
    FSClient *fsClient = initFs();

    WHBLogPrintf("Copying folder %s -> %s", src, dst);
    WHBLogConsoleDraw();

    FILINFO fi;
    if (f_stat(src, &fi) != FR_OK) {
        return false;
    }

    DIR dir;
    if (f_opendir(&dir, src) != FR_OK) {
        return false;
    }

    std::vector<std::string> files;
    std::vector<std::string> folders;

    if (!enumerateFatFsDirectory(src, &files, &folders)) {
        return false;
    }
    
    FSStatus status;
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);
    status = FSMakeDir(fsClient, &cmd, dst, FS_ERROR_FLAG_ALL);
    if (status < 0) {
        int err = translate_error(status);
        if (err != EEXIST) return false;
    }

    for (auto const& fname : files) {
        std::string full_src_path = src;
        full_src_path.push_back('/');
        full_src_path += fname;
        std::string full_dst_path = dst;
        full_dst_path.push_back('/');
        full_dst_path += fname;
        if (!copyFile(full_src_path.c_str(), full_dst_path.c_str(), buffer, buf_size)) {
            return false;
        }
    }

    for (auto const& fname : folders) {
        std::string full_src_path = src;
        full_src_path.push_back('/');
        full_src_path += fname;
        std::string full_dst_path = dst;
        full_dst_path.push_back('/');
        full_dst_path += fname;
        if (!copyFolder(full_src_path.c_str(), full_dst_path.c_str(), buffer, buf_size)) {
            return false;
        }
    }

    return true;
}

bool copyFile(const char *src, const char *dst, void *buffer, size_t buf_size) {
    FSClient *fsClient = initFs();
    
    WHBLogPrintf("Copying file %s -> %s", src, dst);
    WHBLogConsoleDraw();
    FIL fp;
    FILINFO fi;
    if (f_stat(src, &fi) != FR_OK) {
        return false;
    }

    if (f_open(&fp, src, FA_READ) != FR_OK) {
        return false;
    }

    FSFileHandle handle;
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);
    int result = FSOpenFile(fsClient, &cmd, dst, "w", &handle, FS_ERROR_FLAG_ALL);
    if (result < 0) {
        WHBLogPrintf("%s: FSOpenFile error %d", __FUNCTION__, result);
        return false;
    }

    uint64_t bytes_copied = 0;
    while (bytes_copied < fi.fsize) {
        uint bytes_read = 0;
        if (f_read(&fp, buffer, buf_size, &bytes_read) != FR_OK) {
            return false;
        }

        uint bytes_written = 0;
        while (bytes_written < bytes_read) {
            FSInitCmdBlock(&cmd);

            // FSStatus FSWriteFile( FSClient *client, FSCmdBlock *block, const void *source, FSSize size, FSCount count, FSFileHandle fileHandle, FSFlag flag, FSRetFlag errHandling );
            FSStatus result = FSWriteFile(fsClient, &cmd, (uint8_t*) buffer, 1, bytes_read, (FSFileHandle)handle, 0, FS_ERROR_FLAG_ALL);
            if (result < 0) {
                WHBLogPrintf("%s: FSWriteFile error %d", __FUNCTION__, result);
                return 0;
            }
            bytes_written += result;
            bytes_copied += result;
            printProgressBar(bytes_copied, fi.fsize);
        }
    }

    return true;
}
