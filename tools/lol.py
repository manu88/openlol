import subprocess
import tempfile
from os.path import isfile, join
from os import listdir
from typing import List, Optional


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


class WSAFileInfo:
    def __init__(self, desc: List[str]):
        self.file = ""
        self.pak_file = ""
        self.frame_count = 0
        self.x = 0
        self.y = 0
        self.w = 0
        self.h = 0
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
        # numFrame=38 x=0 y=0 w=72 h=64 palette=0 delta=EF9


def _do_exec(argv: List, output=True):
    return subprocess.run(
        argv, stdout=subprocess.PIPE if output else None, check=False)


class LOL:
    def __init__(self):
        self.tool_path = "./lol"
        self.data_dir = "./data"
        self.temp_dir = tempfile.mkdtemp()
        self.pak_files = [join(self.data_dir, f) for f in listdir(self.data_dir) if isfile(
            join(self.data_dir, f)) and f.endswith(".PAK")]

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

    def list(self, pak: str) -> Optional[List[str]]:
        argv = ["./lol", "-p", pak, "pak", "list"]
        resp = _do_exec(argv)
        proc_output = resp.stdout.decode()
        if resp.returncode != 0:
            print(resp)
            return None
        return proc_output.splitlines()

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

    def extract_wsa_frame(self, wsa_info: WSAFileInfo, frame_id: int, out_file: str) -> bool:
        argv = [self.tool_path, "-p", wsa_info.pak_file, "wsa"]
        argv += ["extract", wsa_info.file, str(frame_id), out_file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            return False
        return True


lol = LOL()
