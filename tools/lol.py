import subprocess
import tempfile
import fnmatch
from os.path import isfile, join
from os import listdir
from typing import List, Optional


class LangFileInfo:
    def __init__(self, desc: List[str]):
        self.desc = desc
        self.lines: List[str] = []
        self._parse()

    def _parse(self):
        # i=42 offset=1164 size=8 text=A lock.
        for line in self.desc:
            text = line.split("text=")[1]
            self.lines.append(text)


class SHPFileInfo:
    class SHPFrameInfo:
        def __init__(self):
            self.flags0 = 0
            self.flags1 = 1
            self.width = 0
            self.height = 0

        def from_data(self, data: str):
            for param in data.split(" "):
                if param == "":
                    continue
                name, val = param.split("=")
                if name == "w":
                    self.width = int(val)
                elif name == "h":
                    self.height = int(val)

    def __init__(self, desc: List[str], compressed=False):
        self.file = ""
        self.pak_file = ""
        self.compressed = compressed
        self.desc: List[str] = desc
        self.frames: List[SHPFileInfo.SHPFrameInfo] = []
        self._parse()

    def _parse(self):
        index = 0
        for l in self.desc:
            info = SHPFileInfo.SHPFrameInfo()
            frame_id_str, the_rest = l.split(":")

            frame_id = int(frame_id_str)
            assert frame_id == index
            info.from_data(the_rest)
            self.frames.append(info)
            index += 1


class VOCFileInfo:
    class VOCBlock:
        def __init__(self):
            self.type = 0
            self.size = 0

        def from_data(self, data: str):
            for param in data.split(" "):
                if param == "":
                    continue
                name, val = param.split("=")
                if name == "type":
                    self.type = int(val)
                elif name == "size":
                    self.size = int(val)

    def __init__(self, desc: List[str]):
        self.desc = desc
        self.lines: List[str] = []
        self.blocks: List[VOCFileInfo.VOCBlock] = []
        self._parse()

    def _parse(self):
        for l in self.desc:
            block = VOCFileInfo.VOCBlock()
            block.from_data(l)
            self.blocks.append(block)


class WSAFileInfo:
    class WSAFrameInfo:
        def __init__(self):
            self.offset = 0
            self.size = 0

        def from_data(self, data: str):
            # i=0 offset=46 size=5F04:
            for param in data.split(" "):
                if param == "":
                    continue
                name, val = param.split("=")
                if name == "offset":
                    self.offset = int(val, 0)
                elif name == "size":
                    self.size = int(val, 0)

    def __init__(self, desc: List[str]):
        self.file = ""
        self.pak_file = ""
        self.frame_count = 0
        self.x = 0
        self.y = 0
        self.w = 0
        self.h = 0
        self.has_palette = False
        self.frames: List[WSAFileInfo.WSAFrameInfo] = []
        self._parse(desc)

    def _parse(self, desc: List[str]):
        general_info = desc[0].split(" ")
        for param in general_info:
            if param == "":
                continue
            name, val = param.split("=")
            if name == "numFrame":
                self.frame_count = int(val)
            elif name == "x":
                self.x = int(val)
            elif name == "y":
                self.y = int(val)
            elif name == "w":
                self.w = int(val)
            elif name == "h":
                self.h = int(val)
            elif name == "palette":
                if int(val) != 0:
                    self.has_palette = True
        for line in desc[1:]:
            if line.startswith("IS ZERO"):
                continue
            frame = WSAFileInfo.WSAFrameInfo()
            frame.from_data(line)
            self.frames.append(frame)

            # i=0 offset=46 size=5F04:
        # numFrame=38 x=0 y=0 w=72 h=64 palette=0 delta=EF9


def _do_exec(argv: List, output=True):
    return subprocess.run(
        argv, stdout=subprocess.PIPE if output else None, check=False)


