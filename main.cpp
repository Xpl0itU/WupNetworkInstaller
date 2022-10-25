#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>

#include <whb/proc.h>
#include <coreinit/mcp.h>
#include <mocha/mocha.h>
#include <thread>
#include <future>
#include <malloc.h>
#include "utils.h"
#include "fs.h"
#include "ios_fs.h"
#include "Installer.h"
#include "fatfs/extusb_devoptab/extusb_devoptab.h"

#include "state.h"

#define COPY_BUFFER_SZ (1024 * 1024)

void installCallback(MCPInstallProgress *progress, OSTime elapsed) {
    printProgressBarWithSpeed(progress->sizeProgress, progress->sizeTotal, elapsed);
}


static std::tuple<MCPError, std::string> runInstall(const std::string &tempPath) {
    auto installer = Installer::instance();
    return installer->install(MCP_INSTALL_TARGET_USB, tempPath, installCallback);
}


bool copyAndInstall(const std::string &srcPath, void *copyBuffer) {
    std::string nativePath = "/vol/storage_installer/install/";
    std::string tempPath = "fs:" + nativePath;
    int rc = mkdir(tempPath.c_str(), 0777);
    if (rc < 0) {
        if (errno != EEXIST) {
            WHBLogPrintf("mkdir failed %d", errno);
            WHBLogConsoleDraw();
            return false;
        } else {
            rc = chmod(tempPath.c_str(), 0777);
            if (rc < 0) {
                WHBLogPrintf("chmod failed %d", errno);
                WHBLogConsoleDraw();
                return false;
            }
        }
    }
    tempPath.append(srcPath);
    nativePath.append(srcPath);
    WHBLogPrintf("Copying %s to temp storage", srcPath.c_str());
    if (copyFolder(srcPath, tempPath, copyBuffer, COPY_BUFFER_SZ)) {
        WHBLogPrintf("Installing %s", srcPath.c_str());
        WHBLogPrintf("Press HOME to cancel");
        WHBLogConsoleDraw();
        std::future<std::tuple<MCPError, std::string>> fut = std::async(&runInstall, nativePath);
        auto installer = Installer::instance();
        while (!installer->processing) {}
        OSTime startTime = OSGetSystemTime();
        while (installer->processing) {
            uint32_t keys = getKey();
            if (keys & VPAD_BUTTON_HOME) {
                installer->cancel();
            }
        }
        std::tuple<MCPError, std::string> retval = fut.get();
        OSTime endTime = OSGetSystemTime();
        auto err = static_cast<uint32_t>(std::get<0>(retval));
        auto str = std::get<1>(retval);
        WHBLogPrintf("(%d) %s", err, str.c_str());
        WHBLogPrintf("Took %f seconds", OSTicksToMilliseconds(endTime - startTime) / 1000.0);
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
    uint32_t keys;
    std::vector<std::string> files;
    std::vector<std::string> folders;

    WHBProcInit();
    initState();
    WHBLogConsoleInit();
    WHBLogUdpInit();
    WHBLogConsoleSetColor(0);
    Mocha_InitLibrary();

    WHBLogPrint("Initialize heap memory");
    WHBLogConsoleDraw();

    void *copyBuffer = memalign(0x40, COPY_BUFFER_SZ);
    if (copyBuffer == nullptr) {
        WHBLogPrint("Memory allocation failed!");
        WHBLogConsoleDraw();
        returncode = -1;
        goto cleanup;
    }

    WHBLogPrint("Heap memory initialized");
    WHBLogConsoleDraw();

    initFs();
    returncode = init_extusb_devoptab();
    if (returncode != 0) {
        WHBLogPrintf("Initializing devoptab failed %d!", returncode);
        WHBLogConsoleDraw();
        goto cleanup;
    }
    WHBLogPrint("Devoptab initialized, mounting wiiu disk");
    WHBLogConsoleDraw();
    /*
    if (!mountWiiUDisk()) {
        WHBLogPrintf("Mounting wiiu disk failed!");
        WHBLogConsoleDraw();
        goto cleanup;
    }*/

    if (!enumerateFatFsDirectory("sd:/", &files, &folders)) {
        WHBLogPrint("Enumerating disk failed!");
        WHBLogConsoleDraw();
        goto cleanup;
    }

    WHBLogPrint("Installing the following titles:");
    WHBLogConsoleDraw();
    for (const auto &f : folders) {
        WHBLogPrintf("> %s", f.c_str());
        WHBLogConsoleDraw();
    }

cleanup:
    fini_extusb_devoptab();
    unmountWiiUDisk();
    cleanupFs();
    Mocha_DeInitLibrary();

    free(copyBuffer);

    WHBLogPrint("Press any key to exit");
    WHBLogConsoleDraw();
    waitForKey();

    WHBLogUdpDeinit();
    shutdownState();
    WHBProcShutdown();
    return returncode;
}
