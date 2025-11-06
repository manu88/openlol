import tkinter as tk
from tkinter import ttk
from enum import StrEnum
from dbg_connector import Connection, MsgStatus


class ToolViews(StrEnum):
    STATUS = "status"
    OTHER = "other"


class StatusFrame(tk.Frame):
    def __init__(self, master):
        super().__init__(master, bg="green")
        self.last_status = MsgStatus()
        self.str_var = tk.StringVar()
        self.game_flags_var = [tk.StringVar(value=f"0X00") for _ in range(100)]
        self.game_flags = [None for _ in range(100)]
        tk.Label(master=self,
                 textvariable=self.str_var).grid(column=0, row=0)
        tk.Label(master=self, text="game flags").grid(
            column=0, row=1, sticky='w')
        grid_y_offset = 2
        for x in range(10):
            for y in range(10):
                label_idx = x + (y*10)
                self.game_flags[label_idx] = tk.Label(
                    master=self, textvariable=self.game_flags_var[label_idx], anchor="e")
                self.game_flags[label_idx].grid(
                    column=x+2, row=y+grid_y_offset, sticky='w')
        grid_y_offset += 10

    def update(self, status: MsgStatus):
        self.str_var.set(
            f"current block {status.current_block}")
        for i in range(100):
            label = self.game_flags_var[i]
            label.set(f"0X{status.game_flags[i]:02X}")
            if self.last_status and self.last_status.game_flags[i] != status.game_flags[i]:
                self.game_flags[i].config(bg="red")
            else:
                self.game_flags[i].config(bg="gray")

        self.last_status = status


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

        self.status_frame = StatusFrame(self.right_frame)
        self.setup_left()
        self.setup_right()

        self.view_list.select_set(0)
        self.change_tool_view(ToolViews.STATUS)

    def setup_right(self):
        self.tool_frames = {
            ToolViews.STATUS: self.status_frame,
            ToolViews.OTHER: tk.Frame(self.right_frame, bg="yellow"),
        }

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
        status = self.conn.get_status()
        if status:
            self.status_frame.update(status)

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
