
#include "bytes.h"
#include "formats/format_cmz.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_fnt.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_tim.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_wll.h"
#include "formats/format_wsa.h"
#include "game.h"
#include "pak_file.h"
#include "renderer.h"
#include "script.h"
#include "script_disassembler.h"
#include "tests.h"
#include "tim_animator.h"
#include "tim_interpreter.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint8_t *getFileContent(const char *filepath, size_t *dataSize,
                               int *freeBuffer) {
  uint8_t *buffer = NULL;
  if (PakFileGetMain()) {
    int index = PakFileGetEntryIndex(PakFileGetMain(), filepath);
    if (index == -1) {
      return NULL;
    }
    buffer = PakFileGetEntryData(PakFileGetMain(), index);
    *dataSize = PakFileGetMain()->entries[index].fileSize;
  } else {
    size_t fileSize = 0;
    buffer = readBinaryFile(filepath, &fileSize, dataSize);
    if (!buffer) {
      perror("malloc error");
      return NULL;
    }
    *freeBuffer = 1;
  }
  return buffer;
}

static int cmdWLL(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);

  WllHandle handle = {0};
  WllHandleFromBuffer(&handle, buffer, readSize);
  WLLHandlePrint(&handle);
  WllHandleRelease(&handle);
  return 0;
}

static char *strAppend(const char *str, char *toAdd) {
  char *outStr = malloc(strlen(str) + strlen(toAdd) + 1);
  if (outStr == NULL) {
    return NULL;
  }
  strcpy(outStr, str);
  outStr = strcat(outStr, toAdd);
  return outStr;
}

static int cmdVCN(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  const char *inputFile = argv[0];
  uint8_t *buffer = readBinaryFile(inputFile, &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);
  VCNHandle handle = {0};
  if (!VCNHandleFromLCWBuffer(&handle, buffer, fileSize)) {
    printf("VCNDataFromLCWBuffer error\n");
    free(buffer);
    return 1;
  }
  char *outFilePath = strAppend(inputFile, ".png");
  if (!outFilePath) {
    VCNHandleRelease(&handle);
    return 1;
  }
  printf("Write VCN image '%s'\n", outFilePath);
  VCNImageToPng(&handle, outFilePath);
  VCNHandleRelease(&handle);
  free(outFilePath);
  return 0;
}

static int cmdVMP(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    return 1;
  }
  if (readSize == 0) {
    free(buffer);
    return 1;
  }
  assert(readSize == fileSize);
  VMPHandle handle = {0};
  if (!VMPHandleFromLCWBuffer(&handle, buffer, fileSize)) {
    printf("VMPDataFromLCWBuffer error\n");
    return 1;
  }
  VMPHandlePrint(&handle);
  VMPHandleRelease(&handle);
  return 0;
}

static void usageScript(void) {
  printf("script subcommands: test|offsets|disasm [filepath]\n");
}

static int cmdScriptOffsets(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *iffData = getFileContent(filepath, &dataSize, &freeBuffer);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, dataSize)) {
    printf("INFScriptFromBuffer error\n");
    if (freeBuffer) {
      free(iffData);
    }
    return 1;
  }

  for (int i = 0; i < INFScriptGetNumFunctions(&script); i++) {
    int offset = INFScriptGetFunctionOffset(&script, i);
    if (offset != -1) {
      printf("0X%X %i %X\n", i, i, offset);
    }
  }
  if (freeBuffer) {
    free(iffData);
  }
  return 0;
}

static int cmdScriptDisasm(const char *filepath, int functionNum) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *iffData = getFileContent(filepath, &dataSize, &freeBuffer);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, dataSize)) {
    printf("INFScriptFromBuffer error\n");
    return 1;
  }

  EMCInterpreter interp = {0};
  EMCDisassembler disassembler = {0};
  EMCDisassemblerInit(&disassembler);
  interp.disassembler = &disassembler;
  disassembler.showDisamComment = 1;
  EMCState state = {0};
  EMCStateInit(&state, &script);
  EMCStateSetOffset(&state, 0);
  if (functionNum >= 0) {
    EMCStateStart(&state, functionNum);
  }

  int n = 0;

  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
  }

  INFScriptRelease(&script);
  printf("%s\n", disassembler.disasmBuffer);
  EMCDisassemblerRelease(&disassembler);
  printf("Exec'ed %i / %i instructions\n", n, script.dataSize);
  if (freeBuffer) {
    free(iffData);
  }
  return 0;
}

