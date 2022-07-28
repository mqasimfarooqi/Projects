#include "socketapp.h"
#include "./ui_socketapp.h"

SocketApp::SocketApp(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SocketApp)
{
    ui->setupUi(this);
    threadpool = new QThreadPool();
    threadpool->setMaxThreadCount(3);

    // Needs to be implemented on button invoke.
    QHostAddress addr;
    addr.setAddress("127.0.0.1");
    FileExchanger *send = new FileExchanger(addr, true, true, 1234, "/home/qasim/Desktop/Rampup");
    FileExchanger *receive = new FileExchanger(addr, false, true, 1234, "/home/qasim/Desktop/Recv");

    //send->setAutoDelete(true);
    //receive->setAutoDelete(true);
    threadpool->start(receive);
    threadpool->start(send);
    //qInfo() << "Running the main thread on " << QThread::currentThread();
}

SocketApp::~SocketApp()
{
    delete ui;
}

