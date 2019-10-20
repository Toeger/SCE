#ifndef PROJECT_H
#define PROJECT_H

#include <QDir> //TODO: Replace with std::filesystem::path as soon as it stops crashing
#include <string>

class QString;

struct Target {
    std::string build_type;
    std::string name;
};

struct Project {
    static QDir get_default_build_path(QDir project_path);

    Project(QDir project_path);
    Project(QDir project_path, QDir build_path);
    void update_from_disk();

    QDir project_path;
    QDir build_path;
    QFileInfo project_file;
    std::vector<QDir> source_files;
    std::vector<Target> targets;
};

#endif // PROJECT_H
