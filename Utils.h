/*
    Uxplore
    Copyright (C) 2019-2021, Kuruyia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UXPLORE_CPP_UTILS_H
#define UXPLORE_CPP_UTILS_H

#include <stdint-gcc.h>

class Utils {
public:
    static uint16_t swapEndian16(const uint16_t n)
    {
        return (n >> 8) | (n << 8);
    }

    static uint32_t swapEndian32(const uint32_t n)
    {
        return (uint32_t) (swapEndian16(n & 0xFFFF)) << 16 | swapEndian16(n >> 16);
    }

    static uint64_t swapEndian64(const uint64_t n)
    {
        return (uint64_t) (swapEndian32(n & 0xFFFFFFFF)) << 32 | swapEndian32(n >> 32);
    }
};

#endif //UXPLORE_CPP_UTILS_H