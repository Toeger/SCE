#include "project.h"
#include "settings.h"

#include <QDir>

QString Project::get_default_build_path(std::string_view project_path) {
	auto path = QDir{QString::fromLocal8Bit(project_path.data(), project_path.size())};
	path.cd(Settings::get<Settings::Key::build_folder>("../build"));
	return path.path();
}

Project::Project(std::string p_project_path)
	: project_path{std::move(p_project_path)}
	, build_path{get_default_build_path(project_path).toStdString()} {}

Project::Project(std::string p_project_path, std::string p_build_path)
	: project_path{std::move(p_project_path)}
	, build_path{std::move(p_build_path)} {}
