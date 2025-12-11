import subprocess
import tempfile
from os.path import isfile, join
from os import listdir
from typing import List, Optional


class LOL:
    def __init__(self):
        self.tool_path = "./lol"
        self.data_dir = "./data"
        self.temp_dir = tempfile.mkdtemp()
        self.pak_files = [join(self.data_dir, f) for f in listdir(self.data_dir) if isfile(
            join(self.data_dir, f)) and f.endswith(".PAK")]

    def get_temp_path_for(self, file_name: str)->str:
        return join(self.temp_dir, file_name)

    def extract(self, file: str, pak: str) -> bool:
        argv = [self.tool_path, "-p", pak, "pak", "extract", file]
        resp = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False)
        if resp.returncode != 0:
            print(resp)
            return False
        return True

    def extract_cps(self, file: str, pak: str, out_file: str) -> bool:
        argv = [self.tool_path, "-p", pak, "cps", "extract", file, out_file]
        resp = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False)
        if resp.returncode != 0:
            print(resp)
            return False
        return True

    def list(self, pak: str) -> Optional[List[str]]:
        argv = ["./lol", "-p", pak, "pak", "list"]
        resp = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False)
        proc_output = resp.stdout.decode()
        if resp.returncode != 0:
            print(resp)
            return None
        return proc_output.splitlines()

    def file_info(self, file: str, pak: str, typ: str) -> Optional[List[str]]:
        argv = [self.tool_path, "-p", pak, typ, "info", file]
        resp = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False)
        if resp.returncode != 0:
            print(resp)
            return None
        proc_output = resp.stdout.decode()
        return proc_output.splitlines()
