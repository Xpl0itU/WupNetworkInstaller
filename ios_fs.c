#include "ios_fs.h"
#include <coreinit/filesystem.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <iosuhax.h>

#ifdef __cplusplus
extern "C" {
#endif

static FSClient fsClient;
static bool initialized = false;

static uint8_t *alloc_iobuf() {
    uint8_t *iobuf = (uint8_t*) memalign(0x40, 0x828);
    memset(iobuf, 0, 0x828);
    return iobuf;
}

int initFs(FSClient *outClient) {
    if (initialized) {
        if (outClient) {
            *outClient = fsClient;
        }
        return 0;
    }

    FSInit();
    if (IOSUHAX_Open(NULL) < 0) {
        WHBLogPrint("IOSUHAX failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }
    if (FSAddClient(&fsClient, FS_ERROR_FLAG_ALL) != FS_STATUS_OK) {
        WHBLogPrint("FSAddClient failed! Press any key to exit");
        WHBLogConsoleDraw();
        return -1;
    }
    int returncode = IOSUHAX_UnlockFSClient(&fsClient);
    if (returncode < 0) return returncode;

/*
    returncode = mountWiiUDisk();
    if (returncode < 0) return returncode;

    returncode = mountExternalFat32Disk();
    if (returncode < 0) return returncode;
*/
    initialized = true;
    if (outClient) {
        *outClient = fsClient;
    }
    return 0;
}

int cleanupFs() {
    FSDelClient(&fsClient, FS_ERROR_FLAG_ALL);
    initialized = false;
    return 0;
}


FSError FSA_Ioctl(FSClient *client, int ioctl, void *in_buf, uint32_t in_len, void *out_buf, uint32_t out_len) {
    if (!client) {
        return FS_ERROR_INVALID_CLIENTHANDLE;
    }

    int handle = FSGetClientBody(client)->clientHandle;
    return FSA_IoctlEx(handle, ioctl, in_buf, in_len, out_buf, out_len);
}

FSError FSA_IoctlEx(int clientHandle, int ioctl, void *in_buf, uint32_t in_len, void *out_buf, uint32_t out_len) {
    DCFlushRange(in_buf, in_len);
    int ret = IOS_Ioctl(clientHandle, ioctl, in_buf, in_len, out_buf, out_len);
    DCFlushRange(out_buf, out_len);
    return (FSError) ret;
}


FSError FSA_Ioctlv(FSClient *client, uint32_t request, uint32_t vectorCountIn, uint32_t vectorCountOut, IOSVec *vector) {
    if (!client) {
        return FS_ERROR_INVALID_CLIENTHANDLE;
    }

    int handle = FSGetClientBody(client)->clientHandle;
    return FSA_IoctlvEx(handle, request, vectorCountIn, vectorCountOut, vector);
}


FSError FSA_IoctlvEx(int clientHandle, uint32_t request, uint32_t vectorCountIn, uint32_t vectorCountOut, IOSVec *vector) {
    DCFlushRange(vector, sizeof(IOSVec) * (vectorCountIn + vectorCountOut));
    int ret = IOS_Ioctlv(clientHandle, request, vectorCountIn, vectorCountOut, vector);
    return (FSError) ret;
}


/*
        case IOCTL_FSA_RAW_OPEN: {
            int fd     = message->ioctl.buffer_in[0];
            char *path = ((char *) message->ioctl.buffer_in) + message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawOpen(fd, path, (int *) (message->ioctl.buffer_io + 1));
            break;
        }
        case IOCTL_FSA_RAW_READ: {
            int fd            = message->ioctl.buffer_in[0];
            u32 block_size    = message->ioctl.buffer_in[1];
            u32 cnt           = message->ioctl.buffer_in[2];
            u64 sector_offset = ((u64) message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int deviceHandle  = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawRead(fd, ((u8 *) message->ioctl.buffer_io) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_WRITE: {
            int fd            = message->ioctl.buffer_in[0];
            u32 block_size    = message->ioctl.buffer_in[1];
            u32 cnt           = message->ioctl.buffer_in[2];
            u64 sector_offset = ((u64) message->ioctl.buffer_in[3] << 32ULL) | message->ioctl.buffer_in[4];
            int deviceHandle  = message->ioctl.buffer_in[5];

            message->ioctl.buffer_io[0] = FSA_RawWrite(fd, ((u8 *) message->ioctl.buffer_in) + 0x40, block_size, cnt, sector_offset, deviceHandle);
            break;
        }
        case IOCTL_FSA_RAW_CLOSE: {
            int fd           = message->ioctl.buffer_in[0];
            int deviceHandle = message->ioctl.buffer_in[1];

            message->ioctl.buffer_io[0] = FSA_RawClose(fd, deviceHandle);
            break;
        }
        */

int FSA_RawOpen(FSClient *client, const char *device_path, int *out_handle) {
    // https://github.com/wiiu-env/MochaPayload/blob/8015a18f24611c424e207b43176272fd04ea8440/source/ios_mcp/source/fsa.c#L342
    uint8_t *iobuf = alloc_iobuf();

    uint32_t *in_buf = (uint32_t*) iobuf;
    uint32_t *out_buf = (uint32_t*) &iobuf[0x520];
    strncpy((char *) &in_buf[0x01], device_path, 0x27F);

    int ret = FSA_Ioctl(client, 0x6A, in_buf, 0x520, out_buf, 0x293);

    if (out_handle) *out_handle = out_buf[1];
    free(iobuf);
    return ret;
}


int FSA_RawClose(FSClient *client, int deviceHandle) {
    // https://github.com/wiiu-env/MochaPayload/blob/8015a18f24611c424e207b43176272fd04ea8440/source/ios_mcp/source/fsa.c#L357
    uint8_t *iobuf = alloc_iobuf();

    uint32_t *in_buf = (uint32_t*) iobuf;
    uint32_t *out_buf = (uint32_t*) &iobuf[0x520];

    in_buf[1] = deviceHandle;

    int ret = FSA_Ioctl(client, 0x6D, in_buf, 0x520, out_buf, 0x293);

    free(iobuf);
    return ret;
}


// offset in blocks of 0x1000 bytes
int FSA_RawRead(FSClient *client, void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int device_handle) {
    // https://github.com/wiiu-env/MochaPayload/blob/8015a18f24611c424e207b43176272fd04ea8440/source/ios_mcp/source/fsa.c#L371
    uint8_t *iobuf = alloc_iobuf();

    uint32_t *in_buf = (uint32_t*) iobuf;
    uint32_t *out_buf = (uint32_t*) &iobuf[0x520];
    IOSVec *iovec = (IOSVec *) &iobuf[0x7C0];

    // note : offset_bytes = blocks_offset * size_bytes
    in_buf[0x08 / 4] = (blocks_offset >> 32);
    in_buf[0x0C / 4] = (blocks_offset & 0xFFFFFFFF);
    in_buf[0x10 / 4] = cnt;
    in_buf[0x14 / 4] = size_bytes;
    in_buf[0x18 / 4] = device_handle;

    iovec[0].vaddr = in_buf;
    iovec[0].len = 0x520;

    iovec[1].vaddr = data;
    iovec[1].len = size_bytes * cnt;

    iovec[2].vaddr = out_buf;
    iovec[2].len = 0x293;

    int ret = FSA_Ioctlv(client, 0x6B, 1, 2, iovec);
    DCFlushRange(data, size_bytes * cnt);

    free(iobuf);
    return ret;
}


int FSA_RawWrite(FSClient *client, const void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int device_handle) {
    // https://github.com/wiiu-env/MochaPayload/blob/8015a18f24611c424e207b43176272fd04ea8440/source/ios_mcp/source/fsa.c#L401
    uint8_t *iobuf = alloc_iobuf();

    uint32_t *in_buf = (uint32_t*) iobuf;
    uint32_t *out_buf = (uint32_t*) &iobuf[0x520];
    IOSVec *iovec = (IOSVec *) &iobuf[0x7C0];

    // note : offset_bytes = blocks_offset * size_bytes
    in_buf[0x08 / 4] = (blocks_offset >> 32);
    in_buf[0x0C / 4] = (blocks_offset & 0xFFFFFFFF);
    in_buf[0x10 / 4] = cnt;
    in_buf[0x14 / 4] = size_bytes;
    in_buf[0x18 / 4] = device_handle;

    iovec[0].vaddr = in_buf;
    iovec[0].len = 0x520;

    iovec[1].vaddr = (void*) data;
    iovec[1].len = size_bytes * cnt;

    iovec[2].vaddr = out_buf;
    iovec[2].len = 0x293;

    DCFlushRange(data, size_bytes * cnt);
    int ret = FSA_Ioctlv(client, 0x6C, 2, 1, iovec);

    free(iobuf);
    return ret;
}


int FSA_FlushVolume(FSClient *client, const char *volume_path) {
    uint8_t *iobuf = alloc_iobuf();
    uint32_t *in_buf  = (uint32_t *) iobuf;
    uint32_t *out_buf = (uint32_t *) &iobuf[0x520];

    strncpy((char *) &in_buf[0x01], volume_path, 0x27F);

    int ret = FSA_Ioctl(client, 0x1B, in_buf, 0x520, out_buf, 0x293);

    free(iobuf);
    return ret;
}


// type 4 :
// 		0x08 : device size in sectors (u64)
// 		0x10 : device sector size (u32)
int FSA_GetDeviceInfo(FSClient *client, const char *device_path, int type, void *out_data) {
    uint8_t *iobuf = alloc_iobuf();
    uint32_t *in_buf  = (uint32_t *) iobuf;
    uint32_t *out_buf = (uint32_t *) &iobuf[0x520];

    strncpy((char *) &in_buf[0x01], device_path, 0x27F);
    in_buf[0x284 / 4] = type;

    int ret = FSA_Ioctl(client, 0x18, in_buf, 0x520, out_buf, 0x293);

    int size = 0;

    switch (type) {
        case 0:
        case 1:
        case 7:
            size = 0x8;
            break;
        case 2:
            size = 0x4;
            break;
        case 3:
            size = 0x1E;
            break;
        case 4:
            size = 0x28;
            break;
        case 5:
            size = 0x64;
            break;
        case 6:
        case 8:
            size = 0x14;
            break;
    }

    memcpy(out_data, &out_buf[1], size);

    free(iobuf);
    return ret;
}


#ifdef __cplusplus
}
#endif