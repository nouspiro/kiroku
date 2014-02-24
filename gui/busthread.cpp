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
