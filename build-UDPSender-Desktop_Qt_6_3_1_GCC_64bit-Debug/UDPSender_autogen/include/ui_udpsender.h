/********************************************************************************
** Form generated from reading UI file 'udpsender.ui'
**
** Created by: Qt User Interface Compiler version 6.3.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UDPSENDER_H
#define UI_UDPSENDER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_UDPSender
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QLabel *lblPath;
    QLineEdit *lePath;
    QLabel *lblSrcIPAddress;
    QLineEdit *leSrcIPAddress;
    QLabel *lblDstIPAddress;
    QLineEdit *leDstIPAddress;
    QLabel *lblSrcPort;
    QLineEdit *leSrcPort;
    QLabel *lblDstPort;
    QLineEdit *leDstPort;
    QLabel *lblStatus;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnSend;

    void setupUi(QDialog *UDPSender)
    {
        if (UDPSender->objectName().isEmpty())
            UDPSender->setObjectName(QString::fromUtf8("UDPSender"));
        UDPSender->resize(446, 248);
        verticalLayout = new QVBoxLayout(UDPSender);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lblPath = new QLabel(UDPSender);
        lblPath->setObjectName(QString::fromUtf8("lblPath"));

        gridLayout->addWidget(lblPath, 0, 0, 1, 1);

        lePath = new QLineEdit(UDPSender);
        lePath->setObjectName(QString::fromUtf8("lePath"));

        gridLayout->addWidget(lePath, 0, 1, 1, 1);

        lblSrcIPAddress = new QLabel(UDPSender);
        lblSrcIPAddress->setObjectName(QString::fromUtf8("lblSrcIPAddress"));

        gridLayout->addWidget(lblSrcIPAddress, 1, 0, 1, 1);

        leSrcIPAddress = new QLineEdit(UDPSender);
        leSrcIPAddress->setObjectName(QString::fromUtf8("leSrcIPAddress"));

        gridLayout->addWidget(leSrcIPAddress, 1, 1, 1, 1);

        lblDstIPAddress = new QLabel(UDPSender);
        lblDstIPAddress->setObjectName(QString::fromUtf8("lblDstIPAddress"));

        gridLayout->addWidget(lblDstIPAddress, 2, 0, 1, 1);

        leDstIPAddress = new QLineEdit(UDPSender);
        leDstIPAddress->setObjectName(QString::fromUtf8("leDstIPAddress"));

        gridLayout->addWidget(leDstIPAddress, 2, 1, 1, 1);

        lblSrcPort = new QLabel(UDPSender);
        lblSrcPort->setObjectName(QString::fromUtf8("lblSrcPort"));

        gridLayout->addWidget(lblSrcPort, 3, 0, 1, 1);

        leSrcPort = new QLineEdit(UDPSender);
        leSrcPort->setObjectName(QString::fromUtf8("leSrcPort"));

        gridLayout->addWidget(leSrcPort, 3, 1, 1, 1);

        lblDstPort = new QLabel(UDPSender);
        lblDstPort->setObjectName(QString::fromUtf8("lblDstPort"));

        gridLayout->addWidget(lblDstPort, 4, 0, 1, 1);

        leDstPort = new QLineEdit(UDPSender);
        leDstPort->setObjectName(QString::fromUtf8("leDstPort"));

        gridLayout->addWidget(leDstPort, 4, 1, 1, 1);


        verticalLayout->addLayout(gridLayout);

        lblStatus = new QLabel(UDPSender);
        lblStatus->setObjectName(QString::fromUtf8("lblStatus"));

        verticalLayout->addWidget(lblStatus);

        horizontalSpacer = new QSpacerItem(425, 16, QSizePolicy::Expanding, QSizePolicy::Minimum);

        verticalLayout->addItem(horizontalSpacer);

        btnSend = new QPushButton(UDPSender);
        btnSend->setObjectName(QString::fromUtf8("btnSend"));

        verticalLayout->addWidget(btnSend);


        retranslateUi(UDPSender);

        QMetaObject::connectSlotsByName(UDPSender);
    } // setupUi

    void retranslateUi(QDialog *UDPSender)
    {
        UDPSender->setWindowTitle(QCoreApplication::translate("UDPSender", "UDPSender", nullptr));
        lblPath->setText(QCoreApplication::translate("UDPSender", "File Path", nullptr));
        lblSrcIPAddress->setText(QCoreApplication::translate("UDPSender", "Src IPv4 Addr", nullptr));
        lblDstIPAddress->setText(QCoreApplication::translate("UDPSender", "Dst IPv4 Addr", nullptr));
        lblSrcPort->setText(QCoreApplication::translate("UDPSender", "Src Port", nullptr));
        lblDstPort->setText(QCoreApplication::translate("UDPSender", "Dst Port", nullptr));
        lblStatus->setText(QCoreApplication::translate("UDPSender", "TextLabel", nullptr));
        btnSend->setText(QCoreApplication::translate("UDPSender", "Send", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UDPSender: public Ui_UDPSender {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UDPSENDER_H
