//#include <string>
//#include "Application.h"
//#include "common/common.h"
#include <cstdio>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <malloc.h>
#include <zlib.h>

#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_console.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>
#include <coreinit/ios.h>

#include <coreinit/mcp.h>
#include <coreinit/memory.h>

#include <whb/proc.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/filesystem.h>
#include "os_functions.h"
#include "fatfs/ff.h"

#define MCP_COMMAND_INSTALL_ASYNC 0x81
#define MAX_INSTALL_PATH_LENGTH     0x27F

#define COPY_BUFFER_SZ 1024 * 1024 * 64


static int installCompleted = 0;
static uint32_t installError = 0;
static FATFS fs;
static int fsaFdWiiU = -1;
static FSClient fsClient;


int someFunc(IOSError err, int* arg)
{
    return 0;
}

static void IosInstallCallback(IOSError errorCode, void * priv_data)
{
	installError = (uint32_t) errorCode;
	installCompleted = 1;
}

int install()
{
    int target = 1;
	int result = 0;
	
	//!---------------------------------------------------
	//! This part of code originates from Crediars MCP patcher assembly code
	//! it is just translated to C
	//!---------------------------------------------------
	unsigned int mcpHandle = MCP_Open();
	if(mcpHandle == 0)
	{
        WHBLogPrint("Failed to acquire mcp");
        WHBLogConsoleDraw();
		
		return -1;
	}
	else
	{
        //char installPath[256] = "/vol/app_usb/tmp/install";
        char installPath[256] = "/vol/storage_usb01/usr/tmp/WUP-N-D43E_0005000244343345";
		unsigned int * mcpInstallInfo = (unsigned int *)OSAllocFromSystem(0x24, 0x40);
		char * mcpInstallPath = (char *)OSAllocFromSystem(MAX_INSTALL_PATH_LENGTH, 0x40);
		unsigned int * mcpPathInfoVector = (unsigned int *)OSAllocFromSystem(0x0C, 0x40);

        if(!mcpInstallInfo || !mcpInstallPath || !mcpPathInfoVector)
        {
            WHBLogPrint("out of memory");
            WHBLogConsoleDraw();
            
            return -2;
        }
        
        //std::string installFolder = folderList->GetPath(index);
        //installFolder.erase(0, 19);
        //installFolder.insert(0, "/vol/app_sd/");
        
        int res = MCP_InstallGetInfo(mcpHandle, installPath, (MCPInstallInfo*)mcpInstallInfo);
        if(res != 0)
        {
            WHBLogPrint("missing wup files");
            WHBLogConsoleDraw();
            return -3;
        }
        
        uint32_t titleIdHigh = mcpInstallInfo[0];
        //uint32_t titleIdLow = mcpInstallInfo[1];
        bool spoofFiles = false;
        
        if (spoofFiles
            || (titleIdHigh == 0x0005000E)     // game update
            || (titleIdHigh == 0x00050000)     // game
            || (titleIdHigh == 0x0005000C)     // DLC
            || (titleIdHigh == 0x00050002))    // Demo
        {
            res = MCP_InstallSetTargetDevice(mcpHandle, (MCPInstallTarget)(target));
            if(res != 0)
            {
                WHBLogPrintf("mcp_installsettargetdevice 0x%08X", MCP_GetLastRawError());
                WHBLogConsoleDraw();
                result = -5;
                goto cleanup;
            }
            res = MCP_InstallSetTargetUsb(mcpHandle, (MCPInstallTarget)(target));
            if(res != 0)
            {
                WHBLogPrintf("mcp_installsettargetusb 0x%08X", MCP_GetLastRawError());
                WHBLogConsoleDraw();
                result = -6;
                goto cleanup;
            }
            
            mcpInstallInfo[2] = (unsigned int)MCP_COMMAND_INSTALL_ASYNC;
            mcpInstallInfo[3] = (unsigned int)mcpPathInfoVector;
            mcpInstallInfo[4] = (unsigned int)1;
            mcpInstallInfo[5] = (unsigned int)0;
            
            memset(mcpInstallPath, 0, MAX_INSTALL_PATH_LENGTH);
            snprintf(mcpInstallPath, MAX_INSTALL_PATH_LENGTH, installPath);
            memset(mcpPathInfoVector, 0, 0x0C);
            
            mcpPathInfoVector[0] = (unsigned int)mcpInstallPath;
            mcpPathInfoVector[1] = (unsigned int)MAX_INSTALL_PATH_LENGTH;
            
            res = IOS_IoctlvAsync(mcpHandle, MCP_COMMAND_INSTALL_ASYNC, 1, 0, (IOSVec *)mcpPathInfoVector, IosInstallCallback, mcpInstallInfo);
            if(res != 0)
            {
                WHBLogPrintf("mcp_installtitleasync 0x%08X", MCP_GetLastRawError());
                WHBLogConsoleDraw();
                result = -7;
                goto cleanup;
            }
            
            while(!installCompleted)
            {
                memset(mcpInstallInfo, 0, 0x24);
                
                MCP_InstallGetProgress(mcpHandle, (MCPInstallProgress*)mcpInstallInfo);
                
                if(mcpInstallInfo[0] == 1)
                {
                    uint64_t totalSize = ((uint64_t)mcpInstallInfo[3] << 32ULL) | mcpInstallInfo[4];
                    uint64_t installedSize = ((uint64_t)mcpInstallInfo[5] << 32ULL) | mcpInstallInfo[6];
                    int percent = (totalSize != 0) ? ((installedSize * 100.0f) / totalSize) : 0;
                    
                    WHBLogPrintf("%0.1f / %0.1f MB (%i percent)", installedSize / (1024.0f * 1024.0f), totalSize / (1024.0f * 1024.0f), percent);
                    WHBLogConsoleDraw();
                }
                
                usleep(50000);
            }
            
            if(installError != 0)
            {
                if ((installError == 0xFFFCFFE9))
                {
                    WHBLogPrintf("0x%08X access failed", installError);
                    WHBLogConsoleDraw();
                    result = -8;
                    goto cleanup;
                }
                else
                {
                    WHBLogPrintf("0x%08X install failed", installError);
                    //__os_snprintf(errorText1, sizeof(errorText1), "Error: install error code 0x%08X", installError);
                    if (installError == 0xFFFBF446 || installError == 0xFFFBF43F)
                        WHBLogPrint("Possible missing or bad title.tik");
                    else if (installError == 0xFFFBF441)
                        WHBLogPrint("Possible incorrect console for DLC title.tik");
                    else if (installError == 0xFFFCFFE4)
                        WHBLogPrint("Possible not enough memory on target device");
                    else if (installError == 0xFFFFF825)
                        WHBLogPrint("Possible bad SD card.  Reformat (32k blocks) or replace");
                    else if ((installError & 0xFFFF0000) == 0xFFFB0000)
                        WHBLogPrint("Verify WUP files are correct & complete. DLC/E-shop require Sig Patch");
                    WHBLogConsoleDraw();
                    result = -9;
                    goto cleanup;
                }
            }
        }
        else
        {
            WHBLogPrint("Not a game, game update, dlc, demo or version title");
            WHBLogConsoleDraw();
            result = -4;
            goto cleanup;
        }
		
        cleanup:
		MCP_Close(mcpHandle);
		if(mcpPathInfoVector)
			OSFreeToSystem(mcpPathInfoVector);
		if(mcpInstallPath)
			OSFreeToSystem(mcpInstallPath);
		if(mcpInstallInfo)
			OSFreeToSystem(mcpInstallInfo);
        WHBLogPrint("Cleaned up");
        WHBLogConsoleDraw();
	}

    return result;
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
    sprintf(buf, "]  %d/%d (%d pct)", current, total, percent);
    bar += buf;
    WHBLogPrintf(bar.c_str());
    WHBLogConsoleDraw();
}

