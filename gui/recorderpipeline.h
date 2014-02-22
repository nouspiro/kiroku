#ifndef RECORDERPIPELINE_H
#define RECORDERPIPELINE_H

#include <QObject>
#include <gst/gstelement.h>
#include "busthread.h"

class RecorderPipeline : public QObject
{
    Q_OBJECT
    GstElement *pipeline;
    BusThread *busThread;
public:
    explicit RecorderPipeline(QObject *parent = 0);
    ~RecorderPipeline();
    void addToPipeline(GstElement *element);
    GstElement *getPipeline();
    void start();
    void stop();
signals:

public slots:
    void onMessage(GstMessage *msg);
};

#endif // RECORDERPIPELINE_H
