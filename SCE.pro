#-------------------------------------------------
#
# Project created by QtCreator 2017-09-03T11:58:05
#
#-------------------------------------------------

QT       += core gui

CONFIG -= warn
CONFIG += c++1z
CONFIG += strict_c++

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SCE
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CXXFLAGS += -Wall -Wextra -Werror -pedantic
QMAKE_CXXFLAGS += -std=c++1z

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

OTHER_FILES += \
        .travis.yml