static int cmdScriptTest(const char *filepath, int functionNum) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *iffData = getFileContent(filepath, &dataSize, &freeBuffer);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, dataSize)) {
    printf("INFScriptFromBuffer error\n");
    if (freeBuffer) {
      free(iffData);
    }
    return 1;
  }

  EMCInterpreter interp = {0};
  EMCState state = {0};
  EMCStateInit(&state, &script);
  EMCStateSetOffset(&state, functionNum);
  if (!EMCStateStart(&state, functionNum)) {
    printf("EMCInterpreterStart: invalid\n");
  }
  int n = 0;

  state.regs[0] = -1; // flags
  state.regs[1] = -1; // charnum
  state.regs[2] = 0;  // item
  state.regs[3] = 0;
  state.regs[4] = 0;
  state.regs[5] = functionNum;

  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
  }
  printf("Exec'ed %i instructions\n", n);
  INFScriptRelease(&script);
  if (freeBuffer) {
    free(iffData);
  }
  return 0;
}

static int cmdScript(int argc, char *argv[]) {
  if (argc < 2) {
    usageScript();
    return 1;
  }
  if (strcmp(argv[0], "test") == 0) {
    if (argc < 3) {
      printf("missing functionid\n");
      return 1;
    }
    return cmdScriptTest(argv[1], atoi(argv[2]));
  } else if (strcmp(argv[0], "offsets") == 0) {
    return cmdScriptOffsets(argv[1]);
  } else if (strcmp(argv[0], "disasm") == 0) {
    return cmdScriptDisasm(argv[1], argc >= 3 ? atoi(argv[2]) : -1);
  }
  usageScript();
  return 1;
}

static void usageSHP(void) {
  printf("shp subcommands: list|extract filepath [index] [palette]\n");
}

static int cmdShp(int argc, char *argv[]) {
  if (argc < 2) {
    usageSHP();
    return 1;
  }

  if (strcmp(argv[0], "list") != 0 && strcmp(argv[0], "extract") != 0) {
    usageSHP();
    return 1;
  }
  if (strcmp(argv[0], "extract") == 0 && argc < 3) {
    usageSHP();
    return 1;
  }
  const char *shpFile = argv[1];
  printf("open SHP file '%s'\n", shpFile);
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(shpFile, &fileSize, &readSize);
  if (!buffer) {
    perror("readBinaryFile");
    return 1;
  }
  SHPHandle handle = {0};
  if (!SHPHandleFromBuffer(&handle, buffer, readSize)) {
    perror("SHPHandleFromBuffer");
    free(buffer);
    return 1;
  }
  if (strcmp(argv[0], "list") == 0) {
    SHPHandlePrint(&handle);
  } else if (strcmp(argv[0], "extract") == 0) {
    int index = atoi(argv[2]);

    VCNHandle vcnHandle = {0};
    char *vcnPaletteFile = NULL;
    if (argc > 3) {
      vcnPaletteFile = argv[3];
      printf("using vcn palette file '%s'\n", vcnPaletteFile);
      uint8_t *buffer = readBinaryFile(vcnPaletteFile, &fileSize, &readSize);
      if (!buffer) {
        return 1;
      }
      if (readSize == 0) {
        free(buffer);
        return 1;
      }
      if (!VCNHandleFromLCWBuffer(&vcnHandle, buffer, fileSize)) {
        printf("VCNDataFromLCWBuffer error\n");
        free(buffer);
        return 1;
      }
      assert(readSize == fileSize);
    }
    printf("extracting index %i\n", index);
    SHPFrame frame = {0};
    SHPHandleGetFrame(&handle, &frame, index);
    SHPFramePrint(&frame);
    SHPFrameGetImageData(&frame);
    SHPFrameToPng(&frame, "frame.png",
                  vcnPaletteFile ? vcnHandle.palette : NULL);
    SHPFrameRelease(&frame);
    if (vcnPaletteFile != NULL) {
      VCNHandleRelease(&vcnHandle);
    }
  }

  SHPHandleRelease(&handle);
  return 0;
}

