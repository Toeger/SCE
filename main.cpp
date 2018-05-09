#include "tests/test.h"
#include "ui/mainwindow.h"
#include "utility/raii.h"

#include <QApplication>
#include <cassert>
#include <cstring>
#include <sce.pb.h>

int main(int argc, char *argv[]) {
	RAII protobuf_lib{[] { GOOGLE_PROTOBUF_VERIFY_VERSION; }, google::protobuf::ShutdownProtobufLibrary};

	QApplication a{argc, argv};
	assert((test(), true)); //don't run tests in release mode
	if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
		return 0;
	}
	MainWindow w;
	w.show();

	return a.exec();
}
