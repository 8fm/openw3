/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "toolkit.h"
#include "floating_point.h"
#include <algorithm>
#include "embedded_shader.h"
#include <gnmx/shader_parser.h>
#include <sys/dmem.h>
using namespace sce;

void sce::Gnmx::Toolkit::setViewport(Gnm::DrawCommandBuffer *dcb, uint32_t viewportId, int32_t left, int32_t top, int32_t right, int32_t bottom, float dmin, float dmax)
{
	dcb->pushMarker("Toolkit::setViewport");
	const Vector3 scale( (right-left)*0.5f, (top-bottom)*0.5f, 0.5f );
	const Vector3 bias( left + (right-left)*0.5f, top + (bottom-top)*0.5f, 0.5f );
	dcb->setViewport(viewportId, dmin, dmax, (const float*)&scale, (const float*)&bias);
	dcb->popMarker();
}

namespace
{
	bool s_initialized = false;

	const uint32_t pix_clear_p[] = {
#include "pix_clear_p.h"
	};
	const uint32_t pix_dummy_p[] = {
#include "pix_dummy_p.h"
	};
	const uint32_t pix_generateMipMaps_2d_p[] = {
#include "pix_generateMipMaps_2d_p.h"
	};
	const uint32_t pix_generateMipMaps_3d_p[] = {
#include "pix_generateMipMaps_3d_p.h"
	};

	const uint32_t vex_clear_vv[] = {
#include "vex_clear_vv.h"
	};

	const uint32_t cs_set_uint_c[] = {
#include "cs_set_uint_c.h"
	};
	const uint32_t cs_set_uint_fast_c[] = {
#include "cs_set_uint_fast_c.h"
	};

	const uint32_t cs_copybuffer_c[] = {
#include "cs_copybuffer_c.h"
	};
	const uint32_t cs_copytexture1d_c[] = {
#include "cs_copytexture1d_c.h"
	};
	const uint32_t cs_copytexture2d_c[] = {
#include "cs_copytexture2d_c.h"
	};
	const uint32_t cs_copytexture3d_c[] = {
#include "cs_copytexture3d_c.h"
	};
	const uint32_t cs_copyrawtexture1d_c[] = {
#include "cs_copyrawtexture1d_c.h"
	};
	const uint32_t cs_copyrawtexture2d_c[] = {
#include "cs_copyrawtexture2d_c.h"
	};
	const uint32_t cs_copyrawtexture3d_c[] = {
#include "cs_copyrawtexture3d_c.h"
	};

	const uint32_t copytexture2d_p[] = {
#include "copytexture2d_p.h"
	};

