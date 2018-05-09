#include "test_plugin.h"
#include "logic/process_reader.h"
#include "test.h"
#include "ui/mainwindow.h"

#include <grpc++/grpc++.h>
#include <iterator>
#include <memory>
#include <sce.grpc.pb.h>
#include <sce.pb.h>
#include <sstream>
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
			assert(response->IsInitialized());
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

	auto run_sh_script = [](QString script, std::ostream &output, std::ostream &error) {
		Tool sh_script;
		sh_script.path = "sh";
		sh_script.arguments = std::move(script);
		sh_script.working_directory = TEST_DATA_PATH "/interop_scripts";
		Process_reader{sh_script, [&output](std::string_view data) { output << data; }, [&error](std::string_view data) { error << data; }}.join();
	};

	auto test_sh_script = [&run_sh_script](QString script, std::string_view expected_output, std::string_view expected_error) {
		std::ostringstream output, error;
		run_sh_script(std::move(script), output, error);
		assert_equal(expected_error, error.str());
		assert_equal(expected_output, output.str());
	};

	//set up python2
	std::cout << "Python2 setup:\n";
	run_sh_script("setup_python.sh python2", std::cout, std::cerr);
	//set up python3
	std::cout << "Python3 setup:\n";
	run_sh_script("setup_python.sh python3", std::cout, std::cerr);
	//python2
	test_sh_script(R"(run_python_script.sh "python2 rpc_call.py")", "testresponse", "");
	//python3
	test_sh_script(R"(run_python_script.sh "python3 rpc_call.py")", "testresponse", "");
}

void test_plugin() {
	test_local_rpc_call();
	test_python_rpc_call();
}
