#define CATCH_CONFIG_RUNNER

#include "test.h"
#include "test_mainwindow.h"
#include "threading/thread_call.h"
#include "ui/mainwindow.h"
#include "utility/color.h"
#include "utility/raii.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QtMessageHandler>
#include <cassert>
#include <csignal>
#include <functional>
#include <iostream>
#include <sce.pb.h>
#include <string_view>

MainWindow_tester *test_main_window;

static void broken_pipe_signal_handler(int) {
	//don't do anything in the handler, it just exists so the program doesn't get killed when reading or writing a pipe fails and instead receives an error code
}

static QtMessageHandler old_handler;

static std::ostream &operator<<(std::ostream &os, QtMsgType type) {
    switch (type) {
        case QtCriticalMsg:
            return os << "Critical Error";
        case QtFatalMsg:
            return os << "Fatal Error";
        case QtWarningMsg:
            return os << "Warning";
        case QtDebugMsg:
            return os << "Debug Message";
        case QtInfoMsg:
            return os << "Info Message";
    }
    return os << "Unknown Message Type";
}

static void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::cerr << msg.toStdString() << '\n';
    std::cerr << type << " in " << context.file << ':' << context.line << '\n';
	switch (type) {
        case QtCriticalMsg:
        case QtFatalMsg:
            __builtin_trap();
		case QtWarningMsg:
		case QtDebugMsg:
		case QtInfoMsg:;
	}
	old_handler(type, context, msg);
}

int main(int argc, char *argv[]) {
	signal(SIGPIPE, &broken_pipe_signal_handler);

	RAII protobuf_lib{[] { GOOGLE_PROTOBUF_VERIFY_VERSION; }, google::protobuf::ShutdownProtobufLibrary};

	old_handler = qInstallMessageHandler(message_handler);

	QApplication a{argc, argv};
	MainWindow_tester mw;
	test_main_window = &mw;
	mw.close_notification_server();
	mw.close_rpc_server();

	auto tester = std::async(std::launch::async, [&] {
		Catch::Session().run(argc, argv);
		Utility::sync_gui_thread_execute([&mw] { mw.close(); });
	});
	a.exec();
	Utility::get_future_value(std::move(tester));
}
