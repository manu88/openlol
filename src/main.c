
#include "bytes.h"
#include "format_40.h"
#include "format_cmz.h"
#include "format_cps.h"
#include "format_dat.h"
#include "format_inf.h"
#include "format_lang.h"
#include "format_lcw.h"
#include "format_shp.h"
#include "format_tim.h"
#include "format_vcn.h"
#include "format_vmp.h"
#include "format_wll.h"
#include "format_wsa.h"
#include "game.h"
#include "pak_file.h"
#include "renderer.h"
#include "script.h"
#include "script_disassembler.h"
#include "tests.h"
#include "tim_interpreter.h"
#include <_string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int cmdRender(int argc, char *argv[]) {
  if (argc < 3) {
    printf("render vcn-file vmp-file wallpos\n");
    return 0;
  }
  const char *vcnFile = argv[0];
  const char *vmpFile = argv[1];
  int wallpos = atoi(argv[2]);
  printf("vcn='%s' vmp='%s'\n", vcnFile, vmpFile);
  VCNHandle vcnHandle = {0};
  VMPHandle vmpHandle = {0};
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vcnFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VCNHandleFromLCWBuffer(&vcnHandle, buffer, fileSize)) {
      printf("VCNDataFromLCWBuffer error\n");
      return 1;
    }
  }
  {
    size_t fileSize = 0;
    size_t readSize = 0;
    uint8_t *buffer = readBinaryFile(vmpFile, &fileSize, &readSize);
    if (!buffer) {
      return 1;
    }
    if (readSize == 0) {
      free(buffer);
      return 1;
    }
    assert(readSize == fileSize);

    if (!VMPHandleFromLCWBuffer(&vmpHandle, buffer, fileSize)) {
      printf("VMPDataFromLCWBuffer error\n");
      return 1;
    }
  }
  printf("Got both files\n");
  testRenderScene(&vcnHandle, &vmpHandle, wallpos);
  VMPHandleRelease(&vmpHandle);
  VCNHandleRelease(&vcnHandle);
  return 0;
}

static void usageScript(void) {
  printf("script subcommands: test|offsets|disasm [filepath]\n");
}

static int cmdScriptOffsets(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *iffData = readBinaryFile(filepath, &fileSize, &readSize);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, readSize)) {
    printf("INFScriptFromBuffer error\n");
    return 1;
  }
  EMCInterpreter interp = {0};
  EMCData dat = {0};
  EMCInterpreterLoad(&interp, &script, &dat);
  for (int i = 0; i < dat.ordrSize; i++) {
    if (dat.ordr[i] != 0XFFFF) {
      printf("%i %X\n", i, swap_uint16(dat.ordr[i]));
    }
  }
  return 0;
}

static int cmdScriptDisasm(const char *filepath, int offset) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *iffData = readBinaryFile(filepath, &fileSize, &readSize);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, readSize)) {
    printf("INFScriptFromBuffer error\n");
    return 1;
  }

  EMCInterpreter interp = {0};
  EMCDisassembler disassembler = {0};
  EMCDisassemblerInit(&disassembler);
  interp.disassembler = &disassembler;
  EMCData dat = {0};
  EMCInterpreterLoad(&interp, &script, &dat);
  EMCState state = {0};
  EMCStateInit(&state, &dat);

  EMCStateSetOffset(&state, offset);
  int n = 0;

  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
    if (n > 80) {
      break;
    }
  }
  printf("Exec'ed %i instructions\n", n);
  INFScriptRelease(&script);
  printf("%s\n", disassembler.disasmBuffer);
  EMCDisassemblerRelease(&disassembler);
  return 0;
}

static int cmdScriptTest(const char *filepath, int functionId) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *iffData = readBinaryFile(filepath, &fileSize, &readSize);

  INFScript script = {0};
  if (!INFScriptFromBuffer(&script, iffData, readSize)) {
    printf("INFScriptFromBuffer error\n");
    return 1;
  }

  EMCInterpreter interp = {0};
  EMCData dat = {0};
  EMCInterpreterLoad(&interp, &script, &dat);
  EMCState state = {0};
  EMCStateInit(&state, &dat);

  if (!EMCStateStart(&state, functionId)) {
    printf("EMCInterpreterStart: invalid\n");
  }
  int n = 0;

  state.regs[0] = -1; // flags
  state.regs[1] = -1; // charnum
  state.regs[2] = 0;  // item
  state.regs[3] = 0;
  state.regs[4] = 0;
  state.regs[5] = functionId;

  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
  }
  printf("Exec'ed %i instructions\n", n);
  INFScriptRelease(&script);
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
    if (argc < 3) {
      printf("missing functionid\n");
      return 1;
    }
    return cmdScriptDisasm(argv[1], atoi(argv[2]));
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

