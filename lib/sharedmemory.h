/****************************************************************************
*
*    Kiroku, software to record OpenGL programs
*    Copyright (C) 2014  Dušan Poizl
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

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <unistd.h>

class SharedMemory
{
public:
    enum Mode
    {
        Master,
        Slave
    };
    SharedMemory(const char *name, Mode mode = Slave);
    ~SharedMemory();
    void resize(size_t _size);
    void* lock();
    void unlock();
    void autoResize();
    size_t getSize();
private:
    Mode mode;
    int fd;
    void *rptr;
    size_t size;
    char name[256];
};

#endif // SHAREDMEMORY_H
