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
#include "shaders.h"

using namespace std;

typedef void (*PFNGLXSWAPBUFFERSPROC)(Display *dpy, GLXDrawable drawable);
typedef void* (*PFNDLSYMPROC)(void*, const char*);
typedef void* (*PFNGLXGETPROCADDRESSPROC)(const GLubyte*);

static PFNDLSYMPROC o_dlsym = 0;
static PFNGLXGETPROCADDRESSPROC _glXGetProcAddress = 0;
static PFNGLBINDFRAMEBUFFERPROC _glBindFrameBuffer = 0;
static PFNGLGENFRAMEBUFFERSPROC _glGenFramebuffers = 0;
static PFNGLFRAMEBUFFERTEXTURE2DPROC _glFramebufferTexture2D = 0;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC _glCheckFramebufferStatus = 0;
static PFNGLBLITFRAMEBUFFERPROC _glBlitFramebuffer = 0;
static PFNGLGENBUFFERSPROC _glGenBuffers = 0;
static PFNGLBINDBUFFERPROC _glBindBuffer = 0;
static PFNGLMAPBUFFERRANGEPROC _glMapBufferRange = 0;
static PFNGLCREATEPROGRAMPROC _glCreateProgram = 0;
static PFNGLCREATESHADERPROC _glCreateShader = 0;
static PFNGLCOMPILESHADERPROC _glCompileShader = 0;
static PFNGLLINKPROGRAMPROC _glLinkProgram = 0;
static PFNGLUSEPROGRAMPROC _glUseProgram = 0;
static PFNGLSHADERSOURCEPROC _glShaderSource = 0;
static PFNGLATTACHSHADERPROC _glAttachShader = 0;
static PFNGLGETPROGRAMINFOLOGPROC _glGetProgramInfoLog = 0;
static PFNGLGETSHADERINFOLOGPROC _glGetShaderInfoLog = 0;
static PFNGLUNIFORMMATRIX4FVPROC _glUniformMatrix4fv = 0;
static PFNGLUNIFORM1IPROC _glUniform1i = 0;
static PFNGLGETUNIFORMLOCATIONPROC _glGetUniformLocation = 0;
static PFNGLBINDATTRIBLOCATIONPROC _glBindAttribLocation = 0;
static PFNGLBINDFRAGDATALOCATIONPROC _glBindFragDataLocation = 0;
static PFNGLDRAWBUFFERSPROC _glDrawBuffers = 0;

static GLuint fbo, fboTex[2];
static GLuint pbo[2];
static GLuint yuvFbo, yuvTex[3];
static GLuint program, fragmentShader, vertexShader;
static GLint rgb2yuvLocation, texLocation;

float rgb2yuv_mat[] = { 0.182586,  0.614231,  0.062007,  0.062745,
                       -0.100644, -0.338572,  0.439216,  0.501961,
                        0.439216, -0.398942, -0.040274,  0.501961};

extern "C" void *glXGetProcAddress(const GLubyte * str)
{
    if(_glXGetProcAddress==0)
        _glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)o_dlsym(RTLD_NEXT, "glXGetProcAddress");

    if(strcmp((const char*)str, "glXSwapBuffers")==0)return (void*)glXSwapBuffers;
    if(strcmp((const char*)str, "glBindFrameBuffer")==0)return (void*)glBindFrameBuffer;
    if(strcmp((const char*)str, "glBindFrameBufferEXT")==0)return (void*)glBindFrameBufferEXT;
    return _glXGetProcAddress(str);
}

extern "C" void *glXGetProcAddressARB(const GLubyte *str)
{
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

    return (*o_dlsym)(handle, name);
}

