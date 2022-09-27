QT -= gui

QT += core xml network concurrent

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
unix {
    LIBS += -L$$PWD/quazip -lz
}

system(git):DEFINES += HAS_GIT

#ifdef HAS_GIT
system(git branch):DEFINES += GIT_TRACKED
#endif

#ifdef GIT_TRACKED
GIT_BRANCH=$$system(git branch --show-current)
GIT_COMMIT_HASH=$$system(git log -n 1 --pretty=format:\"%H\")
GIT_COMMIT_DATE=$$system(git log -n 1 --pretty=format:\"%aD\")
DEFINES += "GIT_BRANCH=\"\\\"$${GIT_BRANCH}\\\"\"" \
           "GIT_COMMIT_HASH=\"\\\"$${GIT_COMMIT_HASH}\\\"\"" \
           "GIT_COMMIT_DATE=\"\\\"$${GIT_COMMIT_DATE}\\\"\""
#endif

SOURCES += \
        cameraapi.cpp \
        caminterface.cpp \
        gvcp/gvcp.cpp \
        gvsp/gvsp.cpp \
        main.cpp \
        packethandler.cpp \
        quazip/JlCompress.cpp \
        quazip/qioapi.cpp \
        quazip/quaadler32.cpp \
        quazip/quachecksum32.cpp \
        quazip/quacrc32.cpp \
        quazip/quagzipfile.cpp \
        quazip/quaziodevice.cpp \
        quazip/quazip.cpp \
        quazip/quazipdir.cpp \
        quazip/quazipfile.cpp \
        quazip/quazipfileinfo.cpp \
        quazip/quazipnewinfo.cpp \
        quazip/unzip.c \
        quazip/zip.c

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    cameraapi.h \
    caminterface.h \
    gigev.h \
    gvcp/gvcp.h \
    gvcp/gvcpAckHeaders.h \
    gvcp/gvcpCmdHeaders.h \
    gvcp/gvcpHeaders.h \
    gvsp/gvsp.h \
    gvsp/gvspHeaders.h \
    packethandler.h \
    quazip/JlCompress.h \
    quazip/ioapi.h \
    quazip/minizip_crypt.h \
    quazip/quaadler32.h \
    quazip/quachecksum32.h \
    quazip/quacrc32.h \
    quazip/quagzipfile.h \
    quazip/quaziodevice.h \
    quazip/quazip.h \
    quazip/quazip_global.h \
    quazip/quazip_qt_compat.h \
    quazip/quazipdir.h \
    quazip/quazipfile.h \
    quazip/quazipfileinfo.h \
    quazip/quazipnewinfo.h \
    quazip/unzip.h \
    quazip/zip.h

DISTFILES +=
