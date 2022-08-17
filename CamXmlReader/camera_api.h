#ifndef CAMERA_API_H
#define CAMERA_API_H

#include <QObject>
#include <QRunnable>
#include <QUdpSocket>
#include <QEventLoop>
#include <QMutex>
#include <QString>
#include <QFile>
#include <QtXml>
#include <QThreadPool>
#include "camerainterface.h"
#include "quazip/JlCompress.h"

#define CAMERA_API_COMMAND_READXML (1)

class camera_api : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit camera_api(QObject *parent = nullptr);
    camera_api(QUdpSocket *udp_socket, QString addr, quint8 command);

signals:
    void signal_ack_received();

public slots:
    void slot_ready_read();

public:
    void run();
    quint32 byteArrayToUint32(const QByteArray &bytes);

private:
    /* Private camera functionalities. */
    bool camera_xml_read_xml_file_from_device(QDomDocument& xml_file);
    bool camera_xml_read_camera_attribute(const QString feature, const QDomDocument& xml_file);
    bool camera_fetch_ack(str_non_std_gvcp_ack_hdr& ack_hdr, const quint16 req_id);
    bool camera_read_memory_block(const quint32 address, const quint16 size,
                                  const QHostAddress addr, QByteArray& returned_data);
    bool camera_fetch_first_url(QHostAddress addr, QByteArray& byte_array);
    bool camera_fetch_xml_from_device(QByteArray file_name, QByteArray start_address,
                                      QByteArray size, QHostAddress addr,
                                      QByteArray& xml_data);

    /* Private variables. */
    /* This class will thread send/receive */
    CameraInterface *m_camera;
    QUdpSocket *m_udp_sock;
    QVector<quint16> m_vector_pending_req;
    QString m_addr;
    quint8 m_command;
};

#endif // CAMERA_API_H
