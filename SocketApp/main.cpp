#include "socketapp.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SocketApp w;
    w.show();
    return a.exec();
}
