#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include <QPair>
#include "recorderpipeline.h"
#include "recorderbin.h"

struct RecorderSetting
{
    int width,heigh;
    int fps_n, fps_d;
    QByteArray videoCodec;
    QByteArray audioCodec;
    QString outputFile;
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
public:
    explicit Recorder(QObject *parent = 0);
    ~Recorder();
    void initRecorder(RecorderSetting settings);
    void startRecording();
    void stopRecording();
    void pushFrame(const void *data);

    static StringPairList getElementsForCaps(GstElementFactoryListType type, QList<GstCaps *> capsList);
    static StringPairList codecsForFormat(GstElementFactoryListType type, const QString &format);
};

#endif // RECORDER_H
