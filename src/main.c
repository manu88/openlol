
#include "bytes.h"
#include "config.h"
#include "dbg/debugger.h"
#include "formats/format_cmz.h"
#include "formats/format_config.h"
#include "formats/format_cps.h"
#include "formats/format_dat.h"
#include "formats/format_fnt.h"
#include "formats/format_inf.h"
#include "formats/format_lang.h"
#include "formats/format_sav.h"
#include "formats/format_shp.h"
#include "formats/format_vcn.h"
#include "formats/format_vmp.h"
#include "formats/format_voc.h"
#include "formats/format_wll.h"
#include "formats/format_wsa.h"
#include "formats/format_xxx.h"
#include "game.h"
#include "pak_file.h"
#include "renderer.h"
#include "script.h"
#include "script_disassembler.h"
#include <assert.h>
#include <sndfile.h>
#include <stddef.h>
#include <stdint.h>
#define _POSIX_C_SOURCE 200809L
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
  if (argc < 1) {
    printf("missing input file\n");
    return 1;
  }
  const char *inputFile = argv[0];
  size_t fileSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(inputFile, &fileSize, &freeBuffer);
  if (!buffer) {
    printf("file not found\n");
    return 1;
  }
  if (fileSize == 0) {
    if (freeBuffer) {
      free(buffer);
    }
    printf("invalid file\n");
    return 1;
  }
  assert(fileSize == fileSize);

  WllHandle handle = {0};
  WllHandleFromBuffer(&handle, buffer, fileSize);
  WLLHandlePrint(&handle);
  if (freeBuffer) {
    WllHandleRelease(&handle);
  }
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

static void usageVCN(void) { printf("vcn subcommands: input output\n"); }

static int cmdVCN(int argc, char *argv[]) {
  if (argc < 2) {
    usageVCN();
    return 1;
  }
  size_t fileSize = 0;
  int freebuffer = 0;
  const char *inputFile = argv[0];
  const char *outputFile = argv[1];
  uint8_t *buffer = getFileContent(inputFile, &fileSize, &freebuffer);
  if (!buffer) {
    return 1;
  }
  if (fileSize == 0) {
    free(buffer);
    return 1;
  }

  VCNHandle handle = {0};
  if (!VCNHandleFromLCWBuffer(&handle, buffer, fileSize)) {
    printf("VCNDataFromLCWBuffer error\n");
    if (freebuffer) {
      free(buffer);
    }

    return 1;
  }

  VCNImageToPng(&handle, outputFile);
  if (freebuffer) {
    VCNHandleRelease(&handle);
  }
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
  printf("script subcommands: strings|offsets|disasm [infile] [outfile] \n");
}

static int cmdScriptStrings(const char *filepath) {
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

  for (int i = 0; i < script.numTextStrings; i++) {
    printf("%i '%s'\n", i, INFScriptGetDataString(&script, i));
  }

  if (freeBuffer) {
    free(iffData);
  }
  return 0;
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
      printf("0X%X %X\n", i, offset);
    }
  }
  if (freeBuffer) {
    free(iffData);
  }
  return 0;
}

static int cmdScriptDisasm(const char *filepath, const char *outFile) {
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

  int n = 0;
  while (EMCInterpreterIsValid(&interp, &state)) {
    if (EMCInterpreterRun(&interp, &state) == 0) {
      printf("EMCInterpreterRun returned 0\n");
    }
    n++;
  }

  FILE *file = fopen(outFile, "w");
  if (file) {
    fprintf(file, "%s\n", disassembler.disasmBuffer);
    fclose(file);
  }

  EMCDisassemblerRelease(&disassembler);
  printf("Exec'ed %i / %i instructions\n", n, script.scriptDataSize);
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
  if (strcmp(argv[0], "offsets") == 0) {
    return cmdScriptOffsets(argv[1]);
  } else if (strcmp(argv[0], "disasm") == 0) {
    if (argc < 3) {
      usageScript();
      return 1;
    }
    return cmdScriptDisasm(argv[1], argv[2]);
  } else if (strcmp(argv[0], "strings") == 0) {
    return cmdScriptStrings(argv[1]);
  }
  usageScript();
  return 1;
}