void initGLProc()
{
    _glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)_glXGetProcAddress((const GLubyte*)"glGenFramebuffers");
    _glBindFrameBuffer = (PFNGLBINDFRAMEBUFFERPROC)_glXGetProcAddress((const GLubyte*)"glBindFramebuffer");
    _glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)_glXGetProcAddress((const GLubyte*)"glFramebufferTexture2D");
    _glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)_glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatus");
    _glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)_glXGetProcAddress((const GLubyte*)"glBlitFramebuffer");
    _glGenBuffers = (PFNGLGENBUFFERSPROC)_glXGetProcAddress((const GLubyte*)"glGenBuffers");
    _glBindBuffer = (PFNGLBINDBUFFERPROC)_glXGetProcAddress((const GLubyte*)"glBindBuffer");
    _glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)_glXGetProcAddress((const GLubyte*)"glMapBufferRange");
    _glCreateProgram = (PFNGLCREATEPROGRAMPROC)_glXGetProcAddress((const GLubyte*)"glCreateProgram");
    _glCreateShader = (PFNGLCREATESHADERPROC)_glXGetProcAddress((const GLubyte*)"glCreateShader");
    _glCompileShader = (PFNGLCOMPILESHADERPROC)_glXGetProcAddress((const GLubyte*)"glCompileShader");
    _glLinkProgram = (PFNGLLINKPROGRAMPROC)_glXGetProcAddress((const GLubyte*)"glLinkProgram");
    _glUseProgram = (PFNGLUSEPROGRAMPROC)_glXGetProcAddress((const GLubyte*)"glUseProgram");
    _glShaderSource = (PFNGLSHADERSOURCEPROC)_glXGetProcAddress((const GLubyte*)"glShaderSource");
    _glAttachShader = (PFNGLATTACHSHADERPROC)_glXGetProcAddress((const GLubyte*)"glAttachShader");
    _glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)_glXGetProcAddress((const GLubyte*)"glGetProgramInfoLog");
    _glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)_glXGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
    _glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)_glXGetProcAddress((const GLubyte*)"glUniformMatrix4fv");
    _glUniform1i = (PFNGLUNIFORM1IPROC)_glXGetProcAddress((const GLubyte*)"glUniform1i");
    _glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)_glXGetProcAddress((const GLubyte*)"glGetUniformLocation");
    _glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)_glXGetProcAddress((const GLubyte*)"glBindAttribLocation");
    _glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)_glXGetProcAddress((const GLubyte*)"glBindFragDataLocation");
    _glDrawBuffers = (PFNGLDRAWBUFFERSPROC)_glXGetProcAddress((const GLubyte*)"glDrawBuffers");
    //_gl = (PFNGLPROC)_glXGetProcAddress((const GLubyte*)"gl");
    //_gl = (PFNGLPROC)_glXGetProcAddress((const GLubyte*)"gl");
}

void initYuvShader()
{
    program = _glCreateProgram();
    vertexShader = _glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = _glCreateShader(GL_FRAGMENT_SHADER);
    _glShaderSource(vertexShader, 1, &rgb2yuv_source_vertex, NULL);
    _glShaderSource(fragmentShader, 1, &rgb2yuv_source_frag, NULL);
    _glAttachShader(program, vertexShader);
    _glAttachShader(program, fragmentShader);
    _glCompileShader(vertexShader);
    _glCompileShader(fragmentShader);

    _glBindAttribLocation(program, 0, "vertex");
    _glBindFragDataLocation(program, 0, "y");
    _glBindFragDataLocation(program, 1, "u");
    _glBindFragDataLocation(program, 2, "v");
    _glLinkProgram(program);

    rgb2yuvLocation = _glGetUniformLocation(program, "rgb2yuv");
    texLocation = _glGetUniformLocation(program, "tex");

    GLchar log[10000];
    GLsizei len;
    _glGetProgramInfoLog(program, 10000, &len, log);
    cout << "initYuvShader: " << log << texLocation << rgb2yuvLocation <<  endl;
    _glGetShaderInfoLog(vertexShader, 10000, &len, log);
    cout << "initYuvShader: " << log << endl;
    _glGetShaderInfoLog(fragmentShader, 10000, &len, log);
    cout << "initYuvShader: " << log << glGetError() << endl;
}

void initFBO(int w, int h)
{
    GLint fboDrawBinding, texBinding;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fboDrawBinding);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texBinding);

    glBindTexture(GL_TEXTURE_2D, fboTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, fboTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, fbo);
    _glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex[0], 0);
    _glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fboTex[1], 0);

    cout << "initFBO: " << _glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) << " " << GL_FRAMEBUFFER_COMPLETE << endl;

    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, yuvFbo);
    static const GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    _glDrawBuffers(3, drawBuffers);
    for(int i=0;i<3;i++)
    {
        glBindTexture(GL_TEXTURE_2D, yuvTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        _glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, yuvTex[i], 0);
    }

    cout << "initFBO YUV: " << _glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) << " " << GL_FRAMEBUFFER_COMPLETE << " " << fboTex[0] << endl;

    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, fboDrawBinding);
    glBindTexture(GL_TEXTURE_2D, texBinding);
}

