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

void SocketApp::receiveStarted()
{
    qInfo() << "Receiving file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Receiving file");
}

void SocketApp::receiveFinished()
{
    qInfo() << "File received successfully on " << QThread::currentThread();
    ui->te_status->appendPlainText("File received successfully");
}

void SocketApp::sendStarted()
{
    qInfo() << "Sending file on " << QThread::currentThread();
    ui->te_status->appendPlainText("Sending file");
}

void SocketApp::sendFinished()
{
    qInfo() << "File sent on " << QThread::currentThread();
    ui->te_status->appendPlainText("File sent");
}

void SocketApp::on_pb_send_clicked()
{
    QHostAddress addr;

    ui->te_status->clear();

    addr.setAddress(ui->le_ip->text());
    FileExchanger *receive = new FileExchanger(addr, false, FileExchanger::protocol::UDP,
                                               1234, ui->le_receivepath->text());

    FileExchanger *send = new FileExchanger(addr, true, FileExchanger::protocol::UDP,
                                            1234, ui->le_sendfile->text());

    connect(receive, &FileExchanger::receiveStarted, this,
            &SocketApp::receiveStarted, Qt::QueuedConnection);

    connect(receive, &FileExchanger::receiveFinished, this,
            &SocketApp::receiveFinished, Qt::QueuedConnection);

    connect(send, &FileExchanger::sendStarted, this,
            &SocketApp::sendStarted, Qt::QueuedConnection);

    connect(send, &FileExchanger::sendFinished, this,
            &SocketApp::sendFinished, Qt::QueuedConnection);

    receive->setAutoDelete(true);
    threadpool->start(receive);

    send->setAutoDelete(true);
    threadpool->start(send);
}

