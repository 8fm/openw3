/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


namespace GpuApi
{

// https://www.opengl.org/registry/specs/ARB/texture_compression_bptc.txt
// #define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB        0x8E8F
// #define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB				0x8E8C

// https://www.opengl.org/registry/specs/ARB/texture_compression_rgtc.txt
// #define GL_COMPRESSED_RG_RGTC2							0x8DBD

	//inline DXGI_FORMAT Map( eIndexFormat format )
	//{
	//	switch ( format )
	//	{
	//		case INDEXFMT_UShort:	return DXGI_FORMAT_R16_UINT;
	//		case INDEXFMT_UInt:		return DXGI_FORMAT_R32_UINT;
	//		default:				GPUAPI_HALT( TXT( "invalid" ) ); return DXGI_FORMAT_R32_UINT;
	//	}
	//}

	inline GLuint MapFormat( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return GL_RED;
		case TEXFMT_L8:						return GL_RED;
		case TEXFMT_R8_Uint:				return GL_RED;
		case TEXFMT_R8G8B8A8:				return GL_RGBA;
		case TEXFMT_R8G8B8X8:				return GL_RGB;
		case TEXFMT_Uint_16_norm:			return GL_RED;
		case TEXFMT_Uint_16:				return GL_RED_INTEGER;
		case TEXFMT_R16G16_Uint:			return GL_RG;
		case TEXFMT_Float_R10G10B10A2:		return GL_RGBA;
		case TEXFMT_Float_R16G16B16A16:		return GL_RGBA;
		case TEXFMT_Float_R11G11B10:		return GL_RGB;
		case TEXFMT_Float_R16G16:			return GL_RG;
		case TEXFMT_Float_R32G32:			return GL_RG;
		case TEXFMT_Float_R32G32B32A32:		return GL_RGBA;
		case TEXFMT_Float_R32:				return GL_RED;
		case TEXFMT_Float_R16:				return GL_RED;
		case TEXFMT_BC1:					return GL_RGBA;
		case TEXFMT_BC2:					return GL_RGBA;
		case TEXFMT_BC3:					return GL_RGBA;
		case TEXFMT_BC4:					return GL_RGBA;
		case TEXFMT_BC6H:					return GL_RGBA;
		case TEXFMT_BC7:					return GL_RGBA;
		case TEXFMT_D24S8:					return GL_DEPTH_STENCIL;
		case TEXFMT_D32F:					return GL_DEPTH_COMPONENT;
		case TEXFMT_A8L8:					return GL_RG;
		default:							GPUAPI_HALT( "invalid or not available" ); return GL_RGBA8;
		}
	}

	inline GLuint MapInternalFormat( eTextureFormat format )
	{
		switch ( format )
		{
		case TEXFMT_A8:						return GL_ALPHA8;
		case TEXFMT_L8:						return GL_R8;
		case TEXFMT_R8_Uint:				return GL_R8UI;
		case TEXFMT_R8G8B8A8:				return GL_RGBA8;
		case TEXFMT_R8G8B8X8:				return GL_RGBA8;
		case TEXFMT_Uint_16_norm:			return GL_R16_SNORM;
		case TEXFMT_Uint_16:				return GL_R16UI;
		case TEXFMT_R16G16_Uint:			return GL_RG16UI;
		case TEXFMT_Float_R10G10B10A2:		return GL_RGB10_A2;
		case TEXFMT_Float_R16G16B16A16:		return GL_RGBA16F;
		case TEXFMT_Float_R11G11B10:		return GL_R11F_G11F_B10F;
		case TEXFMT_Float_R16G16:			return GL_RG16F;
		case TEXFMT_Float_R32G32:			return GL_RG32F;
		case TEXFMT_Float_R32G32B32A32:		return GL_RGBA32F;
		case TEXFMT_Float_R32:				return GL_R32F;
		case TEXFMT_Float_R16:				return GL_R16F;
		case TEXFMT_BC1:					return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case TEXFMT_BC2:					return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case TEXFMT_BC3:					return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TEXFMT_BC4:					return GL_COMPRESSED_RG_RGTC2;
		case TEXFMT_BC6H:					return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
		case TEXFMT_BC7:					return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
		case TEXFMT_D24S8:					return GL_DEPTH24_STENCIL8;
		case TEXFMT_D32F:					return GL_DEPTH_COMPONENT32F;
		case TEXFMT_A8L8:					return GL_LUMINANCE8_ALPHA8;
		default:							GPUAPI_HALT( "invalid or not available" ); return GL_RGBA8;
		}
	}

	//inline DXGI_FORMAT MapShaderResourceView( eTextureFormat format )
	//{
	//	switch ( format )
	//	{
	//	case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
	//	case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
	//	case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
	//	case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	case TEXFMT_Uint_16_norm:			return DXGI_FORMAT_R16_UNORM;
	//	case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
	//	case TEXFMT_R16G16_Uint:			return DXGI_FORMAT_R16G16_UINT;
	//	case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
	//	case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	//	case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	//	case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
	//	case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
	//	case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
	//	case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
	//	case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
	//	case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
	//	case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
	//	case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
	//	case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
	//	case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
	//	case TEXFMT_D24S8:					return DXGI_FORMAT_R24G8_TYPELESS;
	//	case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_UNORM;
	//	default:							GPUAPI_HALT( TXT( "invalid or not available" ) ); return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	}
	//}

	//inline DXGI_FORMAT MapRenderTargetView( eTextureFormat format )
	//{
	//	switch ( format )
	//	{
	//	case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
	//	case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
	//	case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
	//	case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	//case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
	//	case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
	//	case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	//	case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	//	case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
	//	case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
	//	case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
	//	case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
	//	case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
	//	case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
	//	case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
	//	case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
	//	case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
	//	case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
	//	case TEXFMT_D24S8:					return DXGI_FORMAT_R24G8_TYPELESS;
	//	case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_TYPELESS;
	//	default:							GPUAPI_HALT( TXT( "invalid or not available" ) ); return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	}
	//}

	//inline DXGI_FORMAT MapDepthStencilView( eTextureFormat format )
	//{
	//	switch ( format )
	//	{
	//	case TEXFMT_A8:						return DXGI_FORMAT_A8_UNORM;
	//	case TEXFMT_L8:						return DXGI_FORMAT_R8_UNORM;
	//	case TEXFMT_R8_Uint:				return DXGI_FORMAT_R8_UINT;
	//	case TEXFMT_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	case TEXFMT_R8G8B8X8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	//case TEXFMT_Uint_16:				return DXGI_FORMAT_R16_UINT;
	//	case TEXFMT_Float_R10G10B10A2:		return DXGI_FORMAT_R10G10B10A2_UNORM;
	//	case TEXFMT_Float_R16G16B16A16:		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	//	case TEXFMT_Float_R11G11B10:		return DXGI_FORMAT_R11G11B10_FLOAT;
	//	case TEXFMT_Float_R16G16:			return DXGI_FORMAT_R16G16_FLOAT;
	//	case TEXFMT_Float_R32G32:			return DXGI_FORMAT_R32G32_FLOAT; //dex
	//	case TEXFMT_Float_R32G32B32A32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	case TEXFMT_Float_R32:				return DXGI_FORMAT_R32_FLOAT;
	//	case TEXFMT_Float_R16:				return DXGI_FORMAT_R16_FLOAT;
	//	case TEXFMT_BC1:					return DXGI_FORMAT_BC1_UNORM;
	//	case TEXFMT_BC2:					return DXGI_FORMAT_BC2_UNORM;
	//	case TEXFMT_BC3:					return DXGI_FORMAT_BC3_UNORM;
	//	case TEXFMT_BC4:					return DXGI_FORMAT_BC4_UNORM;
	//	case TEXFMT_BC6H:					return DXGI_FORMAT_BC6H_UF16;
	//	case TEXFMT_BC7:					return DXGI_FORMAT_BC7_UNORM;
	//	case TEXFMT_D24S8:					return DXGI_FORMAT_R24G8_TYPELESS;
	//	case TEXFMT_A8L8:					return DXGI_FORMAT_R8G8_TYPELESS;
	//	default:							GPUAPI_HALT( TXT( "invalid or not available" ) ); return DXGI_FORMAT_R8G8B8A8_UNORM;
	//	}
	//}

	//inline eTextureFormat Map( DXGI_FORMAT format )
	//{
	//	switch ( format )
	//	{
	//	case DXGI_FORMAT_A8_UNORM:				return TEXFMT_A8;
	//	case DXGI_FORMAT_R8_UNORM:				return TEXFMT_L8;
	//	case DXGI_FORMAT_R8_UINT:				return TEXFMT_R8_Uint;
	//	case DXGI_FORMAT_R8G8B8A8_UNORM:		return TEXFMT_R8G8B8A8;
	//	case DXGI_FORMAT_R8G8_TYPELESS:			return TEXFMT_A8L8;
	//	case DXGI_FORMAT_R8G8_UNORM:			return TEXFMT_A8L8;
	//	case DXGI_FORMAT_R16_UNORM:				return TEXFMT_Uint_16_norm;
	//	case DXGI_FORMAT_R16_UINT:				return TEXFMT_Uint_16;
	//	case DXGI_FORMAT_R16G16_UINT:			return TEXFMT_R16G16_Uint;
	//	case DXGI_FORMAT_R10G10B10A2_UNORM:		return TEXFMT_Float_R10G10B10A2;
	//	case DXGI_FORMAT_R16G16B16A16_FLOAT:	return TEXFMT_Float_R16G16B16A16;
	//	case DXGI_FORMAT_R11G11B10_FLOAT:		return TEXFMT_Float_R11G11B10;
	//	case DXGI_FORMAT_R16G16_FLOAT:			return TEXFMT_Float_R16G16;
	//	case DXGI_FORMAT_R32G32_FLOAT:			return TEXFMT_Float_R32G32; //dex
	//	case DXGI_FORMAT_R32G32B32A32_FLOAT:	return TEXFMT_Float_R32G32B32A32;
	//	case DXGI_FORMAT_R32_FLOAT:				return TEXFMT_Float_R32;
	//	case DXGI_FORMAT_R16_FLOAT:				return TEXFMT_Float_R16;
	//	case DXGI_FORMAT_R24G8_TYPELESS:		return TEXFMT_D24S8;
	//	case DXGI_FORMAT_BC1_UNORM:				return TEXFMT_BC1;
	//	case DXGI_FORMAT_BC2_UNORM:				return TEXFMT_BC2;
	//	case DXGI_FORMAT_BC3_UNORM:				return TEXFMT_BC3;
	//	case DXGI_FORMAT_BC4_UNORM:				return TEXFMT_BC4;
	//	case DXGI_FORMAT_BC6H_UF16:				return TEXFMT_BC6H;
	//	case DXGI_FORMAT_BC7_UNORM:				return TEXFMT_BC7;
	//	default:							GPUAPI_HALT( TXT( "invalid" ) ); return TEXFMT_Max;
	//	}
	//}

	inline GLenum Map( ePrimitiveType primitive )
	{
		switch ( primitive )
		{
		case PRIMTYPE_LineList:			return GL_LINES;
		case PRIMTYPE_LineStrip:		return GL_LINE_STRIP;
		case PRIMTYPE_TriangleList:		return GL_TRIANGLES;
		case PRIMTYPE_TriangleStrip:	return GL_TRIANGLE_STRIP;
		case PRIMTYPE_PointList:		return GL_POINTS;

		case PRIMTYPE_1CP_PATCHLIST:	return GL_PATCHES;
		case PRIMTYPE_3CP_PATCHLIST:	return GL_PATCHES;
		case PRIMTYPE_4CP_PATCHLIST:	return GL_PATCHES;

		default:						GPUAPI_HALT("invalid"); return GL_TRIANGLES;
		}
	}

	inline GLuint Map( eTextureAddress address )
	{
		switch ( address )
		{
		case TEXADDR_Wrap:			return GL_REPEAT;
		case TEXADDR_Mirror:		return GL_MIRRORED_REPEAT;
		case TEXADDR_Clamp:			return GL_CLAMP_TO_EDGE;
		case TEXADDR_MirrorOnce:	return GL_MIRRORED_REPEAT; //GL_MIRROR_CLAMP_TO_EDGE; //invalid enum???
		default:					GPUAPI_HALT("invalid"); return GL_REPEAT;
		}
	}

	inline GLuint Map( eTextureFilterMin filterMin, eTextureFilterMip filterMip )
	{
		if (filterMip == TEXFILTERMIP_None)
		{
			if (filterMin == TEXFILTERMIN_Point)
			{
				return GL_NEAREST;
			}
			else
			{
				return GL_LINEAR;
			}
		}
		else
		{
			if (filterMip == TEXFILTERMIN_Point)
			{
				if (filterMin == TEXFILTERMIN_Point)
				{
					return GL_NEAREST_MIPMAP_NEAREST;
				}
				else
				{
					return GL_LINEAR_MIPMAP_NEAREST;
				}
			}
			else
			{
				if (filterMin == TEXFILTERMIN_Point)
				{
					return GL_NEAREST_MIPMAP_LINEAR;
				}
				else
				{
					return GL_LINEAR_MIPMAP_LINEAR;
				}
			}
		}
	}

	inline GLuint Map( eTextureFilterMag filterMag )
	{
		if (filterMag == TEXFILTERMAG_Point)
		{
			return GL_NEAREST;
		}
		else
		{
			return GL_LINEAR;
		}
	}

	inline GLuint MapCompareFunc( eComparison comparison )
	{
		switch ( comparison )
		{
			case COMP_None:			return GL_ALWAYS;
			case COMP_Less:			return GL_LESS;
			default:				GPUAPI_HALT("invalid"); return GL_ALWAYS;
		}
	}

	inline GLuint MapCompareMode(eComparison comparison)
	{
		switch (comparison)
		{
		case COMP_None:			return GL_NONE;
		case COMP_Less:			return GL_COMPARE_REF_TO_TEXTURE;
		default:				GPUAPI_HALT("invalid"); return GL_NONE;
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

	//inline Int32 Map( eCubeFace face )
	//{
	//	return (Int32)face;
	//}

	inline GLenum MapBuffLockFlagsToOGLLockType( Uint32 flags )
	{
		Uint32 maskedFlags = flags & (BLF_Read | BLF_Write);

		if (BLF_Read == maskedFlags)
		{
			return GL_READ_ONLY;
		}

		if ( BLF_Write == maskedFlags )
		{
			return GL_WRITE_ONLY;
		}

		if ( maskedFlags == (BLF_Read | BLF_Write) )
		{
			return GL_READ_WRITE;
		}

		return GL_WRITE_ONLY;
	}

	inline GLenum MapAccessFlagsAndUsageToOGLUsage( Uint32 accessFlags, Uint32 usage )
	{
		GLenum glUsage = GL_STATIC_DRAW;
		if (accessFlags == 0)
		{
			if (usage == BUT_Dynamic)
			{
				glUsage = GL_DYNAMIC_DRAW;
			}
		}
		else if ( accessFlags & BAF_CPURead )
		{
			glUsage = GL_STATIC_READ;
			if (usage == BUT_Dynamic)
			{
				glUsage = GL_DYNAMIC_READ;
			}
		}
		return glUsage;
	}

	//inline Uint32 MapBuffLockFlagsToD3DLockFlags( Uint32 flags )
	//{
	//	Uint32 result = 0;
	//	result |= (BLF_DoNotWait			& flags) ? D3D11_MAP_FLAG_DO_NOT_WAIT				: 0;
	//	return result;
	//}

	//inline Bool IsTextureFormatDXT( eTextureFormat format )
	//{
	//	return TEXFMT_BC1 == format 
	//		|| TEXFMT_BC2 == format 
	//		|| TEXFMT_BC3 == format 
	//		|| TEXFMT_BC4 == format 
	//		|| TEXFMT_BC6H == format 
	//		|| TEXFMT_BC7 == format;
	//}

	//inline Bool IsTextureSizeValidForFormat( Uint32 width, Uint32 height, eTextureFormat format )
	//{
	//	return width > 0 && height > 0 && !(IsTextureFormatDXT(format) && (width % 4 || height % 4));
	//}


	///// Convert number of draw elements (vertices or indices) to number of primitives, based on primitive type.
	//inline Int32 MapDrawElementCountToPrimitiveCount( ePrimitiveType primitiveType, Int32 elementCount )
	//{
	//	switch ( primitiveType )
	//	{
	//	case PRIMTYPE_LineList:			return elementCount / 2;
	//	case PRIMTYPE_LineStrip:		return elementCount - 1;
	//	case PRIMTYPE_TriangleList:		return elementCount / 3;
	//	case PRIMTYPE_TriangleStrip:	return elementCount - 2;
	//	case PRIMTYPE_PointList:		return elementCount;
	//	case PRIMTYPE_QuadList:			return elementCount / 4;
	//	case PRIMTYPE_1CP_PATCHLIST:	return elementCount;
	//	case PRIMTYPE_3CP_PATCHLIST:	return elementCount / 3;
	//	case PRIMTYPE_4CP_PATCHLIST:	return elementCount / 4;
	//	case PRIMTYPE_Invalid:			GPUAPI_HALT();
	//	};

	//	return 0;
	//}

	//// Get D3D semantic and index from a given PackingElement. If invalid element usage, outputs are unmodified and return false.
	//inline Bool MapPackingElementToSemanticAndIndex( const VertexPacking::PackingElement& element, const char*& outSemantic, Uint32& outIndex )
	//{
	//	const char* semanticName = NULL;
	//	Uint32 semanticIndex = element.m_usageIndex;

	//	switch ( element.m_usage )
	//	{
	//	case VertexPacking::PS_SVPosition:			semanticName = "SV_POSITION"; break;
	//	case VertexPacking::PS_Position:			semanticName = "POSITION"; break;
	//	case VertexPacking::PS_Normal:				semanticName = "NORMAL"; break;
	//	case VertexPacking::PS_Tangent:				semanticName = "TANGENT"; break;
	//	case VertexPacking::PS_Binormal:			semanticName = "BINORMAL"; break;
	//	case VertexPacking::PS_TexCoord:			semanticName = "TEXCOORD"; break;
	//	case VertexPacking::PS_Color:				semanticName = "COLOR"; break;
	//	case VertexPacking::PS_SkinIndices:			semanticName = "BLENDINDICES"; break;
	//	case VertexPacking::PS_SkinWeights:			semanticName = "BLENDWEIGHT"; break;
	//	case VertexPacking::PS_InstanceTransform:	semanticName = "INSTANCE_TRANSFORM"; break;
	//	case VertexPacking::PS_InstanceLODParams:	semanticName = "INSTANCE_LOD_PARAMS"; break;

	//	case VertexPacking::PS_PatchSize:			semanticName = "PATCH_SIZE"; break;
	//	case VertexPacking::PS_PatchBias:			semanticName = "PATCH_BIAS"; break;

	//	case VertexPacking::PS_ExtraData:			semanticName = "EXTRA_DATA"; break;

	//	case VertexPacking::PS_Padding: break;

	//	default:
	//		return false;
	//	}

	//	outSemantic = semanticName;
	//	outIndex = semanticIndex;

	//	return true;
	//}

}
