#ifndef SOCKETAPP_H
#define SOCKETAPP_H

#include <QWidget>
#include <QThreadPool>
#include "fileexchanger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SocketApp; }
QT_END_NAMESPACE

class SocketApp : public QWidget
{
    Q_OBJECT

public:
    SocketApp(QWidget *parent = nullptr);
    ~SocketApp();

private:
    Ui::SocketApp *ui;
    QThreadPool *threadpool;
};
#endif // SOCKETAPP_H
