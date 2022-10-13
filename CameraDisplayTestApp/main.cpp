#include "cameradisplay.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CameraDisplay w;
    w.show();
    return a.exec();
}
