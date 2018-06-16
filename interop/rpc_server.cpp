#include "rpc_server.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"
#include "utility/thread_call.h"

RPC_server::RPC_server()
	: server{grpc::ServerBuilder{}
				 .SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_NONE)
				 .SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE)
				 .AddListeningPort(default_rpc_address, grpc::InsecureServerCredentials())
				 .RegisterService(&rpc_server)
				 .BuildAndStart()}
	, server_thread{[this] { server->Wait(); }} {}

RPC_server::~RPC_server() {
	close();
}

void RPC_server::close() {
	if (server_thread.joinable()) {
		server->Shutdown();
		server_thread.join();
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
	response->set_filename(MainWindow::get_current_path().toStdString());
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::GetCurrentBuffer([[maybe_unused]] grpc::ServerContext *context,
														   [[maybe_unused]] const sce::proto::GetCurrentBufferIn *request,
														   sce::proto::GetCurrentBufferOut *response) {
	auto edit = MainWindow::get_current_edit_window();
	sce::proto::FileState state;
	state.set_id(MainWindow::get_current_path().toStdString());
	state.set_state(edit->get_state());
	response->set_allocated_filestate(&state);
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::AddNote([[maybe_unused]] grpc::ServerContext *context, const sce::proto::AddNoteIn *request,
												  sce::proto::AddNoteOut *response) {
	if (request->has_range() == false || request->has_state() == false) {
		return grpc::Status::CANCELLED;
	}
	Edit_window::Note note;
	note.color = QColor::fromRgb(request->has_color() ? request->color().rgb() : Qt::red);
	note.line = request->range().start().line();
	note.char_start = request->range().start().character();
	note.char_end = request->range().end().character();
	note.text = QString::fromStdString(request->note());
	Utility::gui_call([note = std::move(note), id = std::move(request->state().id())] {
		auto edit = MainWindow::get_main_window()->get_edit_window(id);
		edit->add_note(std::move(note));
	});
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::GetBuffer([[maybe_unused]] grpc::ServerContext *context, const sce::proto::GetBufferIn *request,
													sce::proto::GetBufferOut *response) {
	if (request->has_filestate() == false) {
		return grpc::Status::CANCELLED;
	}
	const auto &file_state_request = request->filestate();
	const auto &file_id = file_state_request.id();
	const auto &file_state = file_state_request.state();
	auto edit_window = MainWindow::get_main_window()->get_edit_window(file_id);
	if (edit_window != nullptr && edit_window->get_state() == file_state) {
		response->set_buffer(edit_window->toPlainText().toStdString());
		return grpc::Status::OK;
	}
	return grpc::Status::CANCELLED;
}
