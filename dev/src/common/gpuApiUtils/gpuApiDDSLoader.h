//--------------------------------------------------------------------------------------
// Based on the XDK sample loader
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include "gpuApiUtils.h"

namespace GpuApi
{
	enum DDS_ALPHA_MODE
	{
		DDS_ALPHA_MODE_UNKNOWN       = 0,
		DDS_ALPHA_MODE_STRAIGHT      = 1,
		DDS_ALPHA_MODE_PREMULTIPLIED = 2,
		DDS_ALPHA_MODE_OPAQUE        = 3,
		DDS_ALPHA_MODE_CUSTOM        = 4,
	};

	Bool CreateDDSTextureFromMemory( const Uint8* ddsData, size_t ddsDataSize, TextureDesc& textureDesc, TextureInitData& initData );
	void CleanupInitData( TextureInitData& initData );
}