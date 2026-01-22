import sys
from typing import List, Optional
from emc_script import tok_maths, builtins


class ASTNode:
    pass


class AST_ARG(ASTNode):
    def __init__(self, arg_index):
        self.arg_index = arg_index
        super().__init__()


class AST_VAR(ASTNode):
    def __init__(self, var_index):
        self.var_index = var_index
        super().__init__()


class AST_CONST(ASTNode):
    def __init__(self, value):
        self.value = value
        super().__init__()


class Value:
    pass


class ValueLit(Value):
    def __init__(self, value: int):
        super().__init__()
        self.value = value

    def __str__(self):
        return f"val lit={hex(self.value)}"


class ValueRet(Value):
    def __init__(self, call):
        super().__init__()
        self.call = call

    def __str__(self):
        return f"val ret={self.call}"


class ValueARG(Value):
    def __init__(self, index: int):
        super().__init__()
        self.index = index

    def __str__(self):
        return f"val arg{self.index}"


class ValueVAR(Value):
    def __init__(self, index: int):
        super().__init__()
        self.index = index

    def __str__(self):
        return f"val var{self.index}"


class ValueOp(Value):
    def __init__(self, op: str, lhs: Value, rhs: Value, ):
        super().__init__()
        self.op = op
        self.lhs = lhs
        self.rhs = rhs

    def __str__(self):
        return f"val {self.lhs} {self.op} {self.rhs}"


class Instruction:
    def __init__(self):
        self.addr = 0


class FuncCall(Instruction):
    def __init__(self, name: str):
        super().__init__()
        self.name = name
        self.argc = 0

    def __str__(self):
        return f"func call {self.name} ({self.argc} args)"


class Assignment(Instruction):
    def __init__(self, value: Value):
        super().__init__()
        self.value: Value = value

    def __str__(self):
        return f"Assignment {self.value}"


class Goto(Instruction):
    def __init__(self, addr):
        super().__init__()
        self.addr = addr

    def __str__(self):
        return f"Goto {hex(self.addr)}"


class IfNotGoto(Goto):
    def __init__(self, addr, condition: Value):
        super().__init__(addr=addr)
        self.condition = condition

    def __str__(self):
        return f"IfNot ( {self.condition} ) {super().__str__()}"


class Parser:
    def __init__(self):
        self.instructions: List[Instruction] = []
        self.labels: dict[int, int] = {}  # key is label number, val is addr
        self.current_offset = 0

    def _do_math_op(self, mnemonic: str) -> Optional[Instruction]:
        assert len(self.instructions) >= 2
        ass1 = self.instructions.pop()
        ass2 = self.instructions.pop()
        return Assignment(ValueOp(op=mnemonic, lhs=ass1.value, rhs=ass2.value))

    def _emit_call(self, func_name: str) -> Optional[Instruction]:
        return FuncCall(name=func_name)

    def emit_instr(self, mnemonic: str, params: List[str]) -> Optional[Instruction]:
        if mnemonic.startswith("#"):
            return None
        if mnemonic == "LABEL":
            num = int(params[0], 16)
            assert num not in self.labels
            self.labels[num] = self.current_offset+1
            return None
        if mnemonic == "PUSH":
            return Assignment(ValueLit(int(params[0], 16)))
        if mnemonic == "PUSHARG":
            return Assignment(ValueARG(int(params[0], 16)))
        if mnemonic == "PUSHVAR":
            return Assignment(ValueVAR(int(params[0], 16)))
        if mnemonic in tok_maths:
            return self._do_math_op(mnemonic)
        if mnemonic == "CALL":
            return self._emit_call(params[0])
        if mnemonic == "JUMP":
            return Goto(int(params[0], 16))
        if mnemonic == "IFNOTGO":
            condition = self.instructions.pop()
            assert isinstance(condition, Assignment)
            return IfNotGoto(int(params[0], 16), condition.value)
        if mnemonic == "PUSHRC":
            call_instruction = None
            for inst in reversed(self.instructions):
                if isinstance(inst, FuncCall):
                    call_instruction = inst
                    break
            assert call_instruction is not None
            self.instructions.remove(call_instruction)
            return Assignment(ValueRet(call_instruction))
        if mnemonic == "STACKRWD":
            last_instr = self.instructions[-1]
            if isinstance(last_instr, FuncCall):
                last_instr.argc = int(params[0], 16)
                return None
            print(
                f"warning: STACKRWD is currently expected to be after a FuncCall not {last_instr} {type(last_instr)}")
            return None
        print(f"unhandled {mnemonic}")
        return None

    def _process_line(self, line: str):
        tokens = line.rstrip().split(" ")
        if len(tokens[0]) == 0:
            return
        instruction = self.emit_instr(tokens[0], tokens[1:])
        if instruction:
            instruction.addr = self.current_offset
            self.instructions.append(instruction)

    def get_instruction_at(self, addr: int) -> Optional[Instruction]:
        for inst in self.instructions:
            if inst.addr == addr:
                return inst
        return None

    def _check_addrs(self):
        for lbl_addr in self.labels.values():
            inst = self.get_instruction_at(lbl_addr)
            if inst is None:
                print(f"didn't found instruction for addr {hex(lbl_addr)}")
                assert False

        for inst in self.instructions:
            if issubclass(type(inst), Goto):
                inst = self.get_instruction_at(inst.addr)
                assert inst

    def process(self, lines: list[str]):
        for i, line in enumerate(lines):
            try:
                if len(line):
                    self._process_line(line.lstrip())
            except Exception as e:
                print(f"error at line {i}")
                raise e
            self.current_offset += 1

        self._check_addrs()


