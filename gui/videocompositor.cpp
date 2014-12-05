#include "videocompositor.h"
#include "recorderpipeline.h"

VideoCompositor::VideoCompositor(RecorderPipeline *pipeline) :
    numSink(0)
{
    bin = gst_bin_new(NULL);
    pipeline->addToPipeline(GST_ELEMENT(gst_object_ref(bin)));

    videomixer = gst_element_factory_make("videomixer", NULL);
    gst_bin_add(GST_BIN(bin), videomixer);

    g_object_set(videomixer, "background", 1, NULL);

    GstPad *pad = gst_element_get_static_pad(videomixer, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("src", pad));
    gst_object_unref(pad);
}

VideoCompositor::~VideoCompositor()
{
    gst_object_unref(bin);
}

void VideoCompositor::addSource(GstPad *srcPad, QRect rect)
{
    GstPad *sinkPad, *targetPad;
    QByteArray padName = QString("sink_%1").arg(numSink++).toLatin1();
    GstElement *queue = gst_element_factory_make("queue", NULL);
    targetPad = gst_element_get_static_pad(queue, "sink");

    GstElement *filter = gst_element_factory_make("capsfilter", NULL);
    QByteArray capsString = QByteArray("video/x-raw,format=I420");
    if(rect.isValid())capsString.append(QString(",width=(int)%1,height=(int)%2").arg(rect.width()).arg(rect.height()).toLatin1());
    GstCaps *caps = gst_caps_from_string(capsString.constData());
    g_object_set(filter, "caps", caps, NULL);

    gst_bin_add_many(GST_BIN(bin), queue, filter, NULL);
    gst_caps_unref(caps);

    if(rect.isValid())
    {
        GstElement *videoscale = gst_element_factory_make("videoscale", NULL);
        gst_bin_add(GST_BIN(bin), videoscale);
        gst_element_link_many(queue, videoscale, filter, videomixer, NULL);
        GstPad *mixPad = gst_element_get_static_pad(videomixer, padName.constData());
        g_object_set(mixPad, "xpos", rect.x(), "ypos", rect.y(), NULL);
        gst_object_unref(mixPad);
    }
    else
    {
        gst_element_link_many(queue, filter, videomixer, NULL);
    }

    GstState state;
    gst_element_get_state(bin, &state, NULL, GST_SECOND);
    GstPad *ghost = gst_ghost_pad_new(padName.constData(), targetPad);
    if(state==GST_STATE_PLAYING)gst_pad_set_active(ghost, TRUE);
    gst_element_add_pad(bin, ghost);
    sinkPad = gst_element_get_static_pad(bin, padName.constData());
    gst_pad_link(srcPad, sinkPad);
    gst_object_unref(sinkPad);
    gst_object_unref(targetPad);
    gst_object_unref(srcPad);
}

GstPad* VideoCompositor::getSinkPad(int index)
{
    if(index>=numSink)return 0;
    QByteArray padName = QString("sink_%1").arg(numSink++).toLatin1();
    return gst_element_get_static_pad(bin, padName.constData());
}

GstPad* VideoCompositor::getSrcPad()
{
    return gst_element_get_static_pad(bin, "src");
}
