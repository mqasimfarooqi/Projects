#include "camxmlreader.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CamXmlReader w;
    w.show();
    return a.exec();
}
