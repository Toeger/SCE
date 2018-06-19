#include "test.h"
#include "test_mainwindow.h"
#include "test_notification_server.h"
#include "test_process_reader.h"
#include "test_rpc_server.h"
#include "test_settings.h"
#include "test_tool.h"
#include "test_tool_editor_widget.h"
#include "utility/raii.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QtMessageHandler>
#include <cassert>
#include <csignal>
#include <sce.pb.h>
#include <string_view>

thread_local std::stringstream detail::assert_message;
thread_local bool detail::expect_fail;

static void broken_pipe_signal_handler(int) {
	//don't do anything in the handler, it just exists so the program doesn't get killed when reading or writing a pipe fails and instead receives an error code
}

static QtMessageHandler old_handler;

static void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	switch (type) {
		case QtCriticalMsg:
		case QtFatalMsg:
		case QtWarningMsg:
			__builtin_trap();
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

#define TESTS_LIST                                                                                                                                             \
	X(test_notification_server), X(test_rpc_server), X(test_process_reader), X(test_settings), X(test_tool), X(test_tool_editor_widget), X(test_mainwindow),

#define X(TEST)                                                                                                                                                \
	{ &TEST, #TEST }
	struct {
		void (*test_function)();
		const char *test_name;
	} const tests[] = {TESTS_LIST};
	for (const auto &test : tests) {
		std::clog << "Running " << test.test_name;
		const auto start = std::chrono::high_resolution_clock::now();
		test.test_function();
		std::clog << " (" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << "ms)\n";
	}
}