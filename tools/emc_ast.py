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


class UnaryOp(Value):
    def __init__(self, val: Value, unary: str):
        super().__init__()
        self.value = val
        self.unary = unary

    def __str__(self):
        return f"{self.unary}{self.value}"


class Instruction:
    def __init__(self):
        self.addr = 0
        self.is_jump_dest = False


class NOP(Instruction):
    def __init__(self, mnemonic: str):
        super().__init__()
        self.mnemonic = mnemonic

    def __str__(self):
        return f"NOP {self.mnemonic}()"


class FuncCall(Instruction):
    def __init__(self, name: str):
        super().__init__()
        self.name = name

    def __str__(self):
        return f"func call {self.name}()"


class Assignment(Instruction):
    def __init__(self, value: Value):
        super().__init__()
        self.value: Value = value

    def __str__(self):
        return f"Assignment {self.value}"


class Return(Instruction):
    def __str__(self):
        return "Return"


class Goto(Instruction):
    def __init__(self, target):
        super().__init__()
        self.target = target

    def __str__(self):
        return f"Goto {hex(self.target)}"


class IfNotGoto(Goto):
    def __init__(self, target, condition: Value):
        super().__init__(target=target)
        self.condition = condition

    def __str__(self):
        return f"IfNot ( {self.condition} ) {super().__str__()}"


class Parser:
    def __init__(self):
        self.instructions: List[Instruction] = []
        self.labels: dict[int, int] = {}  # key is label number, val is addr
        self.current_offset = 0
        self.goto_targets = set()

    def addr_is_label(self, addr: int) -> int:
        for lbl_num, lbl_addr in self.labels.items():
            if addr == lbl_addr:
                return lbl_num
        return -1

    def _do_unary(self, how: int):
        last_ass = self.instructions[-1]
        assert isinstance(last_ass, Assignment)
        if how == 0:
            new_val = UnaryOp(last_ass.value, "!")
        elif how == 1:
            new_val = UnaryOp(last_ass.value, "-")
        elif how == 2:
            new_val = UnaryOp(last_ass.value, "~")
        else:
            assert False
        last_ass.value = new_val

    def _do_math_op(self, mnemonic: str) -> Optional[Instruction]:
        assert len(self.instructions) >= 2
        ass1 = self.instructions.pop()
        ass2 = self.instructions.pop()
        return Assignment(ValueOp(op=mnemonic, lhs=ass1.value, rhs=ass2.value))

    def _emit_call(self, func_name: str) -> Optional[Instruction]:
        return FuncCall(name=func_name)

    def emit_instr(self, mnemonic: str, params: List[str]) -> Optional[Instruction]:
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
        if mnemonic == "UNARY":
            how = int(params[0], 16)
            self._do_unary(how)
            return None
        if mnemonic == "CALL":
            return self._emit_call(params[0])
        if mnemonic == "JUMP":
            addr = int(params[0], 16)
            self.goto_targets.add(addr)
            return Goto(addr)
        if mnemonic == "IFNOTGO":
            condition = self.instructions.pop()
            assert isinstance(condition, Assignment)
            addr = int(params[0], 16)
            self.goto_targets.add(addr)
            return IfNotGoto(addr, condition.value)
        if mnemonic == "POPRC":
            arg = int(params[0], 16)
            if arg == 1:
                return Return()
            return None
        if mnemonic == "PUSHRC":
            arg = int(params[0], 16)
            if arg == 0:
                call_instruction = None
                for inst in reversed(self.instructions):
                    if isinstance(inst, FuncCall):
                        call_instruction = inst
                        break
                assert call_instruction is not None
                self.instructions.remove(call_instruction)
                return Assignment(ValueRet(call_instruction))
            if arg == 1:
                # saves the ip before JUMP to restore the context
                return None
            print(f"unexpected PUSHRC arg {params[0]}")
            assert False
        if mnemonic == "STACKRWD":
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

    def get_closest_instruction(self, addr: int) -> Instruction:
        for i in range(addr, len(self.instructions)):
            inst = self.get_instruction_at(i)
            if inst:
                return inst
        assert False
        return None

    def get_jump_target(self, addr: int) -> Optional[Instruction]:
        if addr > self.instructions[-1].addr:
            return None
        return self.get_closest_instruction(addr)

    def _check_addrs(self):
        for lbl_addr in self.labels.values():
            inst = self.get_jump_target(lbl_addr)
            if inst is None:
                print(f"didn't found instruction for addr {hex(lbl_addr)}")
                assert False

        for goto_target in self.goto_targets:
            inst = self.get_jump_target(goto_target)
            if inst:
                inst.is_jump_dest = True
            else:
                print(
                    f"warning: no instruction found for jump target address {hex(goto_target)}")

        for inst in self.instructions:
            if issubclass(type(inst), Goto):
                inst = self.get_instruction_at(inst.addr)
                assert inst

    def process(self, lines: list[str]):
        for i, line in enumerate(lines):
            try:
                if len(line) and not line.startswith("#"):
                    self._process_line(line.lstrip())
            except Exception as e:
                print(f"error at line {i}")
                raise e
            self.current_offset += 1
        for inst in self.instructions:
            print(f"{hex(inst.addr)}: {inst}")
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
        self.indent = 0

    def emit_line(self, line: str):
        self.lines.append("".join(["\t" for _ in range(self.indent)]) + line)

    def update_line(self, index: int, line: str):
        self.lines[index] = "".join(["\t" for _ in range(self.indent)]) + line

    def _rewrite_var_name(self, index: int, name: str):
        line_idx = self.index-index-1
        old_line = self.lines[line_idx]
        assign = old_line.split(" := ")[1]
        self.update_line(line_idx, f"{name} := {assign}")

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
        return f"Goto {hex(inst.target)}"

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
        if isinstance(val, UnaryOp):
            return f"{val.unary}" + self._gen_val(val.value)
        print(f"CodeGen: unhandled val type {type(val)}")
        return None

    def _gen_inst(self, inst: Instruction) -> Optional[str]:
        if isinstance(inst, NOP):
            return None
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
        if isinstance(inst, Return):
            return "return"
        print(f"CodeGen: unhandled inst {inst} type{type(inst)}")
        return None

    def process(self) -> List[str]:
        for instruction in self.parser.instructions:
            if instruction.is_jump_dest:
                self.indent = 0
                self.lines.append("")
                self.index += 1
                self.lines.append(
                    f"JUMP_TARGET_{instruction.addr}:")
                self.index += 1
            self.indent = 2
            line = self._gen_inst(instruction)
            if line:
                self.emit_line(line)
            lbl_num = self.parser.addr_is_label(instruction.addr)
            if lbl_num != -1:
                self.lines.append("")
                self.index += 1
                self.indent = 0
                self.lines.append(
                    f"LABEL_{lbl_num}:")
                self.index += 1
            self.index += 1

        return self.lines


def gen_pseudo_code(lines: List[str]) -> List[str]:
    parser = Parser()
    parser.process(lines)
    gen = CodeGen(parser)
    return gen.process()


if __name__ == "__main__":
    with open(sys.argv[1], "r", encoding="utf8") as f_in:
        lines = f_in.readlines()
        ouput = gen_pseudo_code(lines)
        print("--------- Generated Code ---------")
        for l in ouput:
            print(l)
