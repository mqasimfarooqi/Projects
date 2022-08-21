#include <QCoreApplication>
#include "cameraapi.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cameraApi *cam_api;
    QUdpSocket m_sock;

    cam_api = new cameraApi(&m_sock, "172.19.17.20", CAMERA_API_COMMAND_READXML);
    cam_api->setAutoDelete(true);

    //QThreadPool::globalInstance()->start(cam_api);
    std::getchar();

    cam_api->run();

    return a.exec();
}
