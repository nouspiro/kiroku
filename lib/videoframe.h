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

#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <stdint.h>

typedef struct
{
    int width;
    int height;
    int64_t timestamp;
}VideoFrame;

void initVideoFrame(VideoFrame *frame, const int size[2]);

uint64_t getnanoclock();

#endif // VIDEOFRAME_H
