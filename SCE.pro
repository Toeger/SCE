#-------------------------------------------------
#
# Project created by QtCreator 2017-09-03T11:58:05
#
#-------------------------------------------------

QT += core gui widgets

CONFIG += c++1z
CONFIG += strict_c++
CONFIG += debug_and_release

#Prevent these flags from being inserted after our flags and essentially disabling warnings customization
QMAKE_CFLAGS_WARN_ON -= -Wall -W
QMAKE_CXXFLAGS_WARN_ON -= -Wall -W

TARGET = SCE
TEMPLATE = app

#Create variable BUILD for the configuration, should be either "debug" or "release", possibly "profile". It should already exist, but I can't find it.
CONFIG(debug, debug|release) BUILD = debug
CONFIG(release, debug|release) BUILD = release

#Can't find the build directory variable. Should be $${buildDir} but isn't. Here we are using the output dir instead which is wrong.
BUILD_DIR = $${OUT_PWD}

#Add custom target to create protobuffer code
proto.target = $${BUILD_DIR}/sce.pb.cc
proto.depends = $${_PRO_FILE_PWD_}/interop/sce.proto
proto.commands = protoc --proto_path=$${_PRO_FILE_PWD_}/interop --cpp_out=$${BUILD_DIR} $${_PRO_FILE_PWD_}/interop/sce.proto
QMAKE_EXTRA_TARGETS += proto
INCLUDEPATH += $${BUILD_DIR}

#Add custom target to compile protobuffer code with warnings disabled
compiled_proto.target = $${BUILD_DIR}/$${BUILD}/sce.pb.o
compiled_proto.depends = $${BUILD_DIR}/sce.pb.cc
compiled_proto.commands = $${QMAKE_CXX} $(CXXFLAGS) -w $(INCPATH) -c $${compiled_proto.depends} -o $${compiled_proto.target}
QMAKE_EXTRA_TARGETS += compiled_proto
OBJECTS += $${compiled_proto.target}

#Add compiled protobuf as dependency. No clue how to express that, so we just set it as a pre-build depenency.
PRE_TARGETDEPS += $${compiled_proto.target}

QMAKE_CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-braces
QMAKE_CXXFLAGS += -Wno-missing-braces
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
QMAKE_LFLAGS_DEBUG += -fsanitize=undefined,address
unix: LIBS += -lutil
LIBS += -lprotobuf
DEFINES += $$(ENVIRONMENT_DEFINES)
DEFINES += TEST_DATA_PATH=\\\"$${_PRO_FILE_PWD_}/testdata/\\\"

SOURCES += \
    interop/plugin.cpp \
    logic/process_reader.cpp \
    logic/settings.cpp \
    logic/syntax_highligher.cpp \
    logic/tool.cpp \
    logic/tool_actions.cpp \
    main.cpp \
    tests/test.cpp \
    tests/test_mainwindow.cpp \
    tests/test_plugin.cpp \
    tests/test_process_reader.cpp \
    tests/test_settings.cpp \
    tests/test_tool.cpp \
    tests/test_tool_editor_widget.cpp \
    ui/edit_window.cpp \
    ui/mainwindow.cpp \
    ui/tool_editor_widget.cpp \
    utility/thread_call.cpp \
    utility/unique_handle.cpp

HEADERS += \
    interop/plugin.h \
    logic/process_reader.h \
    logic/settings.h \
    logic/syntax_highligher.h \
    logic/tool.h \
    logic/tool_actions.h \
    tests/test.h \
    tests/test_mainwindow.h \
    tests/test_plugin.h \
    tests/test_process_reader.h \
    tests/test_settings.h \
    tests/test_tool.h \
    tests/test_tool_editor_widget.h \
    ui/edit_window.h \
    ui/mainwindow.h \
    ui/tool_editor_widget.h \
    utility/thread_call.h \
    utility/unique_handle.h

FORMS += \
    ui/mainwindow.ui \
    ui/tool_editor_widget.ui

OTHER_FILES += \
    .travis.yml \
    feature_plans.md \
    lsan.supp \
    next_steps.md \
    interop/sce.pb.h

DISTFILES += \
    interop/sce.proto
