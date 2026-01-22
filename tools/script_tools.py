from typing import List, Optional
import sys
from lol import ScriptFileInfo
import tkinter as tk
from emc_script import builtins, PStr, PNum, PStrId, tok_maths, tok_ctl_flow, tok_op_codes



class Value:
    def __init__(self, val: int, is_arg=False, is_var=False, is_ret_code=False, ret_func: str = ""):
        self.val = val
        self.is_arg = is_arg
        self.is_var = is_var
        self.is_ret_code = is_ret_code
        self.ret_func = ret_func

    def get(self) -> str:
        if self.is_arg:
            return f"arg{self.val}"
        if self.is_var:
            return f"var{self.val}"
        if self.is_ret_code:
            return f"retCode({self.ret_func})"
        return int(self.val)


class FunctionCall:
    def __init__(self, original_line_start: int, original_line_end: int, id: str):
        self.original_line_start = original_line_start
        self.original_line_end = original_line_end
        self.id = id
        self.args: List[Value] = []

    def get(self) -> str:
        r = f"{self.id}("
        first = True
        for arg in self.args:
            if not first:
                r += ", "
            r += str(arg.get())
            first = False
        r += ")"
        return r

    def __repr__(self):
        return f"{self.original_line_start}:{self.original_line_end} : {self.get()}"


class ScriptAnalyzer:
    def __init__(self, lines: List[str], script_info: ScriptFileInfo):
        self.lines = lines
        self.script_info = script_info
        self.new_lines: List[str] = []
        self.last_call_func = ""
        self.functions: List[FunctionCall] = []

    def get_push_value(self, line: str) -> Value:
        tokens = line.split(" ")
        if tokens[0] == "PUSH":
            value = tokens[1]
            return Value(int(value, 16))
        if tokens[0] == "PUSHARG":
            value = tokens[1]
            return Value(int(value, 16), is_arg=True)
        if tokens[0] == "PUSHVAR":
            value = tokens[1]
            return Value(int(value, 16), is_var=True)
        if tokens[0] == "PUSHRC":
            return Value(0, is_ret_code=True, ret_func=self.last_call_func)
        print(f"unhandled {tokens[0]}")
        assert False

    def analyse_label(self, params: List[str], line: str,  line_num: int) -> Optional[str]:
        next_line = self.lines[line_num+1]
        tokens = next_line.split(" ")
        if tokens[0] != "JUMP":
            print(f"LABEL: expected JUMP at Line {line_num}")
        return line

    def simplify_math(self, line: str, line_num: int) -> Optional[str]:
        tokens = line.split(" ")
        print(f"simplify_math {line}")
        val1 = self.get_push_value(self.lines[line_num-1])
        val2 = self.get_push_value(self.lines[line_num-2])
        self.new_lines.pop()
        self.new_lines.pop()
        return f"{val1.get()} {tokens[0]} {val2.get()}"

    def analyse_call(self, params: List[str], line: str,  line_num: int) -> Optional[str]:
        line_start = line_num
        func_name = params[0]

        if func_name not in builtins:
            return None

        func = builtins[func_name]

        func_call = FunctionCall(
            original_line_start=line_start-len(func.params),
            original_line_end=line_num+1,
            id=func_name,
        )

        new_line = line
        if func.has_params():
            next_line = self.lines[line_num+1]
            if next_line.split(" ")[0] != "STACKRWD":
                print(f"CALL: unexpected mnemonic at {line_num}")
                assert 0

            for param_i, param in enumerate(func.params):
                new_line = new_line.rstrip() + " args: "
                line_start = line_num-param_i-1
                value: Value = self.get_push_value(
                    self.lines[line_num-param_i-1])
                func_call.args.append(value)
                if isinstance(param, PStr):
                    val = value.get()
                    new_line += f"{param_i}({param.name}):{val}"
                elif isinstance(param, PNum):
                    new_line += f"{param_i}({param.name}):{value.get()}"
                elif isinstance(param, PStrId):
                    new_line += f"{param_i}({param.name}):{value.get()}"
                else:
                    assert 0
            for param_i, param in enumerate(func.params):
                self.new_lines.pop()

        self.functions.append(func_call)
        self.last_call_func = func_name
        return new_line

    def analyze_line(self, line: str, line_num: int) -> Optional[str]:
        tokens = line.split(" ")
        mnemonic = tokens[0]
        params = tokens[1:]
        if mnemonic in tok_maths:
            return self.simplify_math(line, line_num)
        if mnemonic == "CALL":
            return self.analyse_call(params, line, line_num)
        if mnemonic == "LABEL":
            return self.analyse_label(params, line, line_num)
        return None

    def run(self):
        for i, line in enumerate(self.lines):
            new_line = self.analyze_line(line,  i)
            if new_line:
                self.new_lines.append(new_line + "\n")
            else:
                self.new_lines.append(line)


def analyze_script(lines: List[str], script_info: ScriptFileInfo) -> Optional[List[str]]:
    analyzer = ScriptAnalyzer(lines, script_info)
    try:
        analyzer.run()
    except Exception as e:
        print(e)

    ret = []
    for f in analyzer.functions:
        ret.append(f.get() + "\n")
        line_count = f.original_line_end - f.original_line_start
        for _ in range(line_count):
            ret.append("\n")
    return ret


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

    def find(self, text: str) -> List[str]:
        self.tag_remove('found', '1.0', tk.END)
        # ser = modify.get()
        idx = '1.0'
        lines = []
        while 1:
            idx = self.search(text, idx, nocase=1, stopindex=tk.END)
            if not idx:
                break
            lastidx = '%s+%dc' % (idx, len(text))
            self.tag_add('found', idx, lastidx)
            lines.append(idx)
            idx = lastidx
        self.tag_config('found', foreground='yellow')
        return lines


if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf8") as f_in:
        lines = f_in.readlines()
        info = ScriptFileInfo([])
        info.strings = [
            'GUARD6',
            'GUARD',
            'GUARD1',
            'GUARD2',
            'CLEANGD',
            'KING1',
            'CONFRONT.CPS',
            'CONFRONT',
            'TALAMSCA.CPS',
            'TALAMSCA',
            'DAWN1',
            'WILL1',
            'SPELLBKE',
            'SPELLBKF',
            'SPELLBKG',
            'NATE4',
            'GERIM4',
            'PAUL1',
            'DRGERIM',
            'GERIM.CPS',
            'GERIM1',
            'GERIM2',
            'GERIM3',
            'DRVICTOR',
            'VICTOR.CPS',
            'VICTOR1',
            'VICTOR2',
            'VICTOR3',
            'DRNATHAN',
            'Nathnial.CPS',
            'NATE1',
            'NATE2',
            'NATE3',
            'AUTOMAP',
            'PORTCLLS',

        ]
        ret = analyze_script(lines, info)
        with open("out.asm", "w", encoding="utf8") as f_out:
            f_out.writelines(ret)
