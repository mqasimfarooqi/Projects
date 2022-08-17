QT       += core gui network concurrent core5compat xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#LIBDIR = "./Libs/"
#INCLUDEPATH += $${LIBDIR}
#INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib

#unix {
#    LIB += -L$${LIBDIR} -lz
#}

unix {
    LIBS += -L$$PWD/quazip -lz
}

SOURCES += \
    camera_api.cpp \
    camerainterface.cpp \
    main.cpp \
    camxmlreader.cpp \
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
HEADERS += \
    camera_api.h \
    camerainterface.h \
    camxmlreader.h \
    gigevheaders.h \
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

FORMS += \
    camxmlreader.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
