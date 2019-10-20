#include "project.h"
#include "settings.h"

#include <QDir> //TODO: Replace with std::filesystem once it stops crashing for even trivial code
#include <stdexcept>

QDir Project::get_default_build_path(QDir project_path) {
    project_path.cd(Settings::get<Settings::Key::default_build_folder>("../build"));
    return project_path;
}

Project::Project(QDir p_project_path)
    : Project{p_project_path, get_default_build_path(p_project_path)} {}

Project::Project(QDir p_project_path, QDir p_build_path)
    : project_path{std::move(p_project_path)}
    , build_path{std::move(p_build_path)} {}

void Project::update_from_disk() {
    if (not project_path.exists()) {
        throw std::runtime_error{"Failed updating project from disk because project path is invalid"};
    }
    if (project_file.filePath().isEmpty()) {
    }
    if (not project_file.exists()) {
    }
}
