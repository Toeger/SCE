#include "ui/mainwindow.h"
#include "utility/raii.h"

#include <QApplication>
#include <cassert>
#include <csignal>
#include <sce.pb.h>
#include <string_view>

static void broken_pipe_signal_handler(int) {
	//don't do anything in the handler, it just exists so the program doesn't get killed when reading or writing a pipe fails and instead receives an error code
}

int main(int argc, char *argv[]) {
	signal(SIGPIPE, &broken_pipe_signal_handler);

	RAII protobuf_lib{[] { GOOGLE_PROTOBUF_VERIFY_VERSION; }, google::protobuf::ShutdownProtobufLibrary};

	QApplication a{argc, argv};
	MainWindow w;
	w.show();

	return a.exec();
}