static void usageCONF(void) {
  printf("conf subcommands: info|init filepath\n");
}

static int cmdCONFInit(const char *filepath) {

  GameConfig conf;
  if (!GameConfigCreateDefault(&conf)) {
    return 1;
  }

  int ret = GameConfigWriteFile(&conf, filepath);
  return !ret;
}
static int cmdCONFInfo(const char *filepath) {
  ConfigHandle handle = {0};
  if (!ConfigHandleFromFile(&handle, filepath)) {
    return 1;
  }

  for (int i = 0; i < handle.numEntries; i++) {
    printf("entry %i: '%s':'%s'\n", i, handle.entries[i].key,
           handle.entries[i].val);
  }
  ConfigHandleRelease(&handle);
  return 0;
}

static int cmdCONF(int argc, char *argv[]) {
  if (argc < 2) {
    usageCONF();
    return 1;
  }
  if (strcmp(argv[0], "info") == 0) {
    return cmdCONFInfo(argv[1]);
  } else if (strcmp(argv[0], "init") == 0) {
    return cmdCONFInit(argv[1]);
  }
  usageCONF();
  return 1;
}

static void usageVOC(void) {
  printf("voc subcommands: info|extract filepath [outfile]\n");
}

static int doVocExtract(const VOCHandle *handle, const char *outFilePath) {
  const VOCBlock *block = handle->firstBlock;

  if (block->type != VOCBlockType_SoundDataTyped) {
    printf("unhandled block type %i\n", block->type);
    return 1;
  }

  const VOCSoundDataTyped *soundData =
      (const VOCSoundDataTyped *)VOCBlockGetData(block);

  if (soundData->codec != VOCCodec_PCM_U8) {
    printf("unhandled codec %i\n", soundData->codec);
    return 1;
  }

  int sampleRate = 1000000 / (256 - soundData->freqDivisor);

  SF_INFO sfinfo = {0};
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  sfinfo.channels = 1;
  sfinfo.samplerate = sampleRate;
  SNDFILE *outFile = sf_open(outFilePath, SFM_WRITE, &sfinfo);
  if (!outFile) {
    printf("Not able to open output file %s.\n", outFilePath);
    puts(sf_strerror(NULL));
    return 1;
  }

  do {
    const VOCSoundDataTyped *soundData =
        (const VOCSoundDataTyped *)VOCBlockGetData(block);
    const int8_t *samples = ((int8_t *)soundData) + 2;
    size_t numSamples = VOCBlockGetSize(block) - 2;
    for (size_t i = 0; i < numSamples; i++) {
      int16_t v = (samples[i] - 0X80) << 8;
      sf_write_short(outFile, &v, 1);
    }
  } while ((block = VOCHandleGetNextBlock(handle, block)));

  sf_close(outFile);

  return 0;
}

static int cmdVocExtract(const char *vocFile, const char *outFile) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(vocFile, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", vocFile);
    return 1;
  }
  VOCHandle handle = {0};
  if (VOCHandleFromBuffer(&handle, buffer, dataSize) == 0) {
    printf("Error while reading file\n");
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }

  int ret = doVocExtract(&handle, outFile);

  if (freeBuffer) {
    free(buffer);
  }

  return ret;
}

