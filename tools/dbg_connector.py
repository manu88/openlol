from enum import IntEnum
import struct
import socket

class DBGMsgType(IntEnum):
    Hello = 0,
    Goodbye = 1
    StatusRequest = 2
    StatusResponse = 3
    GiveItemRequest = 4
    GiveItemResponse = 5
    SetStateRequest = 6
    SetStateResponse = 7
    QuitRequest = 8
    QuitResponse = 9
    NoClipRequest = 10
    NoClipResponse = 11
    SetLoggerRequest = 12
    SetLoggerResponse = 13
    SetVarRequest = 14
    SetVarResponse = 15


msg_header_struct = "@BI"
msg_header_struct_size = 8

msg_status_response = "@H100B"


class DBGMsgHeader:
    def __init__(self, type, data_size):
        self.type = type
        self.data_size = data_size

    def __str__(self):
        return f"type={self.type} msg size={self.data_size}"


class MsgStatus:
    def __init__(self, data=None):
        self.current_block = 0
        self.game_flags = [0 for _ in range(100)]
        if (data):
            fields = struct.unpack(msg_status_response, data)
            self.current_block = fields[0]
            self.game_flags = fields[1:]

    def __str__(self):
        return f"current block {self.current_block}"


def create_header(type, data_size):
    return struct.pack(msg_header_struct, type, data_size)


class Connection:
    def __init__(self):
        self.host = '127.0.0.1'
        self.port = 9000
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self):
        self.client.connect((self.host, self.port))
        print(f"connected to {self.host}:{self.port}")
        header = self.read_header()
        print(f"received {header.type}")

    def read_header(self) -> DBGMsgHeader:
        data = self.client.recv(msg_header_struct_size)
        fields = struct.unpack(msg_header_struct, data)
        assert (len(fields) == 2)
        return DBGMsgHeader(fields[0], fields[1])

    def get_status(self) -> MsgStatus:
        header = create_header(DBGMsgType.StatusRequest, 0)
        self.client.send(header)
        resp_header = self.read_header()
        print(f"get_state received header {resp_header}")
        assert (resp_header.type == DBGMsgType.StatusResponse)
        msg_size = resp_header.data_size
        data = self.client.recv(msg_size)
        return MsgStatus(data)
