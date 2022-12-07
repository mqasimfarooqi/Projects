#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <iostream>
#include <QTextStream>
#include <QElapsedTimer>
#include <QThread>

#define MAX_LOG_FILE_SIZE (1500000)
#define MAX_LOG_BACKUP_ENTRIES (10000)

class logger : public QObject
{
    Q_OBJECT
public:
    explicit logger(QObject *parent = nullptr);

    /* Public variables for logging control. */
    static bool logging;
    static quint64 recordCount;
    static QString filename;

    /* Public log functions. */
    static void attach();
    static void handler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

#endif // LOGGER_H
