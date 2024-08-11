/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// API headers
#include "../redSystem/os.h"
#include "gpuApiTypes.h"
#include "gpuApiErrorHandling.h"
#include "gpuApiVertexPacking.h"
#include "gpuApiVertexFormats.h"
#include "gpuApiCommon.h"
#include "gpuApiContainer.h"
#include "gpuApiMappingCommon.h"
#include "gpuApiInterface.h"



#define USE_CONSTANT_BUFFERS		// use constant buffers instead of loose constants available on DX10+
#define USE_TEXTURE_ARRAYS			// use texture arrays available on DX10+
#define USE_TESSELLATION			// use tessellation available on DX11+
//#define USE_HALF_PIXEL_OFFSET		// material doesn't need to do half pixel offset because it's done by D3D10 and above
#ifdef RED_PLATFORM_WINPC
#	define USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
#endif

#ifdef RED_PLATFORM_CONSOLE
#	define NO_TEXTURE_IMPORT
#endif
