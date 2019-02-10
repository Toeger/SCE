#ifndef PROJECT_H
#define PROJECT_H

#include <string>
#include <string_view>

class QString;

struct Project {
	static QString get_default_build_path(std::string_view project_path);
	Project(std::string project_path);
	Project(std::string project_path, std::string build_path);
	std::string project_path;
	std::string build_path;
};

#endif // PROJECT_H