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

#ifndef RECORDER_H
#define RECORDER_H

#include <QWidget>
#include <QObject>
#include <QPair>
#include <gst/gst.h>
#include "recorderpipeline.h"
#include "recorderbin.h"
#include "videocompositor.h"

struct RecorderSetting
{
    int width,heigh;
    int fps_n, fps_d;
    QByteArray videoCodec;
    QByteArray audioCodec;
    QString outputFile;
    QByteArray audioSource;
    RecorderSetting(int w = 0, int h = 0);
};

typedef QPair<QString, QString> StringPair;
typedef QList<StringPair> StringPairList;

class Recorder : public QObject
{
    Q_OBJECT
    RecorderSetting settings;
    RecorderPipeline *pipeline;
    VideoBin *videoBin;
    AudioBin *audioBin;
    CameraBin *cameraBin;
    VideoCompositor *compositor;
    GstElement *videoTee, *audioTee;
    GstElement *xvQueue, *xvsink;
    GstPad *teePad;
    bool xvLinked;
    WId winID;
public:
    explicit Recorder(QObject *parent = 0);
    ~Recorder();
    void initRecorder(RecorderSetting settings);
    void startRecording();
    void stopRecording();
    void pushFrame(const void *data);
    void setVideoOverlay(WId id);

    static StringPairList getElementsForCaps(GstElementFactoryListType type, QList<GstCaps *> capsList);
    static StringPairList codecsForFormat(GstElementFactoryListType type, const QString &format);

    void togglePreview();
    GstPadProbeReturn unlinkXvsink();
protected:
    void setupPipeline();
};

#endif // RECORDER_H
