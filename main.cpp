#include "ui/mainwindow.h"
#include "tests/test.h"

#include <QApplication>
#include <cstring>
#include <cassert>

int main(int argc, char *argv[]) {
	QApplication a{argc, argv};
	assert((test(), true)); //don't run tests in debug mode
	if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
		return 0;
	}
	MainWindow w;
	w.show();

	return a.exec();
}
