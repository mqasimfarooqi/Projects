#ifndef CAMERADISPLAY_H
#define CAMERADISPLAY_H

#include <QMainWindow>
#include "cameraapi/cameraapi.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CameraDisplay; }
QT_END_NAMESPACE

class CameraDisplay : public QMainWindow
{
    Q_OBJECT

public:
    CameraDisplay(QWidget *parent = nullptr);
    ~CameraDisplay();

private slots:
    void on_pbInitialize_clicked();

private:
    Ui::CameraDisplay *ui;
    cameraApi *cheetah;
};
#endif // CAMERADISPLAY_H
