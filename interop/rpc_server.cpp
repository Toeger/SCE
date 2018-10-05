#include "rpc_server.h"
#include "threading/thread_call.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

#include <tuple>

/* The callbacks in this file are run by the RPC_server thread which is different from the GUI thread. Therefore all access to GUI-related functions such as
 * MainWindow must be encapsulated in a Utility::gui_call. */

RPC_server::RPC_server()
	: server{grpc::ServerBuilder{}
				 .SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_NONE)
				 .SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE)
				 .AddListeningPort(default_rpc_address, grpc::InsecureServerCredentials())
				 .RegisterService(&rpc_server)
				 .BuildAndStart()}
	, server_thread{std::async(std::launch::async, [this] { server->Wait(); })} {}

RPC_server::~RPC_server() {
	close();
}

void RPC_server::close() {
	if (server_thread.valid()) {
		server->Shutdown();
		Utility::get_future_value(std::move(server_thread));
	}
}

grpc::Status RPC_server::RPC_server_impl::Test([[maybe_unused]] grpc::ServerContext *context, [[maybe_unused]] const sce::proto::TestIn *request,
											   sce::proto::TestOut *response) {
	response->set_message(test_response);
	return grpc::Status::OK;
}
grpc::Status RPC_server::RPC_server_impl::GetCurrentFileName([[maybe_unused]] grpc::ServerContext *context,
															 [[maybe_unused]] const sce::proto::GetCurrentFileNameIn *request,
															 sce::proto::GetCurrentFileNameOut *response) {
	response->set_filename(Utility::sync_gui_thread_execute(MainWindow::get_current_path).toStdString());
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::GetCurrentBuffer([[maybe_unused]] grpc::ServerContext *context,
														   [[maybe_unused]] const sce::proto::GetCurrentBufferIn *request,
														   sce::proto::GetCurrentBufferOut *response) {
	int state;
	std::tie(state, *response->mutable_filestate()->mutable_id(), *response->mutable_buffer()) = Utility::sync_gui_thread_execute([] {
		auto edit = MainWindow::get_current_edit_window();
		return std::make_tuple(edit->get_state(), edit->get_id().toStdString(), edit->get_buffer().toStdString());
	});
	response->mutable_filestate()->set_state(state);
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::AddNote([[maybe_unused]] grpc::ServerContext *context, const sce::proto::AddNoteIn *request,
												  [[maybe_unused]] sce::proto::AddNoteOut *response) {
	if (request->has_range() == false || request->has_state() == false) {
		return grpc::Status::CANCELLED;
	}
	Edit_window::Note note;
	note.color = QColor::fromRgb(request->has_color() ? request->color().rgb() : Qt::red);
	note.line = request->range().start().line();
	note.char_start = request->range().start().character();
	note.char_end = request->range().end().character();
	note.text = QString::fromStdString(request->note());
	Utility::async_gui_thread_execute([note = std::move(note), id = std::move(request->state().id())]() mutable {
		auto edit = MainWindow::get_main_window().get_edit_window(id);
		edit->add_note(std::move(note));
	});
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::GetBuffer([[maybe_unused]] grpc::ServerContext *context, const sce::proto::GetBufferIn *request,
													sce::proto::GetBufferOut *response) {
	if (request->has_filestate() == false) {
		return grpc::Status::CANCELLED;
	}
	return Utility::sync_gui_thread_execute([&file_state_request = request->filestate(), response] {
		const auto &file_id = file_state_request.id();
		const auto &file_state = file_state_request.state();
		const auto edit_window = MainWindow::get_main_window().get_edit_window(file_id);
		if (edit_window != nullptr && edit_window->get_state() == file_state) {
			*response->mutable_buffer() = edit_window->get_buffer().toStdString();
			return grpc::Status::OK;
		}
		return grpc::Status::CANCELLED;
	});
}

grpc::Status RPC_server::RPC_server_impl::GetCurrentDocuments([[maybe_unused]] grpc::ServerContext *context,
															  [[maybe_unused]] const sce::proto::GetCurrentDocumentsIn *request,
															  sce::proto::GetCurrentDocumentsOut *response) {
	auto file_states = Utility::sync_gui_thread_execute([] {
		std::vector<std::pair<int, std::string>> current_documents;
		//TODO: currently we only support a single open document
		//once you can have side-by-side edit windows each window must be returned
		const auto edit_window = MainWindow::get_current_edit_window();
		current_documents.emplace_back(edit_window->get_state(), edit_window->get_id().toStdString());
		return current_documents;
	});
	for (auto &file_state : file_states) {
		const auto response_file_state = response->add_filestates();
		response_file_state->set_state(file_state.first);
		*response_file_state->mutable_id() = std::move(file_state.second);
	}
	return grpc::Status::OK;
}
