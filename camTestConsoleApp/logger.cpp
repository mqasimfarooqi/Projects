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

    if (!strcmp(context.file, "")) {
        fileName = strrchr(context.file, '/') ? strrchr(context.file, '/') + 1 : context.file;
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

        if (file.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
            logger::recordCount++;
            stream << "[ " << logger::recordCount << " " << QDateTime::currentDateTime().time().msecsSinceStartOfDay()
                   << " " << QDateTime::currentDateTime().toString() << " ] " << "(" << fileName << " : " << context.function
                   << " : " << context.line << ")" << " " << msg << endl;
            stream.flush();

            if (file.size() > MAX_LOG_FILE_SIZE) {
                if (backup.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
                    if (stream.seek(stream.pos() - MAX_LOG_BACKUP_ENTRIES)) {
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

        if (executeBackup) {
            if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                backup.seek(0);
                stream << backup.readAll();
                file.close();
            }

            backup.close();
        }
    }

    (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}