	sce::Gnmx::Toolkit::EmbeddedCsShader s_set_uint = {cs_set_uint_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_set_uint_fast = {cs_set_uint_fast_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyBuffer = {cs_copybuffer_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyTexture1d = {cs_copytexture1d_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyTexture2d = {cs_copytexture2d_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyTexture3d = {cs_copytexture3d_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyRawTexture1d = {cs_copyrawtexture1d_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyRawTexture2d = {cs_copyrawtexture2d_c};
	sce::Gnmx::Toolkit::EmbeddedCsShader s_copyRawTexture3d = {cs_copyrawtexture3d_c};
	sce::Gnmx::Toolkit::EmbeddedPsShader s_pixel = {pix_clear_p};
	sce::Gnmx::Toolkit::EmbeddedPsShader s_dummy = {pix_dummy_p};
	sce::Gnmx::Toolkit::EmbeddedPsShader s_generateMipMaps_2d = {pix_generateMipMaps_2d_p};
	sce::Gnmx::Toolkit::EmbeddedPsShader s_generateMipMaps_3d = {pix_generateMipMaps_3d_p};
	sce::Gnmx::Toolkit::EmbeddedPsShader s_copyTexture2d_p = {copytexture2d_p};
	sce::Gnmx::Toolkit::EmbeddedVsShader s_vex_clear = {vex_clear_vv};

	sce::Gnmx::Toolkit::EmbeddedCsShader *s_embeddedCsShader[] =
	{
		&s_copyTexture1d,  	 &s_copyTexture2d,    &s_copyTexture3d,
		&s_copyRawTexture1d, &s_copyRawTexture2d, &s_copyRawTexture3d, 		
		&s_copyBuffer,		 &s_set_uint,         &s_set_uint_fast,
	};
	sce::Gnmx::Toolkit::EmbeddedPsShader *s_embeddedPsShader[] = {&s_pixel, &s_dummy, &s_copyTexture2d_p, &s_generateMipMaps_2d, &s_generateMipMaps_3d};
	sce::Gnmx::Toolkit::EmbeddedVsShader *s_embeddedVsShader[] = {&s_vex_clear};
	sce::Gnmx::Toolkit::EmbeddedShaders s_embeddedShaders =
	{
		s_embeddedCsShader, sizeof(s_embeddedCsShader)/sizeof(s_embeddedCsShader[0]),
		s_embeddedPsShader, sizeof(s_embeddedPsShader)/sizeof(s_embeddedPsShader[0]),
		s_embeddedVsShader, sizeof(s_embeddedVsShader)/sizeof(s_embeddedVsShader[0]),
	};
};



static bool isTileModeLinear(Gnm::TileMode tileMode)
{
	return (tileMode == Gnm::kTileModeDisplay_LinearAligned);
}

static bool tileModesAreSafeForMsaaResolve(Gnm::TileMode destMode, Gnm::TileMode srcMode)
{
	Gnm::MicroTileMode mtmSrc = Gnm::kMicroTileModeRotated, mtmDest = Gnm::kMicroTileModeRotated;
	if ( GpuAddress::kStatusSuccess != GpuAddress::getMicroTileMode(&mtmSrc,  srcMode) )
		return false;
	if ( GpuAddress::kStatusSuccess != GpuAddress::getMicroTileMode(&mtmDest, destMode) )
		return false;
	return mtmSrc == mtmDest && isTileModeLinear(destMode) == isTileModeLinear(srcMode);
}

void sce::Gnmx::Toolkit::SurfaceUtil::resolveMsaaBuffer(Gnmx::GfxContext &gfxc, Gnm::RenderTarget *destTarget, Gnm::RenderTarget *srcTarget)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_ASSERT(s_dummy.m_shader);
	SCE_GNM_VALIDATE(tileModesAreSafeForMsaaResolve(destTarget->getTileMode(), srcTarget->getTileMode()), "srcTarget and destTarget must both be tiled, or both be linear, and must have matching MicroTileModes (Display, Thin, etc.)");
	SCE_GNM_VALIDATE(!srcTarget->getFmaskCompressionEnable() || (srcTarget->getFmaskAddress() != NULL), "srcTarget (0x%p) uses FMASK compression, but its FMASK surface is NULL.", srcTarget);
	SCE_GNM_VALIDATE(s_dummy.m_shader->m_psStageRegisters.m_cbShaderMask == 0x0000000F, "pixel shader must only write to MRT0.");
	SCE_GNM_VALIDATE(srcTarget->getDataFormat().m_asInt == destTarget->getDataFormat().m_asInt, "srcTarget and destTarget must have the same DataFormat");
	SCE_GNM_VALIDATE(destTarget->getNumFragments() == Gnm::kNumFragments1, "destTarget->getNumFragments() must be kNumFragments1");
	SCE_GNM_VALIDATE(!destTarget->getCmaskFastClearEnable(), "destTarget CMASK fast clear must be disabled");

	gfxc.pushMarker("Toolkit::resolveMsaaBuffer");
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbPixelData);
	
	gfxc.setRenderTarget(0, srcTarget);
	gfxc.setRenderTarget(1, destTarget);
	gfxc.setDepthRenderTarget((Gnm::DepthRenderTarget*)NULL);
	gfxc.setPsShader(s_dummy.m_shader);

	Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc.setBlendControl(0, blendControl);
	gfxc.setRenderTargetMask(0x0000000F);

	Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(Gnm::kDepthControlZWriteDisable, Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(false);
	gfxc.setDepthStencilControl(dsc);
	gfxc.setupScreenViewport(0, 0, destTarget->getWidth(), destTarget->getHeight(), 0.5f, 0.5f);
	gfxc.setDepthStencilDisable();

	gfxc.setCbControl(Gnm::kCbModeResolve, Gnm::kRasterOpSrcCopy);
	renderFullScreenQuad(gfxc);
	gfxc.setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);
	// This post-resolve pixel flush is necessary in case the resolved surface will be bound as a texture input.
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbPixelData);

	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::fillDwordsWithCpu(uint32_t* address, uint32_t dwords, uint32_t val)
{
	std::fill(address, address+dwords, val);
}

void sce::Gnmx::Toolkit::SurfaceUtil::fillDwordsWithDma(Gnmx::GfxContext &gfxc, void *address, uint32_t dwords, uint32_t val)
{
	gfxc.fillData(address, val, dwords*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearMemoryToUints(sce::Gnmx::GfxContext &gfxc, void *destination, uint32_t destUints, uint32_t *source, uint32_t srcUints)
{
	gfxc.setShaderType(Gnm::kShaderTypeCompute);

	const bool srcUintsIsPowerOfTwo = (srcUints & (srcUints-1)) == 0;

	gfxc.setCsShader(srcUintsIsPowerOfTwo ? s_set_uint_fast.m_shader : s_set_uint.m_shader);

	Gnm::Buffer destinationBuffer;
	destinationBuffer.initAsDataBuffer(destination, Gnm::kDataFormatR32Uint, destUints);
	destinationBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeGC);
	gfxc.setRwBuffers(Gnm::kShaderStageCs, 0, 1, &destinationBuffer);

	Gnm::Buffer sourceBuffer;
	sourceBuffer.initAsDataBuffer(source, Gnm::kDataFormatR32Uint, srcUints);
	sourceBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);
	gfxc.setBuffers(Gnm::kShaderStageCs, 0, 1, &sourceBuffer);

	struct Constants
	{
		uint32_t m_destUints;
		uint32_t m_srcUints;
	};
	Constants *constants = (Constants*)gfxc.allocateFromCommandBuffer(sizeof(Constants), Gnm::kEmbeddedDataAlignment4);
	constants->m_destUints = destUints;
	constants->m_srcUints = srcUints - (srcUintsIsPowerOfTwo ? 1 : 0);
	Gnm::Buffer constantBuffer;
	constantBuffer.initAsConstantBuffer(constants, sizeof(*constants));
	gfxc.setConstantBuffers(Gnm::kShaderStageCs, 0, 1, &constantBuffer);

	gfxc.dispatch((destUints + Gnm::kThreadsPerWavefront - 1) / Gnm::kThreadsPerWavefront, 1, 1);

	Toolkit::synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);
}

void sce::Gnmx::Toolkit::SurfaceUtil::fillDwordsWithCompute(Gnmx::GfxContext &gfxc, void *address, uint32_t dwords, uint32_t val)
{
	uint32_t *source = static_cast<uint32_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint32_t), Gnm::kEmbeddedDataAlignment4));
	*source = val;
	clearMemoryToUints(gfxc, address, dwords, source, 1);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearBuffer(Gnmx::GfxContext &gfxc, const Gnm::Buffer* buffer, uint32_t *source, uint32_t sourceUints)
{
	clearMemoryToUints(gfxc, buffer->getBaseAddress(), buffer->getSize() / sizeof(uint32_t), source, sourceUints);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearBuffer(Gnmx::GfxContext &gfxc, const Gnm::Buffer* buffer, const Vector4 &vector)
{
	uint32_t *source = static_cast<uint32_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint32_t) * 4, Gnm::kEmbeddedDataAlignment4));
	uint32_t dwords = 0;
	Gnmx::Toolkit::dataFormatEncoder(source, &dwords, (Reg32*)&vector, buffer->getDataFormat());
	clearBuffer(gfxc, buffer, source, dwords);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearTexture(Gnmx::GfxContext &gfxc, const Gnm::Texture* texture, uint32_t *source, uint32_t sourceUints)
{
	uint64_t totalTiledSize = 0;
	Gnm::AlignmentType totalTiledAlign;
	int32_t status = GpuAddress::computeTotalTiledTextureSize(&totalTiledSize, &totalTiledAlign, texture);
	SCE_GNM_ASSERT(status == GpuAddress::kStatusSuccess);
	clearMemoryToUints(gfxc, texture->getBaseAddress(), totalTiledSize / sizeof(uint32_t), source, sourceUints);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearTexture(Gnmx::GfxContext &gfxc, const Gnm::Texture* texture, const Vector4 &color)
{
	uint32_t *source = static_cast<uint32_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint32_t) * 4, Gnm::kEmbeddedDataAlignment4));
	uint32_t dwords = 0;
	Gnmx::Toolkit::dataFormatEncoder(source, &dwords, (Reg32*)&color, texture->getDataFormat());
	clearTexture(gfxc, texture, source, dwords);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget* renderTarget, uint32_t *source, uint32_t sourceUints)
{
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	const uint32_t numSlices = renderTarget->getLastArraySliceIndex() - renderTarget->getBaseArraySliceIndex() + 1;
	clearMemoryToUints(gfxc, renderTarget->getBaseAddress(), renderTarget->getSliceSizeInBytes()*numSlices / sizeof(uint32_t), source, sourceUints);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget* renderTarget, const Vector4 &color)
{
	uint32_t *source = static_cast<uint32_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint32_t) * 4, Gnm::kEmbeddedDataAlignment4));
	uint32_t dwords = 0;
	Gnmx::Toolkit::dataFormatEncoder(source, &dwords, (Reg32*)&color, renderTarget->getDataFormat());
	clearRenderTarget(gfxc, renderTarget, source, dwords);
}

void sce::Gnmx::Toolkit::SurfaceUtil::copyBuffer(Gnmx::GfxContext &gfxc, const Gnm::Buffer* bufferDst, const Gnm::Buffer* bufferSrc)
{
	SCE_GNM_VALIDATE(bufferDst != 0 && bufferSrc != 0, "bufferDst and bufferSrc must not be NULL.");
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(bufferDst->getResourceMemoryType() != Gnm::kResourceMemoryTypeRO, "bufferDst can't have memory type Gnm::kResourceMemoryTypeRO");
	SCE_GNM_ASSERT(s_copyBuffer.m_shader);
	SCE_GNM_ASSERT(bufferDst->getNumElements() == bufferSrc->getNumElements());

	gfxc.setShaderType(Gnm::kShaderTypeCompute);

	gfxc.setBuffers(Gnm::kShaderStageCs, 0, 1, bufferSrc);
	gfxc.setRwBuffers(Gnm::kShaderStageCs, 0, 1, bufferDst);

	gfxc.setCsShader(s_copyBuffer.m_shader);
	gfxc.dispatch((bufferDst->getNumElements() + Gnm::kThreadsPerWavefront - 1) / Gnm::kThreadsPerWavefront, 1, 1);

	Toolkit::synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);
}

