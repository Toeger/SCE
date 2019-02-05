#include "lsp_feature.h"
#include "interop/language_server_protocol.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QTextBlock>
#include <algorithm>
#include <array>
#include <initializer_list>

static std::vector<QMetaObject::Connection> connections;

template <class... Args>
void autoconnect(Args &&... args) {
	connections.push_back(QObject::connect(args...));
}

static void set_up_completion_provider(LSP_feature &f) {
	auto &action = f.action;
	action.setShortcuts({f.activation1, f.activation2});
	autoconnect(&action, &QAction::triggered, [&f] {
		if (f.clients.empty()) {
			MainWindow::get_main_window().set_status(
				QObject::tr("Warning: Action \"%1\" triggered, but no LSP server selected for that action."
							"<br><a href=\"add server\">Add an LSP server</a> or <a href=\"lsp features\">activate an LSP feature</a>.")
					.arg(f.description));
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
		const int line = text_cursor.blockNumber();
		const int character = text_cursor.positionInBlock();
		nlohmann::json params = {{"context", {{"triggerKind", {{"CompletionTriggerKind", "Invoked"}}} /*triggerCharacter = none*/}},
								 {"textDocument", {{"uri", "file://" + MainWindow::get_current_path().toStdString()}}},
								 {"position", {{"line", line}, {"character", character}}}};

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
			auto &result = response.result.value();
			if (result.count("items")) {
				auto &items = result["items"];
				if (not items.is_array()) {
					MainWindow::get_main_window().set_status("Invalid response from LSP server");
					return;
				}
				std::vector<std::string> suggestions;
				for (auto &item : items) {
					if (item.count("insertText")) {
						auto &insert_text = item["insertText"];
						if (not insert_text.is_string()) {
							MainWindow::get_main_window().set_status("Invalid response from LSP server");
							return;
						}
						suggestions.push_back(std::move(insert_text));
					}
				}
				if (suggestions.empty()) {
					MainWindow::get_main_window().set_status("No suggestions");
					return;
				}
				auto apply_suggestion = [&](std::string &suggestion) {
					const auto text_line = text_cursor.block().text().toStdString();
					static constexpr char identifier_ender[] = "()[]{} \n\t+-*/&%<>|.,;=";
					const std::size_t start = std::rend(text_line) - std::find_if(std::rend(text_line) - character, std::rend(text_line), [](char c) {
												  return std::find(std::begin(identifier_ender), std::end(identifier_ender), c) != std::end(identifier_ender);
											  });
					const std::size_t end = std::end(text_line) - std::find_if(std::begin(text_line) + character, std::end(text_line), [](char c) {
												return std::find(std::begin(identifier_ender), std::end(identifier_ender), c) != std::end(identifier_ender);
											});
					auto new_cursor = text_cursor;
					new_cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, character - start);
					new_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, text_line.size() - end - start);
					edit_window.setTextCursor(new_cursor);
					edit_window.insertPlainText(QString::fromStdString(suggestion));
				};
				if (suggestions.size() == 1) {
					apply_suggestion(suggestions.front());
				} else {
					//got multiple suggestions, make a combobox to choose
					QMenu menu;
					bool suggestion_list_incomplete = false;
					if (suggestions.size() > 5) {
						suggestion_list_incomplete = true;
						suggestions.resize(5);
					}
					if (result.count("isIncomplete")) {
						auto &is_incomplete = result["isIncomplete"];
						if (is_incomplete.is_boolean()) {
							if (static_cast<bool>(is_incomplete)) {
								suggestion_list_incomplete = true;
							}
						}
					}
					if (suggestions.size() > 5) {
						suggestions.resize(5);
					}
					for (auto &suggestion : suggestions) {
						QObject::connect(menu.addAction(suggestion.c_str()), &QAction::triggered,
										 [&apply_suggestion, &suggestion] { apply_suggestion(suggestion); });
					}
					if (suggestion_list_incomplete) {
						menu.addAction("..."); //TODO: Attach the "Load more suggestions" action
					}
					menu.exec(MainWindow::get_current_edit_window()->mapToGlobal(MainWindow::get_current_edit_window()->cursorRect().bottomLeft()));
				}
			}
		}
	});
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

static void send_to_lsp_server(const LSP::Notification &notification, LSP::Client &client) {
	client.notify(notification);
}

static void send_to_all_lsp_servers(const LSP::Notification &notification) {
	for (auto &[name, client] : LSP::Client::get_clients()) {
		try {
			send_to_lsp_server(notification, *client);
		} catch (std::exception &e) {
			MainWindow::get_main_window().set_status("Error notifying LSP client " + name + " with " + notification.method.c_str() + " " + e.what());
		}
	}
}

static LSP::Notification get_file_open_notification(const File_info &fileinfo) {
	const std::string content = fileinfo.window->get_buffer().toStdString();
	return {
		.method = "textDocument/didOpen",
		.params = {{"textDocument", {{"uri", "file://" + fileinfo.path}, {"languageId", "cpp"}, {"version", fileinfo.version}, {"text", content}}}},
	};
}

static void report_file_change_to_lsp_servers(File_info &fileinfo, const Edit_window::Edit &edit) {
	const LSP::Notification notification = {
		.method = "textDocument/didChange",
		.params =
			{
				{"textDocument", {{"uri", "file://" + fileinfo.path}, {"version", ++fileinfo.version}}},
				{"contentChanges",
				 {{
					 {"text", edit.added},
					 {"range",
					  {{"start", {{"line", edit.start.line}, {"character", edit.start.character}}},
					   {"end", {{"line", edit.end.line}, {"character", edit.end.character}}}}},
					 {"rangeLength", edit.length},
				 }}},
			},
	};
	send_to_all_lsp_servers(notification);
}

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

