#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <grpc++/grpc++.h>
#include <sce.grpc.pb.h>
#include <sce.pb.h>
#include <thread>

struct RPC_server {
    static constexpr auto test_response = "testresponse";
    static constexpr auto default_rpc_address = "localhost:53676";

    //create RPC server and run it in a thread
    RPC_server();
    ~RPC_server();
    RPC_server(const RPC_server &) = delete;

    private:
    class RPC_server_impl : public sce::proto::Query::Service {
        grpc::Status Test(grpc::ServerContext *context, const sce::proto::TestIn *request, sce::proto::TestOut *response) override;
        grpc::Status GetCurrentFileName(grpc::ServerContext *context, const sce::proto::GetCurrentFileNameIn *request,
                                        sce::proto::GetCurrentFileNameOut *response) override;
        grpc::Status GetCurrentBuffer(grpc::ServerContext *context, const sce::proto::GetCurrentBufferIn *request,
                                      sce::proto::GetCurrentBufferOut *response) override;
        grpc::Status AddNote(grpc::ServerContext *context, const sce::proto::AddNoteIn *request, sce::proto::AddNoteOut *response) override;
        grpc::Status SetBufferUpdateNotifications(grpc::ServerContext *context, const sce::proto::SetBufferUpdateNotificationsIn *request,
                                                  sce::proto::SetBufferUpdateNotificationsOut *response) override;
    };

    RPC_server_impl rpc_server;
    decltype(grpc::ServerBuilder{}.BuildAndStart()) server;
    std::thread server_thread;
};

#endif // RPC_SERVER_H