static int cmdDat(int argc, char *argv[]) {
  const char *datFile = argv[0];
  printf("open DAT file '%s'\n", datFile);
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(datFile, &fileSize, &readSize);
  if (!buffer) {
    perror("readBinaryFile");
    return 1;
  }
  DatHandle handle = {0};
  if (!DatHandleFromBuffer(&handle, buffer, readSize)) {
    perror("DatHandleFromBuffer");
    free(buffer);
    return 1;
  }
  DatHandlePrint(&handle);
  DatHandleRelease(&handle);
  return 0;
}
static int cmdMap(int argc, char *argv[]) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(argv[0], &fileSize, &readSize);
  if (!buffer) {
    perror("readBinaryFile");
    return 1;
  }
  MazeHandle handle = {0};
  if (!MazeHandleFromBuffer(&handle, buffer, readSize)) {
    return 1;
  }
  const Maze *maze = handle.maze;
  printf("Maze w=%0X h=%0X Nof=%0X\n", maze->width, maze->height,
         maze->tileSize);
  FILE *fout = fopen("test.txt", "w");

  for (int i = 0; i < MAZE_NUM_CELL; i++) {
    fprintf(fout, "%i %i %i %i\n", maze->wallMappingIndices[i].face[0],
            maze->wallMappingIndices[i].face[1],
            maze->wallMappingIndices[i].face[2],
            maze->wallMappingIndices[i].face[3]);
  }
  fprintf(fout, "\n");
  fclose(fout);
  MazeHandleRelease(&handle);
  return 0;
}

static void usageCPS(void) { printf("cps extract cpsfile \n"); }

static int cmdCPSExtract(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  CPSImage image;
  int ok = CPSImageFromFile(&image, buffer, dataSize);
  if (freeBuffer) {
    free(buffer);
  }
  if (ok) {
    char *destFilePath = strdup(filepath);
    assert(destFilePath);
    destFilePath[strlen(destFilePath) - 3] = 'p';
    destFilePath[strlen(destFilePath) - 2] = 'n';
    destFilePath[strlen(destFilePath) - 1] = 'g';
    CPSImageToPng(&image, destFilePath);
    free(destFilePath);
    CPSImageRelease(&image);
  }

  return !ok;
}

static int cmdCPS(int argc, char *argv[]) {
  if (argc < 2) {
    usageCPS();
    return 1;
  }
  if (strcmp(argv[0], "extract") == 0) {
    return cmdCPSExtract(argv[1]);
  }
  usageCPS();
  return 1;
}

static void usageCMZ(void) { printf("cmz subcommands: unzip cpsFilepath\n"); }

static int cmdCMZUnzip(const char *cmzfilePath) {
  printf("unzip cmz '%s'\n", cmzfilePath);
  FILE *inFile = fopen(cmzfilePath, "rb");
  if (!inFile) {
    perror("open: ");
    return 1;
  }
  fseek(inFile, 0, SEEK_END);
  long inFileSize = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);
  uint8_t *buffer = malloc(inFileSize);
  if (!buffer) {
    perror("malloc error");
    fclose(inFile);
    return 1;
  }
  fread(buffer, inFileSize, 1, inFile);
  fclose(inFile);
  size_t unzipedDataSize = 0;
  printf("CMZ_Uncompress %zi bytes\n", inFileSize);
  uint8_t *unzipedData = CMZDecompress(buffer, inFileSize, &unzipedDataSize);
  int err = 0;
  if (unzipedData == NULL) {
    printf("error while unzipping file\n");
    err = 1;
  } else {
    char *outFilePath = malloc(strlen(cmzfilePath) + 5);
    strcpy(outFilePath, cmzfilePath);
    outFilePath = strcat(outFilePath, ".maz");
    FILE *outFile = fopen(outFilePath, "wb");
    if (outFile) {
      fwrite(unzipedData, unzipedDataSize, 1, outFile);
      fclose(outFile);
    }
    free(outFilePath);
  }

  free(buffer);
  free(unzipedData);
  return err;
}

static int cmdCMZ(int argc, char *argv[]) {
  if (argc < 1) {
    printf("cmz command, missing arguments\n");
    usageCMZ();
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "unzip") == 0) {
    if (argc < 2) {
      printf("cmz unzip: missing cmz file path\n");
      return 1;
    }

    return cmdCMZUnzip(filepath);
  }
  printf("unkown subcommand '%s'\n", argv[0]);
  usageCMZ();
  return 1;
}

static void usagePak(void) { printf("pak subcommands: list|extract [file]\n"); }

static int cmdPakList(void) {
  const PAKFile *file = PakFileGetMain();
  assert(file);
  for (int i = 0; i <= file->count; i++) {
    PAKEntry *entry = &file->entries[i];
    printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", i, i,
           entry->offset, entry->filename, PakFileEntryGetExtension(entry),
           entry->fileSize);
  }
  return 0;
}

