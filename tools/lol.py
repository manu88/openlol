import subprocess
from os.path import isfile, join
from os import listdir


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
