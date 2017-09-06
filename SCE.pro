#-------------------------------------------------
#
# Project created by QtCreator 2017-09-03T11:58:05
#
#-------------------------------------------------

QT += core gui

CONFIG -= warn
CONFIG += c++1z
CONFIG += strict_c++
CONFIG += debug_and_release

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SCE
TEMPLATE = app

QMAKE_CXXFLAGS += -Wall -Wextra -Werror -pedantic

SOURCES += \
    logic/settings.cpp \
    main.cpp \
    tests/test.cpp \
    tests/test_mainwindow.cpp \
    tests/test_settings.cpp \
    ui/edit_window.cpp \
    ui/mainwindow.cpp \
    ui/tool_editor_widget.cpp

HEADERS += \
    logic/settings.h \
    tests/test.h \
    tests/test_mainwindow.h \
    tests/test_settings.h \
    ui/edit_window.h \
    ui/mainwindow.h \
    ui/tool_editor_widget.h

FORMS += \
    ui/mainwindow.ui \
    ui/tool_editor_widget.ui

OTHER_FILES += \
    .travis.yml