static int pakFileExtract(const PAKFile *file, int index, const PAKEntry *entry,
                          const char *toFile) {
  uint8_t *fileData = PakFileGetEntryData(file, index);
  if (!fileData) {
    return 1;
  }
  FILE *f = fopen(toFile, "wb");
  if (!f) {
    perror("open");
    return 1;
  }
  if (fwrite(fileData, entry->fileSize, 1, f) != 1) {
    perror("write");
    fclose(f);
    return 1;
  }
  fclose(f);
  return 0;
}

static int cmdPakExtract(const char *fileToShow) {
  const PAKFile *file = PakFileGetMain();
  assert(file);

  int index = PakFileGetEntryIndex(file, fileToShow);
  if (index == -1) {
    printf("File '%s' not found in PAK\n", fileToShow);
    return 1;
  }
  PAKEntry *entry = &file->entries[index];

  printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", index, index,
         entry->offset, entry->filename, PakFileEntryGetExtension(entry),
         entry->fileSize);

  return pakFileExtract(file, index, entry, fileToShow);
}

static int cmdPak(int argc, char *argv[]) {
  if (argc < 1) {
    printf("pak command, missing arguments\n");
    usagePak();
    return 1;
  }
  if (!PakFileGetMain()) {
    printf("pak list: missing pak file path, use -p option\n");
    return 1;
  }
  if (strcmp(argv[0], "list") == 0) {
    return cmdPakList();
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 2) {
      printf("pak extract: missing file name \n");
      return 1;
    }
    return cmdPakExtract(argv[1]);
  }

  printf("Unknown pak command '%s'\n", argv[0]);
  usagePak();
  return 1;
}

static void usageSAV(void) { printf("sav subcommands: show file\n"); }

static int cmdSAVShow(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  SAVHandle handle = {0};
  SAVHandleFromBuffer(&handle, buffer, dataSize);
  SAVSlot slot = {0};
  if (SAVHandleGetSlot(&handle, &slot)) {
    printf("Slot name '%s'\n", slot.header->name);
    printf("+CHARACTERS\n");
    for (int i = 0; i < 4; i++) {
      const SAVCharacter *ch = slot.characters[i];
      printf("character %i : flags:%X name:'%s' raceClassSex=%X id=%X "
             "magicPointsCur=%X "
             "magicPointsMax=%X\n",
             i, ch->flags, ch->name, ch->raceClassSex, ch->id,
             ch->magicPointsCur, ch->magicPointsMax);
    }

    printf("+GENERAL\n");
    uint16_t x = 0;
    uint16_t y = 0;
    GetRealCoords(slot.general->posX, slot.general->posY, &x, &y);
    printf("block=%X x=%X y=%X (real %i %i)\n", slot.general->currentBlock,
           slot.general->posX, slot.general->posY, x, y);
    printf("orientation=%X compass=%X\n", slot.general->currentDirection,
           slot.general->compassDirection);
    printf("level %i\n", slot.general->currentLevel);
    printf("selected char %i\n", slot.general->selectedChar);
    printf("+INVENTORY\n");
    for (int i = 0; i < INVENTORY_SIZE; i++) {
      if (slot.inventory[i]) {
        printf("%i: 0X%X\n", i, slot.inventory[i]);
      }
    }
  }
  free(buffer);
  return 0;
}
static int cmdSAV(int argc, char *argv[]) {
  if (argc < 2) {
    usageSAV();
    return 1;
  }
  if (strcmp(argv[0], "show") == 0) {
    return cmdSAVShow(argv[1]);
  }
  return 1;
}

static int cmdFNTExtract(const char *filepath, int charNum) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  FNTHandle handle = {0};
  if (!FNTHandleFromBuffer(&handle, buffer, dataSize)) {
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }
  printf("font has %i chars, max w.=%i max h.=%i\n", handle.numGlyphs,
         handle.maxWidth, handle.maxHeight);

  char *outFilePath = strAppend(filepath, ".png");
  if (!outFilePath) {
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }
  printf("Write FNT image '%s'\n", outFilePath);
  FNTToPng(&handle, outFilePath, charNum);
  if (freeBuffer) {
    free(buffer);
  }
  free(outFilePath);
  return 0;
}

static int cmdFNTShow(const char *filepath, int charNum) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  FNTHandle handle = {0};
  if (!FNTHandleFromBuffer(&handle, buffer, dataSize)) {
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }
  printf("font has %i chars, max w.=%i max h.=%i\n", handle.numGlyphs,
         handle.maxWidth, handle.maxHeight);

  uint16_t dataOff = handle.bitmapOffsets[charNum];
  printf("Data offset %x w=%i h=%i yOff=%i\n", dataOff,
         handle.widthTable[charNum], FNTHandleGetCharHeight(&handle, charNum),
         FNTHandleGetYOffset(&handle, charNum));