void sce::Gnmx::Toolkit::SurfaceUtil::copyTexture(Gnmx::GfxContext &gfxc, const Gnm::Texture* textureDst, const Gnm::Texture* textureSrc)
{
	SCE_GNM_VALIDATE(textureDst != 0 && textureSrc != 0, "textureDst and textureSrc must not be NULL.");
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(s_copyTexture2d.m_shader, "embedded copytexture2d shader not found.");
	SCE_GNM_VALIDATE(textureDst->getWidth()  == textureSrc->getWidth(), "source and destination texture widths do not match (dest=%d, source=%d).", textureDst->getWidth(), textureSrc->getWidth());
	SCE_GNM_VALIDATE(textureDst->getLastMipLevel() - textureDst->getBaseMipLevel() <= textureSrc->getLastMipLevel() - textureSrc->getBaseMipLevel(),
		"textureDst can not have more mip levels than textureSrc.");
	SCE_GNM_VALIDATE(textureDst->getLastArraySliceIndex() - textureDst->getBaseArraySliceIndex() <= textureSrc->getLastArraySliceIndex() - textureSrc->getBaseArraySliceIndex(),
		"textureDst can not have more array slices than textureSrc.");

	// If the data formats of the two textures are identical, we use a different shader that loads and stores the raw pixel bits directly, without any format conversion.
	// This not only preserves precision, but allows some additional tricks (such as copying otherwise-unwritable block-compressed formats by "spoofing" them as writable formats with identical
	// per-pixel sizes).
	bool copyRawPixels = (textureDst->getDataFormat().m_asInt == textureSrc->getDataFormat().m_asInt);

	Gnm::Texture textureDstCopy = *textureDst;
	Gnm::Texture textureSrcCopy = *textureSrc;

	if(copyRawPixels)
	{
		Gnm::DataFormat dataFormat = textureDstCopy.getDataFormat();
		switch(dataFormat.getSurfaceFormat())
		{
			case Gnm::kSurfaceFormatBc1: 
			case Gnm::kSurfaceFormatBc4:
				dataFormat.m_bits.m_channelType = Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = Gnm::kSurfaceFormat32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 3) / 4 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 3) / 4 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 3) / 4 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 3) / 4 - 1);
				break;
			case Gnm::kSurfaceFormatBc2: 
			case Gnm::kSurfaceFormatBc3:
			case Gnm::kSurfaceFormatBc5:
			case Gnm::kSurfaceFormatBc6:
			case Gnm::kSurfaceFormatBc7:
				dataFormat.m_bits.m_channelType = Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = Gnm::kSurfaceFormat32_32_32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 3) / 4 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 3) / 4 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 3) / 4 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 3) / 4 - 1);
				break;
			default:
				break;
		}
		textureDstCopy.setDataFormat(dataFormat);
		textureSrcCopy.setDataFormat(dataFormat);
	}

	Gnmx::CsShader *shader = 0;

	switch(textureDst->getTextureType())
	{
	case Gnm::kTextureType1d:
	case Gnm::kTextureType1dArray:
		SCE_GNM_VALIDATE(textureSrc->getTextureType() == Gnm::kTextureType1d || textureSrc->getTextureType() == Gnm::kTextureType1dArray,
			"textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType());
		shader = copyRawPixels ? s_copyRawTexture1d.m_shader : s_copyTexture1d.m_shader;
		break;
	case Gnm::kTextureTypeCubemap:
		// Spoof the cubemap textures as 2D texture arrays.
		textureDstCopy.initAs2dArray(textureDstCopy.getWidth(), textureDstCopy.getHeight(), textureDstCopy.getLastArraySliceIndex()+1, textureDstCopy.getLastMipLevel()+1, textureDstCopy.getDataFormat(), textureDstCopy.getTileMode(), textureDstCopy.getNumFragments(), false);
		textureSrcCopy.initAs2dArray(textureSrcCopy.getWidth(), textureSrcCopy.getHeight(), textureSrcCopy.getLastArraySliceIndex()+1, textureSrcCopy.getLastMipLevel()+1, textureSrcCopy.getDataFormat(), textureSrcCopy.getTileMode(), textureSrcCopy.getNumFragments(), false);
		textureDstCopy.setBaseAddress(textureDst->getBaseAddress());
		textureSrcCopy.setBaseAddress(textureSrc->getBaseAddress());
		// Intentional fall-through
	case Gnm::kTextureType2d:
	case Gnm::kTextureType2dArray:
		SCE_GNM_VALIDATE(textureDst->getHeight() == textureSrc->getHeight(), "source and destination texture heights do not match (dest=%d, source=%d).", textureDst->getHeight(), textureSrc->getHeight());
		SCE_GNM_VALIDATE(textureSrc->getTextureType() == Gnm::kTextureType2d || textureSrc->getTextureType() == Gnm::kTextureType2dArray || textureSrc->getTextureType() == Gnm::kTextureTypeCubemap,
			"textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType());
		shader = copyRawPixels ? s_copyRawTexture2d.m_shader : s_copyTexture2d.m_shader;
		break;
	case Gnm::kTextureType3d:
		SCE_GNM_VALIDATE(textureDst->getHeight() == textureSrc->getHeight(), "source and destination texture heights do not match (dest=%d, source=%d).", textureDst->getHeight(), textureSrc->getHeight());
		SCE_GNM_VALIDATE(textureDst->getDepth() == textureSrc->getDepth(), "source and destination texture depths do not match (dest=%d, source=%d).", textureDst->getDepth(), textureSrc->getDepth());
		SCE_GNM_VALIDATE(textureSrc->getTextureType() == Gnm::kTextureType3d,
			"textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X).", textureDst->getTextureType(), textureSrc->getTextureType());
		shader = copyRawPixels ? s_copyRawTexture3d.m_shader : s_copyTexture3d.m_shader;
		break;
	default:
		break; // unsupported texture type -- handled below
	}
	if(shader == 0)
	{
		SCE_GNM_ERROR("textureDst's dimensionality (0x%02X) is not supported by this function.", textureDst->getTextureType());
		return;
	}

	gfxc.setShaderType(Gnm::kShaderTypeCompute);
	gfxc.setCsShader(shader);

	textureDstCopy.setResourceMemoryType(Gnm::kResourceMemoryTypeGC); // The destination texture is GPU-coherent, because we will write to it.
	textureSrcCopy.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // The source texture is read-only, because we'll only ever read from it.

	const uint32_t oldDstMipBase   = textureDstCopy.getBaseMipLevel();
	const uint32_t oldDstMipLast   = textureDstCopy.getLastMipLevel();
	const uint32_t oldDstSliceBase = textureDstCopy.getBaseArraySliceIndex();
	const uint32_t oldDstSliceLast = textureDstCopy.getLastArraySliceIndex();
	for(uint32_t iMip=oldDstMipBase; iMip <= oldDstMipLast; ++iMip)
	{
		textureSrcCopy.setMipLevelRange(iMip, iMip);
		textureDstCopy.setMipLevelRange(iMip, iMip);
		const uint32_t mipWidth  = std::max(textureDstCopy.getWidth() >> iMip, 1U);
		const uint32_t mipHeight = std::max(textureDstCopy.getHeight() >> iMip, 1U);
		const uint32_t mipDepth  = std::max(textureDstCopy.getDepth() >> iMip, 1U);
		for(uint32_t iSlice=oldDstSliceBase; iSlice <= oldDstSliceLast; ++iSlice)
		{
			textureSrcCopy.setArrayView(iSlice, iSlice);
			textureDstCopy.setArrayView(iSlice, iSlice);

			gfxc.setTextures( Gnm::kShaderStageCs, 0, 1, &textureSrcCopy );
			gfxc.setRwTextures( Gnm::kShaderStageCs, 0, 1, &textureDstCopy );

			switch(textureDstCopy.getTextureType())
			{
			case Gnm::kTextureType1d:
			case Gnm::kTextureType1dArray:
				gfxc.dispatch( (mipWidth+63)/64, 1, 1);
				break;
			case Gnm::kTextureTypeCubemap:
			case Gnm::kTextureType2d:
			case Gnm::kTextureType2dArray:
				gfxc.dispatch( (mipWidth+7)/8, (mipHeight+7)/8, 1);
				break;
			case Gnm::kTextureType3d:
				gfxc.dispatch( (mipWidth+3)/4, (mipHeight+3)/4, (mipDepth+3)/4 );
				break;
			default:
				SCE_GNM_ASSERT(0); // This path should have been caught in the previous switch statement
				return;
			}
		}
	}

	Toolkit::synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);
}

void sce::Gnmx::Toolkit::SurfaceUtil::copyTextureToRenderTarget(sce::Gnmx::GfxContext &gfxc, const sce::Gnm::RenderTarget* renderTargetDestination, const sce::Gnm::Texture* textureSource)
{
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);
	gfxc.setRenderTarget(0, renderTargetDestination);
	Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc.setBlendControl(0, blendControl);
	gfxc.setRenderTargetMask(0x0000000F); // enable MRT0 output only
	gfxc.setDepthStencilDisable();
	gfxc.setupScreenViewport(0, 0, renderTargetDestination->getWidth(), renderTargetDestination->getHeight(), 0.5f, 0.5f);
	gfxc.setPsShader(s_copyTexture2d_p.m_shader);
	gfxc.setTextures(Gnm::kShaderStagePs, 0, 1, textureSource);
	renderFullScreenQuad(gfxc);
}

