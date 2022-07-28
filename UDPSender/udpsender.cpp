#include "udpsender.h"
#include "./ui_udpsender.h"

UDPSender::UDPSender(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UDPSender)
{
    ui->setupUi(this);

    /* Setup socket */
    socket = new QUdpSocket(this);
    ui->leSrcIPAddress->setText("127.0.0.1");
    ui->leDstIPAddress->setText("127.0.0.1");
    ui->leSrcPort->setText("1234");
    ui->leDstPort->setText("1234");
    ui->lblStatus->setText("");
}

UDPSender::~UDPSender()
{
    delete ui;
}

void UDPSender::readyRead()
{
    QByteArray buffer = {0};
    buffer.resize(socket->pendingDatagramSize());
    ui->lblStatus->setText("Received");
    socket->readDatagram(buffer.data(), buffer.size());
    qDebug() << buffer.data();
}

void UDPSender::on_btnSend_clicked()
{
    bool ok = true;
    static bool init = false;
    qint64 bytesSent = 0;
    QHostAddress src_address, dst_address;
    QFile xmlFile(ui->lePath->text());
    QByteArray xmlText = {0};

    /* Disconnect if there is any previous connection. */
    socket->disconnectFromHost();

    /* Setup address and port to send data to. */
    if (!(src_address.setAddress(ui->leSrcIPAddress->text()) &&
          dst_address.setAddress(ui->leDstIPAddress->text())))
    {
        ui->lblStatus->setText("Unsuccessful, check IP and Port!");
        ok = false;
    }

    if (!socket->bind(src_address, ui->leSrcPort->text().toInt()))
    {
        ui->lblStatus->setText("Unsuccessful, check src IP and Port!");
        ok = false;
    }

    connect(socket, &QUdpSocket::readyRead, this, &UDPSender::readyRead);

    /* Read file from file path. */
    if (!xmlFile.open(QFile::ReadOnly | QFile::Text))
    {
        ui->lblStatus->setText("File opening unsuccessful!");
        ok = false;
    }

    if (ok)
    {
        xmlText = xmlFile.readAll();
        bytesSent = socket->writeDatagram(xmlText, dst_address, ui->leDstPort->text().toInt());
        if (bytesSent > 0)
            ui->lblStatus->setText("Sent " + QString::number(bytesSent) + " bytes");
        else
            ui->lblStatus->setText("Bytes not sent");

        xmlFile.close();
    }
}

