/****************************************************************************
 * Copyright (C) 2015
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#ifndef __OS_FUNCTIONS_2_H_
#define __OS_FUNCTIONS_2_H_

#include <stdint.h>
#include <wut_types.h>
#include <coreinit/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _OsSpecifics
{
    unsigned int addr_OSDynLoad_Acquire;
    unsigned int addr_OSDynLoad_FindExport;
    unsigned int addr_OSTitle_main_entry;

    unsigned int addr_KernSyscallTbl1;
    unsigned int addr_KernSyscallTbl2;
    unsigned int addr_KernSyscallTbl3;
    unsigned int addr_KernSyscallTbl4;
    unsigned int addr_KernSyscallTbl5;
} OsSpecifics;

#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)

#define usleep(usecs)                   OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define sleep(secs)                     OSSleepTicks(SECS_TO_TICKS(secs))

#define FLUSH_DATA_BLOCK(addr)          asm volatile("dcbf 0, %0; sync" : : "r"(((addr) & ~31)))
#define INVAL_DATA_BLOCK(addr)          asm volatile("dcbi 0, %0; sync" : : "r"(((addr) & ~31)))

#define OS_MUTEX_SIZE                   44

/* Handle for coreinit */
//extern unsigned int coreinit_handle;
void InitOSFunctionPointers(void);

//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! MCP functions
//!----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern int (*MCP_GetLastRawError)(void);

typedef unsigned char (*exception_callback)(void * interruptedContext);
extern void (* OSSetExceptionCallback)(uint8_t exceptionType, exception_callback newCallback);


#ifdef __cplusplus
}
#endif

#endif // __OS_FUNCTIONS_2_H_