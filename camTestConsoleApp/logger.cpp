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
    QString fileName = strrchr(context.file, '/') ? strrchr(context.file, '/') + 1 : context.file;
    QTextStream stream(&file);
    bool fileOpen = false;
    QString txt;

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

        if (!(logger::recordCount % MAX_LOG_ENTRIES)) {
            if (file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
                fileOpen = true;
            }

        } else {
            if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
                fileOpen = true;
            }
        }

        if (fileOpen) {
            logger::recordCount++;
            stream << "[ " << logger::recordCount << " " << QDateTime::currentDateTime().time().msecsSinceStartOfDay()
                   << " ] " << "(" << fileName << " : " << context.line << ")" << " " << txt << endl;
            file.close();
        }
    }

    (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}
