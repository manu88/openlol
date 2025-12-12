import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional
import PIL.Image
import PIL.ImageTk
from lol import lol, SHPFileInfo


def get_type(file: str) -> str:
    return file.split(".")[1]


type_info = {
    "CMZ": "maze map",
    "CPS": "image",
    "DAT": "level decoration data",
    "FNT": "font",
    "INF": "EMC script",
    "INI": "EMC script",
    "SHP": "Sprites sheet",

    "TIM": "animation script",
    "VCN": "maze wall and background graphics",
    "VMP": "how to assemble VCN blocks into proper walls",
    "WLL": "maze wall mappings",
    "WSA": "animation",
    "XXX": "automap data",

    "FRE": "French text",
    "ENG": "English text",
    "GER": "German text",
}


def get_type_info(typ: str) -> str:
    desc = "unsupported"
    if typ in type_info:
        desc = type_info[typ]
    return typ + ": " + desc


class InfoFrame(tk.Frame):
    def __init__(self, master):
        super().__init__(master, height=200)

        tk.Label(master=self, text="File:").grid(column=0, row=0)
        self.file_name_var = tk.StringVar()
        self.file_name_var.set("-- select a file -- ")
        tk.Label(master=self, textvariable=self.file_name_var).grid(
            column=1, row=0)

        tk.Label(master=self, text="Pak:").grid(column=2, row=0)
        self.pak_name_var = tk.StringVar()
        tk.Label(master=self, textvariable=self.pak_name_var).grid(
            column=3, row=0)

        tk.Label(master=self, text="type:").grid(column=0, row=1)
        self.type_desc_var = tk.StringVar(value="some description about type")
        tk.Label(master=self, textvariable=self.type_desc_var).grid(
            column=1, row=1)

    def update_for_item(self, file_name: str, pak_name: str):
        self.file_name_var.set(file_name)
        self.pak_name_var.set(pak_name)
        self.type_desc_var.set(get_type_info(get_type(file_name)))


class BaseRender(tk.Frame):
    def __init__(self, parent):
        super().__init__(parent, bg="pink")

    def update_for_item(self, file_name: str, pak_name: str):
        pass


class CPSRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        self.img_label = tk.Label(self)
        self.img_label.pack()

    def update_for_item(self, file_name: str, pak_name: str):
        out_file = lol.get_temp_path_for("display.png")
        if lol.extract_cps(file_name, pak_name, out_file):
            print(f"written to {out_file}")
            img_data = PIL.Image.open(out_file)
            img = PIL.ImageTk.PhotoImage(img_data)
            self.img_label.configure(image=img)
            self.img_label.image = img


class WSARender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)


class SHPRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        self.shp_info: Optional[SHPFileInfo] = None
        style = ttk.Style(parent)
        style.theme_use("clam")
        style.configure("Treeview", background="black",
                        fieldbackground="black", foreground="white")
        self.table = ttk.Treeview(self, columns=("width", "height"))
        self.table.bind("<<TreeviewSelect>>", self.frame_sel_changed)
        self.table.heading("width", text="width")
        self.table.heading("height", text="height")

        self.table.column("width", width=100)
        self.table.column("height", width=50, anchor="center")

        self.table.pack(fill=tk.X, expand=True)

        self.img_label = tk.Label(self)
        self.img_label.pack()

    def frame_sel_changed(self, _):
        if len(self.table.selection()) == 0:
            return
        frame_iid = self.table.selection()[0]
        sel_item = self.table.item(frame_iid)
        frame_id = int(sel_item["text"])
        out_file = lol.get_temp_path_for("display.png")
        if not lol.extract_shp_frame(self.shp_info, frame_id, out_file):
            print("extract_shp_frame error")
            return
        img_data = PIL.Image.open(out_file)
        img = PIL.ImageTk.PhotoImage(img_data)
        self.img_label.configure(image=img)
        self.img_label.image = img

    def clear_table(self):
        for i in self.table.get_children():
            self.table.delete(i)

    def update_for_item(self, file_name: str, pak_name: str):
        self.shp_info = lol.get_shp_info(file_name, pak_name)
        if self.shp_info is None:
            return
        print(f"compressed = {self.shp_info.compressed}")
        print(f"{len(self.shp_info.desc)} frames")
        self.clear_table()
        for frame_id, frame in enumerate(self.shp_info.frames):
            self.table.insert(
                "", "end", text=f"{frame_id}", values=(frame.width, frame.height))


class UI:
    def __init__(self):
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

        self.file_tree = ttk.Treeview(self.left_frame, columns="type")

        self.file_tree = ttk.Treeview(self.left_frame, columns="type")
        self.file_tree.heading("type", text="type")
        self.file_tree.column("type")
        self.pak_files: Dict[str, str] = {}
        for pak_file in lol.pak_files:
            self.pak_files[pak_file] = []
            for file in lol.list(pak_file):
                self.pak_files[pak_file].append(file)
        self._construct_file_tree()
        self.renders: Dict[str, BaseRender] = {}
        self.current_renderer: Optional[BaseRender] = None
        self._setup_renders()

    def _setup_renders(self):
        self.renders["CPS"] = CPSRender(self.details_frame)
        self.renders["WSA"] = WSARender(self.details_frame)
        self.renders["SHP"] = SHPRender(self.details_frame)

    def change_tool_view(self, typ: str):
        for widget in self.details_frame.winfo_children():
            widget.pack_forget()
        self.current_renderer = None
        if typ in self.renders:
            self.current_renderer = self.renders[typ]
            self.current_renderer.pack(fill=tk.BOTH, expand=True)
            self.current_renderer.update()
        return

    def _construct_file_tree(self):
        for pak_name, files in self.pak_files.items():
            parent_id = self.file_tree.insert(
                "", "end", text=pak_name, values="PAK")

            for file in files:
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

    def selected_item_changed(self, file_name: str, pak_name: str):
        self.info_frame.update_for_item(file_name, pak_name)
        self.change_tool_view(get_type(file_name))
        if self.current_renderer:
            self.current_renderer.update_for_item(file_name, pak_name)


if __name__ == "__main__":
    ui = UI()
    ui.run()
