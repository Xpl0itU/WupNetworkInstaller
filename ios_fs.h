#pragma once

#include <coreinit/filesystem.h>

#ifdef __cplusplus
extern "C" {
#endif

FSError FSA_Ioctl(FSClient *client, int ioctl, void *inBuf, uint32_t inLen, void *outBuf, uint32_t outLen);
FSError FSA_IoctlEx(int clientHandle, int ioctl, void *inBuf, uint32_t inLen, void *outBuf, uint32_t outLen);
FSError FSA_Ioctlv(FSClient *client, uint32_t request, uint32_t vectorCountIn, uint32_t vectorCountOut, IOSVec *vector);
FSError FSA_IoctlvEx(int clientHandle, uint32_t request, uint32_t vectorCountIn, uint32_t vectorCountOut, IOSVec *vector);
int FSA_RawOpen(FSClient *client, const char *device_path, int *out_handle);
int FSA_RawClose(FSClient *client, int device_handle);
int FSA_RawRead(FSClient *client, void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int device_handle);
int FSA_RawWrite(FSClient *client, const void *data, uint32_t size_bytes, uint32_t cnt, uint64_t blocks_offset, int device_handle);
int FSA_FlushVolume(FSClient *client, const char *volume_path);
int FSA_GetDeviceInfo(FSClient *client, const char *device_path, int type, void *out_data);

int initFs(FSClient *outClient);
int cleanupFs();

#ifdef __cplusplus
}
#endif