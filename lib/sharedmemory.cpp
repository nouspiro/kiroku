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

#include "sharedmemory.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <syscall.h>
#include <linux/futex.h>

typedef struct
{
    u_int64_t size;
    int mutex;
    int align;
}SegmentHeader;

#define SH(p) ((SegmentHeader*)p)
#define SH_SIZE(p) (SH(p)->size)
#define SH_MUTEX(p) (SH(p)->mutex)

int sys_futex(void *addr1, int op, int val, const struct timespec *timeout, void *addr2, int val3)
{
    return syscall(SYS_futex, addr1, op, val, timeout, addr2, val3);
}

#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define xchg(P, N) __sync_lock_test_and_set((P), (N))

SharedMemory::SharedMemory(const char *name, Mode mode) :
    mode(mode),
    err(0)
{
    strcpy(this->name, name);

    std::cout << "init sharedmemory " << sizeof(SegmentHeader) << std::endl;

    size = sizeof(SegmentHeader);
    fd = shm_open(name, mode==Master ? (O_CREAT | O_RDWR) : O_RDWR, S_IRUSR | S_IWUSR);
    if(fd<0)
    {
        err = errno;
        return;
    }
    int r=0;
    if(mode==Master)r = ftruncate(fd, size);
    if(r)
    {
        std::cerr << "Can't resize file" << std::endl;
        return;
    }
    rptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(mode==Master)
    {
        SH_SIZE(rptr) = 0;
        SH_MUTEX(rptr) = 0;
    }
}

SharedMemory::~SharedMemory()
{
    munmap(rptr, size);
    close(fd);
    if(mode==Master)shm_unlink(name);
}

void SharedMemory::resize(size_t _size)
{
    if(rptr)munmap(rptr, size);
    size = _size+sizeof(SegmentHeader);
    if(mode==Master)
    {
        if(ftruncate(fd, size))
        {
            std::cerr << "Can'ŧ resize memory" << std::endl;
        }
    }
    rptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(rptr==MAP_FAILED)std::cerr << "Failed to map memory" << std::endl;
    if(mode==Master)SH_SIZE(rptr) = size;
}

void *SharedMemory::lock()
{
    int c;
    c = cmpxchg(&SH_MUTEX(rptr), 0, 1);
    if(!c)return (char*)rptr+sizeof(SegmentHeader);

    if(c==1)c=xchg(&SH_MUTEX(rptr), 2);
    while(c)
    {
        sys_futex(&SH_MUTEX(rptr), FUTEX_WAIT, 2, NULL, NULL, 0);
        c = xchg(&SH_MUTEX(rptr), 2);
    }
    return (char*)rptr+sizeof(SegmentHeader);
}

void SharedMemory::unlock()
{
    if(SH_MUTEX(rptr)==2)SH_MUTEX(rptr)=0;
    else if(xchg(&SH_MUTEX(rptr), 0)==1)return;

    sys_futex(&SH_MUTEX(rptr), FUTEX_WAKE, 1, NULL, NULL, 0);
}

void SharedMemory::autoResize()
{
    resize(SH_SIZE(rptr));
}

size_t SharedMemory::getSize()
{
    return SH_SIZE(rptr);
}

int SharedMemory::error()
{
    return err;
}

