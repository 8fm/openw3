/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "simplet-common.h"
#include <video_out.h>
#include "toolkit/stack_allocator.h"
#include "toolkit/embedded_shader.h"
using namespace sce;

// Video Out Infomation
struct VideoInfo{
		int32_t handle;
		uint64_t* label;
		uint32_t label_num;
		uint32_t flip_index;
		uint32_t buffer_num;
		SceKernelEqueue eq;
	};
static VideoInfo s_videoInfo;

Gnmx::Toolkit::StackAllocator s_garlicAllocator;
Gnmx::Toolkit::StackAllocator s_onionAllocator;

sce::Gnmx::Toolkit::IAllocator SimpletUtil::getGarlicAllocator()
{
	return GetInterface(&s_garlicAllocator);
}
sce::Gnmx::Toolkit::IAllocator SimpletUtil::getOnionAllocator()
{
	return GetInterface(&s_onionAllocator);
}

void SimpletUtil::registerRenderTargetForDisplay(sce::Gnm::RenderTarget *renderTarget)
{
	const uint32_t kPlayerId = 0;
	int ret = sceVideoOutOpen(kPlayerId, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
	SCE_GNM_VALIDATE(ret >= 0, "sceVideoOutOpen() returned error code %d.", ret);
	s_videoInfo.handle = ret;
	// Create Attribute
	SceVideoOutBufferAttribute attribute;
	sceVideoOutSetBufferAttribute(&attribute, SCE_VIDEO_OUT_PIXEL_FORMAT_B8_G8_R8_A8_SRGB,
								  SCE_VIDEO_OUT_TILING_MODE_TILE,
								  SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
								  renderTarget->getWidth(), renderTarget->getHeight(),renderTarget->getWidth());
	ret = sceVideoOutSetFlipRate(s_videoInfo.handle, 0);
	SCE_GNM_VALIDATE(ret >= 0, "sceVideoOutSetFlipRate() returned error code %d.", ret);
	// Prepare Equeue for Flip Sync
	ret = sceKernelCreateEqueue(&s_videoInfo.eq, __FUNCTION__);
	SCE_GNM_VALIDATE(ret >= 0, "sceKernelCreateEqueue() returned error code %d.", ret);
	ret=sceVideoOutAddFlipEvent(s_videoInfo.eq, s_videoInfo.handle, NULL);
	SCE_GNM_VALIDATE(ret >= 0, "sceVideoOutAddFlipEvent() returned error code %d.", ret);
	s_videoInfo.flip_index = 0;
	s_videoInfo.buffer_num = 1;
	void* address[1];
	address[0] = renderTarget->getBaseAddress();
	ret= sceVideoOutRegisterBuffers(s_videoInfo.handle, 0, address, 1, &attribute );
	SCE_GNM_VALIDATE(ret >= 0, "sceVideoOutRegisterBuffers() returned error code %d.", ret);
}

void SimpletUtil::requestFlipAndWait()
{
	// Set Flip Request
	int ret=sceVideoOutSubmitFlip(s_videoInfo.handle, s_videoInfo.flip_index, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);
	SCE_GNM_VALIDATE(ret >= 0, "sceVideoOutSubmitFlip() returned error code %d.", ret);
	// Wait Flip
	SceKernelEvent ev;
	int out;
	ret=sceKernelWaitEqueue(s_videoInfo.eq, &ev, 1, &out, 0);
	s_videoInfo.flip_index = (s_videoInfo.flip_index + 1) % s_videoInfo.buffer_num;
}

bool SimpletUtil::handleUserEvents()
{
	return false;
}

int32_t SimpletUtil::SaveTextureToTGA(const sce::Gnm::Texture *texture, const char* tgaFileName)
{
	if(!texture)
		return -3;

	// only support the UNORM 32bpp formats
	if(texture->getDataFormat().m_asInt != sce::Gnm::kDataFormatB8G8R8A8UnormSrgb.m_asInt
		&& texture->getDataFormat().m_asInt != sce::Gnm::kDataFormatR8G8B8A8UnormSrgb.m_asInt)
	{
		return -5;
	}

	char path[256];
	// Use the file serving dir
	strncpy( path, SAMPLE_FRAMEWORK_FSROOT "/", 64 );
	strncat(path, tgaFileName, 128);

	FILE *tgaFile = fopen(path, "wb");
	if (tgaFile == 0)
	{
		return -1;
	}

#pragma pack(push, 1)
    struct TargaHeader
    {
		uint8_t  idLength;          /* 00h  Size of Image ID field */
		uint8_t  colorMapType;      /* 01h  Color map type */
		uint8_t  imageType;         /* 02h  Image type code */
		uint16_t colorMapIndex;     /* 03h  Color map origin */
		uint16_t colorMapLength;    /* 05h  Color map length */
		uint8_t  colorMapBits;      /* 07h  Depth of color map entries */
		uint16_t xOrigin;           /* 08h  X origin of image */
		uint16_t yOrigin;           /* 0Ah  Y origin of image */
		uint16_t width;             /* 0Ch  Width of image */
		uint16_t height;            /* 0Eh  Height of image */
		uint8_t  pixelDepth;        /* 10h  Image pixel size */
		uint8_t  imageDescriptor;   /* 11h  bits 3-0 give the alpha channel depth, bits 5-4 give direction */

    };
#pragma pack(pop)

	TargaHeader tgaHeader;
    memset(&tgaHeader,0, sizeof(tgaHeader));
    tgaHeader.imageType = 2;
    tgaHeader.width = (uint16_t)texture->getPitch(); // for simplicity assume pitch as width
    tgaHeader.height = (uint16_t)texture->getHeight();
    tgaHeader.pixelDepth = (uint8_t) texture->getDataFormat().getBitsPerElement();
    tgaHeader.imageDescriptor = 0x20; // y direction is reversed (from up to down)
	fwrite(&tgaHeader, sizeof(tgaHeader), 1, tgaFile);

	uint8_t* tiledPixels = static_cast<uint8_t*>(texture->getBaseAddress());
	uint8_t* untiledPixels = static_cast<uint8_t*>(allocateMemory(tgaHeader.width*texture->getHeight() * 4, 256));
	GpuAddress::TilingParameters tp;
	tp.initFromTexture(texture, 0, 0); // We'll update these further inside the loop
	GpuAddress::detileSurface(untiledPixels, tiledPixels, &tp); // completely redundant; included for test coverage purposes

	if(texture->getDataFormat().m_asInt != sce::Gnm::kDataFormatB8G8R8A8UnormSrgb.m_asInt)
	{
		// have to reverse the channels within each pixel
		uint32_t numPixels = tgaHeader.width*texture->getHeight();
		for(uint32_t pixel = 0; pixel < numPixels; ++pixel)
		{
			std::swap(untiledPixels[pixel*4 + 0], untiledPixels[pixel*4 + 3]);
			std::swap(untiledPixels[pixel*4 + 1], untiledPixels[pixel*4 + 2]);
		}
	}

	uint32_t bytes = tgaHeader.width*texture->getHeight()*4;
	fwrite(untiledPixels, 1, bytes, tgaFile);

	fclose(tgaFile);
	return 0;
}

int32_t SimpletUtil::SaveDepthTargetToTGA(const sce::Gnm::DepthRenderTarget *target, const char* tgaFileName)
{
	if(!target)
		return -3;

	// only support the UNORM 32bpp formats
	if (target->getZFormat() != sce::Gnm::kZFormat32Float)
	{
		return -5;
	}

	// Only support single-slice targets
	const uint32_t numSlices = target->getLastArraySliceIndex() - target->getBaseArraySliceIndex() + 1;
	if (numSlices != 0)
	{
		return -7;
	}

	FILE *tgaFile = fopen(tgaFileName, "wb");
	if (tgaFile == 0)
	{
		return -1;
	}

#pragma pack(push, 1)
	struct TargaHeader
	{
		uint8_t  idLength;          /* 00h  Size of Image ID field */
		uint8_t  colorMapType;      /* 01h  Color map type */
		uint8_t  imageType;         /* 02h  Image type code */
		uint16_t colorMapIndex;     /* 03h  Color map origin */
		uint16_t colorMapLength;    /* 05h  Color map length */
		uint8_t  colorMapBits;      /* 07h  Depth of color map entries */
		uint16_t xOrigin;           /* 08h  X origin of image */
		uint16_t yOrigin;           /* 0Ah  Y origin of image */
		uint16_t width;             /* 0Ch  Width of image */
		uint16_t height;            /* 0Eh  Height of image */
		uint8_t  pixelDepth;        /* 10h  Image pixel size */
		uint8_t  imageDescriptor;   /* 11h  bits 3-0 give the alpha channel depth, bits 5-4 give direction */

	};
#pragma pack(pop)

	TargaHeader tgaHeader;
	memset(&tgaHeader,0, sizeof(tgaHeader));
	tgaHeader.imageType = 2;
	tgaHeader.width = (uint16_t)target->getWidth();
	tgaHeader.height = (uint16_t)target->getHeight();
	tgaHeader.pixelDepth = (uint8_t)32;
	tgaHeader.imageDescriptor = 0x20; // y direction is reversed (from up to down)
	fwrite(&tgaHeader, sizeof(tgaHeader), 1, tgaFile);

	uint8_t* pixels_out = (uint8_t*)malloc( target->getHeight()*target->getWidth()*4 );

	if (target->getZFormat() == sce::Gnm::kZFormat32Float)
	{
		// have to convert F32 to output color (currently simple grayscale)
		sce::GpuAddress::TilingParameters tp;
		tp.initFromDepthSurface(target, 0);

		float* pixels_in_float = (float*)target->getZReadAddress();

		uint32_t const height = target->getHeight(), width = target->getWidth()/*, pitch = target->getPitch()*/;
		for (uint32_t y = 0; y < height; ++y) {
			for (uint32_t x = 0; x < width; ++x) {
				uint64_t offsetUntiled, offsetTiled;
				sce::GpuAddress::computeTiledElementByteOffset(&offsetTiled, &tp, x, y, 0, 0);
				sce::GpuAddress::computeLinearElementByteOffset(&offsetUntiled, x,y,0,0, tp.m_linearWidth, tp.m_linearWidth*tp.m_linearHeight, 32, 1);
				SCE_GNM_ASSERT( offsetTiled < target->getZSliceSizeInBytes()*numSlices && !(offsetTiled & 0x3) );
				SCE_GNM_ASSERT( offsetUntiled < height*width*4 && !(offsetUntiled & 0x3) );
				float fDepth = pixels_in_float[offsetTiled/4];
				uint8_t grayscale = (fDepth <= 0.0f) ? 0x00 : (fDepth >= 1.0f) ? 0xFF : (uint8_t)( fDepth * 255.0f + 0.5f );
				pixels_out[offsetUntiled + 0] = (fDepth < 0.5f) ? grayscale/2 : grayscale;	//B
				pixels_out[offsetUntiled + 1] = (fDepth < 0.5f) ? grayscale/2 : grayscale;	//G
				pixels_out[offsetUntiled + 2] = grayscale;	//R
				pixels_out[offsetUntiled + 3] = 0xFF;	//A
			}
		}
	}

	fwrite(pixels_out, tgaHeader.width*tgaHeader.height, 4, tgaFile);
	free(pixels_out);

	fclose(tgaFile);
	return 0;
}

void SimpletUtil::AcquireFileContents(void*& pointer, uint32_t& bytes, const char* filename)
{
	FILE *fp = fopen(filename, "rb");
	SCE_GNM_VALIDATE(fp, "fopen(\"%s\") returned NULL.", filename);
	fseek(fp, 0, SEEK_END);
	bytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	pointer = malloc(bytes);
	fread(pointer, 1, bytes, fp);
	fclose(fp);
}

void SimpletUtil::ReleaseFileContents(void* pointer)
{
	free(pointer);
}

sce::Gnmx::VsShader* SimpletUtil::LoadVsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedVsShader embeddedVsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedVsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedVsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedVsShader.m_shader;
}

