#include "mainwindow.h"
#include "tests/test.h"

#include <QApplication>
#include <cstring>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
		test();
		return 0;
	}
	MainWindow w;
	w.show();

	return a.exec();
}
