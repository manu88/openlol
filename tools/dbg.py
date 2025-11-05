import tkinter as tk
from tkinter import ttk
from enum import StrEnum


class ToolViews(StrEnum):
    STATUS = "status"
    OTHER = "other"


class UIDebugger:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Lands Of Lore - Visual Debugger")
        self.root.geometry("800x800")
        self.left_frame = tk.Frame(self.root, bg="grey", width=250)
        self.left_frame.pack(side=tk.LEFT, fill=tk.Y)

        self.right_frame = tk.Frame(self.root, bg="lightblue")
        self.right_frame.pack(fill=tk.BOTH, expand=True)

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
        tk.Button(master=frame, text="test").pack()

    def setup_left(self):
        self.go = tk.Button(master=self.left_frame,
                            text='Connect', command=self.connect)
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

    def connect(self):
        print("click")

    def view_list_changed(self, evt):
        i = self.view_list.curselection()
        tool_index = ToolViews(self.view_list.get(i))
        print(f"view click {tool_index}")
        self.change_tool_view(tool_index)


if __name__ == "__main__":
    d = UIDebugger()
    d.run()
