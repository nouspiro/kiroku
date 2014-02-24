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
