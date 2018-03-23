#include "test_plugin.h"
#include "test.h"

#include <grpc++/grpc++.h>
#include <memory>
#include <sce.grpc.pb.h>
#include <sce.pb.h>
#include <thread>

static void test_rpc_call() {
    //class that implements the RPC functions
    constexpr auto test_response = "testresponse";
    class Rpc_server : public sce::proto::Query::Service {
        grpc::Status GetCurrentFile(grpc::ServerContext *context, const sce::proto::GetCurrentFileParams *request, sce::proto::String *response) override {
            response->set_text(test_response);
            return grpc::Status::OK;
        }
    };

    //create RPC server and run it in a thread
    constexpr auto rpc_address = "localhost:53676";
    Rpc_server rpc_server;
    auto server = grpc::ServerBuilder{}.AddListeningPort(rpc_address, grpc::InsecureServerCredentials()).RegisterService(&rpc_server).BuildAndStart();
    std::thread server_thread{[&server] { server->Wait(); }};

    //make an RPC call
    sce::proto::String reply;
    sce::proto::GetCurrentFileParams request({});
    grpc::ClientContext client_context;
    auto stub = sce::proto::Query::NewStub(grpc::CreateChannel(rpc_address, grpc::InsecureChannelCredentials()));
    auto status = stub->GetCurrentFile(&client_context, request, &reply);
    assert_true(status.ok());
    assert_equal(reply.text(), test_response);

    //shut down server
    server->Shutdown();
    server_thread.join();
}

void test_plugin() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
	
    test_rpc_call();
	
    google::protobuf::ShutdownProtobufLibrary();
}
