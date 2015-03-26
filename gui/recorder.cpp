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

extern "C" GstPadProbeReturn padProbeCallback(GstPad *, GstPadProbeInfo *, gpointer user_data)
{
    return static_cast<Recorder*>(user_data)->unlinkXvsink();
}

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
    compositor(0),
    xvLinked(false)
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
    pipeline->sendEos();
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

void Recorder::togglePreview()
{
    if(pipeline)
    {
        if(xvLinked)
        {
            gst_pad_add_probe(teePad, GST_PAD_PROBE_TYPE_IDLE, padProbeCallback, this, NULL);
        }
        else
        {
            xvQueue = gst_element_factory_make("queue", NULL);
            xvsink = gst_element_factory_make("xvimagesink", NULL);
            g_object_set(xvsink, "sync", FALSE, "async", TRUE, NULL);
            gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(xvsink), winID);
            pipeline->addToPipeline(static_cast<GstElement*>(gst_object_ref(xvQueue)), static_cast<GstElement*>(gst_object_ref(xvsink)));

            teePad = gst_element_get_request_pad(videoTee, "src_%u");
            GstPad *sinkpad = gst_element_get_static_pad(xvQueue, "sink");
            gst_pad_link(teePad, sinkpad);
            gst_object_unref(sinkpad);
            gst_element_sync_state_with_parent(xvQueue);
            gst_element_sync_state_with_parent(xvsink);
            gst_element_link(xvQueue, xvsink);
            xvLinked = true;
        }
    }
}

GstPadProbeReturn Recorder::unlinkXvsink()
{
    GstPad *sinkpad = gst_element_get_static_pad(xvQueue, "sink");
    gst_pad_unlink(teePad, sinkpad);

    pipeline->removeFromPipeline(xvQueue);
    pipeline->removeFromPipeline(xvsink);

    gst_element_set_state(xvQueue, GST_STATE_NULL);
    gst_element_set_state(xvsink, GST_STATE_NULL);

    gst_element_release_request_pad(videoTee, teePad);

    gst_object_unref(teePad);
    gst_object_unref(sinkpad);
    gst_object_unref(xvQueue);
    gst_object_unref(xvsink);
    xvLinked = false;
    return GST_PAD_PROBE_REMOVE;
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
    //fileBin = new FileBin("/home/nou/Obrázky/nagato.png", pipeline);

    GstElement *videoEncoder = gst_element_factory_make(settings.videoCodec.constData(), NULL);
    GstElement *mux = gst_element_factory_make("matroskamux", NULL);
    GstElement *sink = gst_element_factory_make("filesink", NULL);
    videoTee = gst_element_factory_make("tee", NULL);
    GstElement *filter = gst_element_factory_make("capsfilter", NULL);
    GstElement *queue[3];
    for(int i=0;i<3;i++)
    {
        queue[i] = gst_element_factory_make("queue", NULL);
        pipeline->addToPipeline(queue[i]);
    }

    GstCaps *caps = gst_caps_from_string("video/x-raw,format=I420");
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    pipeline->addToPipeline(videoEncoder, mux, sink, videoTee, filter);

    if(settings.videoCodec=="x264enc")g_object_set(videoEncoder, "speed-preset", 1, "bitrate", 5000, NULL);
    else if(settings.videoCodec=="theoraenc")g_object_set(videoEncoder, "speed-level", 3, NULL);
    else g_object_set(videoEncoder, "bitrate", 50000000, NULL);

    QByteArray path = QFile::encodeName(settings.outputFile);
    g_object_set(sink, "location", path.data(), NULL);

    gst_element_link_many(videoTee, filter, queue[0], videoEncoder, queue[1], mux, sink, NULL);

    GstPad *srcpad, *sinkpad;

//    srcpad = videoBin->getSrcPad();
//    compositor->addSource(srcpad);
//    srcpad = cameraBin->getSrcPad();
//    compositor->addSource(srcpad, QRect(960, 480, 320, 240));
//    srcpad = fileBin->getSrcPad();
//    compositor->addSource(srcpad);

//    srcpad = compositor->getSrcPad();
    srcpad = videoBin->getSrcPad();
    sinkpad = gst_element_get_static_pad(videoTee, "sink");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);

    if(settings.audioCodec=="audio/x-raw")
    {
        sinkpad = gst_element_get_request_pad(mux, "audio_%u");
    }
    else
    {
        GstElement *audioEncoder = gst_element_factory_make(settings.audioCodec.constData(), NULL);
        pipeline->addToPipeline(audioEncoder);
        gst_element_link_many(audioEncoder, queue[2], mux, NULL);
        sinkpad = gst_element_get_static_pad(audioEncoder, "sink");
    }
    srcpad = audioBin->getSrcPad();
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);
}

void Recorder::fileSrcPadAdded()
{
    GstPad *srcpad = fileBin->getSrcPad();
    compositor->addSource(srcpad);
}