#if 0         
  for (int i = 0; i < handle.numGlyphs; i++) {
    printf("%i w=%i h=%i yOff=%i\n", i, handle.widthTable[i],
           FNTHandleGetCharHeight(&handle, i), FNTHandleGetYOffset(&handle, i));
  }
#endif
  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static void usageFNT(void) {
  printf("fnt subcommands: show|extract file charNum\n");
}

static int cmdFNT(int argc, char *argv[]) {
  if (argc < 3) {
    usageFNT();
    return 1;
  }
  if (strcmp(argv[0], "show") == 0) {
    return cmdFNTShow(argv[1], atoi(argv[2]));
  } else if (strcmp(argv[0], "extract") == 0) {
    return cmdFNTExtract(argv[1], atoi(argv[2]));
  }
  usageFNT();
  return 1;
}

static int cmdWSAExtract(const char *filepath, int frameNum) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  WSAHandle handle;
  WSAHandleInit(&handle);
  WSAHandleFromBuffer(&handle, buffer, dataSize);
  printf("numFrame %i, x=%i y=%i w=%i h=%i palette=%X delta=%i\n",
         handle.header.numFrames, handle.header.xPos, handle.header.yPos,
         handle.header.width, handle.header.height, handle.header.hasPalette,
         handle.header.delta);
  if (frameNum < 0 || frameNum >= handle.header.numFrames) {
    printf("invalid frameNum\n");
    WSAHandleRelease(&handle);
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }
  printf("Extract frame %i/%i\n", frameNum, handle.header.numFrames);

  uint8_t *frameData = malloc(handle.header.width * handle.header.height);
  assert(frameData);
  if (WSAHandleGetFrame(&handle, frameNum, frameData, 1)) {
    size_t fullSize = handle.header.width * handle.header.height;

    char ext[8] = "";
    snprintf(ext, 8, "-%i.png", frameNum);
    char *outFilePath = strAppend(filepath, ext);
    assert(outFilePath);
    printf("Rendering png file '%s'\n", outFilePath);
    WSAFrameToPng(frameData, fullSize, handle.header.palette, outFilePath,
                  handle.header.width, handle.header.height);
    free(outFilePath);
    free(frameData);
  }
  WSAHandleRelease(&handle);
  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static int cmdWSAShow(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  WSAHandle handle;
  WSAHandleInit(&handle);
  WSAHandleFromBuffer(&handle, buffer, dataSize);
  printf("numFrame %i, x=%i y=%i w=%i h=%i palette=%X delta=%X\n",
         handle.header.numFrames, handle.header.xPos, handle.header.yPos,
         handle.header.width, handle.header.height, handle.header.hasPalette,
         handle.header.delta);

  for (int i = 0; i < handle.header.numFrames + 2; i++) {
    uint32_t frameOffset = WSAHandleGetFrameOffset(&handle, i);
    printf("%i %X %zX:\n", i, frameOffset, handle.bufferSize);
    if (frameOffset == handle.bufferSize) {
      printf("IS ZERO\n");
    }
  }

  WSAHandleRelease(&handle);
  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static void usageWSA(void) {
  printf("wsa subcommands: show|extract file [framenum]\n");
}

static int cmdWSA(int argc, char *argv[]) {
  if (argc < 2) {
    printf("wsa command, missing arguments\n");
    usageWSA();
    return 1;
  }

  const char *filepath = argv[1];
  if (strcmp(argv[0], "show") == 0) {
    return cmdWSAShow(filepath);
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 3) {
      printf("missing frameNum argument\n");
      usageWSA();
      return 1;
    }
    return cmdWSAExtract(filepath, atoi(argv[2]));
  }
  usageWSA();
  return 1;
}

static void usageTim(void) {
  printf("tim subcommands: show|anim file [langfile]\n");
}

static int cmdShowTim(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }

  TIMHandle handle = {0};
  TIMHandleInit(&handle);
  TIMHandleFromBuffer(&handle, buffer, readSize);

  if (handle.text) {
    for (int i = 0; i < handle.numTextStrings; i++) {
      printf("Text: %i '%s'\n", i, TIMHandleGetText(&handle, i));
    }
  } else {
    printf("No text\n");
  }

  TIMInterpreter interp;
  TIMInterpreterInit(&interp);
  interp.dontLoop = 1;

  TIMInterpreterStart(&interp, &handle);
  while (TIMInterpreterIsRunning(&interp)) {
    TIMInterpreterUpdate(&interp);
  }

  // TimeHandleTest(&handle);
  TIMHandleRelease(&handle);
  return 0;
}

