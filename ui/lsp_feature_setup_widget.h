#ifndef LSP_FEATURE_SETUP_WIDGET_H
#define LSP_FEATURE_SETUP_WIDGET_H

#include "logic/tool.h"

#include <QWidget>
#include <memory>
#include <thread>

namespace Ui {
    class LSP_feature_setup_widget;
}

class LSP_feature_setup_widget : public QWidget {
    Q_OBJECT

    public:
    explicit LSP_feature_setup_widget(QWidget *parent = nullptr);
    ~LSP_feature_setup_widget() override;
    void update_lsp_features();

    private:
    std::vector<Tool> tools;
    std::thread feature_loader;
    std::unique_ptr<Ui::LSP_feature_setup_widget> ui;
    Ui::LSP_feature_setup_widget *_; //Qt Designer only works correctly if it finds this string
};

#endif // LSP_FEATURE_SETUP_WIDGET_H
