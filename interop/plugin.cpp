#include "plugin.h"
#include "ui/edit_window.h"
#include "ui/mainwindow.h"

RPC_server::RPC_server()
	: server{grpc::ServerBuilder{}
				 .SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_NONE)
				 .SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE)
				 .AddListeningPort(rpc_address, grpc::InsecureServerCredentials())
				 .RegisterService(&rpc_server)
				 .BuildAndStart()}
	, server_thread{[this] { server->Wait(); }} {}

RPC_server::~RPC_server() {
	server->Shutdown();
	server_thread.join();
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
	state.set_file(MainWindow::get_current_path().toStdString());
	state.set_state(edit->get_state());
	response->set_allocated_filestate(&state);
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::AddNote([[maybe_unused]] grpc::ServerContext *context, const sce::proto::AddNoteIn *request,
												  sce::proto::AddNoteOut *response) {
	auto edit = MainWindow::get_current_edit_window();
	if (request->has_range() == false || request->has_state() == false) {
		return grpc::Status::CANCELLED;
	}
	if (request->state().state() != edit->get_state() || request->state().file() != MainWindow::get_current_path().toStdString()) { //outdated
		response->set_success(false);
		return grpc::Status::OK;
	}
	Edit_window::Note note;
	note.color = QColor::fromRgb(request->has_color() ? request->color().rgb() : Qt::red);
	note.line = request->range().start().line();
	note.char_start = request->range().start().character();
	note.char_end = request->range().end().character();
	note.text = QString::fromStdString(request->note());
	edit->add_note(std::move(note));
	return grpc::Status::OK;
}

grpc::Status RPC_server::RPC_server_impl::SetBufferUpdateNotifications([[maybe_unused]] grpc::ServerContext *context,
																	   const sce::proto::SetBufferUpdateNotificationsIn *request,
																	   const sce::proto::SetBufferUpdateNotificationsOut *response) {
	//TODO: make it so that the given plugin gets notifications via standard in
	return grpc::Status::OK;
}