static int cmdAnimateTim(const char *filepath, const char *langFilepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }

  TIMHandle handle = {0};
  TIMHandleInit(&handle);
  TIMHandleFromBuffer(&handle, buffer, readSize);

  TIMAnimator animator;
  TIMAnimatorInit(&animator);
  LangHandle lang = {0};
  if (langFilepath) {
    printf("using lang file '%s'\n", langFilepath);
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(langFilepath, &fileSize, &readSize);
    if (!buffer) {
      perror("malloc error");
      TIMHandleRelease(&handle);
      return 1;
    }
    LangHandleFromBuffer(&lang, buffer, readSize);
    animator.lang = &lang;
  }
  if (TIMAnimatorRunAnim(&animator, &handle) == 0) {
    printf("TIMAnimatorRunAnim error\n");
  }

  TIMAnimatorRelease(&animator);
  TIMHandleRelease(&handle);
  if (lang.originalBuffer) {
    LangHandleRelease(&lang);
  }
  return 0;
}

static int cmdTim(int argc, char *argv[]) {
  if (argc < 2) {
    printf("tim command, missing arguments\n");
    usageTim();
    return 1;
  }
  if (strcmp(argv[0], "show") == 0) {
    return cmdShowTim(argv[1]);
  } else if (strcmp(argv[0], "anim") == 0) {
    return cmdAnimateTim(argv[1], argc > 2 ? argv[2] : NULL);
  }
  usageTim();
  return 1;
}

static void usageLang(void) { printf("lang subcommands: show file\n"); }

static int cmdLangShow(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  printf("lang file '%s'\n", filepath);
  LangHandle handle = {0};
  LangHandleFromBuffer(&handle, buffer, dataSize);
  LangHandleShow(&handle);
  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static int cmdLang(int argc, char *argv[]) {
  if (argc < 1) {
    printf("lang command, missing arguments\n");
    usageLang();
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "show") == 0) {
    return cmdLangShow(filepath);
  }
  usageLang();
  return 1;
}

static void usage(const char *progName) {
  printf("%s: pak|cmz|map|script|inf|tests|wll|render|game|dat|shp|lang "
         "subcommand "
         "...\n",
         progName);
}

static int doCMD(int argc, char *argv[]) {
  if (strcmp(argv[1], "pak") == 0) {
    return cmdPak(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cmz") == 0) {
    return cmdCMZ(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "wll") == 0) {
    return cmdWLL(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "vcn") == 0) {
    return cmdVCN(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "vmp") == 0) {
    return cmdVMP(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cps") == 0) {
    return cmdCPS(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "map") == 0) {
    return cmdMap(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "script") == 0) {
    return cmdScript(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "tests") == 0) {
    return UnitTests();
  } else if (strcmp(argv[1], "game") == 0) {
    return cmdGame(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "dat") == 0) {
    return cmdDat(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "shp") == 0) {
    return cmdShp(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "lang") == 0) {
    return cmdLang(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "tim") == 0) {
    return cmdTim(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "wsa") == 0) {
    return cmdWSA(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "sav") == 0) {
    return cmdSAV(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "fnt") == 0) {
    return cmdFNT(argc - 2, argv + 2);
  }
  printf("Unknown command '%s'\n", argv[1]);
  usage(argv[0]);
  return 1;
}

int main(int argc, char *argv[]) {
  const char *progName = argv[0];
  const char *pakFilePath = NULL;
  char c;
  while ((c = getopt(argc, argv, "hp:")) != -1) {
    switch (c) {
    case 'h':
      usage(progName);
      return 0;
    case 'p':
      pakFilePath = optarg;
      break;
    }
  }

  argc = argc - optind + 1;
  argv = argv + optind - 1;

  if (argc < 2) {
    usage(progName);
    return 0;
  }
  if (pakFilePath) {
    printf("Using pak file '%s'\n", pakFilePath);
    if (!PakFileLoadMain(pakFilePath)) {
      printf("error while reading pak file %s\n", pakFilePath);
      return 1;
    }
  }
  int ret = doCMD(argc, argv);
  PakFileReleaseMain();
  return ret;
}
