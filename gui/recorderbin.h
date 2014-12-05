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

#ifndef SOURCEBIN_H
#define SOURCEBIN_H

#include <QObject>
#include <QAtomicInt>
#include <gst/gstelement.h>
#include <gst/gstbin.h>
#include <gst/video/video-info.h>
#include "recorderpipeline.h"

class SourceBin : public QObject
{
protected:
    GstElement *bin;
    void setupSrcPad(GstElement *element, const char *name);
    void setupRunningSrcPad(GstElement *element, const char *name);
public:
    explicit SourceBin(RecorderPipeline *pipeline = 0);
    virtual ~SourceBin();
    GstPad* getSrcPad();
};

class VideoBin : public SourceBin
{
    int width, height;
    GstVideoInfo info;
    GstElement *appsrc, *queue, *videorate, *flip, *videoconvert;
    bool enough;
public:
    explicit VideoBin(int w, int h, RecorderPipeline *pipeline = 0);
    ~VideoBin();
    void pushFrame(const void *data);
    void needData(guint lenght);
    void enoughData();
};

class AudioBin : public SourceBin
{
    GstElement *src, *audioconvert;
public:
    explicit AudioBin(const QByteArray &device, RecorderPipeline *pipeline = 0);
    ~AudioBin();
};

class CameraBin : public SourceBin
{
    GstElement *src;
public:
    explicit CameraBin(RecorderPipeline *pipeline = 0);
    ~CameraBin();
    void setDevice(const char *device);
};

class FileBin : public SourceBin
{
    Q_OBJECT
    GstElement *src, *decodebin, *videoconvert, *imgfreeze;
public:
    explicit FileBin(const QString &path, RecorderPipeline *pipeline = 0);
    void sendEos();
    void padAdded(GstPad *pad);
};

class XImageBin : public SourceBin
{
    GstElement *src, *videoconvert;
public:
    XImageBin(RecorderPipeline *pipeline = 0);
};

#endif // SOURCEBIN_H