void sce::Gnmx::Toolkit::SurfaceUtil::copyRenderTarget(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget* renderTargetDst, const Gnm::RenderTarget* renderTargetSrc)
{
	SCE_GNM_ASSERT(renderTargetSrc->getWidth() == renderTargetDst->getWidth() && renderTargetSrc->getHeight() == renderTargetDst->getHeight());
	Gnm::Texture textureSrc;
	textureSrc.initFromRenderTarget(renderTargetSrc, false); // TODO: we sure this isn't a cubemap?
	textureSrc.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // We aren't going to do anything to this texture, except read from it. So let's mark it as read-only.
	copyTextureToRenderTarget(gfxc, renderTargetDst, &textureSrc);
}

void sce::Gnmx::Toolkit::SurfaceUtil::copyDepthTargetZ(sce::Gnmx::GfxContext &gfxc, const sce::Gnm::DepthRenderTarget* depthTargetDst, const sce::Gnm::DepthRenderTarget* depthTargetSrc)
{
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t dstNumSlices = depthTargetDst->getLastArraySliceIndex() - depthTargetDst->getBaseArraySliceIndex() + 1;
	uint32_t srcNumSlices = depthTargetSrc->getLastArraySliceIndex() - depthTargetSrc->getBaseArraySliceIndex() + 1;
	SCE_GNM_ASSERT(dstNumSlices >= srcNumSlices);
	SCE_GNM_ASSERT(depthTargetSrc->getZFormat() == depthTargetDst->getZFormat() && depthTargetSrc->getZSliceSizeInBytes() == depthTargetDst->getZSliceSizeInBytes() && !(depthTargetDst->getZSliceSizeInBytes() & 0xFF));
	synchronizeDepthRenderTargetZGraphicsToCompute(&gfxc.m_dcb, depthTargetDst);
	synchronizeDepthRenderTargetZGraphicsToCompute(&gfxc.m_dcb, depthTargetSrc);
	Gnm::Buffer bufferDst, bufferSrc;
	bufferDst.initAsDataBuffer(depthTargetDst->getZWriteAddress(), Gnm::kDataFormatR32G32B32A32Uint, (depthTargetDst->getZSliceSizeInBytes()*dstNumSlices+15)/16);
	bufferDst.setResourceMemoryType(Gnm::kResourceMemoryTypeGC); // we'll bind this as RWTexture, so it's GPU coherent.
	bufferSrc.initAsDataBuffer(depthTargetSrc->getZReadAddress(),  Gnm::kDataFormatR32G32B32A32Uint, (depthTargetSrc->getZSliceSizeInBytes()*srcNumSlices+15)/16);
	bufferSrc.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // we won't bind this as RWTexture, so read-only is OK
	copyBuffer(gfxc, &bufferDst, &bufferSrc);
}

void sce::Gnmx::Toolkit::addToMemoryRequests(sce::Gnmx::Toolkit::MemoryRequests *memoryRequests)
{
	s_embeddedShaders.addToMemoryRequests(memoryRequests);
}

void sce::Gnmx::Toolkit::initializeWithMemoryRequests(sce::Gnmx::Toolkit::MemoryRequests *memoryRequests)
{
	s_embeddedShaders.initializeWithMemoryRequests(memoryRequests);
	s_initialized = true;
}

Gnm::SizeAlign sce::Gnmx::Toolkit::SurfaceUtil::calculateRequiredBufferSizeAlign()
{
	Gnm::SizeAlign result;
	result.m_align = Gnm::kAlignmentOfShaderInBytes;
	result.m_size = s_embeddedShaders.addToRequiredSize(0);
	return result;
}

void sce::Gnmx::Toolkit::SurfaceUtil::initialize(void *addr)
{
	SCE_GNM_VALIDATE(!s_initialized, "SurfaceUtil has already been initialized.");
	SCE_GNM_VALIDATE((uintptr_t(addr) & (Gnm::kAlignmentOfShaderInBytes-1)) == 0, "addr (0x%p) must be aligned to a %d-byte boundary.", addr, Gnm::kAlignmentOfShaderInBytes);
	addr = s_embeddedShaders.initialize(addr);
	s_initialized = true;
}

void sce::Gnmx::Toolkit::SurfaceUtil::renderFullScreenQuad(Gnmx::GfxContext &gfxc)
{
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);
	gfxc.setVsShader(s_vex_clear.m_shader, 0, (void*)0);
	gfxc.setPrimitiveType(Gnm::kPrimitiveTypeRectList);
	gfxc.drawIndexAuto(3);
}

void sce::Gnmx::Toolkit::SurfaceUtil::decompressDepthSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(depthTarget, "depthTarget must not be NULL.");
	if (!depthTarget->getHtileAccelerationEnable())
		return; // If depth surface isn't decompressed, there's nothing to do.
	SCE_GNM_VALIDATE(depthTarget->getHtileAddress() != NULL, "Cannot decompress a depth surface with no HTILE buffer.");
	gfxc.pushMarker("Toolkit::SurfaceUtil::decompressDepthSurface");
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	volatile uint32_t *cbLabel = (uint32_t*)gfxc.allocateFromCommandBuffer(sizeof(uint32_t), Gnm::kEmbeddedDataAlignment8);
	*cbLabel = 0;

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta); // kEventDbCacheFlushAndInvalidate);

	gfxc.setDepthRenderTarget(depthTarget);
	// Z_ENABLE = 0, STENCIL_ENABLE = 0
	gfxc.setDepthStencilDisable();
	// DEPTH_COMPRESS_DISABLE = 1, STENCIL_COMPRESS_DISABLE = 1
	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setStencilTileWriteBackPolicy(Gnm::kDbTileWriteBackPolicyCompressionForbidden);
	dbRenderControl.setDepthTileWriteBackPolicy(Gnm::kDbTileWriteBackPolicyCompressionForbidden);
	gfxc.setDbRenderControl(dbRenderControl);
	// DB_RENDER_OVERRIDE.DECOMPRESS_Z_ON_FLUSH = 0
	gfxc.setCbControl(Gnm::kCbModeDisable, Gnm::kRasterOpSrcCopy);
	gfxc.setupScreenViewport(0, 0, depthTarget->getWidth(), depthTarget->getHeight(), 0.5f, 0.5f);

	// draw full screen quad
	gfxc.setPsShader((Gnmx::PsShader*)NULL);
	renderFullScreenQuad(gfxc);

	// Wait for the draw to be finished, and flush the Cb/Db caches:
	gfxc.writeImmediateDwordAtEndOfPipe(Gnm::kEopFlushCbDbCaches, (void*)cbLabel, 1, Gnm::kCacheActionNone);
	gfxc.waitOnAddress((void*)cbLabel, 0xFFFFFFFF, Gnm::kWaitCompareFuncEqual, 1);

	// Restore CB mode to normal
	gfxc.setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);

	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::decompressFmaskSurface(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget *target)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(target, "target must not be NULL.");
	if (!target->getFmaskCompressionEnable())
		return; // nothing to do.
	SCE_GNM_VALIDATE(target->getFmaskAddress() != NULL, "Compressed surface must have an FMASK surface");
	gfxc.pushMarker("Toolkit::SurfaceUtil::decompressFmaskSurface");
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	gfxc.setRenderTarget(0, target);
	//gfxc.setDepthRenderTarget((Gnm::DepthRenderTarget *)NULL);

	// Validation
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta); // FLUSH_AND_INV_CB_META		// Flush the FMask cache

	Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc.setBlendControl(0, blendControl);
	gfxc.setRenderTargetMask(0x0000000F); // enable MRT0 output only
	gfxc.setCbControl(Gnm::kCbModeFmaskDecompress, Gnm::kRasterOpSrcCopy);
	Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(Gnm::kDepthControlZWriteDisable, Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(true);
	gfxc.setDepthStencilControl(dsc);
	gfxc.setupScreenViewport(0, 0, target->getWidth(), target->getHeight(), 0.5f, 0.5f);

	// draw full screen quad using the provided color-clear shader.
	gfxc.setPsShader((Gnmx::PsShader*)NULL);
	renderFullScreenQuad(gfxc);

	// Restore CB mode to normal
	gfxc.setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);

	// Flush caches again
	if (target->getCmaskAddress() == NULL)
	{
		// Flush the CB color cache
		gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbPixelData); // FLUSH_AND_INV_CB_PIXEL_DATA
	}
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta); // FLUSH_AND_INV_CB_META
	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::eliminateFastClear(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget *target)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(target, "target must not be NULL.");
	if (!target->getCmaskFastClearEnable())
		return; // Nothing to do.
	SCE_GNM_VALIDATE(target->getCmaskAddress() != NULL, "Can not eliminate fast clear from a target with no CMASK.");
	gfxc.pushMarker("Toolkit::SurfaceUtil::eliminateFastClear");
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	volatile uint32_t *cbLabel = (uint32_t*)gfxc.allocateFromCommandBuffer(sizeof(uint32_t), Gnm::kEmbeddedDataAlignment8);
	*cbLabel = 0;

	// Needs to flush the CMask data
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);

	gfxc.setRenderTarget(0, target);
	//gfxc.setDepthRenderTarget((Gnm::DepthRenderTarget *)NULL);

	// Validation
	SCE_GNM_VALIDATE(target->getCmaskAddress() != NULL, "Must have a CMASK surface to eliminate fast clear data");

	Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc.setBlendControl(0, blendControl);
	gfxc.setRenderTargetMask(0x0000000F); // enable MRT0 output only
	gfxc.setCbControl(Gnm::kCbModeEliminateFastClear, Gnm::kRasterOpSrcCopy);
	Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(Gnm::kDepthControlZWriteDisable, Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(true);
	gfxc.setDepthStencilControl(dsc);
	gfxc.setupScreenViewport(0, 0, target->getWidth(), target->getHeight(), 0.5f, 0.5f);

	// draw full screen quad using the provided color-clear shader.
	gfxc.setPsShader((Gnmx::PsShader*)NULL);
	renderFullScreenQuad(gfxc);

	// Wait for the draw to be finished, and flush the Cb/Db caches:
	gfxc.writeImmediateDwordAtEndOfPipe(Gnm::kEopFlushCbDbCaches, (void*)cbLabel, 1, Gnm::kCacheActionNone);
	gfxc.waitOnAddress((void*)cbLabel, 0xFFFFFFFF, Gnm::kWaitCompareFuncEqual, 1);
	//gfxc.triggerEvent(Gnm::kEventCacheFlush); // FLUSH_AND_INV_CB_PIXEL_DATA

	// Restore CB mode to normal
	gfxc.setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);

	gfxc.popMarker();
}

