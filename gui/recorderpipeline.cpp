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

#include "recorderpipeline.h"
#include <QDebug>
#include <gst/gstpipeline.h>
#include <gst/gstdebugutils.h>

const char* stateToString(GstState state)
{
    switch(state)
    {
    case GST_STATE_VOID_PENDING:
        return "STATE_VOID_PENDING";
    case GST_STATE_NULL:
        return "STATE_NULL";
    case GST_STATE_READY:
        return "STATE_READY";
    case GST_STATE_PAUSED:
        return "STATE_PAUSED";
    case GST_STATE_PLAYING:
        return "STATE_PLAYING";
    }
    return "";
}

RecorderPipeline::RecorderPipeline(QObject *parent) : QObject(parent)
{
    pipeline = gst_pipeline_new(NULL);

    busThread = new BusThread(gst_pipeline_get_bus(GST_PIPELINE(pipeline)));
    connect(busThread, SIGNAL(newMessage(GstMessage*)), this, SLOT(onMessage(GstMessage*)));
}

RecorderPipeline::~RecorderPipeline()
{
    delete busThread;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void RecorderPipeline::addToPipeline(GstElement *element)
{
    gst_bin_add(GST_BIN(pipeline), element);
}

GstElement *RecorderPipeline::getPipeline()
{
    return pipeline;
}

void RecorderPipeline::start()
{
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void RecorderPipeline::stop()
{
}

void RecorderPipeline::onMessage(GstMessage *msg)
{
    switch(GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_STATE_CHANGED:
        if(msg->src==GST_OBJECT(pipeline))
        {
            GstState oldState, newState;
            gst_message_parse_state_changed(msg, &oldState, &newState, NULL);
            qDebug() << "new state" << stateToString(newState);
            //if(newState==GST_STATE_PLAYING)
                //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "recorder");
            break;
        }
        break;
    case GST_MESSAGE_EOS:
    {
        gchar *name = gst_object_get_name(msg->src);
        qDebug() << "EOS" << name;
        g_free(name);
        break;
    }
    default:
        break;
    }
    gst_message_unref(msg);
}
