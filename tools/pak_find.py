import argparse
from lol import lol


parser = argparse.ArgumentParser()
parser.add_argument("file")
parser.add_argument("-e", action="store_true", help="extract file")


def main():
    lol.scan_dir("./data")
    args = parser.parse_args()
    file_to_find: str = args.file
    num_found_files = 0
    for pak_file in lol.pak_files:
        files = lol.list(pak_file, pattern=file_to_find)
        for l in files:
            print(f"{l} in {pak_file}")
            if args.e:
                lol.extract(l, pak_file)
            num_found_files += 1
            continue
    if num_found_files == 0:
        print("No such file")
    else:
        print(f"found {num_found_files} files")


if __name__ == "__main__":
    main()
