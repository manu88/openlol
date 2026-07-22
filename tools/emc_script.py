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


class PEMCStr(Param):
    # PEMCStr is a stringId from the emc string part
    pass


class Func:
    def __init__(self, params: List[Param] = []):
        self.params = params

    def has_params(self) -> bool:
        return len(self.params) > 0


builtins = {
    "loadBlockProperties": Func([PEMCStr("file")]),
    "loadLangFile": Func([PEMCStr("file")]),
    "loadLevelShapes": Func([PStr("shp"), PStr("datFile")]),
    "loadBitmap": Func([PEMCStr("file"), PNum("param")]),
    "loadMonsterShapes": Func([PEMCStr("file"), PNum("monsterId"), PNum("p2")]),
    "loadTimScript": Func([PStr("scriptId"), PEMCStr("stringId")]),
    "initAnimStruct": Func([PEMCStr("file"), PNum("index"), PNum("x"), PNum("y"), PNum("offscreenBuffer"), PNum("wsaFlags")]),
    "checkRectForMousePointer": Func([PNum("xMin"), PNum("yMin"), PNum("xMax"), PNum("yMax")]),
    "setGameFlag": Func([PNum("flag"), PNum("val")]),
    "setGlobalVar": Func([PNum("how"), PNum("a"), PNum("b")]),
    "setNextFunc": Func([PNum("addr")]),
    "testGameFlag": Func([PNum("flag")]),
    "loadMusicTrack": Func([PEMCStr("file")]),
    "playDialogueTalkText": Func([PStrId("stringId")]),
    "stopTimScript": Func([PNum("scriptId")]),
    "runTimScript": Func([PNum("scriptId"), PNum("loop")]),
    "clearDialogueField": Func([]),
    "playCharacterScriptChat": Func([PNum("charId"), PNum("mode"), PStrId("stringId")]),
    "rollDice": Func([PNum("times"), PNum("max")]),
    "setItemProperty": Func([PNum("index"), PStrId("stringId"), PNum("shpId"), PNum("type"), PNum("scriptFun"), PNum("might"), PNum("skill"), PNum("protection"), PNum("flags"), PNum("unknown")]),
    "allocItemProperties": Func([PNum("size")]),
    "getCharacterStat": Func([PNum("p1"), PNum("p2"), PNum("p3")]),
    "makeItem": Func([PNum("p1"), PNum("p2"), PNum("p3")]),
    "printMessage": Func([PNum("type"), PStrId("stringId"), PNum("soundId")]),
    "setupDialogueButtons": Func([PNum("numButtons"), PStrId("str0"), PStrId("str1"), PStrId("str2")]),
    "checkForCertainPartyMember": Func([PNum("charId")]),
    "drawScene": Func([PNum("pageNum")]),
    "moveMonster": Func([PNum("monsterId"), PNum("destBlock"), PNum("xOff"), PNum("yOff"), PNum("destDir")]),
    "initSceneWindowDialogue": Func([PNum("p0")]),
    "copyRegion": Func([PNum("srcX"), PNum("srcY"), PNum("destX"), PNum("destY"), PNum("w"), PNum("h"), PNum("srcPage"), PNum("destPage")]),
    "giveTakeMoney": Func([PNum("amount")]),
    "delay": Func([PNum("ticks")]),
    "getItemParam": Func([PNum("p0"), PNum("p1")]),
    "getDirection": Func(),
    "getItemInHand": Func(),
    "deleteHandItem": Func(),
    "processDialogue": Func(),
    "getDirection": Func(),
    "getGlobalScriptVar": Func([PNum("index")]),
    "releaseTimScript": Func([PNum("scriptId")]),
    "playSoundEffect": Func([PNum("soundId")]),
    "getWallFlags": Func([PNum("blockId"), PNum("wall")]),
    "triggerDoorSwitch": Func([PNum("blockId"), PNum("p1")]),
    "loadMonsterProperties": Func([PNum("monsterIndex"), PNum("shapeIndex"), PNum("hitChance"), PNum("protection"), PNum("evadeChance"), PNum("speed"), PNum("p6"), PNum("p7"), PNum("p8")]),
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
