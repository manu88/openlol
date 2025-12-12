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


#  framenum: flags0=%02X flags1=%02X slices=%i w=%i h=%i fsize=%i "
#         "zeroCompressedSize=%i hasRemapTable=%i noLCW=%i "
#         "customSizeRemap=%i remapSize %i remapTable %p image data %p "
#         "header=%i\n",


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

    def get_shp_info(self, file: str, pak: str) -> Optional[SHPFileInfo]:
        info = self._get_shp_info_uncompressed(file, pak)
        if info is None:
            info = self._get_shp_info_compressed(file, pak)
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

    def file_info(self, file: str, pak: str, typ: str) -> Optional[List[str]]:
        argv = [self.tool_path, "-p", pak, typ, "info", file]
        resp = _do_exec(argv)
        if resp.returncode != 0:
            print(resp)
            return None
        proc_output = resp.stdout.decode()
        return proc_output.splitlines()
