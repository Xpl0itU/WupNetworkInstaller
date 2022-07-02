#include <stdint.h>
#include <string.h>
#include <string>
#include <whb/log.h>
#include <whb/log_console.h>
#include <vpad/input.h>
#include "utils.h"


void printProgressBar(uint64_t current, uint64_t total) {
    int percent = (int) (current * 100.0f / total);

    std::string bar;
    char buf[64];
    bar.push_back('\r');
    bar.push_back('[');
    int i;
    for (i = 0; i < percent; i += 4) {
        bar.push_back('=');
    }
    bar.push_back('>');
    for (; i < 100; i+= 4) {
        bar.push_back(' ');
    }
    sprintf(buf, "]  %.2f/%.2f MiB (%d pct)", current / 1048576.0, total / 1048576.0, percent);
    bar += buf;
    WHBLogPrintf(bar.c_str());
    WHBLogConsoleDraw();
}


void waitForKey() {
    VPADStatus status;
    VPADReadError error;
    while (true) {
        // Read button, touch and sensor data from the Gamepad
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
                WHBLogConsoleDraw();
                break;
            }
        }

        if (status.trigger) {
            break;
        }
    }
}