sce::Gnmx::PsShader* SimpletUtil::LoadPsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedPsShader embeddedPsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedPsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedPsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedPsShader.m_shader;
}

sce::Gnmx::CsShader* SimpletUtil::LoadCsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedCsShader embeddedCsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedCsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedCsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedCsShader.m_shader;
}

sce::Gnmx::HsShader* SimpletUtil::LoadHsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedHsShader embeddedHsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedHsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedHsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedHsShader.m_shader;
}

sce::Gnmx::EsShader* SimpletUtil::LoadEsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedEsShader embeddedEsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedEsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedEsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedEsShader.m_shader;
}

sce::Gnmx::LsShader* SimpletUtil::LoadLsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedLsShader embeddedLsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedLsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedLsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedLsShader.m_shader;
}

sce::Gnmx::GsShader* SimpletUtil::LoadGsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedGsShader embeddedGsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedGsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedGsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedGsShader.m_shader;
}

sce::Gnmx::CsVsShader* SimpletUtil::LoadCsVsShaderFromMemory(const void *pointer)
{
	Gnmx::Toolkit::EmbeddedCsVsShader embeddedCsVsShader = {(uint32_t*)pointer};
	Gnmx::Toolkit::MemoryRequests memoryRequests;
	memoryRequests.initialize();
	embeddedCsVsShader.addToMemoryRequests(&memoryRequests);
	memoryRequests.m_garlic.fulfill(s_garlicAllocator.allocate(memoryRequests.m_garlic.m_sizeAlign));
	memoryRequests.m_onion.fulfill(s_onionAllocator.allocate(memoryRequests.m_onion.m_sizeAlign));
	embeddedCsVsShader.initializeWithMemoryRequests(&memoryRequests);
	return embeddedCsVsShader.m_shader;
}

