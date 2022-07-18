#include <vector>
#include <string>
#include <malloc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <coreinit/filesystem.h>
#include <nn/spm/storage.h>
#include "fs.h"
#include "utils.h"
#include "fatfs/devices.h"

#include <sys/dirent.h>
#include <mocha/mocha.h>

static bool usbMounted = false;


static MochaUtilsStatus tryMountUsb01() {
    return Mocha_MountFS("storage_usb", USB_EXT1_PATH, "/vol/storage_installer");
}


static MochaUtilsStatus tryMountUsb02() {
    return Mocha_MountFS("storage_usb", USB_EXT2_PATH, "/vol/storage_installer");
}


bool mountWiiUDisk() {
    if(usbMounted)
        return true;

    const char *fatDiskPath = get_fat_usb_path();
    MochaUtilsStatus (*wiiUDiskMountFuncPtr[2])() = {nullptr, nullptr};
    if (fatDiskPath == nullptr) {
        wiiUDiskMountFuncPtr[0] = tryMountUsb01;
        wiiUDiskMountFuncPtr[1] = tryMountUsb02;
    } else if (strcmp(fatDiskPath, USB_EXT1_PATH) == 0) {
        wiiUDiskMountFuncPtr[0] = tryMountUsb02;
    } else if (strcmp(fatDiskPath, USB_EXT2_PATH) == 0) {
        wiiUDiskMountFuncPtr[0] = tryMountUsb01;
    } else {
        wiiUDiskMountFuncPtr[0] = tryMountUsb01;
        wiiUDiskMountFuncPtr[1] = tryMountUsb02;
    }

    MochaUtilsStatus ret;

    for (auto & mountFunc : wiiUDiskMountFuncPtr) {
        if (mountFunc == nullptr) break;
        ret = mountFunc();
    }

    if(ret == 0)
    {
        WHBLogPrintf("Mounting successful");
        WHBLogConsoleDraw();
        usbMounted = true;
        return true;
    } else {
        WHBLogPrintf("Mounting unsuccessful: %d", ret);
        WHBLogConsoleDraw();
    }

    usbMounted = false;
    return false;
}


bool unmountWiiUDisk() {
    MochaUtilsStatus status = Mocha_UnmountFS("storage_usb");
    WHBLogPrintf("Unmounting result: %d", status);
    WHBLogConsoleDraw();
    return status == MOCHA_RESULT_SUCCESS;
}


bool enumerateFatFsDirectory(const std::string &path, std::vector<std::string> *files, std::vector<std::string> *folders) {
    if (files == nullptr || folders == nullptr) return false;
    files->clear();
    folders->clear();

    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }

    errno = 0;
    struct dirent *entry;
    while ((entry = readdir (dir)) != nullptr) {
        switch (entry->d_type) {
            case DT_DIR:
                folders->push_back(std::string(entry->d_name));
                break;
            case DT_REG:
                files->push_back(std::string(entry->d_name));
                break;
            default:
                WHBLogPrintf("weird file: %s %c", entry->d_name, entry->d_type);
                return false;
        }
    }
    if (errno != 0) {
        WHBLogPrintf("readdir fail: %d", errno);
        return false;
    } else {
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
    std::vector<std::string> files;
    std::vector<std::string> folders;

    WHBLogPrintf("Copying to %s", dst.c_str());
    WHBLogConsoleDraw();
    DIR *dir = opendir(src.c_str());
    if (dir == nullptr) {
        WHBLogPrintf("Opendir failed %d", errno);
        WHBLogConsoleDraw();
        return false;
    }

    int rc = mkdir(dst.c_str(), 777);
    if (rc < 0 && errno != EEXIST) {
        WHBLogPrintf("mkdir failed %d", errno);
        WHBLogConsoleDraw();
        return false;
    }
    if (!enumerateFatFsDirectory(src, &files, &folders)) {
        WHBLogPrintf("Enumerating failed");
        WHBLogConsoleDraw();
        return false;
    }

    for (auto const& fname : files) {
        std::string fullSrc = makeAbsolutePath(src, fname);
        std::string fullDst = makeAbsolutePath(dst, fname);
        if (!copyFile(fullSrc, fullDst, buffer, buf_size)) {
            WHBLogPrintf("copyFile failed");
            WHBLogConsoleDraw();
            return false;
        }
    }

    for (auto const& fname : folders) {
        std::string fullSrc = makeAbsolutePath(src, fname);
        std::string fullDst = makeAbsolutePath(dst, fname);
        if (!copyFolder(fullSrc, fullDst, buffer, buf_size)) {
            WHBLogPrintf("copyFolder failed");
            WHBLogConsoleDraw();
            return false;
        }
    }

    return true;
}

bool copyFile(const std::string &src, const std::string &dst, void *buffer, size_t buf_size) {
    WHBLogPrintf("Copying file %s -> %s", src.c_str(), dst.c_str());
    WHBLogConsoleDraw();

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

    off_t total_bytes_written = 0;
    OSTime time = OSGetSystemTime();
    while (total_bytes_written < finfo.st_size) {
        uint bytes_read = 0;
        if ((bytes_read = fread(buffer, 1, buf_size, srcFile)) == 0) {
            if (ferror(srcFile) != 0) {
                WHBLogPrintf("ferror encountered: %d", errno);
                return false;
            }
        }

        uint bytes_written = 0;
        while (bytes_written < bytes_read) {
            size_t write_count = fwrite(buffer, 1, bytes_read, dstFile);
            if (write_count < bytes_read) {
                WHBLogPrintf("fwrite error %d", errno);
                return false;
            }
            bytes_written += write_count;
            total_bytes_written += write_count;

            OSTime now = OSGetSystemTime();
            if(time > 0 && OSTicksToMilliseconds(now - time) >= 1000) {
                time = now;
                printProgressBar(total_bytes_written, finfo.st_size);
            }
        }
    }

    return true;
}
