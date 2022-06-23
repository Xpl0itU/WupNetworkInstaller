#include <whb/log.h>

#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>

#include <string.h>

#include "log_console.h"

#define NUM_LINES (16)
#define LINE_LENGTH (128)
#define CONSOLE_FRAME_HEAP_TAG (0x000DECAF)

static char sConsoleBuffer[NUM_LINES][LINE_LENGTH];
static int sConsoleBufferStart = 0;
static int sConsoleBufferEnd = 0;
static int sLineNum = 0;
static void *sBufferTV = NULL, *sBufferDRC = NULL;
static uint32_t sBufferSizeTV = 0, sBufferSizeDRC = 0;
static BOOL sConsoleHasForeground = TRUE;
static uint32_t consoleColor = 0x993333FF;

static void
ConsoleAddLine(const char *line)
{
    int length = strlen(line);

    if (sLineNum == NUM_LINES) {
        sConsoleBufferStart = (sConsoleBufferStart + 1) % NUM_LINES;
    } else {
        ++sLineNum;
    }
    strncpy_s(sConsoleBuffer[sConsoleBufferEnd], LINE_LENGTH, line, length);
    sConsoleBufferEnd = (sConsoleBufferEnd + 1) % NUM_LINES;
}

static uint32_t
ConsoleProcCallbackAcquired(void *context)
{
   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   if (sBufferSizeTV) {
      sBufferTV = MEMAllocFromFrmHeapEx(heap, sBufferSizeTV, 4);
   }

   if (sBufferSizeDRC) {
      sBufferDRC = MEMAllocFromFrmHeapEx(heap, sBufferSizeDRC, 4);
   }

   sConsoleHasForeground = TRUE;
   OSScreenSetBufferEx(SCREEN_TV, sBufferTV);
   OSScreenSetBufferEx(SCREEN_DRC, sBufferDRC);
   return 0;
}

static uint32_t
ConsoleProcCallbackReleased(void *context)
{
   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   MEMFreeByStateToFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
   sConsoleHasForeground = FALSE;
   return 0;
}

BOOL
WHBLogConsoleInit()
{
   OSScreenInit();
   sBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
   sBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);

   ConsoleProcCallbackAcquired(NULL);
   OSScreenEnableEx(SCREEN_TV, 1);
   OSScreenEnableEx(SCREEN_DRC, 1);

   ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, ConsoleProcCallbackAcquired, NULL, 100);
   ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, ConsoleProcCallbackReleased, NULL, 100);
   WHBAddLogHandler(ConsoleAddLine);
   return FALSE;
}

void
WHBLogConsoleFree()
{
   if (sConsoleHasForeground) {
      OSScreenShutdown();
      ConsoleProcCallbackReleased(NULL);
   }
}

void
WHBLogConsoleSetColor(uint32_t color)
{
   consoleColor = color;
}

void
WHBLogConsoleDraw()
{
   if (!sConsoleHasForeground) {
      return;
   }

   OSScreenClearBufferEx(SCREEN_TV, consoleColor);
   OSScreenClearBufferEx(SCREEN_DRC, consoleColor);

   for (int y = 0; y < NUM_LINES; ++y) {
      int idx = (sConsoleBufferStart + y) % NUM_LINES;
      OSScreenPutFontEx(SCREEN_TV, 0, y, sConsoleBuffer[idx]);
      OSScreenPutFontEx(SCREEN_DRC, 0, y, sConsoleBuffer[idx]);
   }

   DCFlushRange(sBufferTV, sBufferSizeTV);
   DCFlushRange(sBufferDRC, sBufferSizeDRC);
   OSScreenFlipBuffersEx(SCREEN_TV);
   OSScreenFlipBuffersEx(SCREEN_DRC);
}