extern "C" void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    static PFNGLXSWAPBUFFERSPROC _glXSwapBuffers = 0;
    static GLint size[2] = {0, 0};
    static SharedMemory memory("/kiroku-frame", SharedMemory::Master);

    if(_glXSwapBuffers==0)
    {
        _glXSwapBuffers = (PFNGLXSWAPBUFFERSPROC)o_dlsym(RTLD_NEXT, "glXSwapBuffers");
        initGLProc();
        _glGenFramebuffers(1, &fbo);
        glGenTextures(2, fboTex);
        _glGenFramebuffers(1, &yuvFbo);
        glGenTextures(3, yuvTex);
        cout << "INIT KIROKU " << glGetString(GL_VERSION) << endl;
        initYuvShader();
    }

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    viewport[2] &= 0xfffffffe;
    viewport[3] &= 0xfffffffe;
    if(viewport[2]!=size[0] || viewport[3]!=size[1])
    {
        memory.resize(viewport[2]*viewport[3]*3+sizeof(int)*2);
        size[0] = viewport[2];
        size[1] = viewport[3];
        cout << size[0] << "x" << size[1] << endl;
        initFBO(size[0], size[1]);
    }

    GLint depthTest, programOld, stencilTest, blend;
    glGetIntegerv(GL_DEPTH_TEST, &depthTest);
    glGetIntegerv(GL_STENCIL_TEST, &stencilTest);
    glGetIntegerv(GL_CURRENT_PROGRAM, &programOld);
    glGetIntegerv(GL_BLEND, &blend);
    if(depthTest)glDisable(GL_DEPTH_TEST);
    if(stencilTest)glDisable(GL_STENCIL_TEST);
    if(blend)glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTex[0]);
    _glUseProgram(program);
    _glUniformMatrix4fv(rgb2yuvLocation, 1, GL_TRUE, rgb2yuv_mat);
    _glUniform1i(texLocation, 0);
    _glBindFrameBuffer(GL_READ_FRAMEBUFFER, 0);
    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, yuvFbo);
    //glClear(GL_COLOR_BUFFER_BIT);


    glBegin(GL_QUADS);
    glVertex3f(-1, -1, 0);
    glVertex3f( 1, -1, 0);
    glVertex3f( 1,  1, 0);
    glVertex3f(-1,  1, 0);
    glEnd();

    _glUseProgram(programOld);

    _glBindFrameBuffer(GL_READ_FRAMEBUFFER, fbo);
    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, 0);

    char *data = (char*)memory.lock();
    ((int*)data)[0] = size[0];
    ((int*)data)[1] = size[1];

    for(int i=0;i<3;i++)
    {
        glBindTexture(GL_TEXTURE_2D, yuvTex[i]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, data+(size[0]*size[1]*i)+sizeof(int)*2);
    }
    memory.unlock();

    _glBindFrameBuffer(GL_READ_FRAMEBUFFER, fbo);
    _glBlitFramebuffer(0, 0, size[0], size[1], 0, 0, size[0], size[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
    GLenum err = glGetError();
    if(err)cout << err << endl;

    if(depthTest)glEnable(GL_DEPTH_TEST);
    if(stencilTest)glEnable(GL_STENCIL_TEST);
    if(blend)glEnable(GL_BLEND);

    _glXSwapBuffers(dpy, drawable);
    _glBindFrameBuffer(GL_DRAW_FRAMEBUFFER, fbo);
}

extern "C" void glBindFrameBuffer(GLenum target, GLuint framebuffer)
{
    if(_glBindFrameBuffer==0)
        _glBindFrameBuffer = (PFNGLBINDFRAMEBUFFERPROC)o_dlsym(RTLD_NEXT, "glBindFramebuffer");

    if(framebuffer==0)framebuffer = fbo;
    cout << "FBO BIND" << endl;
    _glBindFrameBuffer(target, framebuffer);
}

extern "C" void glBindFrameBufferEXT(GLenum target, GLuint framebuffer)
{
    cout << "FBO BIND" << endl;
    glBindFrameBuffer(target, framebuffer);
}
