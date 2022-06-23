//#include <string>
//#include "Application.h"
//#include "common/common.h"
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <zlib.h>

#include <vpad/input.h>
#include <whb/log.h>
#include "log_console.h"
#include "Application.h"

#include <whb/proc.h>

int main(int argc, char** argv)
{
    WHBProcInit();

    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    WHBLogConsoleInit();
    WHBLogUdpInit();
    WHBLogConsoleSetColor(0);
    WHBLogPrint("Starting WUP Network Installer");
    WHBLogConsoleDraw();

    //!*******************************************************************
    //!                    Initialize heap memory                        *
    //!*******************************************************************
    WHBLogPrint("Initialize heap memory");
    WHBLogConsoleDraw();
    //memoryInitialize();

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    WHBLogPrint("Mount SD card");
    WHBLogConsoleDraw();
    //mount_sd_fat("sd");

    //!*******************************************************************
    //!                    Enter main application                        *
    //!*******************************************************************
    WHBLogPrint("Start main application");
    WHBLogConsoleDraw();
    int returnCode = Application::instance()->exec();

    //DEBUG_FUNCTION_LINE("Main application stopped\n");

    Application::destroyInstance();

    WHBLogPrint("Unmount SD");
    WHBLogConsoleDraw();
    //unmount_sd_fat("sd");
    WHBLogPrint("Release memory");
    WHBLogConsoleDraw();
    //memoryRelease();
    //log_deinit();

    WHBLogConsoleFree();
    WHBProcShutdown();
    return returnCode;
}