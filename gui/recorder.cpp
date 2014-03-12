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

#include "recorder.h"
#include <gst/gstutils.h>
#include <gst/video/videooverlay.h>
#include <QStringList>
#include <QDebug>
#include <QFile>

RecorderSetting::RecorderSetting(int w, int h) :
    width(w),
    heigh(h),
    fps_n(30),
    fps_d(1)
{
}

Recorder::Recorder(QObject *parent) : QObject(parent),
    pipeline(0),
    videoBin(0),
    audioBin(0),
    cameraBin(0),
    compositor(0)
{
}

Recorder::~Recorder()
{
}

void Recorder::initRecorder(RecorderSetting settings)
{
    this->settings = settings;
    setupPipeline();
}

void Recorder::startRecording()
{
    pipeline->start();
}

void Recorder::stopRecording()
{
    if(videoBin)videoBin->sendEos();
    if(audioBin)audioBin->sendEos();
    if(cameraBin)cameraBin->sendEos();
}

void Recorder::pushFrame(const void *data)
{
    if(videoBin)videoBin->pushFrame(data);
}

void Recorder::setVideoOverlay(WId id)
{
    winID = id;
}

StringPairList Recorder::getElementsForCaps(GstElementFactoryListType type, QList<GstCaps*> capsList)
{
    GList *list = gst_element_factory_list_get_elements(type, GST_RANK_NONE);
    StringPairList codecs;

    GList *cur = list;
    while(cur)
    {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(cur->data);
        foreach(GstCaps *caps, capsList)
        {
            if(gst_element_factory_can_src_any_caps(factory, caps))
            {
                const char *longname = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_LONGNAME);
                const char *name = gst_plugin_feature_get_name(factory);
                codecs.append(qMakePair(QString(longname), QString(name)));
                break;
            }
        }
        cur = g_list_next(cur);
    }

    gst_plugin_feature_list_free(list);
    return codecs;
}

StringPairList Recorder::codecsForFormat(GstElementFactoryListType type, const QString &format)
{
    QByteArray f = format.toLatin1();
    StringPairList codecs;

    GstElementFactory *factory = gst_element_factory_find(f.constData());

    const GList *cur, *templates;
    cur = templates = gst_element_factory_get_static_pad_templates(factory);

    QList<GstCaps*> capsList;
    while(cur)
    {
        GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate*)cur->data;
        if(padTemplate->direction==GST_PAD_SINK)
        {
            GstCaps *caps = gst_static_pad_template_get_caps(padTemplate);
            capsList.append(caps);
        }

        cur = g_list_next(cur);
    }

    codecs = getElementsForCaps(type, capsList);

    foreach(GstCaps *caps, capsList)
        gst_caps_unref(caps);

    gst_object_unref(factory);
    return codecs;
}

void Recorder::setupPipeline()
{
    delete videoBin;
    delete audioBin;
    delete cameraBin;
    delete compositor;
    delete pipeline;

    pipeline = new RecorderPipeline(this);
    videoBin = new VideoBin(settings.width, settings.heigh, pipeline);
    audioBin = new AudioBin(settings.audioSource, pipeline);
    //compositor = new VideoCompositor(pipeline);
    //cameraBin = new CameraBin(pipeline);

    GstElement *videoEncoder = gst_element_factory_make(settings.videoCodec.constData(), NULL);
    GstElement *audioEncoder = gst_element_factory_make(settings.audioCodec.constData(), NULL);
    GstElement *mux = gst_element_factory_make("matroskamux", NULL);
    GstElement *sink = gst_element_factory_make("filesink", NULL);
    GstElement *xvsink = gst_element_factory_make("xvimagesink", NULL);
    GstElement *videoTee = gst_element_factory_make("tee", NULL);
    GstElement *queue = gst_element_factory_make("queue", NULL);

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(xvsink), winID);

    pipeline->addToPipeline(videoEncoder, audioEncoder, mux, sink, xvsink, videoTee, queue);

    if(settings.videoCodec=="x264enc")g_object_set(videoEncoder, "speed-preset", 1, "bitrate", 5000, NULL);
    else g_object_set(videoEncoder, "bitrate", 5000000, NULL);

    QByteArray path = QFile::encodeName(settings.outputFile);
    g_object_set(sink, "location", path.data(), NULL);

    g_object_set(xvsink, "sync", FALSE, NULL);

    gst_element_link_many(videoTee, videoEncoder, mux, sink, NULL);
    gst_element_link_many(videoTee, queue, xvsink, NULL);
    gst_element_link(audioEncoder, mux);

    GstPad *srcpad, *sinkpad;

    //compositor->addSource(srcpad);
    //srcpad = cameraBin->getSrcPad();
    //compositor->addSource(srcpad, QRect(0, 0, 640, 480));
    //srcpad = compositor->getSrcPad();

    srcpad = videoBin->getSrcPad();
    sinkpad = gst_element_get_static_pad(videoTee, "sink");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);

    srcpad = audioBin->getSrcPad();
    sinkpad = gst_element_get_static_pad(audioEncoder, "sink");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);
}
