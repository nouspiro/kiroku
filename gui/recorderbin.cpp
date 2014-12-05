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

#include "recorderbin.h"
#include <gst/app/gstappsrc.h>
#include <gst/video/video-info.h>
#include <QFile>
#include <QDebug>

extern "C"
{
void needDataCallback(GstAppSrc *src, guint length, gpointer user_data)
{
    Q_UNUSED(src);
    static_cast<VideoBin*>(user_data)->needData(length);
}

void enoughDataCallback(GstAppSrc *src, gpointer user_data)
{
    Q_UNUSED(src);
    static_cast<VideoBin*>(user_data)->enoughData();
}

void fileBinPadAdded(GstElement *element, GstPad *pad, gpointer user_data)
{
    Q_UNUSED(element);
    static_cast<FileBin*>(user_data)->padAdded(pad);
}
}

void SourceBin::setupSrcPad(GstElement *element, const char *name)
{
    GstPad *pad = gst_element_get_static_pad(element, name);
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(pad);
}

void SourceBin::setupRunningSrcPad(GstElement *element, const char *name)
{
    GstPad *pad = gst_element_get_static_pad(element, name);
    GstPad *ghost = gst_ghost_pad_new("src", pad);
    gst_pad_set_active(ghost, TRUE);
    gst_element_add_pad(bin, ghost);
    gst_object_unref(pad);
}

SourceBin::SourceBin(RecorderPipeline *pipeline) : QObject(pipeline)
{
    bin = gst_bin_new(NULL);
    if(pipeline)pipeline->addToPipeline(GST_ELEMENT(gst_object_ref(bin)));
}

SourceBin::~SourceBin()
{
    gst_object_unref(bin);
}

GstPad *SourceBin::getSrcPad()
{
    return gst_element_get_static_pad(bin, "src");
}

VideoBin::VideoBin(int w, int h, RecorderPipeline *pipeline) : SourceBin(pipeline),
    width(w),
    height(h)
{
    appsrc = gst_element_factory_make("appsrc", NULL);
    videorate = gst_element_factory_make("videorate", NULL);
    flip = gst_element_factory_make("videoflip", NULL);
    videoconvert = gst_element_factory_make("videoconvert", NULL);

    GstVideoFormat format = gst_video_format_from_masks(24, 32, G_BIG_ENDIAN, 255<<8, 255<<16, 255<<24, 0);
    gst_video_info_set_format(&info, format, width, height);
    info.fps_n = 25;
    info.fps_d = 1;
    info.par_n = info.par_d = 1;
    GstCaps *caps = gst_video_info_to_caps(&info);

    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);
    GstAppSrcCallbacks callbacks;
    callbacks.enough_data = enoughDataCallback;
    callbacks.need_data = needDataCallback;
    callbacks.seek_data = NULL;
    gst_app_src_set_callbacks(GST_APP_SRC(appsrc), &callbacks, this, NULL);

    g_object_set(appsrc,
                 "stream-type", GST_APP_STREAM_TYPE_STREAM,
                 "format", GST_FORMAT_TIME,
                 "do-timestamp", TRUE,
                 "is-live", TRUE,
                 "min-latency", (gint64)0,
                 "blocksize", (guint)info.size,
                 "max-bytes", (guint64)info.size*30, NULL);


    g_object_set(flip, "method", 5, NULL);

    gst_bin_add_many(GST_BIN(bin), appsrc, videorate, flip, videoconvert, NULL);
    gst_element_link_many(appsrc, videorate, flip, videoconvert, NULL);
    setupSrcPad(videoconvert, "src");
}

VideoBin::~VideoBin()
{
}

void VideoBin::pushFrame(const void *data)
{
    if(enough)return;
    void *tmp = g_malloc(info.size);
    memcpy(tmp, data, info.size);
    GstBuffer *buffer = gst_buffer_new_wrapped(tmp, info.size);
    gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

void VideoBin::needData(guint lenght)
{
    Q_UNUSED(lenght);
    enough = false;
}

void VideoBin::enoughData()
{
    enough = true;
}

AudioBin::AudioBin(const QByteArray &device, RecorderPipeline *pipeline) : SourceBin(pipeline)
{
    src = gst_element_factory_make("pulsesrc", NULL);
    audioconvert = gst_element_factory_make("audioconvert", NULL);
    GstElement *filter = gst_element_factory_make("capsfilter", NULL);

    GstCaps *caps = gst_caps_from_string("audio/x-raw,rate=(int)48000,channels=(int)2");
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    g_object_set(src, "device", device.constData(), NULL);

    gst_bin_add_many(GST_BIN(bin), src, filter, audioconvert, NULL);
    gst_element_link_many(src, filter, audioconvert, NULL);
    setupSrcPad(audioconvert, "src");
}

AudioBin::~AudioBin()
{
}

CameraBin::CameraBin(RecorderPipeline *pipeline) : SourceBin(pipeline)
{
    src = gst_element_factory_make("v4l2src", NULL);
    GstElement *videoconvert = gst_element_factory_make("videoconvert", NULL);

    gst_bin_add_many(GST_BIN(bin), src, videoconvert, NULL);
    gst_element_link(src, videoconvert);
    setupSrcPad(videoconvert, "src");
}

CameraBin::~CameraBin()
{
}

void CameraBin::setDevice(const char *device)
{
    g_object_set(src, "device", device, NULL);
}

FileBin::FileBin(const QString &path, RecorderPipeline *pipeline) : SourceBin(pipeline)
{
    src = gst_element_factory_make("filesrc", NULL);
    decodebin = gst_element_factory_make("decodebin", NULL);
    videoconvert = gst_element_factory_make("videoconvert", NULL);
    imgfreeze = gst_element_factory_make("imagefreeze", NULL);

    gst_bin_add_many(GST_BIN(bin), src, decodebin, videoconvert, imgfreeze, NULL);
    gst_element_link_many(src, decodebin, NULL);
    gst_element_link(videoconvert, imgfreeze);

    QByteArray file = QFile::encodeName(path);
    g_object_set(src, "location", file.constData(), NULL);

    g_signal_connect(decodebin, "pad-added", G_CALLBACK(fileBinPadAdded), this);
    setupSrcPad(imgfreeze, "src");
}

void FileBin::padAdded(GstPad *pad)
{
    GstCaps *caps = gst_pad_get_current_caps(pad);
    gchar *str = gst_caps_to_string(caps);
    qDebug() << str;
    gst_caps_unref(caps);
    g_free(str);

    GstPad *sink = gst_element_get_static_pad(videoconvert, "sink");
    gst_pad_link(pad, sink);
    gst_object_unref(sink);

    g_signal_handlers_disconnect_by_data(decodebin, this);
}

XImageBin::XImageBin(RecorderPipeline *pipeline) : SourceBin(pipeline)
{
    src = gst_element_factory_make("ximagesrc", NULL);
    videoconvert = gst_element_factory_make("videoconvert", NULL);

    gst_bin_add_many(GST_BIN(bin), src, videoconvert, NULL);
    gst_element_link(src, videoconvert);

    setupSrcPad(videoconvert, "src");
}
