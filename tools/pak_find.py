from os import listdir
from os.path import isfile, join
import subprocess
import sys

pak_path = "data/cd"
pak_files = [join(pak_path, f) for f in listdir(pak_path) if isfile(
    join(pak_path, f)) and f.endswith(".PAK")]
# print(pak_files)


def main():
    file_to_find = sys.argv[1].upper()
    print(f"Looking for {file_to_find} in {pak_path}")
    for pak_file in pak_files:
        args = ["./lol", "-p", f"{pak_file}", "pak", "list"]
        proc_output = subprocess.run(
            args, stdout=subprocess.PIPE).stdout.decode()
        for l in proc_output.splitlines():
            if l == file_to_find:
                print(f"Found {file_to_find} in {pak_file}")
                return
    print("No such file")


if __name__ == "__main__":
    main()