static void usageCPS(void) { printf("cps cpsfile \n"); }

static int cmdCPS(int argc, char *argv[]) {
  if (argc < 1) {
    usageCPS();
    return 1;
  }
  const char *cpsFile = argv[0];
  FILE *inFile = fopen(cpsFile, "rb");
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
  CPSImage image;
  int ok = CPSImageFromFile(&image, buffer, inFileSize);
  free(buffer);
  if (ok) {
    char *destFile = strdup(cpsFile);
    assert(destFile);
    destFile[strlen(destFile) - 3] = 'p';
    destFile[strlen(destFile) - 2] = 'n';
    destFile[strlen(destFile) - 1] = 'g';
    CPSImageToPng(&image, destFile);
    free(destFile);
    CPSImageRelease(&image);
  }

  return !ok;
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

static void usageINF(void) {
  printf("inf subcommands: extract|show| infFile [blockAddr]\n");
}

static int cmdINFShowContent(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }

  INFScript script;
  INFScriptInit(&script);
  INFScriptFromBuffer(&script, buffer, fileSize);
  printf("INF Content from file '%s':\n", filepath);
  printf("\t%zi instructions\n", INFScriptGetCodeBinarySize(&script));
  INFScriptListText(&script);
  printf("Script function segments:\n");
  INFScriptListScriptFunctions(&script);
  INFScriptRelease(&script);
  return 0;
}

static int cmdINFExtractCode(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (!f) {
    perror("open: ");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buffer = malloc(fsize);
  if (!buffer) {
    perror("malloc error");
    fclose(f);
    return 1;
  }
  fread(buffer, fsize, 1, f);
  fclose(f);
  INFScript script;
  INFScriptInit(&script);
  INFScriptFromBuffer(&script, buffer, fsize);

  size_t numInstructions = INFScriptGetCodeBinarySize(&script);
  const uint16_t *instructions = INFScriptGetCodeBinary(&script);

  char outFilePath[64] = "";
  assert(snprintf(outFilePath, 64, "%s.script.bin", filepath) < 64);

  FILE *outFile = fopen(outFilePath, "wb");
  if (outFile) {
    fwrite(instructions, numInstructions * 2, 1, outFile);
    fclose(outFile);
  }

  INFScriptRelease(&script);
  return 0;
}

static int cmdINF(int argc, char *argv[]) {
  if (argc < 1) {
    printf("inf command, missing arguments\n");
    usageINF();
    return 1;
  }
  if (strcmp(argv[0], "extract") == 0 && argc == 2) {
    return cmdINFExtractCode(argv[1]);
  } else if (strcmp(argv[0], "show") == 0 && argc == 2) {
    return cmdINFShowContent(argv[1]);
  }
  usageINF();
  return 1;
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

static void usagePak(void) {
  printf("pak subcommands: list|extract pakFilepath [file]\n");
}

static int cmdPakList(const char *pakfilepath) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(&file, pakfilepath) == 0) {
    perror("Error while reading file");
    return 1;
  }

  for (int i = 0; i <= file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", i, i,
           entry->offset, entry->filename, PakFileEntryGetExtension(entry),
           entry->fileSize);
  }

  PAKFileRelease(&file);
  return 0;
}

static int cmdPakExtract(const char *pakfilepath, const char *fileToShow) {
  PAKFile file;
  PAKFileInit(&file);

  if (PAKFileRead(&file, pakfilepath) == 0) {
    perror("Error while reading file");
    return 1;
  }

  int found = 0;
  int ok = 0;
  for (int i = 0; i < file.count; i++) {
    PAKEntry *entry = &file.entries[i];
    if (strcmp(entry->filename, fileToShow) == 0) {
      printf("%i (%x): Entry offset %u name '%s' ('%s') size %u \n", i, i,
             entry->offset, entry->filename, PakFileEntryGetExtension(entry),
             entry->fileSize);
      found = 1;

      ok = PakFileExtract(&file, entry, fileToShow);
      break;
    }
  }

  PAKFileRelease(&file);
  if (!found) {
    printf("File '%s' not found in PAK\n", fileToShow);
    return 1;
  }
  return ok;
}

