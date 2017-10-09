#-------------------------------------------------
#
# Project created by QtCreator 2017-09-03T11:58:05
#
#-------------------------------------------------

QT += core gui

CONFIG += c++1z
CONFIG += strict_c++
CONFIG += debug_and_release

#These flags are inserted after our flags and essentially disable warnings customization
QMAKE_CFLAGS_WARN_ON -= -Wall -W
QMAKE_CXXFLAGS_WARN_ON -= -Wall -W

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SCE
TEMPLATE = app

QMAKE_CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-braces
QMAKE_CXXFLAGS += -Wno-missing-braces
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
QMAKE_LFLAGS_DEBUG += -fsanitize=undefined,address
unix: LIBS += -lutil

SOURCES += \
    logic/process_reader.cpp \
    logic/settings.cpp \
    logic/syntax_highligher.cpp \
    logic/tool.cpp \
    logic/tool_actions.cpp \
    main.cpp \
    tests/test.cpp \
    tests/test_mainwindow.cpp \
    tests/test_process_reader.cpp \
    tests/test_settings.cpp \
    tests/test_tool.cpp \
    tests/test_tool_editor_widget.cpp \
    ui/edit_window.cpp \
    ui/mainwindow.cpp \
    ui/tool_editor_widget.cpp

HEADERS += \
    logic/process_reader.h \
    logic/settings.h \
    logic/syntax_highligher.h \
    logic/tool.h \
    logic/tool_actions.h \
    tests/test.h \
    tests/test_mainwindow.h \
    tests/test_process_reader.h \
    tests/test_settings.h \
    tests/test_tool.h \
    tests/test_tool_editor_widget.h \
    ui/edit_window.h \
    ui/mainwindow.h \
    ui/tool_editor_widget.h \
    utility/unique_handle.hpp

FORMS += \
    ui/mainwindow.ui \
    ui/tool_editor_widget.ui

OTHER_FILES += \
    .travis.yml

DISTFILES += \
    lsan.supp