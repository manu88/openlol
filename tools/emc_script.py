from typing import List


class Param:
    def __init__(self, name: str):
        self.name = name


class PStr(Param):
    pass


class PNum(Param):
    pass


class PStrId(Param):
    # PStrId is a stringId from a lang file
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
    "setGlobalVar": Func([PNum("how"), PNum("a"), PNum("b")]),
    "testGameFlag": Func([PNum("flag")]),
    "loadMusicTrack": Func([PNum("file")]),
    "playDialogueTalkText": Func([PStrId("stringId")]),
    "stopTimScript": Func([PNum("scriptId")]),
    "runTimScript": Func([PNum("scriptId"), PNum("loop")]),
    "clearDialogueField": Func([]),
    "playCharacterScriptChat": Func([PNum("charId"), PNum("mode"), PStr("stringId")]),
}


tok_op_codes = {
    "CALL",
    "PUSH",
    "GOTO",
    "PUSHARG",
    "POPRC",
    "POPLOCVAR",
    "PUSHRC",
    "STACKRWD",
    "STACKFWD",
    "PUSHLOCVAR",
    "PUSHVAR",
    "POP",
    "POPPARAM",
    "UNARY",
}

tok_ctl_flow = {
    "JUMP",
    "IFNOTGO",
    "IF",
}

tok_maths = {
    "INF",
    "INFEQ",
    "OR",
    "AND",
    "EQUAL",
    "NEQUAL",
    "SUP",
    "SUPEQ",
    "ADD",
    "LAND",
    "LOR",
    "MULTIPLY",
    "MINUS",
    "LSHIFT",
    "XOR",
    "RSHIFT",
}
