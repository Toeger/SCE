#define CATCH_CONFIG_RUNNER

#include "test.h"
#include "utility/color.h"
#include "utility/raii.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QtMessageHandler>
#include <cassert>
#include <csignal>
#include <sce.pb.h>
#include <string_view>

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

	return Catch::Session().run(argc, argv);
}