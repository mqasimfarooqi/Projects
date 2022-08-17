#ifndef SOCKETAPPQMAKE_H
#define SOCKETAPPQMAKE_H

#include <QWidget>
#include <QThreadPool>
#include <QString>
#include "fileexchanger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SocketAppQmake; }
QT_END_NAMESPACE

class SocketAppQmake : public QWidget
{
    Q_OBJECT

public:
    SocketAppQmake(QWidget *parent = nullptr);
    ~SocketAppQmake();

signals:
    void signalTest();

public slots:
    void slotReceiveStarted();
    void slotReceiveFinished();
    void slotSendStarted();
    void slotSendFinished();

private slots:
    void on_pb_send_clicked();

private:
    Ui::SocketAppQmake *ui;
    QThreadPool *threadpool;
};
#endif // SOCKETAPP_H
