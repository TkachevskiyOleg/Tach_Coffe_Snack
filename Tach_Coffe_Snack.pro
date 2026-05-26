QT       += core gui
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gpio_listener.cpp \
    machinesettings.cpp \
    main.cpp \
    mainwindow.cpp \
    pay_price.cpp \
    pulses.cpp \
    rp2040device.cpp \
    settingswindow.cpp

HEADERS += \
    gpio_listener.h \
    machinesettings.h \
    mainwindow.h \
    pay_price.h \
    pulses.h \
    rp2040device.h \
    settingswindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    Tach_Coffe_Snack_uk_UA.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

LIBS += -lgpiod

