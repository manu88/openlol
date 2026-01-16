from typing import List, Optional, Tuple
from enum import IntEnum
import sys
from lol import ScriptFileInfo


class ParamType(IntEnum):
    VALUE = 0
    STRING = 1


class BuiltinFunc:
    def __init__(self, params: List[Tuple[str, ParamType]]):
        self.params = params

    def has_params(self) -> bool:
        return len(self.params) > 0


builtins = {
    "loadBlockProperties": BuiltinFunc([("file", ParamType.STRING)]),
    "loadLangFile": BuiltinFunc([("file", ParamType.STRING)]),
    "loadLevelShapes": BuiltinFunc([("shp", ParamType.STRING), ("datFile", ParamType.STRING)]),
}


def get_push_value(line: str) -> int:
    tokens = line.split(" ")
    assert (tokens[0] == "PUSH")
    value = tokens[1]
    return int(value, 16)


def analyze_line(line: str, script_info: ScriptFileInfo, line_num: int, lines: List[str]) -> str:
    tokens = line.split(" ")
    mnemonic = tokens[0]
    params = tokens[1:]
    if mnemonic == "CALL":
        func_name = params[0]
        if func_name not in builtins:
            return line
        func = builtins[func_name]
        if not func.has_params():
            return line
        new_line = line
        for param_i, (param_name, param_type) in enumerate(func.params):
            print(param_type)
            value = get_push_value(lines[line_num-param_i-1])
            if param_type == ParamType.STRING:
                val = script_info.strings[value]
                print(val)
                new_line = f"{new_line.rstrip()} args: {param_i}({param_name}):{val}"
        print(f"returned line :'{new_line}'")
        return new_line
    return line


def analyze_script(lines: List[str], script_info: ScriptFileInfo) -> Optional[List[str]]:
    ret: List[str] = []
    for i, line in enumerate(lines):
        new_line = analyze_line(line, script_info, i, lines)
        ret.append(new_line + "\n")
    return ret


if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf8") as f:
        lines = f.readlines()
        info = ScriptFileInfo([])
        info.strings = [
            'KEEP',
            'LEVEL01',
            'KEEPDOOR.SHP',
            'LEVEL1.CMZ',
            'GUARD.SHP',
            'KEEP.DAT',
            'KEEP.SHP',
            'ORCLDR1',
            'SWING1',
            'MALEOOF2',

        ]
        ret = analyze_script(lines, info)
