import argparse
import os
import sys
from playsound import playsound
import tkinter as tk
from tkinter import ttk
from typing import Dict, Optional
import PIL.Image
import PIL.ImageTk
from lol import lol, SHPFileInfo, WSAFileInfo
from script_tools import analyze_script, CodeViewer

pak_files: Dict[str, str] = {}

parser = argparse.ArgumentParser(prog="lol asset explorer")
parser.add_argument("pak_file", default="data/", nargs="?")


def get_type(file: str) -> str:
    ext = file.split(".")[1]
    if ext in ["FRE", "ENG", "GER"]:
        return "LANG"
    return ext


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
    "VOC": "audio file",
    "VMP": "how to assemble VCN blocks into proper walls",
    "WLL": "maze wall mappings",
    "WSA": "animation",
    "XXX": "automap data",
    "LANG": "game text",
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
        super().__init__(parent)

    def update_for_item(self, file_name: str, pak_name: str):
        pass


class VCNRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        self.img_label = tk.Label(self)
        self.img_label.pack()

    def update_for_item(self, file_name, pak_name):
        out_file = lol.get_temp_path_for("display.png")
        if lol.extract_vcn(file_name, pak_name, out_file):
            img_data = PIL.Image.open(out_file)
            img = PIL.ImageTk.PhotoImage(img_data)
            self.img_label.configure(image=img)
            self.img_label.image = img


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
        self.wsa_info: Optional[WSAFileInfo] = None
        tk.Label(master=self, text="Frames:").grid(column=0, row=0)
        self.frame_count_var = tk.StringVar()
        tk.Label(master=self, textvariable=self.frame_count_var).grid(
            column=1, row=0)

        tk.Label(master=self, text="Size:").grid(column=2, row=0)
        self.size_var = tk.StringVar()
        tk.Label(master=self, textvariable=self.size_var).grid(
            column=3, row=0)

        tk.Label(master=self, text="Origin:").grid(column=4, row=0)
        self.origin_var = tk.StringVar()
        tk.Label(master=self, textvariable=self.origin_var).grid(
            column=5, row=0)

        tk.Label(master=self, text="Palette:").grid(column=0, row=1)
        self.palette_var = tk.StringVar()
        tk.Label(master=self, textvariable=self.palette_var).grid(
            column=1, row=1)

        style = ttk.Style(parent)
        style.theme_use("clam")
        style.configure("Treeview", background="black",
                        fieldbackground="black", foreground="white")
        self.table = ttk.Treeview(self, columns=("offset", "size"))

        self.current_pal_combo_var = tk.StringVar()
        self.pal_combo = ttk.Combobox(
            self, textvariable=self.current_pal_combo_var)
        self.pal_combo.grid(column=0, row=2)
        self.pal_combo.bind('<<ComboboxSelected>>', self.pal_index_changed)
        self.pal_combo["state"] = "readonly"
        self.table.grid(column=0, row=3, columnspan=5)
        self.table.bind("<<TreeviewSelect>>", self.frame_sel_changed)
        self.table.heading("offset", text="offset")
        self.table.heading("size", text="size")

        self.table.column("offset", width=100)
        self.table.column("size", width=50, anchor="center")

        self.img_label = tk.Label(self)
        self.img_label.grid(column=0, row=4)

    def pal_index_changed(self, _):
        self.current_pal_combo_var.set(self.pal_combo.get())

    def frame_sel_changed(self, _):
        if len(self.table.selection()) == 0:
            return
        frame_iid = self.table.selection()[0]
        sel_item = self.table.item(frame_iid)
        frame_id = int(sel_item["text"])
        out_file = lol.get_temp_path_for("display.png")
        custom_pal = None if self.wsa_info.has_palette else self.current_pal_combo_var.get()
        if not lol.extract_wsa_frame(self.wsa_info, frame_id, out_file, custom_pal):
            return
        img_data = PIL.Image.open(out_file)
        img = PIL.ImageTk.PhotoImage(img_data)
        self.img_label.configure(image=img)
        self.img_label.image = img

    def clear_table(self):
        for i in self.table.get_children():
            self.table.delete(i)

    def update_for_item(self, file_name: str, pak_name: str):
        print(f"WSA Render: update for {file_name}")
        info = lol.get_wsa_info(file_name, pak_name)
        self.wsa_info = info
        if info is None:
            return
        self.frame_count_var.set(f"{info.frame_count}")
        self.size_var.set(f"{info.w}/{info.h}")
        self.origin_var.set(f"{info.x}/{info.y}")
        self.clear_table()
        for frame_id in range(self.wsa_info.frame_count):
            frame_info = self.wsa_info.frames[frame_id]
            self.table.insert(
                "", "end", text=f"{frame_id}", values=(frame_info.offset, frame_info.size))

        if not info.has_palette:
            print("No palette, looking for a cps file in the pak file")
            cps_files = lol.list(pak_name, "*.CPS")
            self.pal_combo["values"] = cps_files
            if len(cps_files) > 0:
                self.current_pal_combo_var.set(cps_files[0])

        self.palette_var.set("Yes" if info.has_palette else "No")


class VOCRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        self.file_name = ""
        self.pak_name = ""

        self.play_btton = ttk.Button(self, text="play")
        self.play_btton.bind("<Button-1>", self.play_voc)
        self.play_btton.pack()
        style = ttk.Style(parent)
        style.theme_use("clam")
        style.configure("Treeview", background="black",
                        fieldbackground="black", foreground="white")
        self.table = ttk.Treeview(self, columns=("duration", "samplerate"))
        self.table.heading("duration", text="duration")
        self.table.heading("samplerate", text="samplerate")
        self.table.pack(fill=tk.X, expand=True)

    def clear_table(self):
        for i in self.table.get_children():
            self.table.delete(i)

    def play_voc(self, _):
        out_file = lol.get_temp_path_for("voc.wav")
        if lol.extract_voc_file(self.file_name, self.pak_name, out_file):
            playsound(out_file)

    def update_for_item(self, file_name: str, pak_name: str):
        self.file_name = file_name
        self.pak_name = pak_name
        info = lol.get_voc_info(file_name, pak_name)
        # assert len(info.blocks) == 1
        self.clear_table()
        for bId, block in enumerate(info.blocks):
            self.table.insert(
                "", "end", text=f"{bId}", values=(block.duration, block.sample_rate))


class TIMRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)


class LANGRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        style = ttk.Style(parent)
        style.theme_use("clam")
        style.configure("Treeview", background="black",
                        fieldbackground="black", foreground="white")
        self.table = ttk.Treeview(self, columns="text")
        self.table.heading("text", text="text")

        self.table.pack(fill=tk.X, expand=True)

    def clear_table(self):
        for i in self.table.get_children():
            self.table.delete(i)

    def update_for_item(self, file_name: str, pak_name: str):
        print(f"{file_name} in {pak_name}")
        info = lol.extract_lang_file(file_name, pak_name)
        if info is None:
            return
        self.clear_table()
        for line_num, line in enumerate(info.lines):
            self.table.insert(
                "", "end", text=f"{line_num}", values=(line,))


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
        self.vnc_file: Optional[str] = None

    def frame_sel_changed(self, _):
        if len(self.table.selection()) == 0:
            return
        frame_iid = self.table.selection()[0]
        sel_item = self.table.item(frame_iid)
        frame_id = int(sel_item["text"])
        out_file = lol.get_temp_path_for("display.png")
        if not lol.extract_shp_frame(self.shp_info, frame_id, out_file, self.vnc_file):
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

        # is there a matching vcn file?
        vnc_file = self.shp_info.file.split(".")[0] + ".VCN"
        print(f"looking for {vnc_file} in {self.shp_info.pak_file}")
        if vnc_file in pak_files[self.shp_info.pak_file]:
            self.vnc_file = vnc_file
        else:
            self.vnc_file = None
        print(f"compressed = {self.shp_info.compressed}")
        print(f"{len(self.shp_info.desc)} frames")
        self.clear_table()
        for frame_id, frame in enumerate(self.shp_info.frames):
            self.table.insert(
                "", "end", text=f"{frame_id}", values=(frame.width, frame.height))


class ScriptRender(BaseRender):
    def __init__(self, parent):
        super().__init__(parent)
        self.original_asm = CodeViewer(self)
        self.original_asm.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.analysis_code = CodeViewer(self)
        self.analysis_code.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

    def update_for_item(self, file_name: str, pak_name: str):
        out_file = lol.get_temp_path_for("script.asm")
        self.original_asm.delete("1.0", tk.END)
        script_info = lol.get_script_info(file_name, pak_name, out_file)
        print(out_file)

        with open(out_file, "r", encoding="utf8") as f:
            lines = f.readlines()

            acc = 1
            for l in lines:
                self.original_asm.insert(f"{acc}.0", f"{acc}: {l}")
                acc += 1

            analysis = analyze_script(lines, script_info)
            if analysis:
                acc = 1
                for l in analysis:
                    self.analysis_code.insert(f"{acc}.0", f"{acc}: {l}")
                    acc += 1


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
        self._construct_file_tree()
        self.renders: Dict[str, BaseRender] = {}
        self.current_renderer: Optional[BaseRender] = None
        self._setup_renders()

    def _setup_renders(self):
        self._register_renderer("CPS", CPSRender)
        self._register_renderer("WSA", WSARender)
        self._register_renderer("SHP", SHPRender)
        self._register_renderer("TIM", TIMRender)
        self._register_renderer("LANG", LANGRender)
        self._register_renderer("VOC", VOCRender)
        self._register_renderer("VCN", VCNRender)
        self._register_renderer("INI", ScriptRender)
        self._register_renderer("INF", ScriptRender)

    def _register_renderer(self, name: str, cls):
        self.renders[name] = cls(self.details_frame)

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
        for pak_name, files in pak_files.items():
            pak_ext = pak_name.split(".")[-1]
            parent_id = self.file_tree.insert(
                "", "end", text=pak_name, values=pak_ext)
            for file in files:
                file_type = get_type(file)
                if pak_ext == "TLK":
                    file_type = "VOC"
                self.file_tree.insert(parent_id, "end", text=file,
                                      values=file_type)
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
        file_type = get_type(file_name)
        if pak_name.split(".")[-1] == "TLK":
            file_type = "VOC"
        self.change_tool_view(file_type)
        if self.current_renderer:
            self.current_renderer.update_for_item(file_name, pak_name)


if __name__ == "__main__":
    args = parser.parse_args()
    if not os.path.exists(args.pak_file):
        print(f"'{args.pak_file}' does not exist")
        sys.exit(1)
    if os.path.isdir(args.pak_file):
        lol.scan_dir(args.pak_file)
    elif os.path.isfile(args.pak_file):
        lol.pak_files.append(args.pak_file)
    else:
        print(f"unsupported path '{args.pak_file}'")
        sys.exit(1)
    for pak_file in lol.pak_files:
        pak_files[pak_file] = []
        for file in lol.list(pak_file):
            pak_files[pak_file].append(file)
    ui = UI()
    ui.run()
    print(f"ui returned, {len(lol.temp_files)} temp files to clean")
    lol.cleanup_temp_files()
