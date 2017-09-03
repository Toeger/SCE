#include "mainwindow.h"

#include <QApplication>
#include <cstring>

int main(int argc, char *argv[]) {
	if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
		//TODO: add tests here
		return 0;
	}
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
