#include <vector>
#include <string>
#include <malloc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <coreinit/filesystem.h>
#include <nn/spm/storage.h>
#include "fs.h"
#include "utils.h"
#include "ios_fs.h"

#ifdef USE_DEVOPTAB
#include <sys/dirent.h>
#include <coreinit/filesystem_fsa.h>

#else
#include "fatfs/ff.h"
static FATFS fs;
#endif

static bool usbMounted = false;


#ifndef USE_DEVOPTAB
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
#endif


bool mountWiiUDisk() {
    if(usbMounted)
        return true;

    FSClient *fsClient = initFs();
    if (fsClient == nullptr) return false;

    nn::spm::VolumeId volumeId{};
    GetDefaultExtendedStorageVolumeId(&volumeId);
    WHBLogPrintf("Default ext storage id: %s", volumeId.id);

    FSError ret = FSAMount(FSGetClientBody(fsClient)->clientHandle, "/dev/usb01", "/vol/storage_installer/usb", FSA_MOUNT_FLAG_GLOBAL_MOUNT, nullptr, 0);

    if(ret != 0)
    {
        WHBLogPrintf("Error mounting /dev/usb01: %#010x", ret);
        return false;
    }

    usbMounted = true;
    return true;
}


bool unmountWiiUDisk() {
    if (!usbMounted) return true;

    FSClient *fsClient = initFs();
    if (fsClient == nullptr) return false;

    FSError ret = FSAUnmount(FSGetClientBody(fsClient)->clientHandle, "/vol/storage_installer/usb", FSA_UNMOUNT_FLAG_BIND_MOUNT);

    if(ret != 0)
    {
        WHBLogPrintf("Error unmounting /dev/usb01: %#010x", ret);
        return false;
    }

    usbMounted = false;
    return true;
}


bool enumerateFatFsDirectory(const std::string &path, std::vector<std::string> *files, std::vector<std::string> *folders) {
    if (files == nullptr || folders == nullptr) {
        WHBLogPrint("vectors null");
        return false;
    }

#ifdef USE_DEVOPTAB
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        WHBLogPrintf("opendir fail");
        return false;
    }

    errno = 0;
    while (true) {
        struct dirent *entry = readdir(dir);
        if (entry == nullptr) {
            if (errno != 0) {
                WHBLogPrintf("readdir fail");
                return false;
            } else {
                return true;
            }
        }

        switch (entry->d_type) {
            case DT_DIR:
                folders->push_back(std::string(entry->d_name));
                break;
            case DT_REG:
                files->push_back(std::string(entry->d_name));
                break;
            default:
                WHBLogPrintf("weird file: %s %c", entry->d_name, entry->d_type);
                break;
        }
    }
    return true;
#else
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
#endif
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
    std::vector<std::string> files;
    std::vector<std::string> folders;

    WHBLogPrintf("Copying folder %s -> %s", src.c_str(), dst.c_str());
    WHBLogConsoleDraw();
#ifdef USE_DEVOPTAB
    DIR *dir = opendir(src.c_str());
    if (dir == nullptr) {
        return false;
    }

    int rc = mkdir(dst.c_str(), 777);
    if (rc < 0 && errno != EEXIST) {
        return false;
    }
#else
    FSClient *fsClient = initFs();

    FILINFO fi;
    if (f_stat(src.c_str(), &fi) != FR_OK) {
        return false;
    }

    DIR dir;
    if (f_opendir(&dir, src.c_str()) != FR_OK) {
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
#endif
    if (!enumerateFatFsDirectory(src, &files, &folders)) {
        return false;
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
    WHBLogPrintf("Copying file %s -> %s", src.c_str(), dst.c_str());
    WHBLogConsoleDraw();

    size_t fsize = 0;

#ifdef USE_DEVOPTAB
    struct stat finfo{};
    int rc = stat(src.c_str(), &finfo);
    if (rc < 0) {
        WHBLogPrintf("stat %s failed: %d", src.c_str(), errno);
        return false;
    }

    FILE *srcFile = fopen(src.c_str(), "r");
    if (srcFile == nullptr) {
        WHBLogPrintf("fopen src %s failed: %d", src.c_str(), errno);
        return false;
    }

    FILE *dstFile = fopen(dst.c_str(), "w");
    if (dstFile == nullptr) {
        WHBLogPrintf("fopen dst %s failed: %d", src.c_str(), errno);
        return false;
    }
#else
    FSClient *fsClient = initFs();

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
    fsize = fi.fsize;
#endif

    uint64_t total_bytes_written = 0;
    while (total_bytes_written < fsize) {
        uint bytes_read = 0;
#ifdef USE_DEVOPTAB
        if ((bytes_read = fread(buffer, 1, 32768, srcFile)) == 0) {
            if (ferror(srcFile) != 0) {
                return false;
            }
        }
#else
        if (f_read(&fp, buffer, buf_size, &bytes_read) != FR_OK) {
            return false;
        }
#endif

        uint bytes_written = 0;
        while (bytes_written < bytes_read) {
#ifdef USE_DEVOPTAB
            size_t write_count = fwrite(buffer, 1, bytes_read, srcFile);
            if (write_count <= 0) {
                WHBLogPrintf("fwrite error");
                return false;
            }
#else
            FSInitCmdBlock(&cmd);

            // FSStatus FSWriteFile( FSClient *client, FSCmdBlock *block, const void *source, FSSize size, FSCount count, FSFileHandle fileHandle, FSFlag flag, FSRetFlag errHandling );
            FSStatus write_count = FSWriteFile(fsClient, &cmd, (uint8_t*) buffer, 1, bytes_read, (FSFileHandle)handle, 0, FS_ERROR_FLAG_ALL);
            if (write_count < 0) {
                WHBLogPrintf("%s: FSWriteFile error %d", __FUNCTION__, write_count);
                return false;
            }
#endif
            bytes_written += write_count;
            total_bytes_written += write_count;
            printProgressBar(total_bytes_written, fsize);
        }
    }

    return true;
}
