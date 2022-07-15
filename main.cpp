#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>

#include <whb/proc.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/mcp.h>
#include <mocha/mocha.h>
#include <thread>
#include <future>
#include "utils.h"
#include "fs.h"
#include "ios_fs.h"
#include "Installer.h"
#include "fatfs/extusb_devoptab/extusb_devoptab.h"

#include <nlohmann/json.hpp>
using json=nlohmann::json;

#define COPY_BUFFER_SZ (1024 * 1024 * 8)


void installCallback(MCPInstallProgress *progress) {
    printProgressBar(progress->sizeProgress, progress->sizeTotal);
}


static MCPError runInstall(const std::string &tempPath) {
    auto installer = Installer::instance();
    return installer->install(MCP_INSTALL_TARGET_USB, tempPath, installCallback);
}


bool copyAndInstall(const std::string &srcPath, void *copyBuffer) {
    std::string tempPath = "/vol/storage_usb02/usr/tmp/";
    tempPath.append(srcPath);
    WHBLogPrintf("Copying %s to temp storage", srcPath.c_str());
    if (copyFolder(srcPath, tempPath, copyBuffer, COPY_BUFFER_SZ)) {
        WHBLogPrintf("Installing %s", srcPath.c_str());
        WHBLogPrintf("Press HOME to cancel");
        WHBLogConsoleDraw();
        std::future<MCPError> fut = std::async(&runInstall, tempPath);
        auto installer = Installer::instance();
        while (installer->processing) {
            uint32_t keys = getKey();
            if (keys & VPAD_BUTTON_HOME) {
                installer->cancel();
            }
        }
        auto err = static_cast<uint32_t>(fut.get());
        if (err == 0xDEAD0002) {
            WHBLogPrintf("Installation cancelled");
        } else if (err != 0) {
            WHBLogPrintf("Installation failed with %08x", err);
        } else {
            WHBLogPrintf("Installation succeeded");
        }
        WHBLogConsoleDraw();
        return err != 0;
    } else {
        WHBLogPrintf("Copy operation failed");
        WHBLogConsoleDraw();
        return false;
    }
}


int main(int argc, char** argv)
{
    int returncode = 0;
    std::vector<std::string> files;
    std::vector<std::string> folders;
    FRESULT fr;

    WHBProcInit();
    WHBLogConsoleInit();
    WHBLogUdpInit();
    WHBLogConsoleSetColor(0);
    Mocha_InitLibrary();

    WHBLogPrint("Initialize heap memory");
    WHBLogConsoleDraw();

    void *copyBuffer = MEMAllocFromDefaultHeapEx(COPY_BUFFER_SZ, 64);
    if (copyBuffer == nullptr) {
        WHBLogPrint("Memory allocation failed! Press any key to exit");
        WHBLogConsoleDraw();
        waitForKey();
        returncode = -1;
        goto cleanup;
    }

    //json object = {{"one", 1}, {"two", 2}};
    //WHBLogPrint(object.dump().c_str());
    //WHBLogConsoleDraw();

    //WHBLogPrint("Start main application");
    //WHBLogConsoleDraw();

#ifdef USE_DEVOPTAB
    fr = init_extusb_devoptab();
    if (fr != FR_OK) {
        WHBLogPrintf("Initializing devoptab failed %d!", returncode);
        WHBLogConsoleDraw();
        waitForKey();
        goto cleanup;
    }
    mountWiiUDisk();
#else
    initFs();
    returncode = mountExternalFat32Disk();
    if (returncode != 0) {
        WHBLogPrintf("Mounting disk failed %d! Press any key to exit", returncode);
        WHBLogConsoleDraw();
        waitForKey();
        goto cleanup;
    }
#endif

    if (!enumerateFatFsDirectory("install", &files, &folders)) {
        WHBLogPrint("Enumerating disk failed!");
        WHBLogConsoleDraw();
        waitForKey();
        goto cleanup;
    }

    WHBLogPrint("Installing the following titles:");
    WHBLogConsoleDraw();
    for (const auto &f : folders) {
        WHBLogPrintf("> %s", f.c_str());
        WHBLogConsoleDraw();
    }
    WHBLogPrint("Press any key to install, press HOME to exit");
    WHBLogConsoleDraw();
    if(waitForKey() & VPAD_BUTTON_HOME) {
        goto cleanup;
    }

    for (const auto &f : folders) {
        copyAndInstall(f, copyBuffer);
    }

    cleanup:
#ifdef USE_DEVOPTAB
    cleanupFs();
    fini_extusb_devoptab();
    unmountWiiUDisk();
#else
    fini_extusb_devoptab();
#endif

    MEMFreeToDefaultHeap(copyBuffer);

    WHBLogConsoleFree();
    WHBLogUdpDeinit();
    WHBProcShutdown();
    return returncode;
}