void SimpletUtil::Init(uint32_t garlicSize, uint32_t onionSize)
{
	s_garlicAllocator.init(SCE_KERNEL_WC_GARLIC, garlicSize);
	s_onionAllocator.init(SCE_KERNEL_WB_ONION, onionSize);
}

void *SimpletUtil::allocateMemory(uint32_t size, sce::Gnm::AlignmentType align)
{
	void *pointer = s_garlicAllocator.allocate(size, align);
	SCE_GNM_VALIDATE(pointer, "allocate(%d,%d) returned NULL.", size, align);
	return pointer;
}

void *SimpletUtil::allocateMemory(sce::Gnm::SizeAlign sizeAlign)
{
	return s_garlicAllocator.allocate(sizeAlign.m_size, sizeAlign.m_align);
}

void *SimpletUtil::allocateGarlicMemory(sce::Gnm::SizeAlign sizeAlign)
{
	return s_garlicAllocator.allocate(sizeAlign.m_size, sizeAlign.m_align);
}

void *SimpletUtil::allocateOnionMemory(sce::Gnm::SizeAlign sizeAlign)
{
	return s_onionAllocator.allocate(sizeAlign.m_size, sizeAlign.m_align);
}
/*
sce::Gnmx::VsShader* SimpletUtil::LoadVsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::VsShader* result = LoadVsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::PsShader* SimpletUtil::LoadPsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::PsShader* result = LoadPsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::CsShader* SimpletUtil::LoadCsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::CsShader* result = LoadCsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::HsShader* SimpletUtil::LoadHsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::HsShader* result = LoadHsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::EsShader* SimpletUtil::LoadEsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::EsShader* result = LoadEsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::LsShader* SimpletUtil::LoadLsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::LsShader* result = LoadLsShaderFromMemory(pointer);
	ReleaseFileContents(pointer);
	return result;
}

sce::Gnmx::GsShader* SimpletUtil::LoadGsShader(const char* filename)
{
	void* pointer;
	uint32_t bytes;
	AcquireFileContents(pointer, bytes, filename);
	Gnmx::GsShader* result = LoadGsShaderFromMemory(pointer);;
	ReleaseFileContents(pointer);
	return result;
}
*/
