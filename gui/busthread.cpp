#include "busthread.h"

BusThread::BusThread(GstBus *bus) :
    bus(bus)
{
    thread = new QThread();
    moveToThread(thread);
    connect(thread, SIGNAL(started()), this, SLOT(run()));
    thread->start();
}

BusThread::~BusThread()
{
    quit = true;
    GstMessage *msg = gst_message_new_application(NULL, gst_structure_new_empty("quit"));
    gst_bus_post(bus, msg);
    thread->wait(1000);
    delete thread;
    gst_object_unref(bus);
}

void BusThread::run()
{
    while(quit)
    {
        GstMessage *msg = gst_bus_pop(bus);
        if(msg)
        {
            if(msg->type==GST_MESSAGE_APPLICATION)
            {
                gst_message_unref(msg);
                break;
            }
            emit newMessage(msg);
        }
    }
    thread->exit();
}
