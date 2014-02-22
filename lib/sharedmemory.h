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