static int cmdVocInfo(const char *vocFile) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(vocFile, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", vocFile);
    return 1;
  }
  VOCHandle handle = {0};
  if (VOCHandleFromBuffer(&handle, buffer, dataSize) == 0) {
    printf("Error while reading file\n");
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }

  const VOCBlock *block = handle.firstBlock;

  int i = 0;
  do {
    printf("blockid=%i type=%X size=%i ", i, block->type,
           VOCBlockGetSize(block));
    switch (block->type) {
    case VOCBlockType_SoundDataTyped: {
      const VOCSoundDataTyped *soundData =
          (const VOCSoundDataTyped *)VOCBlockGetData(block);

      int sampleRate = 1000000 / (256 - soundData->freqDivisor);
      size_t numSamples = VOCBlockGetSize(block) - 2;
      float duration = numSamples * 1.f / sampleRate;
      printf("codec=%X sampleRate=%i numSamples=%zi duration=%f",
             soundData->codec, sampleRate, numSamples, duration);
    } break;
    default:
      assert(0);
    }
    printf("\n");

    i++;
  } while ((block = VOCHandleGetNextBlock(&handle, block)));

  if (freeBuffer) {
    free(buffer);
  }

  return 0;
}
static int cmdVOC(int argc, char *argv[]) {
  if (argc < 2) {
    usageVOC();
    return 1;
  }
  if (strcmp(argv[0], "info") == 0) {
    const char *vocFile = argv[1];
    return cmdVocInfo(vocFile);
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 3) {
      usageVOC();
      return 1;
    }
    const char *vocFile = argv[1];
    const char *outFile = argv[2];
    return cmdVocExtract(vocFile, outFile);
  }
  usageVOC();
  return 0;
}

static void usageSHP(void) {
  printf("shp subcommands: [-c] info|extract  filepath index outfile "
         "[palette]\n");
}

static int doSHPExtract(int index, const SHPHandle *handle,
                        const char *outfilePath, const char *vcnPaletteFile) {
  VCNHandle vcnHandle = {0};
  int freeVcnFileBuffer = 0;
  uint8_t *vcnFileBuffer = NULL;
  if (vcnPaletteFile) {
    printf("using vcn palette file '%s'\n", vcnPaletteFile);
    size_t vcnDataSize = 0;
    vcnFileBuffer =
        getFileContent(vcnPaletteFile, &vcnDataSize, &freeVcnFileBuffer);
    if (!vcnFileBuffer) {
      printf("Error while getting data for '%s'\n", vcnPaletteFile);
      return 1;
    }
    if (!VCNHandleFromLCWBuffer(&vcnHandle, vcnFileBuffer, vcnDataSize)) {
      printf("VCNDataFromLCWBuffer error\n");
      if (freeVcnFileBuffer) {
        free(vcnFileBuffer);
      }
      return 1;
    }
  }
  printf("extracting index %i\n", index);
  SHPFrame frame = {0};
  SHPHandleGetFrame(handle, &frame, index);
  SHPFramePrint(&frame);
  SHPFrameGetImageData(&frame);
  SHPFrameToPng(&frame, outfilePath, vcnPaletteFile ? vcnHandle.palette : NULL);
  SHPFrameRelease(&frame);
  if (vcnPaletteFile != NULL) {
    VCNHandleRelease(&vcnHandle);
  }
  if (freeVcnFileBuffer) {
    free(vcnFileBuffer);
  }
  return 0;
}

static int cmdShp(int argc, char *argv[]) {
  if (argc < 2) {
    usageSHP();
    return 1;
  }
  int compressed = 0;

  if (strcmp(argv[0], "-c") == 0) {
    compressed = 1;
    argc--;
    argv++;
  }
  if (strcmp(argv[0], "info") != 0 && strcmp(argv[0], "extract") != 0) {
    usageSHP();
    return 1;
  }
  if (strcmp(argv[0], "extract") == 0 && argc < 4) {
    usageSHP();
    return 1;
  }

  const char *shpFile = argv[1];

  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(shpFile, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", shpFile);
    return 1;
  }

  SHPHandle handle = {0};
  if (compressed == 0) {
    if (!SHPHandleFromBuffer(&handle, buffer, dataSize)) {
      perror("SHPHandleFromBuffer");
      if (freeBuffer) {
        free(buffer);
      }
      return 1;
    }
  } else {
    if (!SHPHandleFromCompressedBuffer(&handle, buffer, dataSize)) {
      perror("SHPHandleFromBuffer");
      if (freeBuffer) {
        free(buffer);
      }
      return 1;
    }
  }
  int ret = 0;
  if (strcmp(argv[0], "info") == 0) {
    SHPHandlePrint(&handle);
  } else if (strcmp(argv[0], "extract") == 0) {
    int index = atoi(argv[2]);
    const char *outfilePath = argv[3];
    const char *vcnPaletteFile = argc > 4 ? argv[4] : NULL;
    ret = doSHPExtract(index, &handle, outfilePath, vcnPaletteFile);
  }

  SHPHandleRelease(&handle);
  return ret;
}

