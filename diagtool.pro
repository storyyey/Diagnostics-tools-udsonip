QT       += core gui network charts xlsx

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    QtAES/qaesencryption.cpp \
    can.cpp \
    doipclient.cpp \
    fileselect.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    messageview.cpp \
    softupdate.cpp \
    stream.c \
    tabletoxlsx.cpp \
    toolcharhextransform.cpp \
    toolrealtimelog.cpp \
    uds.cpp \
    udsdb.cpp \
    udsprotocolanalysis.cpp \
    upgrade.cpp \
    useauthcode.cpp

HEADERS += \
    QtAES/aesni/aesni-enc-cbc.h \
    QtAES/aesni/aesni-enc-ecb.h \
    QtAES/aesni/aesni-key-exp.h \
    QtAES/aesni/aesni-key-init.h \
    QtAES/qaesencryption.h \
    can.h \
    canframe.h \
    config.h \
    doipclient.h \
    fileselect.h \
    login.h \
    mainwindow.h \
    messageview.h \
    softupdate.h \
    stream.h \
    tabletoxlsx.h \
    toolcharhextransform.h \
    toolrealtimelog.h \
    typedef.h \
    uds.h \
    udsdb.h \
    udsprotocolanalysis.h \
    upgrade.h \
    useauthcode.h \
    zlgcan.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icon.qrc

DISTFILES += \
    icon/delete.png \
    icon/testProcess.gif \
    icon/transfer.png

LIBS += -L./ -lzlgcan

#程序版本
VERSION = 3.1.6
#程序图标
RC_ICONS = car.ico
#产品名称
QMAKE_TARGET_PRODUCT = DiagnosticsTool
#版权所有
QMAKE_TARGET_COPYRIGHT = yeyong

QMAKE_TARGET_DESCRIPTION = DiagnosticsTool

TARGET = DiagnosticsTool.$${VERSION}

FORMS += \
    fileselect.ui \
    login.ui \
    toolcharhextransform.ui \
    toolrealtimelog.ui \
    udsprotocolanalysis.ui \
    upgrade.ui \
    useauthcode.ui
