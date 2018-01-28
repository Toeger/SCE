#-------------------------------------------------
#
# Project created by QtCreator 2017-09-03T11:58:05
#
#-------------------------------------------------

QT += core gui widgets

CONFIG += c++1z
CONFIG += strict_c++
CONFIG += debug_and_release

#These flags are inserted after our flags and essentially disable warnings customization
QMAKE_CFLAGS_WARN_ON -= -Wall -W
QMAKE_CXXFLAGS_WARN_ON -= -Wall -W

TARGET = SCE
TEMPLATE = app

proto.target = $$_PRO_FILE_PWD_/interop/sce.pb.cc
proto.depends = $$_PRO_FILE_PWD_/interop/sce.proto
proto.commands = protoc --proto_path=$$_PRO_FILE_PWD_/interop --cpp_out=$$_PRO_FILE_PWD_/interop $$_PRO_FILE_PWD_/interop/sce.proto
PRE_TARGETDEPS += $$_PRO_FILE_PWD_/interop/sce.pb.cc
QMAKE_EXTRA_TARGETS += proto

generated_sources.name = generated_sources
generated_sources.input = $$_PRO_FILE_PWD_/interop/sce.pb.cc
generated_sources.dependency_type = TYPE_C
generated_sources.variable_out = OBJECTS
generated_sources.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
generated_sources.commands = $${QMAKE_CXX} $(CXXFLAGS) -w $(INCPATH) -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += generated_sources

QMAKE_CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-braces
QMAKE_CXXFLAGS += -Wno-missing-braces
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
QMAKE_LFLAGS_DEBUG += -fsanitize=undefined,address
unix: LIBS += -lutil
LIBS += -lprotobuf
DEFINES += $$(ENVIRONMENT_DEFINES)
DEFINES += TEST_DATA_PATH=\\\"$$_PRO_FILE_PWD_/testdata/\\\"

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
