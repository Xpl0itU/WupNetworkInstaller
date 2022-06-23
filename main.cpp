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

#include <whb/proc.h>

int main(int argc, char** argv)
{
    WHBProcInit();

    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    WHBLogConsoleInit();
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
    //DEBUG_FUNCTION_LINE("Start main application\n");
    //int returnCode = Application::instance()->exec();
    VPADStatus status;
    VPADReadError error;
    while (WHBProcIsRunning()) {
        /*  Read button, touch and sensor data from the Gamepad */
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        switch (error) {
            case VPAD_READ_SUCCESS: {
                // No error
                break;
            }
            case VPAD_READ_NO_SAMPLES: {
                // No new sample data available
                continue;
            }
            case VPAD_READ_INVALID_CONTROLLER: {
                // Invalid controller
                continue;
            }
            /*
            case VPAD_READ_BUSY: {
                // Busy
                continue;
            }
            */
            default: {
                if ((int32_t) error == -4) {
                    continue;
                }
                WHBLogPrintf("Unknown VPAD error! %08X", error);
                break;
            }
        }

        if (status.trigger & VPAD_BUTTON_B) {
            break;
        }
        WHBLogConsoleDraw();
    }

    //DEBUG_FUNCTION_LINE("Main application stopped\n");

    //Application::destroyInstance();

    WHBLogPrint("Unmount SD");
    WHBLogConsoleDraw();
    //unmount_sd_fat("sd");
    WHBLogPrint("Release memory");
    WHBLogConsoleDraw();
    //memoryRelease();
    //log_deinit();

    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}