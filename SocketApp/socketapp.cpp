#include "socketapp.h"
#include "./ui_socketapp.h"

SocketApp::SocketApp(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SocketApp)
{
    ui->setupUi(this);
    threadpool = new QThreadPool();
    threadpool->setMaxThreadCount(2);

    // Needs to be implemented on button invoke.
    QHostAddress addr;
    addr.setAddress("127.0.0.1");

    FileExchanger *receive = new FileExchanger(addr, false, FileExchanger::protocol::UDP, 1234, "C:/Users/mqasimAdmin/Desktop/Practice/Rampup");
    receive->setAutoDelete(true);
    threadpool->start(receive);

    QThread::currentThread()->sleep(1);
    FileExchanger *send = new FileExchanger(addr, true, FileExchanger::protocol::UDP, 1234, "C:/Users/mqasimAdmin/Desktop/Practice/Rampup");
    send->setAutoDelete(true);
    threadpool->start(send);

    qInfo() << "Running the main thread on " << QThread::currentThread();
}

SocketApp::~SocketApp()
{
    delete ui;
}