static int cmdDat(int argc, char *argv[]) {
  if (argc < 1) {
    printf("missing input file\n");
    return 1;
  }
  const char *datFile = argv[0];
  printf("open DAT file '%s'\n", datFile);
  size_t fileSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(datFile, &fileSize, &freeBuffer);
  if (!buffer) {
    perror("readBinaryFile");
    return 1;
  }
  DatHandle handle = {0};
  if (!DatHandleFromBuffer(&handle, buffer, fileSize)) {
    perror("DatHandleFromBuffer");
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }
  DatHandlePrint(&handle);
  if (freeBuffer) {
    DatHandleRelease(&handle);
  }
  return 0;
}

static void usageCPS(void) {
  printf("cps extract|extract-pal|info cpsfile [outputfile]\n");
}

static int cmdCPSInfo(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  CPSImage image = {0};
  int ok = CPSImageFromBuffer(&image, buffer, dataSize);
  if (freeBuffer) {
    free(buffer);
  }
  if (!ok) {
    return 1;
  }

  CPSImageRelease(&image);
  printf("palette size: %zu\n", image.paletteSize);
  printf("image size: %zu\n", image.imageSize);
  return 0;
}

static int cmdCPSExtractPal(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  CPSImage image = {0};
  int ok = CPSImageFromBuffer(&image, buffer, dataSize);
  if (freeBuffer) {
    free(buffer);
  }
  if (!ok) {
    return 1;
  }
  if (image.paletteSize == 0) {
    printf("no palette found\n");
    CPSImageRelease(&image);
    return 1;
  }
  char *destFilePath = strdup(filepath);
  assert(destFilePath);
  destFilePath[strlen(destFilePath) - 3] = 'p';
  destFilePath[strlen(destFilePath) - 2] = 'a';
  destFilePath[strlen(destFilePath) - 1] = 'l';
  printf("Create pal file '%s'\n", destFilePath);
  assert(image.palette);
  writeBinaryFile(destFilePath, image.palette, image.paletteSize);
  free(destFilePath);

  CPSImageRelease(&image);
  return 0;
}

