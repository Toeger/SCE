import grpc
import sce_pb2
import sce_pb2_grpc
import sys
import socket
import re
import select
from subprocess import Popen, PIPE, STDOUT
from os.path import dirname

def read_until_character(stream, delimiter):
    assert(type(delimiter) == type(" "))
    assert(len(delimiter) == 1)
    data = ""
    while True:
        char = stream.recv(1)
        if len(char) == 0:
            return data
        if char == delimiter:
            return data
        data += char

def read_until_size(stream, size):
    data = ""
    while len(data) < size:
        received = stream.recv(size - len(data) )
        if len(received) == 0:
            return data
        data += received
    return data

def read_notification(stream):
    name = read_until_character(stream, ':')
    size = int(read_until_character(stream, ':'))
    data = read_until_size(stream, size)
    return {"name" : name, "data" : data}

channel = grpc.insecure_channel('localhost:53676')
stub = sce_pb2_grpc.QueryStub(channel)

message_regex = re.compile(r'''([^:\n]+):(\d+)(?::(\d+))?: (error|warning|fatal error): ([^\n]*)''')
def parse_error_message_information(message):
    matches = message_regex.finditer(message)
    infos = []
    for match in matches:
        info = {}
        info["file"] = match.group(1)
        info["line"] = int(match.group(2))
        try:
            info["character"] = int(match.group(3))
        except:
            info["character"] = 1
        info["severity"] = match.group(4)
        info["message"] = match.group(5)
        infos.append(info)
    return infos

def message_severity_to_color(severity):
    if severity == "warning":
        return 0x7f7f00
    if severity == "error":
        return 0xff0000
    if severity == "fatal error":
        return 0xff0000
    return 0x0000ff

def message_severity_to_type(severity):
    if severity == "warning":
        return sce_pb2.AddNoteIn.WARNING
    return sce_pb2.AddNoteIn.ERROR

#get notifications from SCE
notification_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
notification_socket.connect(('127.0.0.1', 53677))

def handle_buffer(filestate):
    getBufferIn = sce_pb2.GetBufferIn()
    getBufferIn.filestate.id = filestate.id;
    getBufferIn.filestate.state = filestate.state;
    print("getBufferIn.filestate.id: ", getBufferIn.filestate.id)
    try:
        getBufferOut = stub.GetBuffer(getBufferIn)
    except Exception as e:
        print e.__doc__
        print e.message
        return

    #pass it to clang
    process = Popen(["clang++", "-std=c++17", "-x", "c++", "-fsyntax-only", "-I" + dirname(getBufferIn.filestate.id), "-"], stdin=PIPE, stdout=PIPE, stderr=STDOUT, bufsize=-1)
    output = process.communicate(getBufferOut.buffer.encode("utf-8"))[0].decode("utf-8")

    #parse clang's output
    issues = parse_error_message_information(output)

    #add appropriate squigglies to the text buffer
    for issue in issues:
        note = sce_pb2.AddNoteIn()
        note.state.id = filestate.id
        note.state.state = filestate.state
        note.range.start.line = issue["line"]
        note.range.start.character = issue["character"]
        note.range.end.line = issue["line"]
        note.range.end.character = issue["character"] + 1
        note.note = issue["message"]
        note.color.rgb = message_severity_to_color(issue["severity"])
        note.type = message_severity_to_type(issue["severity"])
        try:
            addNoteOut = stub.AddNote(note)
        except Exception as e:
            print e.__doc__
            print e.message
            continue

GetCurrentDocumentsIn = sce_pb2.GetCurrentDocumentsIn()
GetCurrentDocumentsOut = stub.GetCurrentDocuments(GetCurrentDocumentsIn)
for filestate in GetCurrentDocumentsOut.filestates:
    handle_buffer(filestate)

#wait for buffer change notification
while True:
    #get notification
    try:
        notification = read_notification(notification_socket)
    except ValueError:
        break
    #parse filestate
    if notification["name"] != "EditNotification": #not an edit notification
        continue
    if select.select([notification_socket], [], [], 0)[0]:
        continue
    edit_notification = sce_pb2.EditNotification()
    edit_notification.ParseFromString(notification["data"])
    print("EditNotification.filestate.id: ", edit_notification.filestate.id)
    print("EditNotification.filestate.state: ", edit_notification.filestate.state)

    #get buffer
    handle_buffer(edit_notification.filestate)