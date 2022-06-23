#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include <vpad/input.h>
#include <whb/log.h>
#include <whb/proc.h>
#include "log_console.h"

/*  Note that "DRC" and "Gamepad" both refer to the large tablet controller.
    Calling it a "proto-Switch" or similar will result in me talking at length
    about how good Wind Waker HD is. */

int main(int argc, char** argv) {
    WHBProcInit();

/*  Use the Console backend for WHBLog - this one draws text with OSScreen
    Don't mix with other graphics code! */
    WHBLogConsoleInit();
    WHBLogConsoleSetColor(0);
    WHBLogPrint("A Very Cool And Complete Button Tester");
/*  WHBLog's Console backend won't actually show anything on-screen until you
    call this */
    WHBLogConsoleDraw();

    VPADStatus status;
    VPADReadError error;
    bool vpad_fatal = false;

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
    /*  etc. etc. Other similar buttons:
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

    /*  Check the directions - VPAD_BUTTON_UP is the D-pad, while
        VPAD_STICK_L_EMULATION_UP and VPAD_STICK_R_EMULATION_UP allow you to
        treat the analog sticks like two extra D-pads. Bitwise OR means any
        one of these three will return true. */
        if (status.trigger & (VPAD_BUTTON_UP |
            VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP)) {
            WHBLogPrint("Going up!");
        }
        if (status.trigger & (VPAD_BUTTON_LEFT |
            VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_R_EMULATION_LEFT)) {
            WHBLogPrint("Going... left?");
        }
        WHBLogConsoleDraw();
    }

    WHBLogPrint("Exiting...");
    WHBLogConsoleDraw();

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
}