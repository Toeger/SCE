from __future__ import print_function
import grpc
import sce_pb2
import sce_pb2_grpc

channel = grpc.insecure_channel('localhost:53676')
stub = sce_pb2_grpc.QueryStub(channel)
response = stub.GetCurrentFile(sce_pb2.GetCurrentFileParams())
print(response.text, end='')
