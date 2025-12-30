QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17\	thread

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS		+= -L../lib -lcyusb -lusb-1.0


INCLUDEPATH += ../include
#unix:!macx: LIBS += -L$$PWD/../../libusb-1.0.27/MinGW64/ -llibusb-1.0

#INCLUDEPATH += $$PWD/../../libusb-1.0.27/MinGW64/static
#DEPENDPATH += $$PWD/../../libusb-1.0.27/MinGW64/static
