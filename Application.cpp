/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "Application.h"
//#include "common/common.h"
//#include "dynamic_libs/os_functions.h"
//#include "dynamic_libs/sys_functions.h"
//#include "gui/FreeTypeGX.h"
//#include "gui/DVPadController.h"
//#include "gui/DWPadController.h"
//#include "resources/Resources.h"
//#include "sounds/SoundHandler.hpp"
//#include "utils/logger.h"
#include <vpad/input.h>
#include <whb/log.h>
#include "log_console.h"
#include <sys/stat.h>
#include <sys/dirent.h>


Application *Application::applicationInstance = NULL;
bool Application::exitApplication = false;

Application::Application()
	: CThread(CThread::eAttributeAffCore1 | CThread::eAttributePinnedAff, 0, 0x20000)
    , exitCode(0)
{
	exitApplication = false;
}

Application::~Application()
{
}

int Application::exec()
{
    //! start main thread
    resumeThread();
    //! now wait for thread to finish
	shutdownThread();

	return exitCode;
}

void Application::executeThread(void)
{
    struct dirent *dirent = NULL;
	DIR *dir = NULL;

	dir = opendir("fs:/vol/external01");
	if (dir == NULL) {
		WHBLogPrint("Failed to read /vol/external01");
        WHBLogConsoleDraw();
    } else {
        while ((dirent = readdir(dir)) != 0)
        {
            bool isDir = dirent->d_type & DT_DIR;
            const char *filename = dirent->d_name;
            WHBLogPrintf("%s", filename);
            WHBLogConsoleDraw();
        }
        closedir(dir);
    }

    VPADStatus status;
    VPADReadError error;
    while (!exitApplication) {
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

        if (status.trigger & VPAD_BUTTON_B) {
            exitApplication = true;
        }
        else if (status.trigger & VPAD_BUTTON_HOME) {
            exitApplication = true;
        }
    }
}