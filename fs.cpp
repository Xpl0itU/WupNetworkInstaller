#include <vector>
#include <string>
#include <malloc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <coreinit/filesystem.h>
#include "fatfs/ff.h"
#include "fs.h"
#include "utils.h"
#include "ios_fs.h"
#include "fatfs/devices.h"


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
    char mountPath[0x40];
    sprintf(mountPath, "%d:", DEV_USB_EXT);
    FRESULT fr = f_mount(&fs, mountPath, 1);
    if (fr != FR_OK) return fr;
    fr = f_chdrive(mountPath);
    return fr;
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
    //FSA_Mount(fd, device_path, volume_path, flags, arg_string, arg_string_len);
    return true;
}


bool enumerateFatFsDirectory(const std::string &path, std::vector<std::string> *files, std::vector<std::string> *folders) {
    if (files == nullptr || folders == nullptr) {
        WHBLogPrint("vectors null");
        return false;
    }

    DIR dir;
    if (f_opendir(&dir, path.c_str()) != FR_OK) {
        WHBLogPrint("fopendir fail");
        return false;
    } else {
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

std::string makeAbsolutePath(const std::string &base, const std::string &rel) {
    std::string out(base);
    if (out[out.length() - 1] != '/') {
        out += '/';
    }
    out += rel;
    return out;
}

bool copyFolder(const std::string &src, const std::string &dst, void *buffer, size_t buf_size) {
    FSClient *fsClient = initFs();

    WHBLogPrintf("Copying folder %s -> %s", src.c_str(), dst.c_str());
    WHBLogConsoleDraw();

    FILINFO fi;
    if (f_stat(src.c_str(), &fi) != FR_OK) {
        return false;
    }

    DIR dir;
    if (f_opendir(&dir, src.c_str()) != FR_OK) {
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
    status = FSMakeDir(fsClient, &cmd, dst.c_str(), FS_ERROR_FLAG_ALL);
    if (status < 0) {
        int err = translate_error(status);
        if (err != EEXIST) return false;
    }

    for (auto const& fname : files) {
        std::string fullSrc = makeAbsolutePath(src, fname);
        std::string fullDst = makeAbsolutePath(dst, fname);
        if (!copyFile(fullSrc, fullDst, buffer, buf_size)) {
            return false;
        }
    }

    for (auto const& fname : folders) {
        std::string fullSrc = makeAbsolutePath(src, fname);
        std::string fullDst = makeAbsolutePath(dst, fname);
        if (!copyFolder(fullSrc, fullDst, buffer, buf_size)) {
            return false;
        }
    }

    return true;
}

bool copyFile(const std::string &src, const std::string &dst, void *buffer, size_t buf_size) {
    FSClient *fsClient = initFs();
    
    WHBLogPrintf("Copying file %s -> %s", src.c_str(), dst.c_str());
    WHBLogConsoleDraw();
    FIL fp;
    FILINFO fi;
    if (f_stat(src.c_str(), &fi) != FR_OK) {
        return false;
    }

    if (f_open(&fp, src.c_str(), FA_READ) != FR_OK) {
        return false;
    }

    FSFileHandle handle;
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);
    int result = FSOpenFile(fsClient, &cmd, dst.c_str(), "w", &handle, FS_ERROR_FLAG_ALL);
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
            FSStatus fss = FSWriteFile(fsClient, &cmd, (uint8_t*) buffer, 1, bytes_read, (FSFileHandle)handle, 0, FS_ERROR_FLAG_ALL);
            if (fss < 0) {
                WHBLogPrintf("%s: FSWriteFile error %d", __FUNCTION__, fss);
                return false;
            }
            bytes_written += fss;
            bytes_copied += fss;
            printProgressBar(bytes_copied, fi.fsize);
        }
    }

    return true;
}
