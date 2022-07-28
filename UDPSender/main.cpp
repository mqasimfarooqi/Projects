#include "udpsender.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    UDPSender w;
    w.show();
    return a.exec();
}