bool copyFile(char *src, char *dst, void *buffer, size_t buf_size) {
    FIL fp;
    FILINFO fi;
    if (f_stat(src, &fi) != FR_OK) {
        return false;
    }

    if (f_open(&fp, src, FA_READ) != F_OK) {
        return false;
    }

    FSFileHandle handle;
    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);
    int result = FSOpenFile(&fsClient, &cmd, dst, "w", &handle, FS_ERROR_FLAG_ALL);
    if (result < 0) {
        WHBLogPrintf("%s: FSOpenFile error %d", __FUNCTION__, result);
        return false;
    }

    uint64_t bytes_copied = 0;
    while (bytes_copied < fi.fsize) {
        uint bytes_read = 0;
        if (f_read(&fp, buffer, buf_size, &bytes_read) != FR_OK) {
            return false;
        }

        uint bytes_written = 0;
        while (bytes_written < bytes_read) {
            FSInitCmdBlock(&cmd);

            // FSStatus FSWriteFile( FSClient *client, FSCmdBlock *block, const void *source, FSSize size, FSCount count, FSFileHandle fileHandle, FSFlag flag, FSRetFlag errHandling );
            FSStatus result = FSWriteFile(&fsClient, &cmd, (uint8_t*) buffer, 1, bytes_read, (FSFileHandle)handle, 0, FS_ERROR_FLAG_ALL);
            if (result < 0) {
                WHBLogPrintf("%s: FSWriteFile error %d", __FUNCTION__, result);
                return 0;
            }
            bytes_written += result;
            bytes_copied += result;
            printProgressBar(bytes_copied, fi.fsize);
        }
    }
}

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

    void *copyBuffer = MEMAllocFromDefaultHeapEx(COPY_BUFFER_SZ, 64);
    if (copyBuffer == nullptr) {
        WHBLogPrint("Memory allocation failed! Press any key to exit");
        WHBLogConsoleDraw();
        waitForKey();
        return -1;
    }

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    WHBLogPrint("Mounting external USB (FAT32)");
    WHBLogConsoleDraw();
    if (f_mount(&fs, "", 0) != FR_OK) {
        WHBLogPrint("Mount failed! Press any key to exit");
        WHBLogConsoleDraw();
        waitForKey();
        return -1;
    }

    WHBLogPrint("Mounting external USB (Wii U)");
    WHBLogConsoleDraw();
    if (mount_fs("storage_usb", fsaFdWiiU, NULL, "/vol/storage_usb01") < 0) {
        WHBLogPrint("Mount failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }
    FSInit();
    if (FSAddClient(&fsClient, FS_ERROR_FLAG_ALL) != FS_STATUS_OK) {
        WHBLogPrint("FSAddClient failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }

    //!*******************************************************************
    //!                    Enter main application                        *
    //!*******************************************************************
    WHBLogPrint("Start main application");
    WHBLogConsoleDraw();

    WHBLogPrintf("Copying file");
    WHBLogConsoleDraw();
    if (copyFile("/WUP-N-D43E_0005000244343345/0000000E.app", "/vol/storage_usb01/usr/tmp/0000000E.app", copyBuffer, COPY_BUFFER_SZ)) {
        WHBLogPrintf("Copying file succeeded");
        WHBLogConsoleDraw();
    } else {
        WHBLogPrintf("Copying file failed");
        WHBLogConsoleDraw();
    }

    //WUP-N-D43E_0005000244343345
    DIR dir;
    if (f_opendir(&dir, "/WUP-N-D43E_0005000244343345") != FR_OK) {
        WHBLogPrint("opendir failed");
        WHBLogConsoleDraw();
    } else {
        FILINFO fi;
        do {
            if (f_readdir(&dir, &fi) != FR_OK) {
                WHBLogPrint("readdir failed");
                WHBLogConsoleDraw();
                break;
            }
            WHBLogPrintf("> %s", fi.fname);
            WHBLogConsoleDraw();
        } while (fi.fname[0] != 0);
        f_unmount("");
        WHBLogPrint("fs unmounted");
        WHBLogConsoleDraw();
    }

    //install();

    VPADStatus status;
    VPADReadError error;

    int i = 0;
    while (WHBProcIsRunning()) {
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
                break;
            }
        }
        if (status.trigger & VPAD_BUTTON_B) {
            break;
        }
        else if (status.trigger & VPAD_BUTTON_HOME) {
            break;
        }
        WHBLogConsoleDraw();
    }

    WHBLogPrint("Unmounting external USB (Wii U)");
    WHBLogConsoleDraw();
    FSDelClient(&fsClient, FS_ERROR_FLAG_ALL);
    unmount_fs("storage_usb");
    IOSUHAX_FSA_FlushVolume(fsaFdWiiU, "/vol/storage_usb01");

    MEMFreeToDefaultHeap(copyBuffer);

    WHBLogConsoleFree();
    WHBLogUdpDeinit();
    WHBProcShutdown();
    return 0;
}
