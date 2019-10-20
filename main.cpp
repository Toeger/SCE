#include "ui/mainwindow.h"
#include "utility/raii.h"

#include <QApplication>
#include <cassert>
#include <csignal>
#include <sce.pb.h>
#include <string_view>

int main(int argc, char *argv[]) {
    //don't do anything in the handler, it just exists so the program doesn't get killed when reading or writing a pipe fails and instead receives an error code
    signal(SIGPIPE, [](int) {});
    //hopefully disable spurious aborts on travis
    signal(SIGABRT, [](int) {});

	RAII protobuf_lib{[] { GOOGLE_PROTOBUF_VERIFY_VERSION; }, google::protobuf::ShutdownProtobufLibrary};

	QApplication a{argc, argv};
	MainWindow w;
	w.show();

	return a.exec();
}