static int cmdPak(int argc, char *argv[]) {
  if (argc < 1) {
    printf("pak command, missing arguments\n");
    usagePak();
    return 1;
  }
  if (argc < 2) {
    printf("pak list: missing pak file path\n");
    return 1;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "list") == 0) {
    return cmdPakList(filepath);
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 3) {
      printf("pak extract: missing file name \n");
      return 1;
    }
    return cmdPakExtract(filepath, argv[2]);
  }

  printf("Unknown pak command '%s'\n", argv[0]);
  usagePak();
  return 1;
}

static int cmdLangShow(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }
  printf("lang file '%s'\n", filepath);
  LangHandle handle = {0};
  LangHandleFromBuffer(&handle, buffer, readSize);
  LangHandleShow(&handle);
  LangHandleRelease(&handle);
  return 0;
}

static int cmdWSAExtract(const char *filepath, int frameNum) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }
  WSAHandle handle;
  WSAHandleInit(&handle);
  WSAHandleFromBuffer(&handle, buffer, readSize);
  printf("numFrame %i, x=%i y=%i w=%i h=%i palette=%X delta=%i\n",
         handle.header.numFrames, handle.header.xPos, handle.header.yPos,
         handle.header.width, handle.header.height, handle.header.hasPalette,
         handle.header.delta);
  if (frameNum < 0 || frameNum >= handle.header.numFrames) {
    printf("invalid frameNum\n");
    WSAHandleRelease(&handle);
    return 1;
  }
  printf("Extract frame %i/%i\n", frameNum, handle.header.numFrames);

  uint32_t offset = WSAHandleGetFrameOffset(&handle, frameNum);
  const uint8_t *frameData = handle.originalBuffer + offset;
  size_t frameSize = WSAHandleGetFrameOffset(&handle, frameNum + 1) - offset;
  size_t destSize = handle.header.delta;
  uint8_t *outData = malloc(destSize);

  ssize_t decompressedSize =
      LCWDecompress(frameData, frameSize, outData, destSize);
  printf("LCWDecompress: decompressedSize=%zi\n", decompressedSize);

  size_t fullSize = handle.header.width * handle.header.height;
  uint8_t *outData2 = malloc(fullSize);
  Format40Decode(outData, decompressedSize, outData2);

  WSAFrameToPng(outData2, fullSize, handle.header.palette, "frameWSA.png",
                handle.header.width, handle.header.height);

  free(outData);
  free(outData2);

  WSAHandleRelease(&handle);
  return 0;
}

static int cmdWSAShow(const char *filepath) {
  size_t fileSize = 0;
  size_t readSize = 0;
  uint8_t *buffer = readBinaryFile(filepath, &fileSize, &readSize);
  if (!buffer) {
    perror("malloc error");
    return 1;
  }
  WSAHandle handle;
  WSAHandleInit(&handle);
  WSAHandleFromBuffer(&handle, buffer, readSize);
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

static void usageTim(void) { printf("tim subcommands: show|anim file\n"); }

static int cmdShowTim(int argc, char *argv[]) {
  if (argc < 1) {
    printf("show command, missing arguments\n");
    return 1;
  }
  const char *filepath = argv[0];
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
    printf("Text:'%s'\n", TIMHandleGetText(&handle, 0));
  } else {
    printf("No text\n");
  }

  printf("Got %zu avtl chunks\n", handle.avtlSize);
  printf("Got %i functions\n", handle.numFunctions);

  TimInterpreter interp;
  TimInterpreterInit(&interp);
  TimInterpreterExec(&interp, &handle);

  TIMHandleRelease(&handle);
  return 0;
}

static int cmdAnimateTim(int argc, char *argv[]) {
  if (argc < 1) {
    printf("tim command, missing arguments\n");
    return 1;
  }
  return 0;
}

static int cmdTim(int argc, char *argv[]) {
  if (argc < 1) {
    printf("tim command, missing arguments\n");
    usageTim();
    return 1;
  }
  if (strcmp(argv[0], "show") == 0) {
    return cmdShowTim(argc - 1, argv + 1);
  } else if (strcmp(argv[0], "anim") == 0) {
    return cmdAnimateTim(argc - 1, argv + 1);
  }
  usageTim();
}

static void usageLang(void) { printf("lang subcommands: show file\n"); }

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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }
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
  } else if (strcmp(argv[1], "inf") == 0) {
    return cmdINF(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "cps") == 0) {
    return cmdCPS(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "map") == 0) {
    return cmdMap(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "script") == 0) {
    return cmdScript(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "tests") == 0) {
    return UnitTests();
  } else if (strcmp(argv[1], "render") == 0) {
    return cmdRender(argc - 2, argv + 2);
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
  }
  printf("Unknown command '%s'\n", argv[1]);
  usage(argv[0]);
  return 1;
}
