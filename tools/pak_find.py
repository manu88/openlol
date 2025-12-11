from os import listdir
from os.path import isfile, join
import subprocess
import argparse
from lol import LOL


parser = argparse.ArgumentParser()
parser.add_argument("file")
parser.add_argument("-e", action="store_true", help="extract file")


def extract(file: str, pak: str):
    argv = ["./lol", "-p", pak, "pak", "extract", file]
    subprocess.run(
        argv, stdout=subprocess.PIPE, check=False).stdout.decode()
    print(f"extracted {file}")


def main():
    lol = LOL()

    args = parser.parse_args()
    file_to_find: str = args.file
    print(f"Looking for {file_to_find} in {lol.data_dir}")
    num_found_files = 0
    for pak_file in lol.pak_files:
        argv = ["./lol", "-p", pak_file, "pak", "list"]
        proc_output = subprocess.run(
            argv, stdout=subprocess.PIPE, check=False).stdout.decode()
        for l in proc_output.splitlines():
            if file_to_find == "*":
                print(f"{pak_file}: {l}")
            elif file_to_find[0] == "*":
                ext_to_find = file_to_find.split(".")[1]
                file_ext = l.split(".")[1]
                if ext_to_find == file_ext:
                    print(f"Found {l} in {pak_file}")
                    num_found_files += 1
            elif l == file_to_find:
                print(f"Found {file_to_find} in {pak_file}")
                num_found_files += 1
                if args.e:
                    lol.extract(file_to_find, pak_file)
                return
    if num_found_files == 0:
        print("No such file")
    else:
        print(f"found {num_found_files} files")


if __name__ == "__main__":
    main()
