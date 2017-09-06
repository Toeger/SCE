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
    main.cpp \
    mainwindow.cpp \
    tests/test_mainwindow.cpp \
    tests/test.cpp \
    settings.cpp \
    tests/test_settings.cpp \
    edit_window.cpp \
    tool_editor_widget.cpp

HEADERS += \
    mainwindow.h \
    tests/test_mainwindow.h \
    tests/test.h \
    settings.h \
    tests/test_settings.h \
    edit_window.h \
    tool_editor_widget.h

FORMS += \
    mainwindow.ui \
    tool_editor_widget.ui

OTHER_FILES += \
    .travis.yml