#ifndef VIDEOCOMPOSITOR_H
#define VIDEOCOMPOSITOR_H

#include <QObject>
#include <gst/gstelement.h>
#include <QRect>

class RecorderPipeline;

class VideoCompositor : public QObject
{
    Q_OBJECT
    int numSink;
    GstElement *bin;
    GstElement *videomixer;
public:
    explicit VideoCompositor(RecorderPipeline *pipeline);
    ~VideoCompositor();
    void addSource(GstPad *srcPad, QRect rect = QRect());
    GstPad* getSinkPad(int index);
    GstPad* getSrcPad();
};

#endif // VIDEOCOMPOSITOR_H
