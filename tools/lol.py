import subprocess
from os.path import isfile, join
from os import listdir
from typing import List


class LOL:
    def __init__(self):
        self.tool_path = "./lol"
        self.data_dir = "./data"

        self.pak_files = [join(self.data_dir, f) for f in listdir(self.data_dir) if isfile(
            join(self.data_dir, f)) and f.endswith(".PAK")]

    def extract(self, file: str, pak: str) -> bool:
        argv = [self.tool_path, "-p", pak, "pak", "extract", file]
        subprocess.run(
            argv, stdout=subprocess.PIPE, check=False).stdout.decode()
        return True

    def list(self, pak: str) -> List[str]:
        argv = ["./lol", "-p", pak, "pak", "list"]
        proc_output = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False).stdout.decode()
        return proc_output.splitlines()
