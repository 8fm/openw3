/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


namespace GpuApi
{

	inline DXGI_FORMAT Map( eIndexFormat format )
	{
		switch ( format )
		{
			case INDEXFMT_UShort:	return DXGI_FORMAT_R16_UINT;
			case INDEXFMT_UInt:		return DXGI_FORMAT_R32_UINT;
			default:				GPUAPI_HALT( "invalid" ); return DXGI_FORMAT_R32_UINT;
		}
	}

	// This is used by TexCookXboxOne. If you change it and expect to do any cooking, you should rebuild the tool in /dev/internal/TexCookTool.
	inline DXGI_FORMAT Map( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
		case TEXFMT_A8_Scaleform:			return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
		case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_Uint_16_norm:			return DXGI_FORMAT_R16_UNORM;
		case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
		case TEXFMT_Uint_32:				return DXGI_FORMAT_R32_UINT;
		case TEXFMT_R16G16_Uint:			return DXGI_FORMAT_R16G16_UINT;
		case TEXFMT_R32G32_Uint:			return DXGI_FORMAT_R32G32_UINT;
		case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
		case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	#ifdef RED_PLATFORM_DURANGO
		case TEXFMT_R10G10B10_6E4_A2_FLOAT:	return DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT;
	#endif
		case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
		//dex++
		case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT;
		//dex--
		case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
		case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
		case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
		case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
		case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
		case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
		case TEXFMT_BC5:					return DXGI_FORMAT_BC5_UNORM;
		case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
		case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
		case TEXFMT_D24S8:					return DXGI_FORMAT_R24G8_TYPELESS;
		case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_TYPELESS;
		case TEXFMT_D16U:					return DXGI_FORMAT_R16_TYPELESS;
		case TEXFMT_D32F:					return DXGI_FORMAT_R32_TYPELESS;
		default:							GPUAPI_HALT( "invalid or not available" ); return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	inline DXGI_FORMAT MapShaderResourceView( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
		case TEXFMT_A8_Scaleform:			return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
		case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_Uint_16_norm:			return DXGI_FORMAT_R16_UNORM;
		case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
		case TEXFMT_Uint_32:				return DXGI_FORMAT_R32_UINT;
		case TEXFMT_R16G16_Uint:			return DXGI_FORMAT_R16G16_UINT;
		case TEXFMT_R32G32_Uint:			return DXGI_FORMAT_R32G32_UINT;
		case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
		case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	#ifdef RED_PLATFORM_DURANGO
		case TEXFMT_R10G10B10_6E4_A2_FLOAT:	return DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT;
	#endif
		case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
		case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
		case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
		case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
		case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
		case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
		case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
		case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
		case TEXFMT_BC5:					return DXGI_FORMAT_BC5_UNORM;
		case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
		case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
		case TEXFMT_D24S8:					return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_UNORM;
		case TEXFMT_D16U:					return DXGI_FORMAT_R16_UNORM;
		case TEXFMT_D32F:					return DXGI_FORMAT_R32_FLOAT;
		default:							GPUAPI_HALT( "invalid or not available" ); return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	inline DXGI_FORMAT MapRenderTargetView( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
		case TEXFMT_A8_Scaleform:			return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
		case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_Uint_16_norm:			return DXGI_FORMAT_R16_UNORM;
		//case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
		case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
		case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	#ifdef RED_PLATFORM_DURANGO
		case TEXFMT_R10G10B10_6E4_A2_FLOAT:	return DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT;
	#endif
		case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
		case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
		case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
		case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
		case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
		case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
		case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
		case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
		case TEXFMT_BC5:					return DXGI_FORMAT_BC5_UNORM;
		case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
		case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
		case TEXFMT_D24S8:					return DXGI_FORMAT_R24G8_TYPELESS;
		case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_TYPELESS;
		case TEXFMT_D16U:					return DXGI_FORMAT_D16_UNORM;
		case TEXFMT_D32F:					return DXGI_FORMAT_D32_FLOAT;
		default:							GPUAPI_HALT( "invalid or not available" ); return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	inline DXGI_FORMAT MapDepthStencilView( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
		case TEXFMT_A8_Scaleform:			return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
		case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
		case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		//case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
		case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
		case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	#ifdef RED_PLATFORM_DURANGO
		case TEXFMT_R10G10B10_6E4_A2_FLOAT:	return DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT;
	#endif
		case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
		case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
		case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
		case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
		case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
		case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
		case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
		case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
		case TEXFMT_BC5:					return DXGI_FORMAT_BC5_UNORM;
		case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
		case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
		case TEXFMT_D24S8:					return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_TYPELESS;
		case TEXFMT_D16U:					return DXGI_FORMAT_D16_UNORM;
		case TEXFMT_D32F:					return DXGI_FORMAT_D32_FLOAT;
		default:							GPUAPI_HALT( "invalid or not available" ); return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	inline eTextureFormat Map( DXGI_FORMAT format )
	{
		switch ( format )
		{
		case DXGI_FORMAT_A8_UNORM:				return TEXFMT_A8;
		case DXGI_FORMAT_R8_UNORM:				return TEXFMT_L8;
		case DXGI_FORMAT_R8_UINT:				return TEXFMT_R8_Uint;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return TEXFMT_R8G8B8A8;
		case DXGI_FORMAT_R8G8_TYPELESS:			return TEXFMT_A8L8;
		case DXGI_FORMAT_R8G8_UNORM:			return TEXFMT_A8L8;
		case DXGI_FORMAT_R16_UNORM:				return TEXFMT_Uint_16_norm;
		case DXGI_FORMAT_R16_UINT:				return TEXFMT_Uint_16;
		case DXGI_FORMAT_R32_UINT:				return TEXFMT_Uint_32;
		case DXGI_FORMAT_R16G16_UINT:			return TEXFMT_R16G16_Uint;
		case DXGI_FORMAT_R32G32_UINT:			return TEXFMT_R32G32_Uint;
		case DXGI_FORMAT_R10G10B10A2_UNORM:		return TEXFMT_Float_R10G10B10A2;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return TEXFMT_Float_R16G16B16A16;
		case DXGI_FORMAT_R11G11B10_FLOAT:		return TEXFMT_Float_R11G11B10;
	#ifdef RED_PLATFORM_DURANGO
		case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:	return TEXFMT_R10G10B10_6E4_A2_FLOAT;
	#endif
		case DXGI_FORMAT_R16G16_FLOAT:			return TEXFMT_Float_R16G16;
		case DXGI_FORMAT_R32G32_FLOAT:			return TEXFMT_Float_R32G32; //dex
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return TEXFMT_Float_R32G32B32A32;
		case DXGI_FORMAT_R32_FLOAT:				return TEXFMT_Float_R32;
		case DXGI_FORMAT_R16_FLOAT:				return TEXFMT_Float_R16;
		case DXGI_FORMAT_R24G8_TYPELESS:		return TEXFMT_D24S8;
		case DXGI_FORMAT_BC1_UNORM:				return TEXFMT_BC1;
		case DXGI_FORMAT_BC2_UNORM:				return TEXFMT_BC2;
		case DXGI_FORMAT_BC3_UNORM:				return TEXFMT_BC3;
		case DXGI_FORMAT_BC4_UNORM:				return TEXFMT_BC4;
		case DXGI_FORMAT_BC5_UNORM:				return TEXFMT_BC5;
		case DXGI_FORMAT_BC6H_UF16:				return TEXFMT_BC6H;
		case DXGI_FORMAT_BC7_UNORM:				return TEXFMT_BC7;
		default:							GPUAPI_HALT( "invalid" ); return TEXFMT_Max;
		}
	}

	inline D3D11_PRIMITIVE_TOPOLOGY Map( ePrimitiveType primitive )
	{
		switch ( primitive )
		{
		case PRIMTYPE_LineList:			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		case PRIMTYPE_LineStrip:		return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PRIMTYPE_TriangleList:		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PRIMTYPE_TriangleStrip:	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PRIMTYPE_PointList:		return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;

		case PRIMTYPE_1CP_PATCHLIST:	return D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
		case PRIMTYPE_3CP_PATCHLIST:	return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
		case PRIMTYPE_4CP_PATCHLIST:	return D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;

		default:						GPUAPI_HALT( "invalid" ); return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	inline D3D11_TEXTURE_ADDRESS_MODE Map( eTextureAddress address )
	{
		switch ( address )
		{
		case TEXADDR_Wrap:			return D3D11_TEXTURE_ADDRESS_WRAP;
		case TEXADDR_Mirror:		return D3D11_TEXTURE_ADDRESS_MIRROR;
		case TEXADDR_Clamp:			return D3D11_TEXTURE_ADDRESS_CLAMP;
		case TEXADDR_MirrorOnce:	return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		case TEXADDR_Border:		return D3D11_TEXTURE_ADDRESS_BORDER;
		default:					GPUAPI_HALT( "invalid" ); return D3D11_TEXTURE_ADDRESS_CLAMP;
		}
	}

	inline D3D11_FILTER Map( eTextureFilterMin filterMin, eTextureFilterMag filterMag, eTextureFilterMip filterMip, eComparison comp )
	{
		if (filterMin == TEXFILTERMIN_Aniso || filterMin == TEXFILTERMIN_AnisoLow)
		{
			return D3D11_FILTER_ANISOTROPIC;
		}

		Uint32 ret = 0;
		if (filterMin == TEXFILTERMIN_Linear)
		{
			ret += 16;
		}
		if (filterMag == TEXFILTERMAG_Linear)
		{
			ret += 4;
		}

		if (filterMip == TEXFILTERMIP_Linear)
		{
			ret += 1;
		}

		if ( comp != COMP_None )
		{
			ret += 0x80;
		}

		D3D11_FILTER filter = (D3D11_FILTER)ret;

		return filter;
	}

	inline D3D11_COMPARISON_FUNC Map( eComparison comparison )
	{
		switch ( comparison )
		{
			case COMP_None:			return D3D11_COMPARISON_ALWAYS;
			case COMP_Less:			return D3D11_COMPARISON_LESS;
			default:				GPUAPI_HALT( "invalid" ); return D3D11_COMPARISON_LESS;
		}
	}

	inline D3D11_VIEWPORT Map( const ViewportDesc &viewport )
	{
		D3D11_VIEWPORT d3dViewport;
		d3dViewport.TopLeftX = (Float)viewport.x;
		d3dViewport.TopLeftY = (Float)viewport.y;
		d3dViewport.Width = (Float)viewport.width;
		d3dViewport.Height = (Float)viewport.height;
		d3dViewport.MinDepth = viewport.minZ;
		d3dViewport.MaxDepth = viewport.maxZ;

		return d3dViewport;
	}

	inline Uint32 MapBuffLockFlagsToD3DLockType( Uint32 flags )
	{
		Uint32 result = 0;
		result |= (BLF_Read			& flags) ? D3D11_MAP_READ				: 0;
		result |= (BLF_Write		& flags) ? D3D11_MAP_WRITE				: 0;
		result |= (BLF_Discard		& flags) ? D3D11_MAP_WRITE_DISCARD		: 0;
		result |= (BLF_NoOverwrite	& flags) ? D3D11_MAP_WRITE_NO_OVERWRITE	: 0;
		return result;
	}

	inline Uint32 MapBuffLockFlagsToD3DLockFlags( Uint32 flags )
	{
		Uint32 result = 0;
		result |= (BLF_DoNotWait			& flags) ? D3D11_MAP_FLAG_DO_NOT_WAIT				: 0;
		return result;
	}

	// Get D3D semantic and index from a given PackingElement. If invalid element usage, outputs are unmodified and return false.
	inline Bool MapPackingElementToSemanticAndIndex( const VertexPacking::PackingElement& element, const char*& outSemantic, Uint32& outIndex )
	{
		const char* semanticName = NULL;
		Uint32 semanticIndex = element.m_usageIndex;

		switch ( element.m_usage )
		{
		case VertexPacking::PS_SysPosition:			semanticName = "SV_Position"; break;
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
		case VertexPacking::PS_SpT_Attr:			if ( semanticIndex == 0 )
													{
														semanticName = "POSITION";
													}
													else
													{
														semanticName = "TEXCOORD"; 
														semanticIndex--;
													}
													break;
		// -

		case VertexPacking::PS_Padding:				return false;

		default:
			GPUAPI_ERROR( TXT("Unsupported VertexPacking usage") );
			return false;
		}

		outSemantic = semanticName;
		outIndex = semanticIndex;

		return true;
	}

}
