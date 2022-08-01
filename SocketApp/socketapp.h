#ifndef SOCKETAPP_H
#define SOCKETAPP_H

#include <QWidget>
#include <QThreadPool>
#include <QString>
#include "fileexchanger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SocketApp; }
QT_END_NAMESPACE

class SocketApp : public QWidget
{
    Q_OBJECT

public:
    SocketApp(QWidget *parent = nullptr);
    ~SocketApp();

signals:
    void test();

public slots:
    void receiveStarted();
    void receiveFinished();
    void sendStarted();
    void sendFinished();

private slots:
    void on_pb_send_clicked();

private:
    Ui::SocketApp *ui;
    QThreadPool *threadpool;
};
#endif // SOCKETAPP_H
