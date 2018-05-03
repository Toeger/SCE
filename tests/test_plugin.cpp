#include "test_plugin.h"
#include "logic/process_reader.h"
#include "test.h"
#include "ui/mainwindow.h"

#include <grpc++/grpc++.h>
#include <iterator>
#include <memory>
#include <sce.grpc.pb.h>
#include <sce.pb.h>
#include <string>
#include <thread>

struct Test_RPC_server {
	static constexpr auto test_response = "testresponse";
	static constexpr auto rpc_address = "localhost:53676";

	//class that implements some RPC functions
	class Rpc_server : public sce::proto::Query::Service {
		grpc::Status Test([[maybe_unused]] grpc::ServerContext *context, [[maybe_unused]] const sce::proto::Nothing *request,
						  sce::proto::String *response) override {
			response->set_str(test_response);
			return grpc::Status::OK;
		}
	};

	Test_RPC_server()
		//create RPC server and run it in a thread
		: server{grpc::ServerBuilder{}
					 .SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_NONE)
					 .SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE)
					 .AddListeningPort(rpc_address, grpc::InsecureServerCredentials())
					 .RegisterService(&rpc_server)
					 .BuildAndStart()}
		, server_thread{[this] { server->Wait(); }} {}
	~Test_RPC_server() {
		server->Shutdown();
		server_thread.join();
	}

	Rpc_server rpc_server;
	decltype(grpc::ServerBuilder{}.BuildAndStart()) server;
	std::thread server_thread;
};

static void test_local_rpc_call() {
	Test_RPC_server rpc_server;
	//make an RPC call
	sce::proto::String reply;
	grpc::ClientContext client_context;
	auto stub = sce::proto::Query::NewStub(grpc::CreateChannel(Test_RPC_server::rpc_address, grpc::InsecureChannelCredentials()));
	auto status = stub->Test(&client_context, {}, &reply);
	assert_true(status.ok());
	assert_equal(reply.str(), Test_RPC_server::test_response);
}

static void test_python_rpc_call() {
	Test_RPC_server trpcs;
	MainWindow mw;

	Tool python_test_script;
	python_test_script.path = "sh";
	python_test_script.arguments = "run_rpc_call.sh";
	python_test_script.working_directory = TEST_DATA_PATH "/interop_scripts";
	std::string python_output;
	std::string python_error;
	Process_reader{python_test_script, [&python_output](std::string_view data) { python_output += data; },
				   [&python_error](std::string_view data) { python_error += data; }}
		.join();
	assert_equal(python_error, "");
	assert_equal(python_output, "testresponse");
}

void test_plugin() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	test_local_rpc_call();
	test_python_rpc_call();

	google::protobuf::ShutdownProtobufLibrary();
}
