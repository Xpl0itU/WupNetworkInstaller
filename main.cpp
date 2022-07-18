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

#include <nlohmann/json.hpp>
using json=nlohmann::json;

#define COPY_BUFFER_SZ (1024 * 1024)


void installCallback(MCPInstallProgress *progress) {
    printProgressBar(progress->sizeProgress, progress->sizeTotal);
}


static std::tuple<MCPError, std::string> runInstall(const std::string &tempPath) {
    auto installer = Installer::instance();
    return installer->install(MCP_INSTALL_TARGET_USB, tempPath, installCallback);
}


bool copyAndInstall(const std::string &srcPath, void *copyBuffer) {
    std::string tempPath = "storage_usb:/usr/tmp/";
    tempPath.append(srcPath);
    WHBLogPrintf("Copying %s to temp storage", srcPath.c_str());
    if (copyFolder(srcPath, tempPath, copyBuffer, COPY_BUFFER_SZ)) {
        WHBLogPrintf("Installing %s", srcPath.c_str());
        WHBLogPrintf("Press HOME to cancel");
        WHBLogConsoleDraw();
        std::future<std::tuple<MCPError, std::string>> fut = std::async(&runInstall, tempPath);
        auto installer = Installer::instance();
        while (installer->processing) {
            uint32_t keys = getKey();
            if (keys & VPAD_BUTTON_HOME) {
                installer->cancel();
            }
        }
        std::tuple<MCPError, std::string> retval = fut.get();
        auto err = static_cast<uint32_t>(std::get<0>(retval));
        auto str = std::get<1>(retval);
        WHBLogPrintf("(%d) %s", err, str.c_str());
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

    WHBProcInit();
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

    //json object = {{"one", 1}, {"two", 2}};
    //WHBLogPrint(object.dump().c_str());
    //WHBLogConsoleDraw();

    //WHBLogPrint("Start main application");
    //WHBLogConsoleDraw();

    initFs();
    returncode = init_extusb_devoptab();
    if (returncode != 0) {
        WHBLogPrintf("Initializing devoptab failed %d!", returncode);
        WHBLogConsoleDraw();
        goto cleanup;
    }
    WHBLogPrint("Devoptab initialized, mounting wiiu disk");
    WHBLogConsoleDraw();
    if (!mountWiiUDisk()) {
        WHBLogPrintf("Mounting wiiu disk failed!");
        WHBLogConsoleDraw();
        goto cleanup;
    }

    /*
    if (!enumerateFatFsDirectory("extusb:/", &files, &folders)) {
        WHBLogPrint("Enumerating disk failed!");
        WHBLogConsoleDraw();
        goto cleanup;
    }
     */
    folders.emplace_back("WUP-N-R3ME_0005000252334D45");

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
    fini_extusb_devoptab();
    unmountWiiUDisk();
    Mocha_DeinitLibrary();

    free(copyBuffer);

    WHBLogPrint("Press any key to exit");
    WHBLogConsoleDraw();
    waitForKey();

    WHBLogConsoleFree();
    WHBLogUdpDeinit();
    WHBProcShutdown();
    return returncode;
}
