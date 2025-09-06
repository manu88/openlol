#pragma once

#include "format_cps.h"
#include "format_vcn.h"

void CPSImageToPng(const CPSImage *image, const char *savePngPath);

void VCNImageToPng(const VCNData *image, const char *savePngPath);
