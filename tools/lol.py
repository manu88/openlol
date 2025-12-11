import subprocess


class LOL:
    def __init__(self):
        self.tool_path = "./lol"

    def extract(self, file: str, pak: str) -> bool:
        argv = [self.tool_path, "-p", pak, "pak", "extract", file]
        subprocess.run(
            argv, stdout=subprocess.PIPE, check=False).stdout.decode()
        return True
