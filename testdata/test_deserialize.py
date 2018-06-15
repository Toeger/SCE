import sce_pb2
import sys
with open("/tmp/SCE_test_serialization", "rb") as f:
    data = f.read()
print(type(data))
filestate = sce_pb2.FileState()
filestate.ParseFromString(data)
sys.stdout.write("Filestate id: " + filestate.id)