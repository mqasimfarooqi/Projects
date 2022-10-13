#ifndef CAMERAGLWIDGET_H
#define CAMERAGLWIDGET_H

#include <QOpenGLWidget>
#include <QWidget>

class CameraGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    CameraGLWidget(QWidget* parent = nullptr);

private:

};

#endif // CAMERAGLWIDGET_H