static auto get_file_info_iterator_for(Edit_window &w) {
	auto pos = std::find_if(std::begin(file_infos), std::end(file_infos), [&w](const File_info &fi) { return &w == fi.window; });
	assert(pos != std::end(file_infos));
	return pos;
}

void LSP_feature::init_all_features() {
	apply_to_each([](LSP_feature &f) { f.do_setup(f); });
	autoconnect(&MainWindow::get_main_window(), &MainWindow::file_opened, [](Edit_window &w, std::string path) {
		file_infos.push_back({std::move(path), &w});
		autoconnect(&w, &Edit_window::destroyed, [&w] {
			auto file_info_it = get_file_info_iterator_for(w);
			report_file_close_to_lsp_servers(*file_info_it);
			file_infos.erase(file_info_it);
		});
		autoconnect(&w, &Edit_window::edited, [&w](const Edit_window::Edit &edit) { report_file_change_to_lsp_servers(*get_file_info_iterator_for(w), edit); });
		send_to_all_lsp_servers(get_file_open_notification(file_infos.back()));
	});
}

void LSP_feature::exit_all_features() {
	for (auto connection : connections) {
		QObject::disconnect(connection);
	}
	LSP::Client::get_clients().clear();
}

void LSP_feature::add_all(QWidget &w) {
	apply_to_each([&w](LSP_feature &f) { w.addAction(&f.action); });
}

void LSP_feature::add_lsp_server(LSP::Client &client) {
	for (auto &file_info : file_infos) {
		send_to_lsp_server(get_file_open_notification(file_info), client);
	}
}

nlohmann::json LSP_feature::get_init_params() {
	nlohmann::json workspace_client_capabilities = {
		{"applyEdit", false},
		{"workspaceEdit", {{"documentChanges", true}, {"resourceOperations", {"create", "rename", "delete"}}, {"failureHandling", {"abort"}}}},
		{"didChangeConfiguration", {{"dynamicRegistration", false}}},
		{"didChangeWatchedFiles", {{"dynamicRegistration", false}}},
		{"symbol", {{"dynamicRegistration", false}}},
		{"executeCommand", {{"dynamicRegistration", false}}},
		{"workspaceFolders", false},
		{"configuration", false},
	};
	nlohmann::json text_document_client_capabilities = {
		{"synchronization", {{"dynamicRegistration", true}, {"willSave", true}, {"willSaveWaitUntil", true}, {"didSave", true}}},
		{"completion",
		 {{"dynamicRegistration", true},
		  {"completionItem",
		   {{"snippetSupport", false},
			{"commitCharactersSupport", false},
			{"documentationFormat", {"plaintext", "markdown"}},
			{"deprecatedSupport", true},
			{"preselectSupport", true}}},
		  {"contextSupport", true}}},
		{"hover", {{"dynamicRegistration", false}, {"contentFormat", {"plaintext", "markdown"}}}},
		{"signatureHelp", {{"dynamicRegistration", false}, {"signatureInformation", {{"documentationFormat", {"plaintext", "markdown"}}}}}},
		{"references", {{"dynamicRegistration", false}}},
		{"documentHighlight", {{"dynamicRegistration", false}}},
		{"documentSymbol",
		 {{"dynamicRegistration", false},
		  {"symbolKind",
		   {{"valueSet",
			 {
				 "File",   "Module",	"Namespace", "Package",	"Class",	"Method", "Property", "Field",		  "Constructor",
				 "Enum",   "Interface", "Function",  "Variable",   "Constant", "String", "Number",   "Boolean",		  "Array",
				 "Object", "Key",		"Null",		 "EnumMember", "Struct",   "Event",  "Operator", "TypeParameter",
			 }}}},
		  {"hierarchicalDocumentSymbolSupport", false}}},
		{"formatting", {{"dynamicRegistration", false}}},
		{"rangeFormatting", {{"dynamicRegistration", false}}},
		{"onTypeFormatting", {{"dynamicRegistration", false}}},
		{"definition", {{"dynamicRegistration", false}}},
		{"typeDefinition", {{"dynamicRegistration", false}}},
		{"implementation", {{"dynamicRegistration", false}}},
		{"codeAction", {{"dynamicRegistration", false}}},
		{"codeLens", {{"dynamicRegistration", false}}},
		{"documentLink", {{"dynamicRegistration", false}}},
		{"colorProvider", {{"dynamicRegistration", false}}},
		{"rename", {{"dynamicRegistration", false}, {"prepareSupport", false}}},
		{"publishDiagnostics", {{"relatedInformation", true}}},
		{"foldingRange", {{"dynamicRegistration", false}, {"rangeLimit", 100}, {"lineFoldingOnly", false}}},
	};
	nlohmann::json client_capabilities = {
		{"workspace", std::move(workspace_client_capabilities)},
		{"textDocument", std::move(text_document_client_capabilities)},
		{"experimental", nullptr},
	};
	return {
		{"processId", getpid()},
		{"rootUri", nullptr}, //TODO: change this to project directory once we have such a thing
		{"capabilities", std::move(client_capabilities)},
		{"trace", "off"},
	};
}