static int cmdCPSExtract(const char *filepath, const char *destFilePath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }
  CPSImage image = {0};
  int ok = CPSImageFromBuffer(&image, buffer, dataSize);
  if (freeBuffer) {
    free(buffer);
  }
  if (ok) {
    CPSImageToPng(&image, destFilePath);
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
    if (argc < 3) {
      usageCPS();
      return 1;
    }
    return cmdCPSExtract(argv[1], argv[2]);
  } else if (strcmp(argv[0], "extract-pal") == 0) {
    return cmdCPSExtractPal(argv[1]);
  } else if (strcmp(argv[0], "info") == 0) {
    return cmdCPSInfo(argv[1]);
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
  for (int i = 0; i < file->count; i++) {
    PAKEntry *entry = &file->entries[i];
    printf("%s\n", entry->filename);
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

static void usageSAV(void) {
  printf("sav subcommands: show|set file [set-cmd] [outfile]\n");
}

static int cmdSAVShow(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  SAVHandle handle = {0};
  if (!SAVHandleFromBuffer(&handle, buffer, dataSize)) {
    if (freeBuffer) {
      free(buffer);
    }
    return 1;
  }

  const SAVSlot *slot = &handle.slot;
  printf("Slot name '%s'\n", slot->header.name);
  printf("Slot type '%s'\n", slot->header.type);
  printf("Slot version '%s'\n", slot->header.version);
  printf("+CHARACTERS\n");
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    const SAVCharacter *ch = &slot->characters[i];
    if (!ch->flags) {
      continue;
    }
    printf("character %i : flags:%X name:'%s' raceClassSex=%X id=%i "
           "magicPointsCur=%i "
           "magicPointsMax=%i\n",
           i, ch->flags, ch->name, ch->raceClassSex, ch->id, ch->magicPointsCur,
           ch->magicPointsMax);
    printf("might %i protection %i\n", ch->might, ch->protection);
    printf("\titems: ");
    for (int i = 0; i < 11; i++) {
      printf(" %02X ", ch->items[i]);
    }
    printf("\n");
  }

  printf("+GENERAL\n");
  uint16_t x = 0;
  uint16_t y = 0;
  GetRealCoords(slot->posX, slot->posY, &x, &y);
  printf("block=%X x=%X y=%X (real %i %i)\n", slot->currentBlock, slot->posX,
         slot->posY, x, y);
  printf("orientation=%X compass=%X\n", slot->currentDirection,
         slot->compassDirection);
  printf("level %i\n", slot->currentLevel);
  printf("credits %i\n", slot->credits);
  printf("inventoryCurrentItem %X\n", slot->inventoryCurrentItem);
  printf("item in hand %X\n", slot->itemIndexInHand);
  printf("+GLOBAL SCRIPT VARS2\n");
  for (int i = 0; i < NUM_GLOBAL_SCRIPT_VARS2; i++) {
    printf("%i 0X%X\n", i, slot->globalScriptVars2[i]);
  }
  printf("selected char %i\n", slot->selectedChar);
  printf("+INVENTORY\n");

  for (int i = 0; i < INVENTORY_SIZE; i++) {
    uint16_t gameObjIndex = slot->inventory[i];
    if (gameObjIndex == 0) {
      continue;
    }
    const GameObject *obj = slot->gameObjects + gameObjIndex;
    printf("%i: gameObjIndex=%03i itemPropertyIndex=0X%X\n", i, gameObjIndex,
           obj->itemPropertyIndex);
  }

  printf("+Game flags\n");
  for (int i = 0; i < NUM_FLAGS; i++) {
    printf("%i %X\n", i, slot->flags[i]);
  }
  printf("+Objects\n");
  for (int i = 0; i < MAX_IN_GAME_ITEMS; i++) {
    const GameObject *obj = slot->gameObjects + i;
    if (obj->itemPropertyIndex == 0) {
      continue;
    }
    printf(
        "gameObjIndex=%03i itemPropertyIndex=%04X x=%i y=%i block=%X level=%i",
        i, obj->itemPropertyIndex, obj->x, obj->y, obj->block, obj->level);
    printf("\n");
  }

  for (int levelId = 0; levelId < NUM_LEVELS; levelId++) {
    const TempLevelData *tempData = slot->tempLevelData + levelId;
    if (!SAVSlotHasTempLevelData(slot, levelId)) {
      continue;
    }
    printf("Level %i\n", levelId);
    for (int monsterId = 0; monsterId < MAX_MONSTERS; monsterId++) {
      const Monster *monster = tempData->monsters + monsterId;
      if (!monster->type) {
        continue;
      }
      printf("\tMonster %i type=%X\n", monsterId, monster->type);
    }
    for (int objId = 0; objId < NUM_FLYING_OBJECTS; objId++) {
      const FlyingObject *obj = tempData->flyingObjects + objId;
      if (!obj->enable) {
        continue;
      }
      printf("\t Flying Object %i type=%x\n", objId, obj->objectType);
    }
  }

  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static int cmdSAVSet(const char *filepath, const char *cmd, const char *value,
                     const char *outfilepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  SAVHandle handle = {0};
  SAVHandleFromBuffer(&handle, buffer, dataSize);
  int ret = 1;
  if (strcmp(cmd, "block") == 0) {
    int blockId = strtol(value, NULL, 16);
    printf("setting block to 0X%X\n", blockId);
    handle.slot.currentBlock = blockId;
    ret = 0;
  }
  if (ret == 0) {
    ret = 1;
    if (SAVHandleSaveTo(&handle, outfilepath)) {
      ret = 0;
    }
  }
  if (freeBuffer) {
    free(buffer);
  }
  return ret;
}

static int cmdSAV(int argc, char *argv[]) {
  if (argc < 2) {
    usageSAV();
    return 1;
  }
  if (strcmp(argv[0], "show") == 0) {
    return cmdSAVShow(argv[1]);
  } else if (strcmp(argv[0], "set") == 0) {
    if (argc < 5) {
      printf("missing arguments\n");
      usageSAV();
      return 1;
    }
    return cmdSAVSet(argv[1], argv[2], argv[3], argv[4]);
  }
  return 1;
}

static int cmdFNTExtract(const char *filepath) {
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
  FNTToPng(&handle, outFilePath);
  if (freeBuffer) {
    free(buffer);
  }
  free(outFilePath);
  return 0;
}

static void usageFNT(void) { printf("fnt subcommands: render file charNum\n"); }

static int cmdFNT(int argc, char *argv[]) {
  if (argc < 3) {
    usageFNT();
    return 1;
  }
  if (strcmp(argv[0], "render") == 0) {
    return cmdFNTExtract(argv[1]);
  }
  usageFNT();
  return 1;
}

static void doRenderWSAFrame(const WSAHandle *handle, int frameNum,
                             const uint8_t *palette, const char *outFilePath) {
  printf("Extract frame %i/%i\n", frameNum, handle->header.numFrames);
  size_t frameDataSize = handle->header.width * handle->header.height;
  uint8_t *frameData = malloc(frameDataSize);
  memset(frameData, 0, frameDataSize);
  assert(frameData);

  for (int i = 0; i <= frameNum; i++) {
    WSAHandleGetFrame(handle, i, frameData, 1);
  }

  WSAFrameToPng(frameData, frameDataSize, palette, outFilePath,
                handle->header.width, handle->header.height);

  free(frameData);
}

static int cmdWSAExtract(const char *filepath, int frameNum,
                         const char *outFilePath, const char *cpsPaletteFile) {
  size_t wsaDataSize = 0;
  int freeWsaBuffer = 0;

  int freeCpsBuffer = 0;
  CPSImage img = {0};
  uint8_t *cpsBuffer = NULL;
  uint8_t *wsaBuffer = getFileContent(filepath, &wsaDataSize, &freeWsaBuffer);
  if (!wsaBuffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  if (cpsPaletteFile) {
    size_t cpsDataSize = 0;
    cpsBuffer = getFileContent(cpsPaletteFile, &cpsDataSize, &freeCpsBuffer);
    if (!cpsBuffer) {
      printf("Error while getting data for cps file '%s'\n", cpsPaletteFile);
      if (freeWsaBuffer) {
        free(wsaBuffer);
      }
      return 1;
    }

    if (!CPSImageFromBuffer(&img, cpsBuffer, cpsDataSize)) {
      printf("invalid CPS image\n");
      if (freeWsaBuffer) {
        free(wsaBuffer);
      }
      if (freeCpsBuffer) {
        free(cpsBuffer);
      }
    }
  }

  WSAHandle handle;
  WSAHandleInit(&handle);
  WSAHandleFromBuffer(&handle, wsaBuffer, wsaDataSize);
  if (frameNum < 0 || frameNum >= handle.header.numFrames) {
    printf("invalid frameNum\n");
    if (freeWsaBuffer) {
      free(wsaBuffer);
    }
    if (freeCpsBuffer) {
      free(cpsBuffer);
    }
    if (img.data) {
      CPSImageRelease(&img);
    }
    return 1;
  }
  uint8_t *palette = handle.header.palette;
  if (cpsPaletteFile && img.data) {
    palette = img.palette;
  }

  doRenderWSAFrame(&handle, frameNum, palette, outFilePath);

  if (freeWsaBuffer) {
    free(wsaBuffer);
  }
  if (freeCpsBuffer) {
    free(cpsBuffer);
  }
  if (img.data) {
    CPSImageRelease(&img);
  }
  return 0;
}

static int cmdWSAInfo(const char *filepath) {
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
  printf("numFrame=%i x=%i y=%i w=%i h=%i palette=%X delta=%X\n",
         handle.header.numFrames, handle.header.xPos, handle.header.yPos,
         handle.header.width, handle.header.height, handle.header.hasPalette,
         handle.header.delta);

  for (int i = 0; i < handle.header.numFrames + 2; i++) {
    uint32_t frameOffset = WSAHandleGetFrameOffset(&handle, i);
    printf("i=%i offset=0X%X size=0X%zX\n", i, frameOffset, handle.bufferSize);
    if (frameOffset == handle.bufferSize) {
      printf("IS ZERO\n");
    }
  }

  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static void usageWSA(void) {
  printf("wsa subcommands: info|extract file [framenum] [outfile] "
         "[cpspalette]\n");
}

static int cmdWSA(int argc, char *argv[]) {
  if (argc < 2) {
    printf("wsa command, missing arguments\n");
    usageWSA();
    return 1;
  }

  const char *filepath = argv[1];
  if (strcmp(argv[0], "info") == 0) {
    return cmdWSAInfo(filepath);
  } else if (strcmp(argv[0], "extract") == 0) {
    if (argc < 4) {
      usageWSA();
      return 1;
    }
    const char *outFilePath = argv[3];
    const char *cpsPaletteFile = argc > 4 ? argv[4] : NULL;
    printf("cpsPaletteFile=%s\n", cpsPaletteFile);
    return cmdWSAExtract(filepath, atoi(argv[2]), outFilePath, cpsPaletteFile);
  }
  usageWSA();
  return 1;
}

static void usageXXX(void) { printf("xxx subcommands: show file\n"); }

static int cmdXXXShow(const char *filepath) {
  size_t dataSize = 0;
  int freeBuffer = 0;
  uint8_t *buffer = getFileContent(filepath, &dataSize, &freeBuffer);
  if (!buffer) {
    printf("Error while getting data for '%s'\n", filepath);
    return 1;
  }

  printf("xxx data size = %zi -> %zi\n", dataSize, dataSize / 12);
  XXXHandle handle = {0};
  XXXHandleFromBuffer(&handle, buffer, dataSize);

  for (int i = 0; i < handle.numEntries; i++) {
    const LegendEntry *entry = handle.entries + i;
    printf("%i shapeId=0X%02X enabled=%X p=%02X strId=%i\n", i, entry->shapeId,
           entry->enabled, entry->p, entry->stringId - 0X4000);
  }
  if (freeBuffer) {
    free(buffer);
  }
  return 0;
}

static int cmdXXX(int argc, char *argv[]) {
  if (argc < 1) {
    printf("xxx command, missing arguments\n");
    usageXXX();
    return 0;
  }
  const char *filepath = argv[1];
  if (strcmp(argv[0], "show") == 0) {
    return cmdXXXShow(filepath);
  }
  usageXXX();
  return 0;
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
  printf("%s: pak|cmz|script|inf|wll|render|game|dat|shp|lang "
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
  } else if (strcmp(argv[1], "script") == 0) {
    return cmdScript(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "game") == 0) {
    return cmdGame(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "dat") == 0) {
    return cmdDat(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "shp") == 0) {
    return cmdShp(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "lang") == 0) {
    return cmdLang(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "xxx") == 0) {
    return cmdXXX(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "wsa") == 0) {
    return cmdWSA(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "sav") == 0) {
    return cmdSAV(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "fnt") == 0) {
    return cmdFNT(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "dbg") == 0) {
    return cmdDbg(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "voc") == 0) {
    return cmdVOC(argc - 2, argv + 2);
  } else if (strcmp(argv[1], "conf") == 0) {
    return cmdCONF(argc - 2, argv + 2);
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
    if (!PakFileLoadMain(pakFilePath)) {
      printf("error while reading pak file %s\n", pakFilePath);
      return 1;
    }
  }
  int ret = doCMD(argc, argv);
  PakFileReleaseMain();
  return ret;
}
