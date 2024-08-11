/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <malloc.h>

// We include some GpuApi headers here, but we aren't linking with any engine/core/etc libs. This is just to pull in some enums and
// inline functions and such.
#include "../../src/common/gpuApiUtils/gpuApiUtils.h"


using namespace GpuApi;


#include "texCookTool.h"
using namespace TexCookTool;


struct MipData
{
	Uint32 pitch;
	Uint32 size;
	const void* data;
};


struct TexData
{
	std::vector< MipData > mips;
};

class ProcessingData
{
public:
	Uint16 mipCount, texCount;
	Uint16 baseWidth, baseHeight;
	GpuApi::eTextureFormat texFormat;
	GpuApi::eTextureType texType;

	std::vector< TexData > texs;

	Uint32* mipOffsets;					// mipCount of these

	size_t outputSize, outputAlign;
	void* output;

	ProcessingData()
		: output( nullptr )
		, outputSize( 0 )
		, outputAlign( 0 )
		, mipOffsets( nullptr )
	{}

	void AllocateOutput( size_t size, size_t align );
};


extern std::wstringstream GOutput;


// To be implemented by the platform-specific tool.
Bool DoProcess( ProcessingData& data );
