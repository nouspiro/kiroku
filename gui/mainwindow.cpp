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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QRegExp>

void MainWindow::setupCodecList()
{
    StringPairList videoList = Recorder::codecsForFormat(GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER, "matroskamux");
    StringPairList audioList = Recorder::codecsForFormat(GST_ELEMENT_FACTORY_TYPE_AUDIO_ENCODER, "matroskamux");

    foreach(StringPair pair, videoList)
        ui->videoCodec->addItem(pair.first, pair.second.toLatin1());

    foreach(StringPair pair, audioList)
        ui->audioCodec->addItem(pair.first, pair.second.toLatin1());

    ui->videoCodec->setCurrentIndex(ui->videoCodec->findData(QString("x264enc")));
    ui->audioCodec->setCurrentIndex(ui->audioCodec->findData(QString("voaacenc")));
}

void MainWindow::setupAudioInputs()
{
    QProcess pactl;
    QStringList args;
    args << "list" << "short" << "sources";
    pactl.start("pactl", args);
    if(!pactl.waitForFinished())
    {
        QMessageBox::critical(this, tr("Can't list PulseAudio sources"), tr("Can't get list of input audio sources. Check if pactl utility is installed."));
        return;
    }

    QStringList out = QString(pactl.readAll()).split('\n');
    QRegExp regexp("\\t([^\\s]+)\\s");

    QStringList sources;
    foreach(QString devName, out)
    {
        if(devName.isEmpty())continue;
        regexp.indexIn(devName);
        sources << regexp.cap(1);
    }

    GstElement *pipeline, *device, *fakesink;
    pipeline = gst_pipeline_new(NULL);
    device = gst_element_factory_make("pulsesrc", NULL);
    fakesink = gst_element_factory_make("fakesink", NULL);
    gst_bin_add_many(GST_BIN(pipeline), device, fakesink, NULL);
    gst_element_link(device, fakesink);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

    foreach(QString source, sources)
    {
        gchar *deviceName;
        QByteArray devstring = source.toLatin1();
        g_object_set(G_OBJECT(device), "device", devstring.data(), NULL);
        g_object_get(device, "device-name", &deviceName, NULL);
        ui->audioInput->addItem(deviceName, source.toLatin1());
        g_free(deviceName);
    }
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    timer = new QTimer(this);
    mem = new SharedMemory("/kiroku-frame", SharedMemory::Slave);
    connect(timer, SIGNAL(timeout()), this, SLOT(grabFrame()));

    ui->outputDir->setText(QDir::homePath()+"/Video");

    setupCodecList();
    setupAudioInputs();

    recorder = new Recorder(this);
}

MainWindow::~MainWindow()
{
    delete mem;
    delete ui;
}

void MainWindow::browseDir()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select output dir"), ui->outputDir->text());
    if(!path.isEmpty())
        ui->outputDir->setText(path);
}

void MainWindow::startRecording()
{
    int x,y;
    mem->autoResize();
    uchar *data = (uchar*)mem->lock();
    x = ((int*)data)[0];
    y = ((int*)data)[1];
    mem->unlock();

    RecorderSetting settings(x, y);
    settings.videoCodec = ui->videoCodec->itemData(ui->videoCodec->currentIndex()).toByteArray();
    settings.audioCodec = ui->audioCodec->itemData(ui->audioCodec->currentIndex()).toByteArray();
    settings.outputFile = QDir(ui->outputDir->text()).filePath("out.mkv");
    settings.audioSource = ui->audioInput->itemData(ui->audioInput->currentIndex()).toByteArray();
    recorder->initRecorder(settings);
    recorder->startRecording();
    timer->start(40);

}

void MainWindow::stopRecording()
{
    recorder->stopRecording();
    timer->stop();
}

void MainWindow::grabFrame()
{
    uchar *data = (uchar*)mem->lock();
    recorder->pushFrame(data+sizeof(int)*2);
    mem->unlock();
}
