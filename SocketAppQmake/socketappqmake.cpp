#include "socketappqmake.h"
#include "ui_socketappqmake.h"

#define MAX_THREADS 3

SocketAppQmake::SocketAppQmake(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SocketAppQmake)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<FileExchanger::protocol>();
    quint8 counter = 0;

    ui->setupUi(this);

    QThread::currentThread()->setObjectName("Main Thread");

    ui->te_status->setPlainText("Initializing...");

    /* Creating threadpool for handling send and receive operations. */
    threadpool = new QThreadPool();
    threadpool->setMaxThreadCount(MAX_THREADS);

    ui->te_status->appendPlainText("Threadpool created with " +
                                   QString::number(MAX_THREADS) +
                                   " threads!");

    /* Populating the combobox. */
    for (counter = 0; counter < metaEnum.keyCount(); counter++) {
        ui->cb_protocol->addItem(metaEnum.valueToKey(counter));
    }

    ui->te_status->appendPlainText("Initialized successfully!");
}

SocketAppQmake::~SocketAppQmake()
{
    delete ui;
}

void SocketAppQmake::slotReceiveStarted()
{
    qInfo() << "Receiving file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Started receiving file");
}

void SocketAppQmake::slotReceiveFinished()
{
    qInfo() << "File received successfully on " << QThread::currentThread();
    ui->te_status->appendPlainText("Finished receiving file");

    /* Enable the send button because the send/receive transaction has been completed at this point. */
    ui->pb_send->setEnabled(true);
}

void SocketAppQmake::slotSendStarted()
{
    qInfo() << "Sending file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Starting to sending file");
}

void SocketAppQmake::slotSendFinished()
{
    qInfo() << "File sent on " << QThread::currentThread();
    ui->te_status->appendPlainText("Finished sending file");
}

void SocketAppQmake::on_pb_send_clicked()
{
    QHostAddress addr;
    bool success = false;

    /* Just protection from continuously pressing send button. */
    ui->pb_send->setEnabled(false);

    ui->te_status->clear();
    ui->le_port->text().toUInt(&success);
    if (!success) {
        ui->te_status->appendPlainText("Incorrect port");
    }

    if (success) {

        addr.setAddress(ui->le_ip->text());
        FileExchanger *receive = new FileExchanger(addr, false, ui->cb_protocol->currentText(),
                                                   ui->le_port->text(), ui->le_receivepath->text());

        FileExchanger *send = new FileExchanger(addr, true, ui->cb_protocol->currentText(),
                                                ui->le_port->text(), ui->le_sendfile->text());

        connect(receive, &FileExchanger::sigReceiveStarted, this,
                &SocketAppQmake::slotReceiveStarted, Qt::QueuedConnection);

        connect(receive, &FileExchanger::sigReceiveFinished, this,
                &SocketAppQmake::slotReceiveFinished, Qt::QueuedConnection);

        connect(send, &FileExchanger::sigSendStarted, this,
                &SocketAppQmake::slotSendStarted, Qt::QueuedConnection);

        connect(send, &FileExchanger::sigSendFinished, this,
                &SocketAppQmake::slotSendFinished, Qt::QueuedConnection);

        receive->setAutoDelete(true);
        threadpool->start(receive);

        send->setAutoDelete(true);
        threadpool->start(send);
    }
}

