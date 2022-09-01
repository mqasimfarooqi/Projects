#include <QCoreApplication>
#include <QDebug>

#ifdef GIT_TRACKED
const QString gitCommitHash = GIT_COMMIT_HASH;
const QString gitCommitDate = GIT_COMMIT_DATE;
const QString gitBranch = GIT_BRANCH;
const QString buildDate = __DATE__;
const QString buildTime = __TIME__;
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef GIT_TRACKED
    qDebug() << "Git Commit Hash = " << gitCommitHash;
    qDebug() << "Git Commit Date = " << gitCommitDate;
    qDebug() << "Git Branch = " << gitBranch;
    qDebug() << "Build Date = " << buildDate;
    qDebug() << "Build Time = " << buildTime;
#else
    qDebug() << "This project is not tracked.";
#endif

    return a.exec();
}
