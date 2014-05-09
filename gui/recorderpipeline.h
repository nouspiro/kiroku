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
    template<typename ... T>
    void addToPipeline(GstElement *element, T ... rest)
    {
        addToPipeline(element);
        addToPipeline(rest...);
    }
    void removeFromPipeline(GstElement *element);
    void start();
    void stop();
    void dumpDot();
signals:

public slots:
    void onMessage(GstMessage *msg);
};

#endif // RECORDERPIPELINE_H