static void clearDepthStencil(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_ASSERT(s_pixel.m_shader);
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	gfxc.setRenderTargetMask(0x0);
	gfxc.setDepthRenderTarget(depthTarget);

	Vector4Unaligned* constantBuffer = static_cast<Vector4Unaligned*>(gfxc.allocateFromCommandBuffer(sizeof(Vector4Unaligned), Gnm::kEmbeddedDataAlignment4));
	*constantBuffer = ToVector4Unaligned(Vector4(0.f, 0.f, 0.f, 0.f));
	Gnm::Buffer buffer;
	buffer.initAsConstantBuffer(constantBuffer, sizeof(Vector4Unaligned));
	buffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a constant buffer, so read-only is OK
	gfxc.setConstantBuffers(Gnm::kShaderStagePs, 0, 1, &buffer);

	const uint32_t width = depthTarget->getWidth();
	const uint32_t height = depthTarget->getHeight();
	gfxc.setupScreenViewport(0, 0, width, height, 0.5f, 0.5f);
	gfxc.setPsShader(s_pixel.m_shader); // Could be NULL in theory, but passing NULL causes driver hangs in Windows if the Gnm app loses focus.
	Gnmx::Toolkit::SurfaceUtil::renderFullScreenQuad(gfxc);

	gfxc.setRenderTargetMask(0xF);

	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	gfxc.setDbRenderControl(dbRenderControl);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearDepthStencilTarget(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const float depthValue, const uint8_t stencilValue)
{
	SCE_GNM_VALIDATE(depthTarget->getStencilReadAddress() != NULL, "Must have a stencil buffer to clear.");
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearDepthStencilTarget");

	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setDepthClearEnable(true);
	dbRenderControl.setStencilClearEnable(true);
	gfxc.setDbRenderControl(dbRenderControl);

	Gnm::DepthStencilControl depthControl;
	depthControl.init();
	depthControl.setDepthControl(Gnm::kDepthControlZWriteEnable, Gnm::kCompareFuncAlways);
	depthControl.setStencilFunction(Gnm::kCompareFuncAlways);
	depthControl.setDepthEnable(true);
	depthControl.setStencilEnable(true);
	gfxc.setDepthStencilControl(depthControl);

	Gnm::StencilOpControl stencilOpControl;
	stencilOpControl.init();
	stencilOpControl.setStencilOps(Gnm::kStencilOpReplaceTest, Gnm::kStencilOpReplaceTest, Gnm::kStencilOpReplaceTest);
	gfxc.setStencilOpControl(stencilOpControl);
	const Gnm::StencilControl stencilControl = {0xff, 0xff, 0xff, 0xff};
	gfxc.setStencil(stencilControl);

	gfxc.setDepthClearValue(depthValue);
	gfxc.setStencilClearValue(stencilValue);

	clearDepthStencil(gfxc, depthTarget);

	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearStencilTarget(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const uint8_t stencilValue)
{
	SCE_GNM_VALIDATE(depthTarget->getStencilReadAddress() != NULL, "Must have a stencil buffer to clear.");
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearStencilTarget");

	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setStencilClearEnable(true);
	gfxc.setDbRenderControl(dbRenderControl);

	Gnm::DepthStencilControl depthControl;
	depthControl.init();
	depthControl.setDepthControl(Gnm::kDepthControlZWriteDisable, Gnm::kCompareFuncAlways);
	depthControl.setStencilFunction(Gnm::kCompareFuncAlways);
	depthControl.setDepthEnable(true);
	depthControl.setStencilEnable(true);
	gfxc.setDepthStencilControl(depthControl);

	Gnm::StencilOpControl stencilOpControl;
	stencilOpControl.init();
	stencilOpControl.setStencilOps(Gnm::kStencilOpReplaceTest, Gnm::kStencilOpReplaceTest, Gnm::kStencilOpReplaceTest);
	gfxc.setStencilOpControl(stencilOpControl);
	const Gnm::StencilControl stencilControl = {0xff, 0xff, 0xff, 0xff};
	gfxc.setStencil(stencilControl);

	gfxc.setStencilClearValue(stencilValue);

	clearDepthStencil(gfxc, depthTarget);

	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearDepthTarget(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const float depthValue)
{
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearDepthTarget");

	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setDepthClearEnable(true);
	gfxc.setDbRenderControl(dbRenderControl);

	Gnm::DepthStencilControl depthControl;
	depthControl.init();
	depthControl.setDepthControl(Gnm::kDepthControlZWriteEnable, Gnm::kCompareFuncAlways);
	depthControl.setStencilFunction(Gnm::kCompareFuncNever);
	depthControl.setDepthEnable(true);
	gfxc.setDepthStencilControl(depthControl);

	gfxc.setDepthClearValue(depthValue);

	clearDepthStencil(gfxc, depthTarget);

	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::synchronizeRenderTargetGraphicsToCompute(sce::Gnm::DrawCommandBuffer *dcb, const sce::Gnm::RenderTarget* renderTarget)
{
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t numSlices = renderTarget->getLastArraySliceIndex() - renderTarget->getBaseArraySliceIndex() + 1;
	dcb->waitForGraphicsWrites(renderTarget->getBaseAddress256ByteBlocks(), (renderTarget->getSliceSizeInBytes()*numSlices)>>8,
		Gnm::kWaitTargetSlotCb0 | Gnm::kWaitTargetSlotCb1 | Gnm::kWaitTargetSlotCb2 | Gnm::kWaitTargetSlotCb3 |
		Gnm::kWaitTargetSlotCb4 | Gnm::kWaitTargetSlotCb5 | Gnm::kWaitTargetSlotCb6 | Gnm::kWaitTargetSlotCb7,
		Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, Gnm::kStallCommandBufferParserDisable);
}

void sce::Gnmx::Toolkit::synchronizeDepthRenderTargetZGraphicsToCompute(sce::Gnm::DrawCommandBuffer *dcb, const sce::Gnm::DepthRenderTarget* depthTarget)
{
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t numSlices = depthTarget->getLastArraySliceIndex() - depthTarget->getBaseArraySliceIndex() + 1;
	dcb->waitForGraphicsWrites(depthTarget->getZWriteAddress256ByteBlocks(), (depthTarget->getZSliceSizeInBytes()*numSlices)>>8,
		Gnm::kWaitTargetSlotDb,
		Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionFlushAndInvalidateDbCache, Gnm::kStallCommandBufferParserDisable);
}

void sce::Gnmx::Toolkit::synchronizeGraphicsToCompute( sce::Gnm::DrawCommandBuffer *dcb )
{
	volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer(sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	dcb->writeAtEndOfPipe(
		Gnm::kEopFlushCbDbCaches, 
		Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label),
		Gnm::kEventWriteSource64BitsImmediate, 0x1,
		Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kCachePolicyLru
	);
	dcb->waitOnAddress(const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1); // tell the CP to wait until the memory has the val 1
}

void sce::Gnmx::Toolkit::synchronizeComputeToGraphics( sce::Gnm::DrawCommandBuffer *dcb )
{
	volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer( sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	dcb->writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	dcb->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	dcb->flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$ and L2$
}

void sce::Gnmx::Toolkit::synchronizeComputeToCompute( sce::Gnm::DrawCommandBuffer *dcb )
{
	volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer( sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	dcb->writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	dcb->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	dcb->flushShaderCachesAndWait(Gnm::kCacheActionInvalidateL1, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$, because presumably the consumers of compute shader output may run on different CUs
}

void sce::Gnmx::Toolkit::synchronizeComputeToCompute( sce::Gnm::DispatchCommandBuffer *dcb )
{
	volatile uint64_t* label = (volatile uint64_t*)dcb->allocateFromCommandBuffer( sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	dcb->writeReleaseMemEvent(Gnm::kReleaseMemEventCsDone,
							  Gnm::kEventWriteDestMemory,  const_cast<uint64_t*>(label),
							  Gnm::kEventWriteSource64BitsImmediate, 0x1,
							  Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
	dcb->waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	dcb->flushShaderCachesAndWait(Gnm::kCacheActionInvalidateL1, 0); // tell the CP to flush the L1$, because presumably the consumers of compute shader output may run on different CUs
}
/*
void sce::Gnmx::Toolkit::SurfaceUtil::clearHtileSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const Gnmx::Toolkit::Htile htile)
{
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearHtileSurface");
	SCE_GNM_VALIDATE(depthTarget->getHtileAddress() != NULL, "depthTarget (0x%010llX) has no HTILE surface.", depthTarget);
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta);
	fillDwordsWithCompute(gfxc, depthTarget->getHtileAddress(), depthTarget->getHtileSizeInBytes()/sizeof(uint32_t), htile.m_asInt);
	synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.popMarker();
}
*/
void sce::Gnmx::Toolkit::SurfaceUtil::clearHtileSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const Gnm::Htile htile)
{
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearHtileSurface");
	SCE_GNM_VALIDATE(depthTarget->getHtileAddress() != NULL, "depthTarget (0x%p) has no HTILE surface.", depthTarget);
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta);
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t numSlices = depthTarget->getLastArraySliceIndex() - depthTarget->getBaseArraySliceIndex() + 1;
	fillDwordsWithCompute(gfxc, depthTarget->getHtileAddress(), depthTarget->getHtileSliceSizeInBytes()*numSlices/sizeof(uint32_t), htile.m_asInt);
	synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearHtileSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const float minZ, const float maxZ)
{
	SCE_GNM_VALIDATE(minZ >= 0.f && minZ <= 1.f, "minZ value of %f is not between 0 and 1", minZ);
	SCE_GNM_VALIDATE(maxZ >= 0.f && maxZ <= 1.f, "maxZ value of %f is not between 0 and 1", maxZ);
	Gnm::Htile htile = {};
	htile.m_hiZ.m_zMask = 0;
	htile.m_hiZ.m_minZ = static_cast<uint32_t>(floorf(minZ * Gnm::Htile::kMaximumZValue));
	htile.m_hiZ.m_maxZ = static_cast<uint32_t>(ceilf(maxZ * Gnm::Htile::kMaximumZValue));
	clearHtileSurface(gfxc, depthTarget, htile);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearHtileSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const float z)
{
	clearHtileSurface(gfxc, depthTarget, z, z);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearHtileSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget)
{
	clearHtileSurface(gfxc, depthTarget, 1.f);
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearCmaskSurface(Gnmx::GfxContext &gfxc, const Gnm::RenderTarget *target)
{
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearCmaskSurface");
	SCE_GNM_VALIDATE(target->getCmaskAddress() != NULL, "target (0x%p) has no CMASK surface.", target);
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t numSlices = target->getLastArraySliceIndex() - target->getBaseArraySliceIndex() + 1;
	fillDwordsWithCompute(gfxc, target->getCmaskAddress(), target->getCmaskSliceSizeInBytes()*numSlices/sizeof(uint32_t), 0);
	synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearStencilSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget, const uint8_t value)
{
	gfxc.pushMarker("Toolkit::SurfaceUtil::clearStencilSurface");
	SCE_GNM_VALIDATE(depthTarget->getStencilWriteAddress() != NULL, "depthTarget (0x%p) has no stencil surface.", depthTarget);
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta);
	uint32_t dword = value;
	dword = (dword << 8) | dword;
	dword = (dword << 8) | dword;
	dword = (dword << 8) | dword;
	// NOTE: this slice count is only valid if the array view hasn't changed since initialization!
	uint32_t numSlices = depthTarget->getLastArraySliceIndex() - depthTarget->getBaseArraySliceIndex() + 1;
	fillDwordsWithCompute(gfxc, depthTarget->getStencilWriteAddress(), depthTarget->getStencilSliceSizeInBytes()*numSlices/sizeof(uint32_t), dword);
	synchronizeComputeToGraphics(&gfxc.m_dcb);
	gfxc.popMarker();
}

void sce::Gnmx::Toolkit::SurfaceUtil::clearStencilSurface(Gnmx::GfxContext &gfxc, const Gnm::DepthRenderTarget *depthTarget)
{
	clearStencilSurface(gfxc, depthTarget, 0);
}

sce::Gnm::SizeAlign sce::Gnmx::Toolkit::Timers::getRequiredBufferSizeAlign(uint32_t timers)
{
	Gnm::SizeAlign result;
	result.m_align = sizeof(uint64_t); // timers are 64-bit
	result.m_size = timers * sizeof(Timer);
	return result;
}

void sce::Gnmx::Toolkit::Timers::initialize(void *addr, uint32_t timers)
{
	SCE_GNM_VALIDATE((uintptr_t(addr) % 8)==0, "addr (0x%p) must be 8-byte-aligned.", addr);
	m_addr = addr;
	m_timer = static_cast<Timer*>(addr);
	m_timers = timers;
}

void sce::Gnmx::Toolkit::Timers::begin(sce::Gnm::DrawCommandBuffer& dcb, uint32_t timer)
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	dcb.writeAtEndOfPipe(Gnm::kEopFlushCbDbCaches, Gnm::kEventWriteDestMemory, &m_timer[timer].m_begin, Gnm::kEventWriteSourceGpuCoreClockCounter, 0, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
}

void sce::Gnmx::Toolkit::Timers::end(sce::Gnm::DrawCommandBuffer& dcb, uint32_t timer)
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	dcb.writeAtEndOfPipe(Gnm::kEopFlushCbDbCaches, Gnm::kEventWriteDestMemory, &m_timer[timer].m_end, Gnm::kEventWriteSourceGpuCoreClockCounter, 0, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
}

void sce::Gnmx::Toolkit::Timers::begin(sce::Gnm::DispatchCommandBuffer& dcb, uint32_t timer)
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	dcb.writeReleaseMemEvent(Gnm::kReleaseMemEventFlushAndInvalidateCbDbCaches, Gnm::kEventWriteDestMemory, &m_timer[timer].m_begin, Gnm::kEventWriteSourceGpuCoreClockCounter, 0, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
}

void sce::Gnmx::Toolkit::Timers::end(sce::Gnm::DispatchCommandBuffer& dcb, uint32_t timer)
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	dcb.writeReleaseMemEvent(Gnm::kReleaseMemEventFlushAndInvalidateCbDbCaches, Gnm::kEventWriteDestMemory, &m_timer[timer].m_end, Gnm::kEventWriteSourceGpuCoreClockCounter, 0, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
}

uint64_t sce::Gnmx::Toolkit::Timers::readTimerInGpuClocks(uint32_t timer) const
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	uint64_t begin = m_timer[timer].m_begin & 0xFFFFFFFF;
	uint64_t end = m_timer[timer].m_end & 0xFFFFFFFF;
	if( end < begin )
		end += uint64_t(1) << 32;
	return end - begin;
}

double sce::Gnmx::Toolkit::Timers::readTimerInMilliseconds(uint32_t timer) const
{
	SCE_GNM_VALIDATE(timer < m_timers, "timer index (%d) must be less than m_timers (%d).", timer, m_timers);
	return 1000.0 * readTimerInGpuClocks(timer) / SCE_GNM_GPU_CORE_CLOCK_FREQUENCY;
}

int32_t sce::Gnmx::Toolkit::saveTextureToTga(const Gnm::Texture *texture, const char* filename)
{
	SCE_GNM_VALIDATE(texture, "texture must not be a null pointer");
	SCE_GNM_VALIDATE(texture->getDataFormat().m_asInt == Gnm::kDataFormatB8G8R8A8UnormSrgb.m_asInt, "texture format (%d) must be kDataFormatB8G8R8A8UnormSrgb", texture->getDataFormat().m_asInt);
	SCE_GNM_VALIDATE(texture->getTileMode() == Gnm::kTileModeDisplay_LinearAligned, "texture tilemode (%d) must be kTileModeDisplay_LinearAligned", texture->getTileMode());

	FILE *file = fopen(filename, "wb");
	if(file == 0)
		return -1;

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

	TargaHeader header;
    memset(&header,0, sizeof(header));
    header.imageType = 2;
    header.width = (uint16_t)texture->getPitch(); // for simplicity assume pitch as width
    header.height = (uint16_t)texture->getHeight();
    header.pixelDepth = (uint8_t) texture->getDataFormat().getBitsPerElement();
    header.imageDescriptor = 0x20; // y direction is reversed (from up to down)
	fwrite(&header, sizeof(header), 1, file);

	const void *pixels = texture->getBaseAddress();

	fwrite(pixels, header.width*texture->getHeight()*4, 1, file);

	fclose(file);
	return 0;
}

int32_t sce::Gnmx::Toolkit::saveTextureToPfm(const Gnm::Texture *texture, const uint8_t* pixels, const char *fileName)
{
	//uint32_t df = texture->getDataFormat().m_asInt;
	SCE_GNM_VALIDATE(texture != 0, "texture must not be NULL.");
	SCE_GNM_VALIDATE(pixels != 0, "pixels must not be NULL.");
	SCE_GNM_VALIDATE(texture->getDataFormat().m_asInt == Gnm::kDataFormatR32G32B32A32Float.m_asInt || texture->getDataFormat().m_asInt == Gnm::kDataFormatR32Float.m_asInt,
		"Texture data format must be RGBA32F or R32F.");
	SCE_GNM_VALIDATE(texture->getTileMode() == Gnm::kTileModeDisplay_LinearAligned, "texture tilemode (%d) must be kTileModeDisplay_LinearAligned", texture->getTileMode());

	const uint32_t width = texture->getWidth();
	const uint32_t height = texture->getHeight();
	const uint32_t pitch = texture->getPitch();

	uint8_t format = 0;
	uint32_t bytesPerPixelIn = 0;
	uint32_t bytesPerPixelOut = 0;
	if(texture->getDataFormat().m_asInt == Gnm::kDataFormatR32G32B32A32Float.m_asInt)
	{
		format = 'F';
		bytesPerPixelIn = sizeof(float)*4;
		bytesPerPixelOut = sizeof(float)*3;
	}
	else if(texture->getDataFormat().m_asInt == Gnm::kDataFormatR32Float.m_asInt)
	{
		format = 'f';
		bytesPerPixelIn = sizeof(float)*1;
		bytesPerPixelOut = sizeof(float)*1;
	}
	else
	{
		SCE_GNM_ASSERT( !"PFM file requires float1 or float3 format." );
	}

	FILE *file = fopen(fileName, "wb");
	SCE_GNM_ASSERT(file != 0 && "PFM file can not be opened for writing.");
	fprintf(file, "P%c\n%d %d\n-1.0\n", format, texture->getWidth(), texture->getHeight());
	for( uint32_t scanline=0; scanline<height; ++scanline )
	{
		uint32_t byteOffset = 0;
		for( uint32_t pixel=0; pixel<width; ++pixel )
		{
			const size_t bytesWritten = fwrite(pixels+byteOffset, sizeof(uint8_t), bytesPerPixelOut, file);
			SCE_GNM_ASSERT(bytesWritten == bytesPerPixelOut);
			byteOffset += bytesPerPixelIn;
		}
		pixels += pitch*bytesPerPixelIn;
	}

	fclose(file);
	return 0;
}

void sce::Gnmx::Toolkit::submitAndStall(sce::Gnm::DrawCommandBuffer& dcb)
{
	volatile uint64_t* label = static_cast<uint64_t*>(dcb.allocateFromCommandBuffer(sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8));
	*label = 0;
	dcb.writeAtEndOfPipe(Gnm::kEopFlushCbDbCaches, Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label), Gnm::kEventWriteSource64BitsImmediate, 1, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
	void *cbAddrGPU    = dcb.m_beginptr;
	void *ccbAddrGPU   = 0;
	uint32_t cbSizeInByte = static_cast<uint32_t>(dcb.m_cmdptr - dcb.m_beginptr)*4;
	uint32_t ccbSizeInByte = 0;
	int state = Gnm::submitCommandBuffers(1, &cbAddrGPU, &cbSizeInByte, &ccbAddrGPU, &ccbSizeInByte);
	SCE_GNM_ASSERT(state == sce::Gnm::kSubmissionSuccess);

	volatile uint32_t wait = 0;
	while( *label != 1 )
		wait++;
}

void sce::Gnmx::Toolkit::submitAndStall(sce::Gnmx::GfxContext& gfxc)
{
	volatile uint64_t* label = static_cast<uint64_t*>(gfxc.allocateFromCommandBuffer(sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8));
	*label = 0;
	// TODO: expose release mem / remove eop
	gfxc.writeImmediateAtEndOfPipe(Gnm::kEopFlushCbDbCaches, const_cast<uint64_t*>(label), 1, Gnm::kCacheActionNone);
	int32_t state = gfxc.submit();
	SCE_GNM_ASSERT(state == sce::Gnm::kSubmissionSuccess);
	volatile uint32_t wait = 0;
	while( *label != 1 )
		wait++;
}

//////////

static off_t s_tfOffset = 0;

void* sce::Gnmx::Toolkit::allocateTessellationFactorRingBuffer()
{
	if ( !s_tfOffset )
	{
		const uint32_t tfAlignment = 64 * 1024;
		int	ret = sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE,
												Gnm::kTfRingSizeInBytes, tfAlignment, 
												SCE_KERNEL_WC_GARLIC, &s_tfOffset);
	
		void *tfBufferPtr = sce::Gnm::getTessellationFactorRingBufferBaseAddress();
		if ( !ret )
		{
			ret = sceKernelMapDirectMemory(&tfBufferPtr,
										   Gnm::kTfRingSizeInBytes,
										   SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
										   0, //flags
										   s_tfOffset,
										   tfAlignment);
		}
		SCE_GNM_VALIDATE(!ret, "Allocation of the tessellation factor ring buffer failed!");
		SCE_GNM_VALIDATE(tfBufferPtr == sce::Gnm::getTessellationFactorRingBufferBaseAddress(), "Allocation of the tessellation factor ring buffer failed!");
	}

	return sce::Gnm::getTessellationFactorRingBufferBaseAddress();
}

void sce::Gnmx::Toolkit::deallocateTessellationFactorRingBuffer()
{
	if ( s_tfOffset )
	{
		sceKernelReleaseDirectMemory(s_tfOffset, Gnm::kTfRingSizeInBytes);
	}
	s_tfOffset = 0;
}

void sce::Gnmx::Toolkit::SurfaceUtil::generateMipMaps(sce::Gnmx::GfxContext &gfxc, const sce::Gnm::Texture *texture)
{
	SCE_GNM_VALIDATE(s_initialized, "Must call SurfaceUtil::initialize() before calling this function.");
	SCE_GNM_VALIDATE(texture != 0, "texture must not be NULL.");

	gfxc.pushMarker("Toolkit::SurfaceUtil::generateMipMaps");
	gfxc.setShaderType(Gnm::kShaderTypeGraphics);

	gfxc.setDepthRenderTarget((Gnm::DepthRenderTarget*)0);

	Gnm::BlendControl blendControl;
	blendControl.init();
	blendControl.setBlendEnable(false);
	gfxc.setBlendControl(0, blendControl);
	gfxc.setRenderTargetMask(0x0000000F);

	Gnm::DepthStencilControl dsc;
	dsc.init();
	dsc.setDepthControl(Gnm::kDepthControlZWriteDisable, Gnm::kCompareFuncAlways);
	dsc.setDepthEnable(false);
	gfxc.setDepthStencilControl(dsc);
	gfxc.setDepthStencilDisable();

	gfxc.setVsShader(s_vex_clear.m_shader, 0, (void*)0);

	gfxc.setPrimitiveType(Gnm::kPrimitiveTypeRectList);

	uint64_t textureSizeInBytes;
	Gnm::AlignmentType textureAlignment;
	GpuAddress::computeTotalTiledTextureSize(&textureSizeInBytes, &textureAlignment, texture);

	struct Constants
	{
		Vector4Unaligned m_mul;
		Vector4Unaligned m_add;
	};

	if(texture->getTextureType() == Gnm::kTextureType3d)
		gfxc.setPsShader(s_generateMipMaps_3d.m_shader);
	else
		gfxc.setPsShader(s_generateMipMaps_2d.m_shader);

	Gnm::Sampler trilinearSampler;
	trilinearSampler.init();
	trilinearSampler.setMipFilterMode(Gnm::kMipFilterModeLinear);
	trilinearSampler.setXyFilterMode(Gnm::kFilterModeBilinear, Gnm::kFilterModeBilinear);
	gfxc.setSamplers(Gnm::kShaderStagePs, 0, 1, &trilinearSampler);

	for(unsigned targetMip = texture->getBaseMipLevel() + 1; targetMip <= texture->getLastMipLevel(); ++targetMip)
	{
		const unsigned sourceMip = targetMip - 1;

		const float ooTargetWidth  = 1.f / std::max(1U, texture->getWidth()  >> targetMip);
		const float ooTargetHeight = 1.f / std::max(1U, texture->getHeight() >> targetMip);

		if(texture->getTextureType() == Gnm::kTextureType3d)
		{
			const unsigned targetDepths = std::max(1U, texture->getDepth() >> targetMip);

			const float ooTargetDepth  = 1.f / targetDepths;

			for(unsigned targetSlice = 0; targetSlice < targetDepths; ++targetSlice)
			{
				Gnm::Texture sourceTexture = *texture;

				Gnm::RenderTarget destinationRenderTarget;
				destinationRenderTarget.initFromTexture(&sourceTexture, targetMip);
				destinationRenderTarget.setArrayView(targetSlice, targetSlice);
				gfxc.setupScreenViewport(0, 0, destinationRenderTarget.getWidth(), destinationRenderTarget.getHeight(), 0.5f, 0.5f);
				gfxc.setRenderTarget(0, &destinationRenderTarget);

				sourceTexture.setMipLevelRange(sourceMip, sourceMip);
				gfxc.setTextures(Gnm::kShaderStagePs, 0, 1, &sourceTexture);

				Constants *constants = (Constants*)gfxc.allocateFromCommandBuffer(sizeof(Constants), Gnm::kEmbeddedDataAlignment4);
				constants->m_mul.x = ooTargetWidth;
				constants->m_mul.y = ooTargetHeight;
				constants->m_mul.z = 0.f;
				constants->m_add.x = 0.f;
				constants->m_add.y = 0.f;
				constants->m_add.z = (targetSlice + 0.5f) * ooTargetDepth;
				Gnm::Buffer constantBuffer;
				constantBuffer.initAsConstantBuffer(constants, sizeof(*constants));
				gfxc.setConstantBuffers(Gnm::kShaderStagePs, 0, 1, &constantBuffer);

				gfxc.drawIndexAuto(3);
			}
		}
		else
		{
			for(unsigned slice = texture->getBaseArraySliceIndex(); slice <= texture->getLastArraySliceIndex(); ++slice)
			{
				Gnm::Texture sourceTexture = *texture;
				if(sourceTexture.getTextureType() == Gnm::kTextureTypeCubemap) // When spoofing a cubemap as a 2D array, we also need to multiply the total slice count (stored in the Depth field) by 6.
				{			
					sourceTexture.setTextureType(Gnm::kTextureType2dArray);
					sourceTexture.setDepthMinus1(sourceTexture.getDepth()*6 - 1);
				}

				Gnm::RenderTarget destinationRenderTarget;
				destinationRenderTarget.initFromTexture(&sourceTexture, targetMip);
				destinationRenderTarget.setArrayView(slice, slice);
				gfxc.setupScreenViewport(0, 0, destinationRenderTarget.getWidth(), destinationRenderTarget.getHeight(), 0.5f, 0.5f);
				gfxc.setRenderTarget(0, &destinationRenderTarget);

				sourceTexture.setArrayView(slice, slice);
				sourceTexture.setMipLevelRange(sourceMip, sourceMip);
				gfxc.setTextures(Gnm::kShaderStagePs, 0, 1, &sourceTexture);

				Constants *constants = (Constants*)gfxc.allocateFromCommandBuffer(sizeof(Constants), Gnm::kEmbeddedDataAlignment4);
				constants->m_mul.x = ooTargetWidth;
				constants->m_mul.y = ooTargetHeight;
				constants->m_add.x = 0.f;
				constants->m_add.y = 0.f;
				Gnm::Buffer constantBuffer;
				constantBuffer.initAsConstantBuffer(constants, sizeof(*constants));
				gfxc.setConstantBuffers(Gnm::kShaderStagePs, 0, 1, &constantBuffer);

				gfxc.drawIndexAuto(3);
			}
		}

		// Since each slice is supposed to share no cache lines with any other slice, it should be safe to have no synchronization
		// between slices. But, since each mip reads from the previous mip, we must do synchronization between mips.

		gfxc.waitForGraphicsWrites(
			texture->getBaseAddress256ByteBlocks(), 
			(textureSizeInBytes + 255) / 256,
			Gnm::kWaitTargetSlotCb0, 
			Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
			Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, 
			Gnm::kStallCommandBufferParserDisable
		);
	}

	// unbind all we have bound

	gfxc.setTextures(Gnm::kShaderStagePs, 0, 1, 0);
	gfxc.setRenderTarget(0, 0);
	gfxc.setSamplers(Gnm::kShaderStagePs, 0, 1, 0);
	gfxc.setPsShader(0);
	gfxc.setVsShader(0, 0, (void*)0);
}
