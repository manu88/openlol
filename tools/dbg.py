import tkinter as tk
from tkinter import ttk
from enum import StrEnum, IntEnum
import socket
import struct
from typing import List


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

msg_status_response = "@H"


class DBGMsgHeader:
    def __init__(self, type, data_size):
        self.type = type
        self.data_size = data_size

    def __str__(self):
        return f"type={self.type} msg size={self.data_size}"


class MsgStatus:
    def __init__(self, data=None):
        self.current_block = 0
        if (data):
            fields = struct.unpack(msg_status_response, data)
            self.current_block = fields[0]

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


class ToolViews(StrEnum):
    STATUS = "status"
    OTHER = "other"


class UIDebugger:
    def __init__(self, conn: Connection):
        self.conn = conn
        self.root = tk.Tk()
        self.root.title("Lands Of Lore - Visual Debugger")
        self.root.geometry("800x800")
        self.left_frame = tk.Frame(self.root, bg="grey", width=250)
        self.left_frame.pack(side=tk.LEFT, fill=tk.Y)

        self.right_frame = tk.Frame(self.root, bg="lightblue")
        self.right_frame.pack(fill=tk.BOTH, expand=True)

        self.last_status = MsgStatus()

        self.setup_left()
        self.setup_right()
        self.setup_status_view()

        self.view_list.select_set(0)
        self.change_tool_view(ToolViews.STATUS)

    def setup_right(self):
        self.tool_frames = {
            ToolViews.STATUS: tk.Frame(self.right_frame, bg="green"),
            ToolViews.OTHER: tk.Frame(self.right_frame, bg="yellow"),
        }

    def setup_status_view(self):
        frame = self.tool_frames[ToolViews.STATUS]
        self.str_var = tk.StringVar()
        tk.Label(master=frame,
                 textvariable=self.str_var).pack()

    def setup_left(self):
        self.go = tk.Button(master=self.left_frame,
                            text='state', command=self.get_status)
        self.go.pack()
        self.view_list = tk.Listbox(self.left_frame)
        self.view_list.bind("<ButtonRelease-1>", self.view_list_changed)

        i = 0
        for tool in ToolViews:
            self.view_list.insert(i, str(tool))
            i += 1

        self.view_list.pack()

    def run(self):
        self.root.mainloop()

    def change_tool_view(self, sel_tool: ToolViews):
        for widget in self.right_frame.winfo_children():
            widget.pack_forget()
        self.tool_frames[sel_tool].pack(fill=tk.BOTH, expand=True)
        widget.update()
        return

    def get_status(self):
        print("get status")
        self.last_status = self.conn.get_status()
        self.str_var.set(f"current block {self.last_status.current_block}")

    def view_list_changed(self, evt):
        i = self.view_list.curselection()
        tool_index = ToolViews(self.view_list.get(i))
        print(f"view click {tool_index}")
        self.change_tool_view(tool_index)


if __name__ == "__main__":
    conn = Connection()
    conn.connect()
    d = UIDebugger(conn)
    d.run()
