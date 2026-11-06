QT += core gui quick quickcontrols2 svg testlib dbus network
CONFIG += c++17 testcase
TARGET = plustv_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/theme.h ../src/translator.h
SOURCES += plustv_tests.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp
