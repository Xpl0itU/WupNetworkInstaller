#pragma once
#include <cstdint>
#include <coreinit/time.h>

void printProgressBarWithSpeed(uint64_t current, uint64_t total, OSTime elapsed);
uint32_t getKey();
uint32_t waitForKey();