#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "../lib/sharedmemory.h"
#include "recorder.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Ui::MainWindow *ui;
    SharedMemory *mem;
    Recorder *recorder;
    QTimer *timer;
    void setupCodecList();
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void browseDir();
    void startRecording();
    void stopRecording();
    void grabFrame();
};

#endif // MAINWINDOW_H
