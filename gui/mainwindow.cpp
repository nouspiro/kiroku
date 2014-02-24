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

void MainWindow::setupCodecList()
{
    StringPairList videoList = Recorder::codecsForFormat(GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER, "matroskamux");
    StringPairList audioList = Recorder::codecsForFormat(GST_ELEMENT_FACTORY_TYPE_AUDIO_ENCODER, "matroskamux");

    foreach(StringPair pair, videoList)
        ui->videoCodec->addItem(pair.first, pair.second);

    foreach(StringPair pair, audioList)
        ui->audioCodec->addItem(pair.first, pair.second);

    ui->videoCodec->setCurrentIndex(ui->videoCodec->findData(QString("x264enc")));
    ui->audioCodec->setCurrentIndex(ui->audioCodec->findData(QString("voaacenc")));
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
