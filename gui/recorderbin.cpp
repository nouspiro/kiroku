#include "recorderbin.h"
#include <gst/app/gstappsrc.h>
#include <gst/video/video-info.h>
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
}

RecorderBin::RecorderBin(RecorderPipeline *pipeline) : QObject(pipeline)
{
    bin = gst_bin_new(NULL);
    if(pipeline)pipeline->addToPipeline(GST_ELEMENT(gst_object_ref(bin)));
}

RecorderBin::~RecorderBin()
{
    gst_object_unref(bin);
}

GstPad *RecorderBin::getSrcPad()
{
    return gst_element_get_static_pad(bin, "src");
}

VideoBin::VideoBin(int w, int h, RecorderPipeline *pipeline) : RecorderBin(pipeline),
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

    gst_app_src_set_caps(GST_APP_SRC_CAST(appsrc), caps);
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
                 "min-latency", 0,
                 "blocksize", info.size, NULL);

    g_object_set(videorate, "average-period", GST_MSECOND*10, NULL);

    g_object_set(flip, "method", 5, NULL);

    gst_bin_add_many(GST_BIN(bin), appsrc, videorate, flip, videoconvert, NULL);
    gst_element_link_many(appsrc, videorate, flip, videoconvert, NULL);
    GstPad *pad = gst_element_get_static_pad(videoconvert, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(pad);
}

VideoBin::~VideoBin()
{
}

void VideoBin::pushFrame(const void *data)
{
    if(enough.load())return;
    void *tmp = g_malloc(info.size);
    memcpy(tmp, data, info.size);
    GstBuffer *buffer = gst_buffer_new_wrapped(tmp, info.size);
    gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

void VideoBin::sendEos()
{
    gst_element_send_event(appsrc, gst_event_new_eos());
}

void VideoBin::needData(guint lenght)
{
    Q_UNUSED(lenght);
    enough.store(0);
}

void VideoBin::enoughData()
{
    enough.store(1);
}

AudioBin::AudioBin(RecorderPipeline *pipeline) : RecorderBin(pipeline)
{
    src = gst_element_factory_make("pulsesrc", NULL);
    audioconvert = gst_element_factory_make("audioconvert", NULL);
    GstElement *filter = gst_element_factory_make("capsfilter", NULL);

    GstCaps *caps = gst_caps_from_string("audio/x-raw,rate=(int)48000,channels=(int)2");
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    g_object_set(src, "device", "alsa_output.pci-0000_00_14.2.analog-stereo.monitor", NULL);

    gst_bin_add_many(GST_BIN(bin), src, filter, audioconvert, NULL);
    gst_element_link_many(src, filter, audioconvert, NULL);
    GstPad *pad = gst_element_get_static_pad(audioconvert, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(pad);
}

AudioBin::~AudioBin()
{
}

void AudioBin::sendEos()
{
    gst_element_send_event(src, gst_event_new_eos());
}
