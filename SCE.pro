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
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG

SOURCES += \
    logic/settings.cpp \
    logic/tool.cpp \
    main.cpp \
    tests/test.cpp \
    tests/test_mainwindow.cpp \
    tests/test_settings.cpp \
    tests/test_tool.cpp \
    tests/test_tool_editor_widget.cpp \
    ui/edit_window.cpp \
    ui/mainwindow.cpp \
    ui/tool_editor_widget.cpp

HEADERS += \
    logic/settings.h \
    logic/tool.h \
    tests/test.h \
    tests/test_mainwindow.h \
    tests/test_settings.h \
    tests/test_tool.h \
    tests/test_tool_editor_widget.h \
    ui/edit_window.h \
    ui/mainwindow.h \
    ui/tool_editor_widget.h

FORMS += \
    ui/mainwindow.ui \
    ui/tool_editor_widget.ui

OTHER_FILES += \
    .travis.yml