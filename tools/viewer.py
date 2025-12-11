import tkinter as tk
from tkinter import ttk
from lol import LOL


def get_type(file: str) -> str:
    return file.split(".")[1]


class InfoFrame(tk.Frame):
    def __init__(self, master):
        super().__init__(master, bg="red", height=200)

        tk.Label(master=self, text="File").grid(column=0, row=0)
        self.file_name_var = tk.StringVar()
        self.file_name_var.set("-- select a file -- ")
        tk.Label(master=self, textvariable=self.file_name_var).grid(
            column=1, row=0)

        tk.Label(master=self, text="PAK").grid(column=2, row=0)
        self.pak_name_var = tk.StringVar()
        self.pak_name_var.set("-- select a file -- ")
        tk.Label(master=self, textvariable=self.pak_name_var).grid(
            column=3, row=0)


class UI:
    def __init__(self):
        self.lol = LOL()
        self.root = tk.Tk()
        self.root.title("Lands Of Lore - Viewer")
        self.root.geometry("1000x800")
        self.left_frame = tk.Frame(self.root, bg="grey", width=250)
        self.left_frame.pack(side=tk.LEFT, fill=tk.Y)

        self.right_frame = tk.Frame(self.root, bg="lightblue")
        self.right_frame.pack(fill=tk.BOTH, expand=True)

        self.details_frame = tk.Frame(
            self.right_frame, bg="yellow", height=600)
        self.details_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.info_frame = InfoFrame(self.right_frame)
        self.info_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.file_tree = ttk.Treeview(self.left_frame, columns=("type"))

        self.file_tree = ttk.Treeview(self.left_frame, columns=("type"))
        self.file_tree.heading("type", text="type")
        self.file_tree.column("type")
        self._construct_file_tree()

    def _construct_file_tree(self):

        for pak_file in self.lol.pak_files:
            parent_id = self.file_tree.insert(
                "", "end", text=pak_file, values="PAK")

            for file in self.lol.list(pak_file):
                self.file_tree.insert(parent_id, "end", text=file,
                                      values=get_type(file))
        self.file_tree.pack(fill=tk.BOTH, expand=True)
        self.file_tree.bind("<<TreeviewSelect>>", self.file_tree_sel_changed)

    def run(self):
        self.root.mainloop()

    def file_tree_sel_changed(self, _):
        item_iid = self.file_tree.selection()[0]
        parent_iid = self.file_tree.parent(item_iid)
        sel_item = self.file_tree.item(item_iid)
        if parent_iid:
            self.selected_item_changed(
                sel_item['text'], self.file_tree.item(parent_iid)['text'])

    def selected_item_changed(self, file_name: str, pak_name: str = None):
        print(f"Selected item: {file_name} in {pak_name}")
        self.info_frame.file_name_var.set(file_name)
        self.info_frame.pak_name_var.set(pak_name)


if __name__ == "__main__":
    ui = UI()
    ui.run()
