#ifndef __UNTEXTURENVTT_H__
#define __UNTEXTURENVTT_H__

#include <UEViewer/libs/nvtt/nvimage/Image.h>
#include <UEViewer/libs/nvtt/nvimage/DirectDrawSurface.h>
#undef __FUNC__						// conflicted with our guard macros

void DecodeDDS(const unsigned char* Data, int USize, int VSize, nv::DDSHeader& header, nv::Image& image);
void WriteDDSHeader(unsigned char* Data, nv::DDSHeader& header);

#endif // __UNTEXTURENVTT_H__
