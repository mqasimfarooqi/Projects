/********************************************************************************
** Form generated from reading UI file 'socketapp.ui'
**
** Created by: Qt User Interface Compiler version 6.3.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SOCKETAPP_H
#define UI_SOCKETAPP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SocketApp
{
public:
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout;

    void setupUi(QWidget *SocketApp)
    {
        if (SocketApp->objectName().isEmpty())
            SocketApp->setObjectName(QString::fromUtf8("SocketApp"));
        SocketApp->resize(800, 600);
        gridLayoutWidget = new QWidget(SocketApp);
        gridLayoutWidget->setObjectName(QString::fromUtf8("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(120, 100, 421, 361));
        gridLayout = new QGridLayout(gridLayoutWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);

        retranslateUi(SocketApp);

        QMetaObject::connectSlotsByName(SocketApp);
    } // setupUi

    void retranslateUi(QWidget *SocketApp)
    {
        SocketApp->setWindowTitle(QCoreApplication::translate("SocketApp", "SocketApp", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SocketApp: public Ui_SocketApp {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SOCKETAPP_H
