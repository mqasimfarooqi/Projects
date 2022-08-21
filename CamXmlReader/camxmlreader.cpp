#include "camxmlreader.h"
#include "ui_camxmlreader.h"
#include "cameraapi.h"

CamXmlReader::CamXmlReader(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CamXmlReader)
{
    ui->setupUi(this);
}

CamXmlReader::~CamXmlReader()
{
    delete ui;
}

void CamXmlReader::on_btn_read_clicked()
{
    cam_api = new cameraApi(&m_sock, "172.19.17.20", CAMERA_API_COMMAND_READXML);
    cam_api->setAutoDelete(true);

    //QThreadPool::globalInstance()->start(cam_api);
    cam_api->run();
}

