/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// All API specific stuff should be included here

#include <gnm.h>
#include "redgnmx.h"
#include <gpu_address.h>
#include "redgnmx/shader_parser.h"
#include <shader/binary.h>
#include <video_out.h>
#include <texture_tool.h>

#define GPU_API_DEBUG_PATH

namespace GpuApi
{
	namespace Hacks
	{
		sce::Gnmx::GfxContext& GetGfxContext();
		sce::Gnm::RenderTarget& GetRenderTarget();
		sce::Gnm::DepthRenderTarget& GetDepthStencil();
		sce::Gnm::RenderTarget* GetTextureRTV( const TextureRef& ref );
		sce::Gnm::Texture* GetTextureSRV( const TextureRef& ref );
	}
}