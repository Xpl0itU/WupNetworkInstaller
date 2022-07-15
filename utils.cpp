#include <cstdint>
#include <string>
#include <whb/log.h>
#include <whb/log_console.h>
#include <vpad/input.h>
#include "utils.h"

static uint32_t lastKey = 0;


void printProgressBar(uint64_t current, uint64_t total) {
    int percent = static_cast<int>(current * 100.0 / total);

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

uint32_t getKey() {
    VPADStatus status;
    VPADReadError error;

    VPADRead(VPAD_CHAN_0, &status, 1, &error);
    switch (error) {
        case VPAD_READ_SUCCESS: {
            // No error
            lastKey = status.trigger;
            return lastKey;
        }
        case VPAD_READ_NO_SAMPLES: {
            // No new sample data available
            return lastKey;
        }
        case VPAD_READ_INVALID_CONTROLLER: {
            // Invalid controller
            return 0;
        }
        case VPAD_READ_BUSY: {
            // Busy
            return lastKey;
        }
        default: {
            WHBLogPrintf("Unknown VPAD error! %08X", error);
            WHBLogConsoleDraw();
            return 0;
        }
    }
}

uint32_t waitForKey() {
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
            case VPAD_READ_BUSY: {
                // Busy
                continue;
            }
            default: {
                WHBLogPrintf("Unknown VPAD error! %08X", error);
                WHBLogConsoleDraw();
                break;
            }
        }

        if (status.trigger) {
            return status.trigger;
        }
    }
}
