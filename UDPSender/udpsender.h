#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QDialog>
#include <QUdpSocket>
#include <QDebug>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui { class UDPSender; }
QT_END_NAMESPACE

class UDPSender : public QDialog
{
    Q_OBJECT

public:
    UDPSender(QWidget *parent = nullptr);
    ~UDPSender();

public slots:
    void readyRead();

private slots:
    void on_btnSend_clicked();

private:
    Ui::UDPSender *ui;
    QUdpSocket *socket;
};
#endif // UDPSENDER_H
