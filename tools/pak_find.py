from os import listdir
from os.path import isfile, join
import subprocess
import sys
import argparse

pak_path = "data/cd"
pak_files = [join(pak_path, f) for f in listdir(pak_path) if isfile(
    join(pak_path, f)) and f.endswith(".PAK")]
# print(pak_files)

parser = argparse.ArgumentParser()
parser.add_argument("file")
parser.add_argument("-e", action="store_true", help="extract file")


def extract(file: str, pak: str):
    argv = ["./lol", "-p", pak, "pak", "extract", file]
    proc_output = subprocess.run(
        argv, stdout=subprocess.PIPE).stdout.decode()
    print(f"extracted {file}")


def main():
    args = parser.parse_args()
    file_to_find = args.file
    print(f"Looking for {file_to_find} in {pak_path}")
    for pak_file in pak_files:
        argv = ["./lol", "-p", pak_file, "pak", "list"]
        proc_output = subprocess.run(
            argv, stdout=subprocess.PIPE).stdout.decode()
        for l in proc_output.splitlines():
            if l == file_to_find:
                print(f"Found {file_to_find} in {pak_file}")
                if (args.e):
                    extract(file_to_find, pak_file)
                return
    print("No such file")


if __name__ == "__main__":
    main()
