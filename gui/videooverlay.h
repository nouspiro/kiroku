#ifndef VIDEOOVERLAY_H
#define VIDEOOVERLAY_H

#include <QWidget>

class VideoOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit VideoOverlay(QWidget *parent = 0);
};

#endif // VIDEOOVERLAY_H
