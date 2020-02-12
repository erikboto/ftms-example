TEMPLATE = app
TARGET = ftms-example

QT = core bluetooth
CONFIG += c++11

SOURCES += main.cpp \
    ftmsdevice.cpp

INSTALLS += target

HEADERS += \
    ftmsdevice.h
