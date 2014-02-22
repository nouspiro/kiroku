#ifndef BUSTHREAD_H
#define BUSTHREAD_H

#include <QObject>
#include <QThread>
#include <gst/gst.h>

class BusThread : public QObject
{
    Q_OBJECT
    bool quit;
    QThread *thread;
    GstBus *bus;
public:
    explicit BusThread(GstBus *bus);
    ~BusThread();
signals:
    void newMessage(GstMessage *msg);
public slots:
    void run();

};

#endif // BUSTHREAD_H
