#ifndef RECORDERBIN_H
#define RECORDERBIN_H

#include <QObject>
#include <QAtomicInt>
#include <gst/gstelement.h>
#include <gst/gstbin.h>
#include <gst/video/video-info.h>
#include "recorderpipeline.h"

class RecorderBin : public QObject
{
protected:
    GstElement *bin;
public:
    explicit RecorderBin(RecorderPipeline *pipeline = 0);
    virtual ~RecorderBin();
    GstPad* getSrcPad();
    virtual void sendEos() = 0;
};

class VideoBin : public RecorderBin
{
    int width, height;
    GstVideoInfo info;
    GstElement *appsrc, *videorate, *flip, *videoconvert;
    QAtomicInt enough;
public:
    explicit VideoBin(int w, int h, RecorderPipeline *pipeline = 0);
    ~VideoBin();
    void pushFrame(const void *data);
    void sendEos();
    void needData(guint lenght);
    void enoughData();
};

class AudioBin : public RecorderBin
{
    GstElement *src, *audioconvert;
public:
    explicit AudioBin(RecorderPipeline *pipeline = 0);
    ~AudioBin();
    void sendEos();
};

#endif // RECORDERBIN_H
