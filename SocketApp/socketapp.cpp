#include "socketapp.h"
#include "./ui_socketapp.h"

#define MAX_THREADS 3

SocketApp::SocketApp(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SocketApp)
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

SocketApp::~SocketApp()
{
    delete ui;
}

void SocketApp::slotReceiveStarted()
{
    qInfo() << "Receiving file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Started receiving file");
}

void SocketApp::slotReceiveFinished()
{
    qInfo() << "File received successfully on " << QThread::currentThread();
    ui->te_status->appendPlainText("Finished receiving file");
}

void SocketApp::slotSendStarted()
{
    qInfo() << "Sending file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Starting to sending file");
}

void SocketApp::slotSendFinished()
{
    qInfo() << "File sent on " << QThread::currentThread();
    ui->te_status->appendPlainText("Finished sending file");
}

void SocketApp::on_pb_send_clicked()
{
    QHostAddress addr;
    bool success = false;

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
                &SocketApp::slotReceiveStarted, Qt::QueuedConnection);

        connect(receive, &FileExchanger::sigReceiveFinished, this,
                &SocketApp::slotReceiveFinished, Qt::QueuedConnection);

        connect(send, &FileExchanger::sigSendStarted, this,
                &SocketApp::slotSendStarted, Qt::QueuedConnection);

        connect(send, &FileExchanger::sigSendFinished, this,
                &SocketApp::slotSendFinished, Qt::QueuedConnection);

        receive->setAutoDelete(true);
        threadpool->start(receive);

        send->setAutoDelete(true);
        threadpool->start(send);
    }
}

