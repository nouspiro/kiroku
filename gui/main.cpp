#include "mainwindow.h"
#include <QApplication>
#include <GL/glx.h>
#include <gst/gst.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    gst_init(&argc, &argv);
    MainWindow w;
    w.show();

    return a.exec();
}
