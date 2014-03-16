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

#define __USE_GNU
#include "intercept.h"
#include <iostream>
#include <string.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "sharedmemory.h"

using namespace std;

typedef void (*PFNGLXSWAPBUFFERSPROC)(Display *dpy, GLXDrawable drawable);
typedef void* (*PFNDLSYMPROC)(void*, const char*);
typedef void* (*PFNGLXGETPROCADDRESSPROC)(const GLubyte*);

static PFNDLSYMPROC o_dlsym = 0;
static PFNGLXGETPROCADDRESSPROC _glXGetProcAddress = 0;

extern "C" void *glXGetProcAddress(const GLubyte * str) {
    if(_glXGetProcAddress==0)
        _glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)o_dlsym(RTLD_NEXT, "glXGetProcAddress");

    if(strcmp((const char*)str, "glXSwapBuffers")==0)return (void*)glXSwapBuffers;
    return _glXGetProcAddress(str);
}

extern "C" void *glXGetProcAddressARB (const GLubyte * str) {
    return glXGetProcAddress(str);
}

extern "C" void *dlsym(void *handle, const char *name)
{
    //cout << name << endl;

    if(strcmp(name, "glXGetProcAddressARB") == 0)
        return (void*)glXGetProcAddressARB;

    if(strcmp(name, "glXGetProcAddress") == 0)
        return (void*)glXGetProcAddress;

    if(strcmp(name, "glXSwapBuffers")==0)
        return (void*)glXSwapBuffers;

    if(!o_dlsym)
    {
        o_dlsym = (void*(*)(void *handle, const char *name)) dlvsym(RTLD_NEXT,"dlsym", "GLIBC_2.0");

        if(!o_dlsym)
            o_dlsym = (void*(*)(void *handle, const char *name)) dlvsym(RTLD_NEXT,"dlsym", "GLIBC_2.10");

        if(!o_dlsym)
            o_dlsym = (void*(*)(void *handle, const char *name)) dlvsym(RTLD_NEXT,"dlsym", "GLIBC_2.2.5");

        if(!o_dlsym)
            cout << "FAILED TO FIND DLSYM()" << endl;
        else
            cout << "found dlsym" << endl;
    }

    return (*o_dlsym)( handle,name );
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    static PFNGLXSWAPBUFFERSPROC _glXSwapBuffers = 0;
    static GLint size[2] = {0, 0};
    static SharedMemory memory("/kiroku-frame", SharedMemory::Master);

    if(_glXSwapBuffers==0)
    {
        _glXSwapBuffers = (PFNGLXSWAPBUFFERSPROC)o_dlsym(RTLD_NEXT, "glXSwapBuffers");
        cout << "INIT KIROKU" << endl;
    }

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    viewport[2] &= 0xfffffffe;
    viewport[3] &= 0xfffffffe;
    if(viewport[2]!=size[0] || viewport[3]!=size[1])
    {
        memory.resize(viewport[2]*viewport[3]*4+sizeof(int)*2);
        size[0] = viewport[2];
        size[1] = viewport[3];
        cout << size[0] << "x" << size[1] << endl;
    }

    char *data = (char*)memory.lock();
    ((int*)data)[0] = size[0];
    ((int*)data)[1] = size[1];

    glReadPixels(0, 0, size[0], size[1], GL_BGRA, GL_UNSIGNED_BYTE, data+sizeof(int)*2);
    memory.unlock();

    _glXSwapBuffers(dpy, drawable);
}