_math_op_code = {
    "EQUAL": "==",
    "INF": "<",
    "INFEQ": "<=",
    "OR": "||",
    "AND": "&&",
    "NEQUAL": "!=",
    "SUP": ">",
    "SUPEQ": ">=",
    "ADD": "+",
    "LAND": "&",
    "LOR": "|",
    "MULTIPLY": "*",
    "MINUS": "-",
    "LSHIFT": "<<",
    "XOR": "^",
    "RSHIFT": ">>",
}


not_tok_maths = {
    "INF": "SUPEQ",
    "INFEQ": "SUP",
    "EQUAL": "NEQUAL",
}

not_tok_maths2 = {}
for tok1, tok2 in not_tok_maths.items():
    not_tok_maths2[tok2] = tok1

not_tok_maths.update(not_tok_maths2)


class CodeGen:
    def __init__(self, parser: Parser):
        self.parser = parser
        self.index = 0
        self.lines: List[str] = []

    def _rewrite_var_name(self, index: int, name: str):
        line_idx = self.index-index-1
        print(f"should rewrite var at {line_idx} to {name}")
        old_line = self.lines[line_idx]
        assign = old_line.split(" := ")[1]
        self.lines[line_idx] = f"{name} := {assign}"

    def _gen_func_call(self, call: FuncCall) -> Optional[str]:
        if call.name not in builtins:
            return f"{call.name}(TODO ARGS)"
        func_def = builtins[call.name]
        r = f"{call.name}("
        for i, arg in enumerate(func_def.params):
            if i > 0:
                r += ", "
            r += arg.name
            self._rewrite_var_name(i, arg.name)
        r += ")"
        return r

    def _gen_goto(self, inst: Goto) -> Optional[str]:
        return f"Goto {hex(inst.addr)}"

    def _gen_ifnotgoto(self, inst: IfNotGoto) -> Optional[str]:
        invert_op = inst.condition.op in not_tok_maths
        r = "if("
        if invert_op is False:
            r += "! "
        return r + f"{self._gen_val(inst.condition, invert_op=invert_op)}) then {self._gen_goto(inst)}"

    def _gen_val(self, val: Value, invert_op=False) -> Optional[str]:
        if isinstance(val, ValueLit):
            return hex(val.value)
        if isinstance(val, ValueRet):
            return self._gen_func_call(val.call)
        if isinstance(val, ValueOp):
            assert val.op in _math_op_code
            if invert_op:
                op = not_tok_maths[val.op]
            else:
                op = val.op
            op_code = _math_op_code[op]
            op = f" {op_code} "
            return self._gen_val(val.rhs) + op + self._gen_val(val.lhs)
        if isinstance(val, ValueARG):
            return f"ARG[{hex(val.index)}]"
        print(f"CodeGen: unhandled val type {type(val)}")
        return None

    def _gen_inst(self, inst: Instruction) -> Optional[str]:
        if isinstance(inst, Assignment):
            assign = self._gen_val(inst.value)
            assert assign
            return f"var_{self.index} := {assign}"
        if isinstance(inst, FuncCall):
            return self._gen_func_call(inst)
        if isinstance(inst, IfNotGoto):
            return self._gen_ifnotgoto(inst)
        if isinstance(inst, Goto):
            return self._gen_goto(inst)
        print(f"CodeGen: unhandled inst {inst} type{type(inst)}")
        return None

    def process(self) -> List[str]:
        for instruction in parser.instructions:
            print(instruction)
            line = self._gen_inst(instruction)
            if line:
                self.lines.append(line)
            self.index += 1

        return self.lines


if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf8") as f_in:
        lines = f_in.readlines()
        parser = Parser()
        parser.process(lines)

        gen = CodeGen(parser)
        new_lines = gen.process()
        print("--------- Generated Code ---------")
        for l in new_lines:
            print(l)