class LOL:
    def __init__(self):
        self.tool_path = "./lol"
        self.temp_dir = tempfile.mkdtemp()
        self.pak_files: List[str] = []

    def scan_dir(self, pak_dir: str):
        print(f"Lol: scanning dir {pak_dir}")
        self.pak_files = [join(pak_dir, f) for f in listdir(pak_dir) if isfile(
            join(pak_dir, f)) and f.endswith(".PAK")]

    def get_temp_path_for(self, file_name: str) -> str:
        return join(self.temp_dir, file_name)

    def extract(self, file: str, pak: str) -> bool:
        argv = [self.tool_path, "-p", pak, "pak", "extract", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            print(resp)
            return False
        return True

    def extract_cps(self, file: str, pak: str, out_file: str) -> bool:
        argv = [self.tool_path, "-p", pak, "cps", "extract", file, out_file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            print(resp)
            return False
        return True

    def list(self, pak: str, pattern: str = "*") -> Optional[List[str]]:
        argv = ["./lol", "-p", pak, "pak", "list"]
        resp = _do_exec(argv)
        proc_output = resp.stdout.decode()
        if resp.returncode != 0:
            print(resp)
            return None
        ret = []
        for file in proc_output.splitlines():
            if fnmatch.fnmatch(file, pattern):
                ret.append(file)
        return ret

    def extract_shp_frame(self, shp_info: SHPFileInfo, frame_id: int, out_file: str, vnc_file: Optional[str] = None) -> bool:
        argv = [self.tool_path, "-p", shp_info.pak_file, "shp"]
        if shp_info.compressed:
            argv.append("-c")
        argv += ["extract", shp_info.file, str(frame_id), out_file]
        if vnc_file:
            argv.append(vnc_file)
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return False
        return True

    def get_shp_info(self, file: str, pak: str) -> Optional[SHPFileInfo]:
        info = self._get_shp_info_uncompressed(file, pak)
        if info is None:
            info = self._get_shp_info_compressed(file, pak)
        info.file = file
        info.pak_file = pak
        return info

    def extract_lang_file(self, file: str, pak: str) -> Optional[LangFileInfo]:
        argv = [self.tool_path, "-p", pak, "lang", "show", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return None
        proc_output = resp.stdout.decode()
        return LangFileInfo(proc_output.splitlines())

    def _get_shp_info_uncompressed(self, file: str, pak: str) -> Optional[SHPFileInfo]:
        argv = [self.tool_path, "-p", pak, "shp", "info", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return None
        proc_output = resp.stdout.decode()
        return SHPFileInfo(proc_output.splitlines())

    def _get_shp_info_compressed(self, file: str, pak: str) -> Optional[SHPFileInfo]:
        argv = [self.tool_path, "-p", pak, "shp", "-c", "info", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return None
        proc_output = resp.stdout.decode()
        return SHPFileInfo(proc_output.splitlines(), compressed=True)

    def get_wsa_info(self, file: str, pak: str) -> Optional[WSAFileInfo]:
        argv = [self.tool_path, "-p", pak, "wsa", "info", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return None
        proc_output = resp.stdout.decode()
        info = WSAFileInfo(proc_output.splitlines())
        info.file = file
        info.pak_file = pak
        return info

    def extract_wsa_frame(self, wsa_info: WSAFileInfo, frame_id: int, out_file: str, cps_palette_file: Optional[str] = None) -> bool:
        argv = [self.tool_path, "-p", wsa_info.pak_file, "wsa"]
        argv += ["extract", wsa_info.file, str(frame_id), out_file]
        if cps_palette_file is not None:
            argv.append(cps_palette_file)
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return False
        return True

    def get_voc_info(self, file: str, pak: str) -> Optional[VOCFileInfo]:
        argv = [self.tool_path, "-p", pak, "voc", "info", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return None
        proc_output = resp.stdout.decode()
        return VOCFileInfo(proc_output.splitlines())


lol = LOL()
