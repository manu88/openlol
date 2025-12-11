import argparse
from lol import LOL


parser = argparse.ArgumentParser()
parser.add_argument("file")
parser.add_argument("-e", action="store_true", help="extract file")


def main():
    lol = LOL()

    args = parser.parse_args()
    file_to_find: str = args.file
    print(f"Looking for {file_to_find} in {lol.data_dir}")
    num_found_files = 0
    for pak_file in lol.pak_files:
        files = lol.list(pak_file)
        for l in files:
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
