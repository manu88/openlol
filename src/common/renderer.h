#pragma once

#include "format_cps.h"
#include "format_vcn.h"
#include "format_vmp.h"

void CPSImageToPng(const CPSImage *image, const char *savePngPath);

void VCNImageToPng(const VCNHandle *image, const char *savePngPath);

void testRenderScene(const VCNHandle *vcn, const VMPHandle *vmp);
