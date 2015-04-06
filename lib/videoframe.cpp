/****************************************************************************
*
*    Kiroku, software to record OpenGL programs
*    Copyright (C) 2015  Dušan Poizl
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

#include "videoframe.h"
#include <time.h>

void initVideoFrame(VideoFrame *frame, const int size[])
{
    frame->width = size[0];
    frame->height = size[1];

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    frame->timestamp = getnanoclock();
}

uint64_t getnanoclock()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    return time.tv_sec*1000000000L + time.tv_nsec;
}
