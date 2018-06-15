import sce_pb2
import sys
with open("/tmp/SCE_test_serialization", "rb") as f:
    data = f.read()
edit_notification = sce_pb2.EditNotification()
edit_notification.ParseFromString(data)
sys.stdout.write("Filestate id: " + edit_notification.filestate.id)