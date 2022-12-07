#include "logger.h"

quint64 logger::recordCount = 0;
QString logger::filename = QDir::currentPath() + QDir::separator() + "log.txt";
bool logger::logging = false;
static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER = qInstallMessageHandler(nullptr);

logger::logger(QObject *parent) : QObject{parent} { }

void logger::attach() {
    logger::logging = true;
    qInstallMessageHandler(logger::handler);
}

void logger::handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QFile file(logger::filename);
    QString fileName;
    QTextStream stream(&file);
    bool executeBackup = false;
    QFile backup(file.fileName() + "_backup");
    QString txt;

    /* Guarding condition that context is not null. */
    if (!strcmp(context.file, "")) {
        fileName = strrchr(context.file, '/') ? strrchr(context.file, '/') + 1 : context.file;
    } else {
        fileName = "";
    }

    if (logger::logging) {
        switch (type) {
        case QtInfoMsg:
            txt = QString("Info: %1").arg(msg);
            break;
        case QtDebugMsg:
            txt = QString("Debug: %1").arg(msg);
            break;
        case QtWarningMsg:
            txt = QString("Warning: %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt = QString("Critical: %1").arg(msg);
            break;
        case QtFatalMsg:
            txt = QString("Fatal: %1").arg(msg);
            break;
        }

        /* Open log file. */
        if (file.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
            logger::recordCount++;
            stream << "[ " << logger::recordCount << " " << QDateTime::currentDateTime().time().msecsSinceStartOfDay()
                   << " " << QDateTime::currentDateTime().toString() << " ] " << "(" << fileName << " : " << context.function
                   << " : " << context.line << ")" << " " << msg << endl;
            stream.flush();

            /* Check if the log file is has increased the specified size. */
            if (file.size() > MAX_LOG_FILE_SIZE) {

                /* Open a backup log file. */
                if (backup.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {

                    /* Seek to cursor before a specified number of entries. */
                    if (stream.seek(stream.pos() - MAX_LOG_BACKUP_ENTRIES)) {

                        /* Write into a temporary log file. */
                        backup.write((const char *)QByteArray::fromStdString(stream.read(MAX_LOG_BACKUP_ENTRIES).toStdString()).data(), MAX_LOG_BACKUP_ENTRIES);
                        backup.flush();

                        executeBackup = true;
                    } else {

                        backup.close();
                    }
                }
            }

            file.close();
        }

        /* If backup needs to be created, truncate previous contents of log file and write backup entries. */
        if (executeBackup) {
            if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                backup.seek(0);
                stream << backup.readAll();
                file.close();
            }

            backup.close();
        }
    }

    /* Execute default message handler anyway. */
    (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}
