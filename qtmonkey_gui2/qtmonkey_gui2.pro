
QSCINTILLAPATH = /home/david/src/as7/trunk/src/3rdparty/qscintilla
QSCINTILLABUILDPATH = /home/david/src/as7/trunk/build/5.9.2/unix/m64/debug
QTMONKEYPATH = $$PWD/..

QT       += core gui sql widgets network
CONFIG += c++11

TARGET = qtmonkey_gui2
TEMPLATE = app

include (qtmonkey_agent.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$QSCINTILLAPATH/Qt4Qt5 \
               $$QTMONKEYPATH \
               $$QTMONKEYPATH/contrib/json11

LIBS += -L$$QSCINTILLABUILDPATH -lqscintilla2
LIBS += -L$$QTMONKEYPATH -lcommon_app_lib

SOURCES += \
        main.cpp \
        testsuitesdlg.cpp \
        database.cpp \
        testsuiteseditdlg.cpp \
        testeditdlg.cpp \
        baseeditdlg.cpp \
        qtmonkeyappctrl.cpp \
        qtmonkey.cpp

HEADERS += \
        testsuitesdlg.h \
        database.h \
        testsuiteseditdlg.h \
        testeditdlg.h \
        baseeditdlg.h \
        qtmonkeyappctrl.h \
        qtmonkey.hpp

FORMS += \
        testsuitesdlg.ui \
        testeditdlg.ui \
        testsuiteseditdlg.ui

RESOURCES += \
        resources/qtmonkey_gui2.qrc
