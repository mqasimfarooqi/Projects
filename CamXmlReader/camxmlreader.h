#ifndef CAMXMLREADER_H
#define CAMXMLREADER_H

#include <QWidget>
#include <QThreadPool>
#include <QThread>
#include <QUdpSocket>
#include <QRandomGenerator>
#include "cameraapi.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CamXmlReader; }
QT_END_NAMESPACE

class CamXmlReader : public QWidget
{
    Q_OBJECT

public:
    CamXmlReader(QWidget *parent = nullptr);
    ~CamXmlReader();

    /* Public functions. */
    void cam_read_xml();

private slots:
    void on_btn_read_clicked();

private:
    Ui::CamXmlReader *ui;
    cameraApi *cam_api;
    QUdpSocket m_sock;
};
#endif // CAMXMLREADER_H
