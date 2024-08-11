/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


namespace GpuApi
{

	//inline DXGI_FORMAT Map( eIndexFormat format )
	//{
	//	switch ( format )
	//	{
	//		case INDEXFMT_UShort:	return DXGI_FORMAT_R16_UINT;
	//		case INDEXFMT_UInt:		return DXGI_FORMAT_R32_UINT;
	//		default:				GPUAPI_HALT( TXT( "invalid" ) ); return DXGI_FORMAT_R32_UINT;
	//	}
	//}

	// This is used by TexCookXboxOne. If you change it and expect to do any cooking, you should rebuild the tool in /dev/internal/TexCookTool.
	inline sce::Gnm::DataFormat Map( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return sce::Gnm::kDataFormatA8Unorm;
		case TEXFMT_A8_Scaleform:			return sce::Gnm::kDataFormatA8Unorm;
		case TEXFMT_L8:						return sce::Gnm::kDataFormatL8Unorm;
		case TEXFMT_R8_Uint:				return sce::Gnm::kDataFormatR8Uint;
		case TEXFMT_R8G8B8A8:				return sce::Gnm::kDataFormatR8G8B8A8Unorm;
		case TEXFMT_R8G8B8X8:				return sce::Gnm::kDataFormatR8G8B8A8Unorm;
		case TEXFMT_Uint_16_norm:			return sce::Gnm::kDataFormatR16Unorm;
		case TEXFMT_Uint_16:				return sce::Gnm::kDataFormatR16Uint;
		case TEXFMT_Uint_32:				return sce::Gnm::kDataFormatR32Uint;
		case TEXFMT_R16G16_Uint:			return sce::Gnm::kDataFormatR16G16Uint;
		case TEXFMT_R32G32_Uint:			return sce::Gnm::kDataFormatR32G32Uint;
		case TEXFMT_Float_R10G10B10A2:		return sce::Gnm::kDataFormatR10G10B10A2Unorm;
		case TEXFMT_Float_R16G16B16A16:		return sce::Gnm::kDataFormatR16G16B16A16Float;
		case TEXFMT_Float_R11G11B10:		return sce::Gnm::kDataFormatR11G11B10Float;
		case TEXFMT_Float_R16G16:			return sce::Gnm::kDataFormatR16G16Float;
		case TEXFMT_Float_R32G32:			return sce::Gnm::kDataFormatR32G32Float;
		case TEXFMT_Float_R32G32B32A32:		return sce::Gnm::kDataFormatR32G32B32A32Float;
		case TEXFMT_Float_R32:				return sce::Gnm::kDataFormatR32Float;
		case TEXFMT_Float_R16:				return sce::Gnm::kDataFormatR16Float;
		case TEXFMT_BC1:					return sce::Gnm::kDataFormatBc1Unorm;
		case TEXFMT_BC2:					return sce::Gnm::kDataFormatBc2Unorm;
		case TEXFMT_BC3:					return sce::Gnm::kDataFormatBc3Unorm;
		case TEXFMT_BC4:					return sce::Gnm::kDataFormatBc4Unorm;
		case TEXFMT_BC5:					return sce::Gnm::kDataFormatBc5Unorm;
		case TEXFMT_BC6H:					return sce::Gnm::kDataFormatBc6Uf16;
		case TEXFMT_BC7:					return sce::Gnm::kDataFormatBc7Unorm;
		case TEXFMT_D24S8:					return sce::Gnm::DataFormat::build( sce::Gnm::kZFormat32Float); // HACK GNM?
		case TEXFMT_A8L8:					return sce::Gnm::kDataFormatL8A8Unorm;
		case TEXFMT_D32F:					return sce::Gnm::DataFormat::build( sce::Gnm::kZFormat32Float);
		case TEXFMT_D16U:					return sce::Gnm::DataFormat::build( sce::Gnm::kZFormat16 );
		default:							GPUAPI_HALT( "invalid or not available" ); return sce::Gnm::kDataFormatInvalid;
		}
	}

	inline sce::Gnmx::ShaderType Map( eShaderType shaderType )
	{
		switch ( shaderType )
		{
		case VertexShader:		return sce::Gnmx::kVertexShader;
		case PixelShader:		return sce::Gnmx::kPixelShader;
		case GeometryShader:	return sce::Gnmx::kGeometryShader;
		case HullShader:		return sce::Gnmx::kHullShader;
		case DomainShader:		return sce::Gnmx::kExportShader;
		case ComputeShader:		return sce::Gnmx::kComputeShader;
		default:				GPUAPI_HALT( "invalid" ); return sce::Gnmx::kInvalidShader;
		}
	}

	inline sce::Gnm::ShaderStage MapToShaderStage( eShaderType shaderType )
	{
		// TODO: This will need rewriting to fit proper hardware stages. For now just make it return at least valid stages, so APIs properly.
		switch ( shaderType )
		{
		case VertexShader:		return sce::Gnm::kShaderStageVs;
		case PixelShader:		return sce::Gnm::kShaderStagePs;
		case ComputeShader:		return sce::Gnm::kShaderStageCs;
		case GeometryShader:	return sce::Gnm::kShaderStageEs;
		case HullShader:		return sce::Gnm::kShaderStageHs;
		case DomainShader:		return sce::Gnm::kShaderStageVs;
		default:				GPUAPI_HALT( "invalid" ); return sce::Gnm::kShaderStageCount;
		}
	}

	inline sce::Gnm::ShaderStage MapShaderTypeToShaderStage( sce::Gnmx::ShaderType shaderType )
	{
		switch ( shaderType )
		{
		case sce::Gnmx::kVertexShader:			return sce::Gnm::kShaderStageVs;
		case sce::Gnmx::kPixelShader:			return sce::Gnm::kShaderStagePs;
		case sce::Gnmx::kGeometryShader:		return sce::Gnm::kShaderStageGs;
		case sce::Gnmx::kComputeShader:			return sce::Gnm::kShaderStageCs;
		case sce::Gnmx::kExportShader:			return sce::Gnm::kShaderStageEs;
		case sce::Gnmx::kLocalShader:			return sce::Gnm::kShaderStageLs;
		case sce::Gnmx::kHullShader:			return sce::Gnm::kShaderStageHs;
		case sce::Gnmx::kComputeVertexShader:	return sce::Gnm::kShaderStageVs;
		default:				GPUAPI_HALT( "invalid" ); return sce::Gnm::kShaderStageCount;
		}
	}

	inline sce::Gnm::PrimitiveType Map( ePrimitiveType primitive )
	{
		switch ( primitive )
		{
		case PRIMTYPE_LineList:			return sce::Gnm::kPrimitiveTypeLineList;
		case PRIMTYPE_LineStrip:		return sce::Gnm::kPrimitiveTypeLineStrip;
		case PRIMTYPE_TriangleList:		return sce::Gnm::kPrimitiveTypeTriList;
		case PRIMTYPE_TriangleStrip:	return sce::Gnm::kPrimitiveTypeTriStrip;
		case PRIMTYPE_PointList:		return sce::Gnm::kPrimitiveTypePointList;
		case PRIMTYPE_QuadList:			return sce::Gnm::kPrimitiveTypeRectList;

		case PRIMTYPE_1CP_PATCHLIST:	return sce::Gnm::kPrimitiveTypePatch;
		case PRIMTYPE_3CP_PATCHLIST:	return sce::Gnm::kPrimitiveTypePatch;
		case PRIMTYPE_4CP_PATCHLIST:	return sce::Gnm::kPrimitiveTypePatch;

		default:						GPUAPI_HALT( "invalid" ); return sce::Gnm::kPrimitiveTypeNone;
		}
	}

	inline sce::Gnm::WrapMode Map( eTextureAddress address )
	{
		switch ( address )
		{
		case TEXADDR_Wrap:			return sce::Gnm::kWrapModeWrap;
		case TEXADDR_Mirror:		return sce::Gnm::kWrapModeMirror;
		case TEXADDR_Clamp:			return sce::Gnm::kWrapModeClampLastTexel; // this might not be the same, check documentation if kWrapModeClampHalfBorder or kWrapModeClampBorder should be used instead
		case TEXADDR_MirrorOnce:	return sce::Gnm::kWrapModeMirrorOnceLastTexel; // this might not be the same, check documentation if kWrapModeMirrorOnceHalfBorder or kWrapModeMirrorOnceBorder should be used instead
		case TEXADDR_Border:		return sce::Gnm::kWrapModeClampBorder;
		default:					GPUAPI_HALT( "invalid" ); return sce::Gnm::kWrapModeClampLastTexel;
		}
	}

	inline sce::Gnm::FilterMode Map( eTextureFilterMag filterMag )
	{
		switch ( filterMag )
		{
		case TEXFILTERMAG_Linear:	return sce::Gnm::kFilterModeBilinear;
		case TEXFILTERMAG_Point:	return sce::Gnm::kFilterModePoint;
		default:					GPUAPI_HALT( "invalid" ); return sce::Gnm::kFilterModePoint;
		}
	}

	inline sce::Gnm::FilterMode Map( eTextureFilterMin filterMin )
	{
		switch ( filterMin )
		{
		case TEXFILTERMIN_Point:	return sce::Gnm::kFilterModePoint;
		case TEXFILTERMIN_Linear:	return sce::Gnm::kFilterModeBilinear;
		case TEXFILTERMIN_Aniso:	// falldown
		case TEXFILTERMIN_AnisoLow:	return sce::Gnm::kFilterModeAnisoBilinear;
		default:					GPUAPI_HALT( "invalid" ); return sce::Gnm::kFilterModePoint;
		}
	}

	inline sce::Gnm::MipFilterMode Map( eTextureFilterMip filterMip )
	{
		switch ( filterMip )
		{
		case TEXFILTERMIP_None:		return sce::Gnm::kMipFilterModeNone;
		case TEXFILTERMIP_Point:	return sce::Gnm::kMipFilterModePoint;
		case TEXFILTERMIP_Linear:	return sce::Gnm::kMipFilterModeLinear;
		default:					GPUAPI_HALT( "invalid" ); return sce::Gnm::kMipFilterModeNone;
		}
	}

	inline sce::Gnm::DepthCompare Map( eComparison comparison )
	{
		switch ( comparison )
		{
			case COMP_None:			return sce::Gnm::kDepthCompareAlways;
			case COMP_Less:			return sce::Gnm::kDepthCompareLess;
			default:				GPUAPI_HALT( "invalid" ); return sce::Gnm::kDepthCompareLess;
		}
	}

	//inline D3D11_VIEWPORT Map( const ViewportDesc &viewport )
	//{
	//	D3D11_VIEWPORT d3dViewport;
	//	d3dViewport.TopLeftX = (Float)viewport.x;
	//	d3dViewport.TopLeftY = (Float)viewport.y;
	//	d3dViewport.Width = (Float)viewport.width;
	//	d3dViewport.Height = (Float)viewport.height;
	//	d3dViewport.MinDepth = viewport.minZ;
	//	d3dViewport.MaxDepth = viewport.maxZ;

	//	return d3dViewport;
	//}

	// all locking has to be handled manually in gnm? pretty crazy
	//inline Uint32 MapBuffLockFlagsToD3D( Uint32 flags )
	//{
	//	Uint32 result = 0;
	//	result |= (BLF_Discard		& flags) ? D3D11_MAP_WRITE_DISCARD		: 0;
	//	result |= (BLF_NoOverwrite	& flags) ? D3D11_MAP_WRITE_NO_OVERWRITE	: 0;
	//	result |= (BLF_NoSysLock	& flags) ? 0							: 0;
	//	result |= (BLF_Readonly		& flags) ? D3D11_MAP_READ				: 0;
	//	result |= (BLF_Write		& flags) ? D3D11_MAP_WRITE				: 0;
	//	return result;
	//}

	// Get D3D semantic and index from a given PackingElement. If invalid element usage, outputs are unmodified and return false.
	inline Bool MapPackingElementToSemanticAndIndex( const VertexPacking::PackingElement& element, const char*& outSemantic, Uint32& outIndex )
	{
		const char* semanticName = NULL;
		Uint32 semanticIndex = element.m_usageIndex;

		switch ( element.m_usage )
		{
		case VertexPacking::PS_SysPosition:			semanticName = "S_POSITION"; break;
		case VertexPacking::PS_Position:			semanticName = "POSITION"; break;
		case VertexPacking::PS_Normal:				semanticName = "NORMAL"; break;
		case VertexPacking::PS_Tangent:				semanticName = "TANGENT"; break;
		case VertexPacking::PS_Binormal:			semanticName = "BINORMAL"; break;
		case VertexPacking::PS_TexCoord:			semanticName = "TEXCOORD"; break;
		case VertexPacking::PS_Color:				semanticName = "COLOR"; break;
		case VertexPacking::PS_SkinIndices:			semanticName = "BLENDINDICES"; break;
		case VertexPacking::PS_SkinWeights:			semanticName = "BLENDWEIGHT"; break;
		case VertexPacking::PS_InstanceTransform:	semanticName = "INSTANCE_TRANSFORM"; break;
		case VertexPacking::PS_InstanceLODParams:	semanticName = "INSTANCE_LOD_PARAMS"; break;
		case VertexPacking::PS_InstanceSkinningData:semanticName = "INSTANCE_SKINNING_DATA"; break;

		case VertexPacking::PS_PatchSize:			semanticName = "TESS_PATCH_SIZE"; break;
		case VertexPacking::PS_PatchBias:			semanticName = "TESS_PATCH_BIAS"; break;
		case VertexPacking::PS_PatchOffset:			semanticName = "PATCH_OFFSET"; break;

		case VertexPacking::PS_ExtraData:			semanticName = "EXTRA_DATA"; break;

		// SpeedTree semantics
		case VertexPacking::PS_SpT_Attr:			semanticName = "ST_ATTR"; break;
		// -

		case VertexPacking::PS_Padding:				return false;

		default:
			return false;
		}

		outSemantic = semanticName;
		outIndex = semanticIndex;

		return true;
	}

}
