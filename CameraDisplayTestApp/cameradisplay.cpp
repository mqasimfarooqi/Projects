#include "cameradisplay.h"
#include "ui_cameradisplay.h"

CameraDisplay::CameraDisplay(QWidget *parent) : QMainWindow(parent) , ui(new Ui::CameraDisplay) {
    ui->setupUi(this);
}

CameraDisplay::~CameraDisplay() {
    delete ui;
}


void CameraDisplay::on_pbInitialize_clicked() {
    QList<strGvcpAckDiscoveryHdr> discHeaders;
    cheetah = new cameraApi(QHostAddress("192.168.10.3"));

    cheetah->cameraDiscoverDevice(QHostAddress("255.255.255.255"), discHeaders);
    if (discHeaders.count() > 0) {
        for (int iterator = 0; iterator < discHeaders.count(); iterator++) {
            if (QString::fromLocal8Bit((char *)discHeaders.at(iterator).modelName, 32).contains("POE-C2410C")) {
                cheetah->cameraInitializeDevice(QHostAddress(discHeaders.at(iterator).currentIp));
                break;
            }
        }
    } else {
        qDebug() << "No GigeV devices found.";
    }

    //QOpenGLWindow cameraDisplay;
    //QOpenGLContext context;
    //QOpenGLFunctions *openGLFunctions;
    //QSurfaceFormat formate;

    /* TEST */
//    cameraDisplay.setSurfaceType(QWindow::OpenGLSurface);
//    formate.setProfile(QSurfaceFormat::CompatibilityProfile);
//    formate.setVersion(1,2);
//    cameraDisplay.setFormat(formate);
//    context.setFormat(formate);
//    context.create();
//    context.makeCurrent(&cameraDisplay);

//    openGLFunctions = context.functions();

//    cameraDisplay.show();
    /* TEST */
}

