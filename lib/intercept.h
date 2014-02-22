#ifndef INTERCEPT_H
#define INTERCEPT_H

#include <X11/Xlib.h>

typedef XID GLXDrawable;

extern "C"
{
    void glXSwapBuffers(Display *dpy, GLXDrawable drawable);
}

#endif // INTERCEPT_H
