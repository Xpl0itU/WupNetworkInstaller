//#include <string>
//#include "Application.h"
#include <dynamic_libs/os_functions.h>
#include <dynamic_libs/fs_functions.h>
#include <dynamic_libs/gx2_functions.h>
#include <dynamic_libs/sys_functions.h>
#include <dynamic_libs/vpad_functions.h>
#include <dynamic_libs/padscore_functions.h>
#include <dynamic_libs/socket_functions.h>
#include <dynamic_libs/ax_functions.h>
#include <fs/FSUtils.h>
#include <fs/sd_fat_devoptab.h>
#include <system/memory.h>
#include <utils/logger.h>
#include <utils/utils.h>
//#include "common/common.h"
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
//#include <zlib.h>

#include <vpad/input.h>
#include <whb/log.h>
#include "log_console.h"

/* Entry point */
extern "C" int Menu_Main(void)
{
    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    //! do OS (for acquire) and sockets first so we got logging
    InitOSFunctionPointers();
    InitSocketFunctionPointers();

    WHBLogConsoleInit();
    WHBLogConsoleSetColor(0);
    WHBLogPrint("Starting WUP Network Installer");
    WHBLogConsoleDraw();

    InitFSFunctionPointers();
    // InitGX2FunctionPointers();
    InitSysFunctionPointers();
    InitVPadFunctionPointers();
    InitPadScoreFunctionPointers();
    InitAXFunctionPointers();

    WHBLogPrint("Function exports loaded");
    WHBLogConsoleDraw();

    //!*******************************************************************
    //!                    Initialize heap memory                        *
    //!*******************************************************************
    WHBLogPrint("Initialize heap memory");
    WHBLogConsoleDraw();
    memoryInitialize();

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    WHBLogPrint("Mount SD card");
    WHBLogConsoleDraw();
    mount_sd_fat("sd");

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
                break;
            }
            case VPAD_READ_NO_SAMPLES: {
                continue;
            }
            case VPAD_READ_INVALID_CONTROLLER: {
                continue;
            }
            default: {
                WHBLogPrintf("Unknown VPAD error! %08X", error);
                break;
            }
        }

        if (status.trigger & VPAD_BUTTON_B) {
            break;
        }
        /*
        etc. etc. Other similar buttons:
        - VPAD_BUTTON_X
        - VPAD_BUTTON_Y
        - VPAD_BUTTON_ZL
        - VPAD_BUTTON_ZR
        - VPAD_BUTTON_L
        - VPAD_BUTTON_R
        - VPAD_BUTTON_PLUS (aka Start)
        - VPAD_BUTTON_MINUS (aka Select)
        - VPAD_BUTTON_HOME (this one caught by ProcUI)
        - VPAD_BUTTON_SYNC
        - VPAD_BUTTON_STICK_R (clicking in the right stick)
        - VPAD_BUTTON_STICK_L (clicking in the left stick)
        - VPAD_BUTTON_TV (this one will make the DRC open the TV Remote menu) */

        /*
        Check the directions - VPAD_BUTTON_UP is the D-pad, while
        VPAD_STICK_L_EMULATION_UP and VPAD_STICK_R_EMULATION_UP allow you to
        treat the analog sticks like two extra D-pads. Bitwise OR means any
        one of these three will return true. */
        /*
        if (status.trigger & (VPAD_BUTTON_UP |
            VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP)) {
            WHBLogPrint("Going up!");
        }
        if (status.trigger & (VPAD_BUTTON_LEFT |
            VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_R_EMULATION_LEFT)) {
            WHBLogPrint("Going... left?");
        }
        */
        WHBLogConsoleDraw();
    }

    //DEBUG_FUNCTION_LINE("Main application stopped\n");

    //Application::destroyInstance();

    WHBLogPrint("Unmount SD");
    WHBLogConsoleDraw();
    unmount_sd_fat("sd");
    WHBLogPrint("Release memory");
    WHBLogConsoleDraw();
    memoryRelease();
    //log_deinit();

    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}