import tkinter as tk
from tkinter import ttk
from lol import LOL


def get_type(file: str) -> str:
    return file.split(".")[1]


class UI:
    def __init__(self):
        self.lol = LOL()
        self.root = tk.Tk()
        self.root.title("Lands Of Lore - Viewer")
        self.root.geometry("800x800")
        self.left_frame = tk.Frame(self.root, bg="grey", width=250)
        self.left_frame.pack(side=tk.LEFT, fill=tk.Y)

        self.right_frame = tk.Frame(self.root, bg="lightblue")
        self.right_frame.pack(fill=tk.BOTH, expand=True)
        self.file_tree = ttk.Treeview(
            self.left_frame, columns=("type"))
        self._construct_file_tree()

    def _construct_file_tree(self):
        self.file_tree = ttk.Treeview(
            self.left_frame, columns=("type"))
        self.file_tree.heading("type", text="type")
        self.file_tree.column("type")

        for pak_file in self.lol.pak_files:
            parent_id = self.file_tree.insert(
                "", "end", text=pak_file, values="PAK")

            for file in self.lol.list(pak_file):
                self.file_tree.insert(parent_id, "end", text=file,
                                      values=get_type(file))
        self.file_tree.pack(fill=tk.BOTH, expand=True)

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    ui = UI()
    ui.run()
