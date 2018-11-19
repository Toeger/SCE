#include "lsp_feature.h"
#include "interop/language_server_protocol.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QMenu>
#include <QMessageBox>
#include <algorithm>
#include <array>
#include <initializer_list>

template <class T>
T *ptr(std::optional<T> &opt) {
	return opt.has_value() ? &opt.value() : nullptr;
}
template <class T>
const T *ptr(const std::optional<T> &opt) {
	return opt.has_value() ? &opt.value() : nullptr;
}

static std::vector<QMetaObject::Connection> connections;

static void set_up_completion_provider(LSP_feature &f) {
	auto &action = f.action;
	action.setShortcuts({f.activation1, f.activation2});
	connections.push_back(QObject::connect(&action, &QAction::triggered, [&f] {
		if (f.clients.empty()) {
			MainWindow::get_main_window().set_status(
				QObject::tr("Warning: Action \"%1\" triggered, but no LSP server selected for that action.").arg(f.description));
			return;
		}
		/* textDocument/completion {
			CompletionParams {
				context?: CompletionContext {
					triggerKind: CompletionTriggerKind {
						CompletionTriggerKind {
							Invoked: 1 = 1;
							TriggerCharacter: 2 = 2;
							TriggerForIncompleteCompletions: 3 = 3;
						}
					}
					triggerCharacter?: string;
				}
			}
			TextDocumentPositionParams {
				textDocument: TextDocumentIdentifier{
					uri: DocumentUri {
						string
					}
				}
				position: Position{
					line: number
					character: number
				}
			}
		   }
		 */
		auto &edit_window = *MainWindow::get_current_edit_window();
		const auto &text_cursor = edit_window.textCursor();
		int line = text_cursor.blockNumber();
		int character = text_cursor.positionInBlock();
		nlohmann::json params = {{"context", {{"triggerKind", {{"CompletionTriggerKind", "Invoked"}}} /*triggerCharacter = none*/}},
								 {"textDocument", {{"uri", "file://" + MainWindow::get_current_path().toStdString()}}},
								 {"position", {{"line", line}, {"character", character}}}};

		//QMessageBox::information(nullptr, "Json Params", params.dump(4).c_str());
		LSP::Response response;
		try {
			LSP::Request request{.method = "textDocument/completion", .params = std::move(params)};
			response = f.clients.front()->call(request);
		} catch (const std::runtime_error &e) {
			MainWindow::get_main_window().set_status(e.what());
			return;
		}
		if (response.error) {
			MainWindow::get_main_window().set_status(QObject::tr("LSP error: message: ") + response.error->message.c_str());
		}
		if (response.result) {
			QMessageBox::information(nullptr, "Response", response.result->dump(4).c_str());
		}
		//QMenu menu;
		//menu.addAction("Test1");
		//menu.addAction("Test2");
		//menu.exec(MainWindow::get_current_edit_window()->mapToGlobal(MainWindow::get_current_edit_window()->cursorRect().bottomLeft()));
	}));
}

static LSP_feature lsp_features[] = {{
	"completionProvider",
	QObject::tr("Auto-complete code"),
	LSP_feature::Multi_client_support::yes,
	set_up_completion_provider,
}};

LSP_feature::LSP_feature(std::string_view pname, QString pdescription, LSP_feature::Multi_client_support pmulti_client_support,
						 void (*const pdo_setup)(LSP_feature &), std::vector<std::shared_ptr<LSP::Client> > pclients)
	: name{pname}
	, description{pdescription}
	, multi_client_support{pmulti_client_support}
	, clients{pclients}
	, activation1{QKeySequence::fromString("CTRL+SPACE")}
	, do_setup{pdo_setup} {}

LSP_feature *LSP_feature::lookup(std::string_view name) {
	auto pos = std::find_if(std::begin(lsp_features), std::end(lsp_features), [&name](const LSP_feature &lsp_feature) { return lsp_feature.name == name; });
	if (pos == std::end(lsp_features)) {
		return nullptr;
	}
	return &(*pos);
}

void LSP_feature::apply_to_each(TMP::Function_ref<void(LSP_feature &)> callback) {
	for (auto &lsp_feature : lsp_features) {
		callback(lsp_feature);
	}
}

struct File_info {
	std::string path;
	Edit_window *window;
	unsigned int version = 0;
};

static std::vector<File_info> file_infos;

static void report_file_open_to_lsp_servers(const File_info &fileinfo) {
	const std::string content = fileinfo.window->get_buffer().toStdString();
	const LSP::Notification notification = {
		.method = "textDocument/didOpen",
		.params = {{"textDocument", {{"uri", "file://" + fileinfo.path}, {"languageId", "cpp"}, {"version", fileinfo.version}, {"text", content}}}},
	};
	for (auto &[name, client] : LSP::Client::get_clients()) {
		try {
			client->notify(notification);
			//QMessageBox::information(nullptr, "Opening file", QString::fromStdString(notification.params.dump(4)));
		} catch (std::exception &e) {
			MainWindow::get_main_window().set_status("Error notifying LSP client " + name + e.what());
		}
	}
}
//static void report_file_change_to_lsp_servers(std::string_view filename, std::string_view diff) {}
static void report_file_close_to_lsp_servers(const File_info &fileinfo) {
	const std::string content = fileinfo.window->get_buffer().toStdString();
	const LSP::Notification notification = {
		.method = "textDocument/didClose",
		.params = {{"textDocument", {{"uri", "file://" + fileinfo.path}}}},
	};
	for (auto &[name, client] : LSP::Client::get_clients()) {
		try {
			client->notify(notification);
			//QMessageBox::information(nullptr, "Closing file", QString::fromStdString(notification.params.dump(4)));
		} catch (std::exception &e) {
			MainWindow::get_main_window().set_status("Error notifying LSP client " + name + e.what());
		}
	}
}

void LSP_feature::setup_all() {
	apply_to_each([](LSP_feature &f) { f.do_setup(f); });
	connections.push_back(QObject::connect(&MainWindow::get_main_window(), &MainWindow::file_opened, [](Edit_window *w, std::string path) {
		file_infos.push_back({std::move(path), w});
		report_file_open_to_lsp_servers(file_infos.back());
		connections.push_back(QObject::connect(w, &Edit_window::destroyed, [w] {
			auto pos = std::find_if(std::begin(file_infos), std::end(file_infos), [w](const File_info &fi) { return w == fi.window; });
			assert(pos != std::end(file_infos));
			report_file_close_to_lsp_servers(*pos);
			file_infos.erase(pos);
		}));
	}));
}

void LSP_feature::close_all() {
	for (auto connection : connections) {
		QObject::disconnect(connection);
	}
	LSP::Client::get_clients().clear();
}

void LSP_feature::add_all(QWidget &w) {
	apply_to_each([&w](LSP_feature &f) { w.addAction(&f.action); });
}
