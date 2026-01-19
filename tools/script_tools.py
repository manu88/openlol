from typing import List, Optional
import sys
from lol import ScriptFileInfo
import tkinter as tk


class Param:
    def __init__(self, name: str):
        self.name = name


class PStr(Param):
    pass


class PNum(Param):
    pass


class Func:
    def __init__(self, params: List[Param]):
        self.params = params

    def has_params(self) -> bool:
        return len(self.params) > 0


builtins = {
    "loadBlockProperties": Func([PStr("file")]),
    "loadLangFile": Func([PStr("file")]),
    "loadLevelShapes": Func([PStr("shp"), PStr("datFile")]),
    "loadBitmap": Func([PStr("file"), PNum("param")]),
    "loadMonsterShapes": Func([PStr("file"), PNum("monsterId"), PNum("p2")]),
    "loadTimScript": Func([PStr("scriptId"), PStr("stringId")]),
    "initAnimStruct": Func([PStr("file"), PNum("index"), PNum("x"), PNum("y"), PNum("offscreenBuffer"), PNum("wsaFlags")]),
    "checkRectForMousePointer": Func([PNum("xMin"), PNum("yMin"), PNum("xMax"), PNum("yMax")]),
    "setGameFlag": Func([PNum("flag"), PNum("val")]),
    "testGameFlag": Func([PNum("flag")]),
    "loadMusicTrack": Func([PNum("file")]),
}


def get_push_value(line: str) -> int:
    tokens = line.split(" ")
    if tokens[0] != "PUSH":
        print(f"expected PUSH, got {line}")
        return 0
    value = tokens[1]
    return int(value, 16)


def analyze_line(line: str, script_info: ScriptFileInfo, line_num: int, lines: List[str]) -> Optional[str]:
    tokens = line.split(" ")
    mnemonic = tokens[0]
    params = tokens[1:]
    if mnemonic == "CALL":
        func_name = params[0]
        if func_name not in builtins:
            return None
        func = builtins[func_name]
        if not func.has_params():
            return None
        new_line = line
        for param_i, param in enumerate(func.params):
            new_line = new_line.rstrip() + " args: "
            value = get_push_value(lines[line_num-param_i-1])
            if isinstance(param, PStr):
                val = script_info.strings[value]
                new_line += f"{param_i}({param.name}):{val}"
            elif isinstance(param, PNum):
                new_line += f"{param_i}({param.name}):{value}"
            else:
                assert (0)
        return new_line
    return None


def analyze_script(lines: List[str], script_info: ScriptFileInfo) -> Optional[List[str]]:
    ret: List[str] = []
    for i, line in enumerate(lines):
        new_line = analyze_line(line, script_info, i, lines)
        if new_line:
            ret.append(new_line + "\n")
        else:
            ret.append(line)
    return ret


tok_op_codes = [
    "CALL",
    "PUSH",
    "GOTO",
    "PUSHARG",
    "POPRC",
    "STACKRWD",
    "POPLOCVAR",
    "PUSHRC",
    "STACKRWD",
    "STACKFWD",
    "PUSHLOCVAR",
    "PUSHVAR",
    "POP",
]

tok_ctl_flow = [
    "JUMP",
    "IFNOTGO",
    "IF",
]

tok_maths = {
    "INF",
    "INFEQ",
    "OR",
    "AND",
    "EQUAL",
    "NEQUAL",
    "SUP",
    "ADD",
    "LAND",
    "LOR",
    "MULTIPLY",
    "MINUS",
    "LSHIFT",
}


class CodeViewer(tk.Text):
    def set_lines(self, lines: List[str]):
        acc = 1
        for l in lines:
            self.insert(f"{acc}.0", f"{acc:02}: {l}")
            acc += 1
        self.highlight_syntax()
        self.config(state=tk.DISABLED)

    def _highlight(self, token: str, txt_color: str = "", back_col: str = ""):
        idx = '1.0'
        tag_name = token
        while 1:
            idx = self.search(token, idx, nocase=0,
                              stopindex=tk.END, exact=True)
            if not idx:
                break
            lastidx = '%s+%dc' % (idx, len(token))

            self.tag_add(tag_name, idx, lastidx)
            idx = lastidx
        self.tag_config(tag_name, foreground=txt_color, background=back_col)

    def highlight_syntax(self):
        for tok in tok_op_codes:
            self._highlight(tok + " ", "green")
        for tok in tok_ctl_flow:
            self._highlight(tok + " ", "lightblue")
        for tok in tok_maths:
            self._highlight(tok + " ", "red")
        self._highlight("LABEL ", back_col="red")

    def find(self, text: str):
        self.tag_remove('found', '1.0', tk.END)
        # ser = modify.get()
        idx = '1.0'
        count = 0
        while 1:
            idx = self.search(text, idx, nocase=1, stopindex=tk.END)
            if not idx:
                break
            lastidx = '%s+%dc' % (idx, len(text))

            self.tag_add('found', idx, lastidx)
            idx = lastidx
            count += 1
        self.tag_config('found', foreground='yellow')
        return count


if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf8") as f_in:
        lines = f_in.readlines()
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
        with open("out.asm", "w", encoding="utf8") as f_out:
            f_out.writelines(ret)
