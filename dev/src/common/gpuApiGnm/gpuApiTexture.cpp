/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redMath/redmathbase.h"
#include "../redMath/numericalutils.h"
#include "../redMath/float16compressor.h"

#include "gpuApiDDSLoader.h"

#include "../redMath/random/random.h"
#include "../redMath/random/standardRand.h"


#define	GPUAPI_BLANK2D_TEXTURE_SIZE				4
#define	GPUAPI_DEFAULT2D_TEXTURE_SIZE			16
#define	GPUAPI_DEFAULTCUBE_TEXTURE_SIZE			16
#define	GPUAPI_DISSOLVE_TEXTURE_SIZE			16
#define	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE	32
#define GPUAPI_SSAO_ROTATION_TEXTURE_SIZE		4
#define GPUAPI_MIP_NOISE_TEXTURE_SIZE			64


namespace GpuApi
{
#ifndef RED_FINAL_BUILD
# define DETAILED_RENDER_TARGET_MEM_CLASS

	RED_FORCE_INLINE Red::MemoryFramework::MemoryClass DetailedRenderTargetMemClass( eTextureType texType, eTextureGroup texGroup )
	{
		if ( texGroup == TEXG_UI )
		{
			return MC_RenderTarget_UI;
		}
		else if ( texGroup == TEXG_Shadow )
		{
			return MC_RenderTarget_Shadow;
		}
		else if ( texGroup == TEXG_Terrain )
		{
			return MC_RenderTarget_Terrain;
		}
		else if ( texGroup == TEXG_TerrainShadow )
		{
			return MC_RenderTarget_TerrainShadow;
		}
		else
		{
			return MC_RenderTarget;
		}
	}

	RED_FORCE_INLINE Red::MemoryFramework::MemoryClass DetailedDepthStencilMemClass( eTextureType texType, eTextureGroup texGroup )
	{
		if ( texGroup == TEXG_UI )
		{
			return MC_DepthStencilTarget_UI;
		}
		else if ( texGroup == TEXG_Shadow )
		{
			return MC_DepthStencilTarget_Shadow;
		}
		else if ( texGroup == TEXG_Terrain )
		{
			return MC_DepthStencilTarget_Terrain;
		}
		else if ( texGroup == TEXG_TerrainShadow )
		{
			return MC_DepthStencilTarget_TerrainShadow;
		}
		else
		{
			return MC_DepthStencilTarget;
		}
	}

#endif

	extern const char* GetProfilerMarkerString();

	namespace Utils
	{
				//////////////////////////////////////////////////////////////////////////
		// Random Number interface for GPUApiTexture
		Red::Math::Random::Generator< Red::Math::Random::StandardRand > GRandomNumberGenerator;

		// Texture factory
		void TextureFactoryGNM( const Capabilities &caps, const TextureDesc &desc, Bool* outSupported, sce::Gnm::Texture* outTexture, sce::Gnm::SizeAlign* outSize )
		{
			//////////////////////////////////////////////////////////////////////////
			// NOTE
			// If changes are made to how textures are created here, such as tiling mode, particularly for resource textures (immutable,
			// cookable), then matching changes should be made in /dev/internal/TexCookTools/TexCookPS4.cpp.
			//////////////////////////////////////////////////////////////////////////

			if ( outSupported )
			{
				*outSupported = false;
			}

			if ( !IsTextureSizeValidForFormat( desc.width, desc.height, desc.format ) )
			{
				return;
			}

			if ( outSupported )
			{
				if (desc.format != TEXFMT_Max && (desc.width > 0 && desc.height > 0 && !(desc.usage & TEXUSAGE_BackBuffer)))
				{
					*outSupported = true;
				}
			}

			if ( outTexture )
			{
				// HACK GNM
				sce::Gnm::DataFormat format = Map( desc.format );

				sce::GpuAddress::SurfaceType surfaceType = sce::GpuAddress::kSurfaceTypeTextureFlat;
				if (desc.type == TEXTYPE_CUBE)
				{
					surfaceType = sce::GpuAddress::kSurfaceTypeTextureCubemap;
				}
				if ( desc.usage & TEXUSAGE_RenderTarget )
				{
					surfaceType = sce::GpuAddress::kSurfaceTypeColorTarget;
				}
				if ( desc.usage & TEXUSAGE_DepthStencil )
				{
					surfaceType = sce::GpuAddress::kSurfaceTypeDepthOnlyTarget;
				}

				sce::Gnm::TileMode tileMode;
				if( sce::GpuAddress::computeSurfaceTileMode( &tileMode, surfaceType, format, 1 ) != sce::GpuAddress::kStatusSuccess )
				{
					GPUAPI_ERROR( TXT("Cannot compute the tile mode for the texture.") );
					tileMode = sce::Gnm::kTileModeDisplay_LinearAligned;
				}
				if ( surfaceType == sce::GpuAddress::kSurfaceTypeTextureFlat && (desc.usage & (TEXUSAGE_Staging | TEXUSAGE_Dynamic)) && !IsTextureFormatDXT( desc.format ) )
				{
					tileMode = sce::Gnm::kTileModeDisplay_LinearAligned;
				}

				sce::Gnm::NumFragments fragments = sce::Gnm::kNumFragments1;
				sce::Gnm::SizeAlign sizeAlign;

				switch ( desc.type )
				{
				case TEXTYPE_2D:
					{
						sizeAlign = outTexture->initAs2d( desc.width, desc.height, desc.initLevels, format, tileMode, fragments );
					}
					break;
				case TEXTYPE_ARRAY:
					{
						sizeAlign = outTexture->initAs2dArray( desc.width, desc.height, desc.sliceNum, desc.initLevels, format, tileMode, fragments, false );
					}
					break;
				case TEXTYPE_CUBE:
					{
						sizeAlign = outTexture->initAs2dArray( desc.width, desc.height, desc.sliceNum, desc.initLevels, format, tileMode, fragments, true);
					}
					break;
				default:
					break;
				}

				if (outSize)
					*outSize = sizeAlign;
			}
		}

		typedef void (*Texture2DFillFunction)(RedVector4* pOut, const RedVector2 *pTexCoord, const RedVector2 *pTexelSize, void* pData);
			
		void InitInternalTextureData2D( STextureData &td, Uint32 size, Texture2DFillFunction funcFill, Bool enablePointMips = false )
		{
			GPUAPI_ASSERT( !(enablePointMips && !Red::Math::IsPow2( size )) );
			const Uint32 mipLevels = enablePointMips ? Red::Math::MLog2( size ) + 1 : 1;

			Uint32 width = size;
			Uint32 height = size;

			td.m_Desc.type	 = TEXTYPE_2D;
			td.m_Desc.width  = width;
			td.m_Desc.height = height;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = mipLevels;

			sce::Gnm::DataFormat format = sce::Gnm::kDataFormatB8G8R8A8Unorm;
			sce::Gnm::SizeAlign sizeAlign = td.m_texture.initAs2d( width, height, mipLevels, format, sce::Gnm::kTileModeDisplay_LinearAligned, sce::Gnm::kNumFragments1 );

			GPUAPI_ASSERT( sizeAlign.m_align != 0 && sizeAlign.m_size != 0, TXT("Texture init unsuccessful") )

			Red::MemoryFramework::MemoryRegionHandle memoryHandle = GpuApi::AllocateInPlaceMemoryRegion( INPLACE_Texture, sizeAlign.m_size, GpuApi::MC_InternalTexture, sizeAlign.m_align, Red::MemoryFramework::Region_Longlived );
			Uint8* tex_data = (Uint8*) memoryHandle.GetRawPtr();
			td.m_texture.setBaseAddress(tex_data);
			td.m_memoryRegion = memoryHandle;
			td.m_Desc.inPlaceType = INPLACE_Texture;

			if ( tex_data == nullptr )
			{
				GPUAPI_HALT( "Can't allocate texture init memory" );
				return;
			}

			for ( Uint32 mip_i=0; mip_i<mipLevels; ++mip_i )
			{ // create init data

				const Uint32 bytesPerPixel = 4;

				Uint64 gnmSurfaceOffset;
				Uint64 gnmSurfaceSize;
				sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&gnmSurfaceOffset, &gnmSurfaceSize, &td.m_texture, mip_i, 0);

				Uint8* data = tex_data + gnmSurfaceOffset;

				{
					sce::GpuAddress::SurfaceInfo info;
					sce::GpuAddress::TilingParameters tiling;

					tiling.initFromTexture( &td.m_texture, mip_i, 0 );

					sce::GpuAddress::computeSurfaceInfo( &info, &tiling );

					info.m_pitch *= bytesPerPixel;

					RedVector4 value;
					RedVector2 coord;
					RedVector2 texelSize;

					for (Uint32 y = 0; y < tiling.m_linearHeight; y++)
					{
						coord.Y = ( y + 0.5f ) / tiling.m_linearHeight;

						for (Uint32 x = 0; x < tiling.m_linearWidth; x++)
						{
							coord.X = ( x + 0.5f ) / tiling.m_linearHeight;

							funcFill(&value, &coord, &texelSize, nullptr);

							Uint8 *dst = data + y * info.m_pitch + x * bytesPerPixel;
							dst[0] = (Uint8)(value.X * 255);
							dst[1] = (Uint8)(value.Y * 255);
							dst[2] = (Uint8)(value.Z * 255);
							dst[3] = (Uint8)(value.W * 255);
						}
					}
				}

			}
		}

		typedef void (*Texture3DFillFunction)(RedVector4 *pOut, const RedVector3 *pTexCoord);

		void InitInternalTextureDataCUBE( STextureData &td, Uint32 size, Texture3DFillFunction funcFill )
		{
			Uint32 width = size;
			Uint32 height = size;

			td.m_Desc.type	 = TEXTYPE_CUBE;
			td.m_Desc.width  = width;
			td.m_Desc.height = height;
			td.m_Desc.format = TEXFMT_R8G8B8A8;
			td.m_Desc.usage	 = TEXUSAGE_Samplable;
			td.m_Desc.initLevels = 1;

			sce::Gnm::DataFormat format = sce::Gnm::kDataFormatB8G8R8A8Unorm;
			sce::Gnm::SizeAlign sizeAlign = td.m_texture.initAsCubemap( width, height, 1, format, sce::Gnm::kTileModeDisplay_LinearAligned );

			Uint32 bytesPerPixel = 4;
			Uint32 pitch = td.m_texture.getPitch() * bytesPerPixel;
			{ // create init data
				RedVector4 value;
				RedVector3 coord, texelSize;
				Uint8* pos;

				Red::MemoryFramework::MemoryRegionHandle memoryHandle = GpuApi::AllocateInPlaceMemoryRegion( INPLACE_Texture, sizeAlign.m_size, GpuApi::MC_InternalTexture, sizeAlign.m_align, Red::MemoryFramework::Region_Longlived );
				Uint8* data = (Uint8*) memoryHandle.GetRawPtr();
				td.m_memoryRegion = memoryHandle;
				td.m_texture.setBaseAddress( data );
				td.m_Desc.inPlaceType = INPLACE_Texture;

				for ( Uint32 face = 0; face < 6; ++face )
				{
					Uint64 surfaceOffset, surfaceSize;
					Uint32 ret = sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&surfaceOffset, &surfaceSize, &td.m_texture, 0, face);
					GPUAPI_ASSERT(ret == sce::GpuAddress::kStatusSuccess);
					RED_UNUSED(ret);
					GPUAPI_ASSERT(surfaceSize <= (size * size * bytesPerPixel));

					coord.Z = (Float)face;
					for (Uint32 y = 0; y < size; y++)
					{
						coord.Y = (y + 0.5f) / size;

						for (Uint32 x = 0; x < size; x++)
						{
							coord.X = (x + 0.5f) / size;

							funcFill( &value, &coord );

							pos = data + surfaceOffset + (y * pitch) + (x * bytesPerPixel);

							pos[0] = (Uint8)(value.X * 255);
							pos[1] = (Uint8)(value.Y * 255);
							pos[2] = (Uint8)(value.Z * 255);
							pos[3] = (Uint8)(value.W * 255);
						}
					}
				}
			}
		}

		RED_FORCE_INLINE void DissolveCoordStep( Uint8 &value, Uint8 &outX, Uint8 &outY )
		{
			Uint8 v = (value&3);
			value >>= 2;
			Uint8 dx[] = { 0, 1, 1, 0 };
			Uint8 dy[] = { 0, 1, 0, 1 };
			outX = (outX<<1) + dx[v];
			outY = (outY<<1) + dy[v];
		}

		RED_FORCE_INLINE void DissolveCoord( Uint8 value, Uint8 &outX, Uint8 &outY )
		{
			outX = 0;
			outY = 0;
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
			DissolveCoordStep( value, outX, outY );
		}

		RED_FORCE_INLINE Uint8 DissolveValueForCoord( Uint32 x, Uint32 y )
		{
			GPUAPI_ASSERT( x < 16 && y < 16 );

			// lame (suboptimal)

			Uint8 val = 0;
			for ( Uint32 i=0; i<256; ++i )
			{
				Uint8 _x, _y;
				DissolveCoord( (Uint8)i, _x, _y );
				if ( x==_x && y==_y )
				{
					val = (Uint8)i;
					break;
				}
			}

			return val;
		}

		// Dissolve texture generator
		void GenerateDissolveTexture( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			Uint32 res = GPUAPI_DISSOLVE_TEXTURE_SIZE;
			Uint32 x = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->X), 0, res-1 );
			Uint32 y = Red::Math::NumericalUtils::Clamp< Uint32 >( ( Uint32 ) (res * pTexCoord->Y), 0, res-1 );

			// get dissolve value
			Uint8 val = DissolveValueForCoord( x, y );

			// clamp values so that 0.f clip threshold will give up full opacity, and 1.f full transparency
			val = Red::Math::NumericalUtils::Clamp< Uint8 >( val, 1, 255 );

			// build result
			*pOut = RedVector4(val / 256.f,val / 256.f,val / 256.f,val / 256.f);
		}

		// Poisson rotation texture generator
		void GeneratePoissonRotationTexture( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{	
			RED_UNUSED( pTexCoord );

			Float angle = GRandomNumberGenerator.Get< Float >( 360.0f ); // DissolveValueForCoord( x, y ) * (360.f / 256.f);
			Float dx	= sinf( DEG2RAD( angle ) );
			Float dy	= cosf( DEG2RAD( angle ) );

			pOut->X = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
			pOut->Y = Red::Math::NumericalUtils::Clamp(  dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->Z = Red::Math::NumericalUtils::Clamp( -dy * 0.5f + 0.5f,	0.f, 1.f );
			pOut->W = Red::Math::NumericalUtils::Clamp(  dx * 0.5f + 0.5f,	0.f, 1.f );
		}

		/// Xor fill
		void BlankTextureFill( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			*pOut = RedVector4( 0.0f, 0.0f, 0.0f, 0.0f );
		}
		
		void FlatNormalTextureFill( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			*pOut = RedVector4( 1.0f, 0.5f, 0.5f, 0.0f );
		}

		/// Xor fill
		void DefaultTextureFill( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			Uint32 x = (Uint32)(pTexCoord->X * 255);
			Uint32 y = (Uint32)(pTexCoord->Y * 255);
			*pOut = RedVector4( x / 255.0f, 0.0f, y / 255.0f, 1.0f );
		}

		// Cube fill
		void DefaultCubeTextureFill( RedVector4 *pOut, const RedVector3 *pTexCoord )
		{
			if ( Red::Math::MAbs(pTexCoord->Z) > 2.5f )
			{
				if ( Red::Math::MAbs(pTexCoord->Z) > 3.5f )
				{
					if ( pTexCoord->Z > 4.5f )
					{
						*pOut = RedVector4( 1.0f, 0.5f, 0.5f, 1.0f );
					}
					else
					{
						*pOut = RedVector4( 0.0f, 0.5f, 0.5f, 1.0f );
					}
				}
				else
				{
					*pOut = RedVector4( 0.5f, 0.5f, 1.0f, 1.0f );
				}
			}
			else
			{
				if ( Red::Math::MAbs(pTexCoord->Z) > 0.5f )
				{
					if ( pTexCoord->Z > 1.5f )
					{
						*pOut = RedVector4( 0.5f, 1.0f, 0.5f, 1.0f );
					}
					else
					{
						*pOut = RedVector4( 0.5f, 0.0f, 0.5f, 1.0f );
					}
				}
				else
				{
					*pOut = RedVector4( 0.5f, 0.5f, 0.0f, 1.0f );
				}
			}
		}

		// SSAO rotation generator
		void GenerateSSAORotationNoise( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			const Uint8 values[16][4] =
			{
				{ 129, 253, 0, 0 },
				{  69,  69, 0, 0 },
				{   9, 221, 0, 0 },
				{ 205, 237, 0, 0 },
				{  85,  57, 0, 0 },
				{ 145,   9, 0, 0 },
				{ 221,  25, 0, 0 },
				{  25,  41, 0, 0 },
				{  41, 117, 0, 0 },
				{ 101, 205, 0, 0 },
				{ 161,  85, 0, 0 },
				{ 237, 101, 0, 0 },
				{ 253, 177, 0, 0 },
				{  57, 129, 0, 0 },
				{ 117, 145, 0, 0 },
				{ 177, 161, 0, 0 }
			};

			Uint32 res       = 4;
			Uint32 x         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->X), 0, res-1 );
			Uint32 y         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->Y), 0, res-1 );
			const Uint8 *v = values[x + y * res];

			Float fX		= v[0] - 127.5f;
			Float fY		= v[1] - 127.5f;
			Float fZ		= v[2] - 127.5f;
			Float fLen		= sqrtf( fX*fX + fY*fY + fZ*fZ );
			Float fLenInv	= fLen > 0.0001f ? 1.f / fLen : 0.f;
			Float fMul		= 0.5f * fLenInv;

			pOut->X = fMul * fX + 0.5f;
			pOut->Y = fMul * fY + 0.5f;
			pOut->Z = fMul * fZ + 0.5f;
			pOut->W = 0;
		}


		// MipNoise
		Red::Math::Random::Generator< Red::Math::Random::StandardRand >& RefMipNoiseRandomGen()
		{
			static Red::Math::Random::Generator< Red::Math::Random::StandardRand > gen;
			return gen;
		}

		void GenerateMipNoise( RedVector4* pOut, const RedVector2* pTexCoord, const RedVector2* pTexelSize, void* pData )
		{
			RED_UNUSED( pTexCoord );
			RED_UNUSED( pTexelSize );
			RED_UNUSED( pData );

			//Uint32 res       = GPUAPI_MIP_NOISE_TEXTURE_SIZE;
			//Uint32 x         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->x), 0, res-1 );
			//Uint32 y         = Red::Math::NumericalUtils::Clamp<Uint32>( (Uint32) (res * pTexCoord->y), 0, res-1 );

			const Float randomValue = RefMipNoiseRandomGen().Get( 0.f, 1.f );

			pOut->X = randomValue;
			pOut->Y = randomValue;
			pOut->Z = randomValue;
			pOut->W = 0;
		}




		// internal helper function
		Int32 GetDoubleBufferIndex()
		{
			return (FrameIndex() % 2);
		}


		Bool ShouldCountTextureStats( const TextureDesc& desc )
		{
			// Only count regular samplable textures (not render targets, dynamic textures, etc)
			return ( ( desc.usage & ~TEXUSAGE_Immutable ) == GpuApi::TEXUSAGE_Samplable );
		}

	}



	// ----------------------------------------------------------------------
	// TextureDesc

	TextureDesc::TextureDesc ()
		: type( TEXTYPE_2D )
		, width( 0 )
		, height( 0 )
		, initLevels( 0 )
		, sliceNum ( 1 )
		, usage( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, msaaLevel( 0 )
		, esramOffset( -1 )
		, esramSize( 0 )
		, inPlaceType( INPLACE_None )
	{}

	Uint32 TextureDesc::CalcTargetSlices() const
	{
		return type == TEXTYPE_CUBE ? sliceNum * 6 : sliceNum;
	}

	Uint32 TextureDesc::CalcTargetLevels() const
	{
		return initLevels > 0 ? initLevels : (1 + Red::Math::MLog2( Red::Math::NumericalUtils::Max( width, height ) ));
	}

	Bool TextureDesc::HasMips() const
	{
		Bool hasMips = (initLevels > 1 || ( 0 == initLevels && Red::Math::NumericalUtils::Max( width, height ) >= 2 ) );
		GPUAPI_ASSERT( hasMips == (CalcTargetLevels() > 1) );
		return hasMips;
	}

	Bool TextureDesc::operator==( const TextureDesc &other ) const
	{
		return
			type == other.type &&
			width == other.width &&
			height == other.height &&
			initLevels == other.initLevels &&
			sliceNum == other.sliceNum &&
			usage == other.usage &&
			format == other.format &&
			msaaLevel == other.msaaLevel &&
			esramOffset == other.esramOffset &&
			esramSize == other.esramSize &&
			inPlaceType == other.inPlaceType;
	}

	// ----------------------------------------------------------------------

	TextureDataDesc::TextureDataDesc()
		: width( 0 )
		, height( 0 )
		, format( TEXFMT_R8G8B8A8 )
		, rowPitch( 0 )
		, slicePitch( 0 )
		, data( NULL )
	{}

	// ----------------------------------------------------------------------

	void AddRef( const TextureRef &texture )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );
		GetDeviceData().m_Textures.IncRefCount( texture );
	}

	Int32 Release( const TextureRef &texture )
	{
#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
		if ( GetDeviceData().m_DeviceShutDone )
		{
			GPUAPI_ASSERT( !GetDeviceData().m_Textures.IsInUse(texture) );
			return;
		}
#endif

		//

		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(texture) );

		TextureRef parentRef;
		Int32 refCount = 0;
		// Release given texture
		{
			SDeviceData &dd = GetDeviceData();
			
			// Release and optionally destroy
			GPUAPI_ASSERT( texture );
			GPUAPI_ASSERT( dd.m_Textures.GetRefCount( texture ) >= 1 );
			refCount = dd.m_Textures.DecRefCount( texture );
			if ( 0 == refCount )
			{	
				STextureData &data = dd.m_Textures.Data( texture );

				// Take ownership over parent reference (will be released later)
				parentRef = data.parentTexId;
				data.parentTexId = TextureRef::Null();
				
				// TODO: stats shouldn't be handled on final, but they're used for budeting
				// Count memory usage
				if ( Utils::ShouldCountTextureStats( data.m_Desc ) )
				{
					const Int32 textureMemory = CalcTextureSize( data.m_Desc );
					dd.m_TextureStats.RemoveTexture( textureMemory, data.m_Group );
				}

				// queue for destruction once the GPU has finished with it
				GpuApi::QueueForDestroy(texture);
			}
		}

		// Release parent reference
		if ( parentRef )
		{
			Release( parentRef );
			parentRef = TextureRef::Null();
		}
		return refCount;
	}

	// called when a Released texture is no longer in use to delete the memory
	void Destroy(const TextureRef& textureRef)
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Textures.GetRefCount(textureRef) == 0 );
		STextureData& textureData = dd.m_Textures.Data(textureRef);

		// Don't free memory for a texture alias. Original texture may still be in use, and anyways it'll get
		// double-freed when the original texture is destroyed.
		if ( !textureData.m_isTextureAlias )
		{
			if (textureData.m_Desc.IsInPlace() )
			{
				ReleaseInPlaceMemoryRegion( textureData.m_Desc.inPlaceType, textureData.m_memoryRegion );
				GPUAPI_ASSERT( ( textureData.m_Desc.usage & TEXUSAGE_RenderTarget ) == 0, TXT("InPlace and RenderTarget at the same time - that will cause a leak now.") )
			}
			else
			{
				if (textureData.m_Desc.usage & TEXUSAGE_Dynamic)
				{
					GPU_API_FREE(GpuMemoryPool_BuffersDynamic, MC_DynamicTextureBuffer, textureData.m_dynamicBuffers[0]);
					GPU_API_FREE(GpuMemoryPool_BuffersDynamic, MC_DynamicTextureBuffer, textureData.m_dynamicBuffers[1]);
				}
				else if (textureData.m_Desc.usage & TEXUSAGE_Staging)
				{
					GPU_API_FREE(GpuMemoryPool_BuffersDynamic, MC_StagingTextureBuffer, textureData.m_texture.getBaseAddress());
				}
				else if( textureData.m_Desc.usage & TEXUSAGE_RenderTarget )
				{
					// allocate memory for total texture
					GPU_API_FREE_REGION(GpuMemoryPool_RenderTargets, textureData.m_memoryRegion);

					for (int j = 0; j < textureData.m_aliasedAsRenderTargetsSize; ++j)
					{
						// sometimes cmask seems to have a valid address even when not initialised so use
						// this to check to see if we need to free the pointer or not
						if (textureData.m_aliasedAsRenderTargets[j].getCmaskFastClearEnable())
						{
							void* cmaskAddr = textureData.m_aliasedAsRenderTargets[j].getCmaskAddress();
							if (cmaskAddr)
							{
								GPU_API_FREE(GpuMemoryPool_DefaultGarlic, MC_CMask, cmaskAddr);
							}
						}
					}
					delete [] textureData.m_aliasedAsRenderTargets;
					textureData.m_aliasedAsRenderTargets = nullptr;
				}
				else if ( textureData.m_Desc.usage & TEXUSAGE_DepthStencil )
				{
					GPU_API_FREE_REGION(GpuMemoryPool_RenderTargets, textureData.m_memoryRegion);
				
					// rendertarget memory is shared with texture, and so is depth target, although stencil data is currently separate
					GPUAPI_ASSERT( textureData.m_aliasedAsDepthStencilsSize == 1, TXT("DepthStencil target with mips is not fully implemented!!!") );
					GPU_API_FREE_REGION(GpuMemoryPool_RenderTargets, textureData.m_memoryRegionSecondary);
					for (int j = 0; j < textureData.m_aliasedAsDepthStencilsSize; ++j)
					{
						void* ptr = textureData.m_aliasedAsDepthStencils[j].getHtileAddress();
						if (ptr)
						{
							GPU_API_FREE(GpuMemoryPool_DefaultGarlic, MC_HTile, ptr);
						}
					}
					delete [] textureData.m_aliasedAsDepthStencils;
					textureData.m_aliasedAsDepthStencils = nullptr;
				}
				else
				{
					GPU_API_FREE_REGION(GpuMemoryPool_Textures, textureData.m_memoryRegion);
				}
			}
		}

		dd.m_Textures.Destroy(textureRef);
	}


	void GetTextureDesc( const TextureRef &ref, TextureDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		outDesc = GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	const TextureDesc& GetTextureDesc( const TextureRef &ref )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		return GetDeviceData().m_Textures.Data(ref).m_Desc;
	}

	void SetTextureDebugPath( const TextureRef &ref, const char* debugPath )
	{
#ifdef GPU_API_DEBUG_PATH
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		STextureData &data = GetDeviceData().m_Textures.Data(ref);
		Red::System::StringCopy( data.m_debugPath, debugPath, ARRAY_COUNT(data.m_debugPath) );
#endif
	}

	void GetTextureLevelDesc( const TextureRef &ref, Uint16 level, TextureLevelDesc &outDesc )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse(ref) );
		const STextureData &data = GetDeviceData().m_Textures.Data(ref);

		outDesc.width	= CalculateTextureMipDimension( data.m_Desc.width, level, data.m_Desc.format );
		outDesc.height	= CalculateTextureMipDimension( data.m_Desc.height, level, data.m_Desc.format );


		//GPUAPI_ASSERT( level < data.m_Desc.CalcTargetLevels() );

		//// Get d3d texture level desc
		//D3DSURFACE_DESC d3dDesc;
		//switch ( data.m_Desc.type )
		//{
		//case TEXTYPE_2D:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture || data.m_pSurface );
		//		if ( data.m_pTexture )
		//		{
		//			static_cast<IDirect3DTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//		}
		//		else
		//		{
		//			data.m_pSurface->GetDesc( &d3dDesc );
		//		}
		//	}
		//	break;

		//case TEXTYPE_CUBE:
		//	{
		//		GPUAPI_ASSERT( data.m_pTexture );
		//		static_cast<IDirect3DCubeTexture9*>(data.m_pTexture)->GetLevelDesc( level, &d3dDesc );
		//	}
		//	break;

		//default:
		//	GPUAPI_HALT( TXT( "invalid" ) );
		//	return; // keep compiler happy (uninit variable)
		//}

		//// Get data
		//GPUAPI_ASSERT( d3dDesc.Width <= data.m_Desc.width && d3dDesc.Height <= data.m_Desc.height );
		//outDesc.width = d3dDesc.Width;
		//outDesc.height = d3dDesc.Height;		
	}

	TextureLevelDesc GetTextureLevelDesc( const TextureRef &ref, Uint16 level )
	{
		TextureLevelDesc desc;
		GetTextureLevelDesc( ref, level, desc );
		return desc;
	}

	const TextureStats *GetTextureStats()
	{
		return &GpuApi::GetDeviceData().m_TextureStats;
	}

	TextureRef CreateTextureAlias( const TextureDesc &desc, const TextureRef& textureRef )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}

		if ( textureRef.isNull() || !IsDescSupported( desc ) )
		{
			return TextureRef::Null();
		}

		// Allocate resource
		SDeviceData &dd = GetDeviceData();
		Uint32 texId = dd.m_Textures.Create( 1 );
		if ( !texId )
		{
			return TextureRef::Null();
		}

		const STextureData& srcTexture = dd.m_Textures.Data(textureRef);

		// Create texture
		sce::Gnm::Texture gnmTex;
		sce::Gnm::SizeAlign gnmSizeAlign;
		GPUAPI_ASSERT( TEXTYPE_2D == desc.type || TEXTYPE_CUBE == desc.type || TEXTYPE_ARRAY == desc.type );
		Utils::TextureFactoryGNM( GetDeviceData().m_Caps, desc, nullptr, &gnmTex, &gnmSizeAlign );

		// D3THACK to get around the fact that by default all 2d textures are Linear
		// When we are creating a 32_32 alias for a BC1 texture it must have the same tilemode
		// Note the reverse hack which is if it's a BC Staging texture then assume it's Linear :|
		if (srcTexture.m_texture.getTileMode() != sce::Gnm::kTileModeDisplay_LinearAligned && (desc.usage & TEXUSAGE_Staging)==0)
		{
			gnmTex.setTileMode(sce::Gnm::kTileModeThin_1dThin);
		}

		// Init resource
		GpuApi::STextureData &data = dd.m_Textures.Data( texId );
		data.m_texture = gnmTex;
		data.m_Desc = desc;
		data.m_Group = srcTexture.m_Group;
		data.m_dynamicBuffers[0] = nullptr;
		data.m_dynamicBuffers[1] = nullptr;
		data.m_memorySize = gnmSizeAlign.m_size;
		data.m_isTextureAlias = true;

		// Initialize handle
		GPUAPI_ASSERT( texId && dd.m_Textures.GetRefCount(texId) );
		TextureRef texRef( texId );

		// re-use the texture pointer from the src
		data.m_texture.setBaseAddress(srcTexture.m_texture.getBaseAddress());

		return texRef;
	}

	TextureRef CreateTexture( const TextureDesc &desc, eTextureGroup group, const TextureInitData* initData /*= nullptr*/ )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}
			
		if ( !IsDescSupported( desc ) )
		{
			return TextureRef::Null();
		}

		// Creating an immutable texture requires initial data.
		if ( ( desc.usage & TEXUSAGE_Immutable ) != 0 )
		{
			if ( initData == nullptr )
			{
				GPUAPI_HALT( "Creating immutable texture without any initial data." );
				return TextureRef::Null();
			}
		}

		if ( desc.IsInPlace() )
		{
			if ( ( desc.usage & TEXUSAGE_Immutable ) == 0 )
			{
				GPUAPI_HALT( "In-place texture creation is only supported for immutable textures." );
				return TextureRef::Null();
			}

			// Creating in-place requires a full cooked buffer.
			if ( initData == nullptr || !initData->m_isCooked || !initData->m_cookedData.IsValid() )
			{
				GPUAPI_HALT( "Creating in-place texture without a cooked init buffer." );
				return TextureRef::Null();
			}
		}

		if ( initData != nullptr && initData->m_isCooked )
		{
			if ( ( desc.usage & TEXUSAGE_Immutable ) == 0 )
			{
				GPUAPI_HALT( "Trying to create a non-immutable texture from cooked init data." );
				return TextureRef::Null();
			}
		}


		// Create texture
		sce::Gnm::Texture gnmTex;
		sce::Gnm::SizeAlign gnmSizeAlign;
		GPUAPI_ASSERT( TEXTYPE_2D == desc.type || TEXTYPE_CUBE == desc.type || TEXTYPE_ARRAY == desc.type );
		Utils::TextureFactoryGNM( GetDeviceData().m_Caps, desc, nullptr, &gnmTex, &gnmSizeAlign );

		// Allocate resource
		SDeviceData &dd = GetDeviceData();
		Uint32 texId = dd.m_Textures.Create( 1 );
		if ( !texId )
		{
			return TextureRef::Null();
		}

		// Init resource
		GpuApi::STextureData &data = dd.m_Textures.Data( texId );
		data.m_doCmask = false;
		data.m_texture = gnmTex;
		data.m_Desc = desc;
		data.m_Group = group;
		data.m_dynamicBuffers[0] = nullptr;
		data.m_dynamicBuffers[1] = nullptr;
		data.m_memorySize = gnmSizeAlign.m_size;

		// Initialize handle
		GPUAPI_ASSERT( texId && dd.m_Textures.GetRefCount(texId) );
		TextureRef texRef( texId );

		if ( desc.IsInPlace() )
		{
			data.m_texture.setBaseAddress( const_cast< void* >( initData->m_cookedData.GetRawPtr() ) );
			data.m_memoryRegion = initData->m_cookedData;
		}
		else if(desc.usage & TEXUSAGE_RenderTarget)
		{
			GPUAPI_ASSERT((desc.usage & TEXUSAGE_DepthStencil) == 0);
			GPUAPI_ASSERT(desc.type != TEXTYPE_ARRAY || desc.sliceNum >= 1, TXT ("CreateTexture Rendertarget( TEXTYPE_ARRAY ) sliceNum == %d"), desc.sliceNum);
			GPUAPI_ASSERT(desc.type != TEXTYPE_2D || desc.sliceNum == 1, TXT ("CreateTexture Rendertarget( TEXTYPE_2D ) sliceNum == %d"), desc.sliceNum);

			// allocate memory for total texture
			Red::MemoryFramework::MemoryClass memClass = MC_RenderTarget;
#ifdef DETAILED_RENDER_TARGET_MEM_CLASS
			memClass = DetailedRenderTargetMemClass( desc.type, group );
#endif

			Red::MemoryFramework::RegionLifetimeHint lifeTimeHint = Red::MemoryFramework::Region_Longlived;
			if ( group == TEXG_UI || group == TEXG_Envprobe ) lifeTimeHint = Red::MemoryFramework::Region_Shortlived;

			data.m_memoryRegion = GPU_API_ALLOCATE_REGION( GpuMemoryPool_RenderTargets, memClass, gnmSizeAlign.m_size, gnmSizeAlign.m_align, lifeTimeHint );
			data.m_texture.setBaseAddress( data.m_memoryRegion.GetRawPtr() );
			GPU_API_UNLOCK_REGION( GpuMemoryPool_RenderTargets, data.m_memoryRegion );	// we dont need to keep RT textures locked

			Uint16 numMips = desc.initLevels;
			data.m_aliasedAsRenderTargets = new sce::Gnm::RenderTarget [numMips];
			data.m_aliasedAsRenderTargetsSize = numMips;

			// alias each mip to the correct texture address
			for (Uint16 i = 0; i < numMips; ++i)
			{
#if 0	// MG: This code path is disabled because it doesn't really help our render targets and there are still some issues when using this.
				sce::Gnm::SizeAlign cmaskSizeAlign, *cmaskSizeAlignPtr = nullptr;
				void* cmaskAddr = nullptr;

				sce::Gnm::RenderTarget& aliasRT = data.m_aliasedAsRenderTargets[i];
				sce::Gnm::RenderTarget tempRT;

				const Int32 initResult = aliasRT.initFromTexture( &data.m_texture, i );
				if ( initResult != 0 )
				{
					GPUAPI_HALT( "Failed to create renderTarget from Texture!: 0x%08x", initResult );
				}				

				if ( numMips == 1 && desc.type == TEXTYPE_2D && desc.usage & TEXUSAGE_FastColorClear )
				{
					data.m_doCmask = true;
					cmaskSizeAlignPtr = &cmaskSizeAlign;
					tempRT.init( aliasRT.getWidth(), aliasRT.getHeight(), 1, aliasRT.getDataFormat(), aliasRT.getTileMode(), aliasRT.getNumSamples(), aliasRT.getNumFragments(), cmaskSizeAlignPtr, nullptr );	
					if ( cmaskSizeAlign.m_size > 0 )
					{
						cmaskAddr = GPU_API_ALLOCATE( GpuMemoryPool_DefaultGarlic, MC_CMask, cmaskSizeAlign.m_size, cmaskSizeAlign.m_align );
					}
 					aliasRT.setCmaskFastClearEnable( true );
					aliasRT.setCmaskAddress( cmaskAddr );
					aliasRT.setCmaskSliceNumBlocksMinus1( tempRT.getCmaskSliceNumBlocksMinus1() );
				}
#else
				const Int32 initResult = data.m_aliasedAsRenderTargets[i].initFromTexture(&data.m_texture, i);
				if (initResult != 0)
				{
					GPUAPI_HALT("Failed to create renderTarget from Texture!: 0x%08x", initResult);
				}
#endif
			}
		}
		else if ( desc.usage & TEXUSAGE_DepthStencil )
		{
			GPUAPI_ASSERT((desc.usage & TEXUSAGE_RenderTarget) == 0);
			GPUAPI_ASSERT(desc.type != TEXTYPE_ARRAY || desc.sliceNum >= 1, TXT ("CreateTexture Rendertarget( TEXTYPE_ARRAY ) sliceNum == %d"), desc.sliceNum);
			GPUAPI_ASSERT(desc.type != TEXTYPE_2D || desc.sliceNum == 1, TXT ("CreateTexture Rendertarget( TEXTYPE_2D ) sliceNum == %d"), desc.sliceNum);

			// allocate memory for total texture
			Red::MemoryFramework::MemoryClass memClass = MC_DepthStencilTarget;
#ifdef DETAILED_RENDER_TARGET_MEM_CLASS
			memClass = DetailedDepthStencilMemClass( desc.type, group );
#endif

			Red::MemoryFramework::RegionLifetimeHint lifeTimeHint = Red::MemoryFramework::Region_Longlived;
			if ( group == TEXG_UI )
			{
				lifeTimeHint = Red::MemoryFramework::Region_Shortlived;
			}

			data.m_memoryRegion = GPU_API_ALLOCATE_REGION( GpuMemoryPool_RenderTargets, memClass, gnmSizeAlign.m_size, gnmSizeAlign.m_align, lifeTimeHint );
			data.m_texture.setBaseAddress( data.m_memoryRegion.GetRawPtr() );
			GPU_API_UNLOCK_REGION( GpuMemoryPool_RenderTargets, data.m_memoryRegion );	// we dont need to keep RT textures locked

			sce::Gnm::ZFormat gnm_zformat = (desc.format == TEXFMT_D16U) ? sce::Gnm::kZFormat16 : sce::Gnm::kZFormat32Float;
			Bool hasStencil = (desc.format == TEXFMT_D24S8 || desc.format == TEXFMT_D24FS8);

			Uint16 numMips = desc.initLevels;
			data.m_aliasedAsDepthStencils = new sce::Gnm::DepthRenderTarget [numMips];
			data.m_aliasedAsDepthStencilsSize = numMips;

			GPUAPI_ASSERT (numMips == 1, TXT("Depth target with mips has not been tested!"));

			for (Uint16 mip = 0; mip < numMips; ++mip)
			{
				sce::Gnm::SizeAlign stencilTargetSizeAlign;
				sce::Gnm::SizeAlign htileSizeAlign(0, 0);

				Uint32 w = data.m_texture.getWidth() << mip;
				Uint32 h = data.m_texture.getHeight() << mip;
				sce::Gnm::SizeAlign sizeAlign = data.m_aliasedAsDepthStencils[mip].init(
					w, 
					h, 
					desc.sliceNum, 				
					gnm_zformat,
					hasStencil ? sce::Gnm::kStencil8 : sce::Gnm::kStencilInvalid,
					data.m_texture.getTileMode(),
					sce::Gnm::kNumFragments1,
					hasStencil ? &stencilTargetSizeAlign : nullptr,
					(desc.usage & TEXUSAGE_NoDepthCompression) ? nullptr : &htileSizeAlign);
					//nullptr);			// switch to this line to disable HTILE

				Uint32 slice = 0;
				Uint64 offset;
				Uint64 size;
				Uint32 status = sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &offset, &size, &data.m_texture, mip, slice );
				GPUAPI_ASSERT (status == sce::GpuAddress::kStatusSuccess);
				RED_UNUSED(status);
				GPUAPI_ASSERT (sizeAlign.m_size == size, TXT( "%d - %d" ), sizeAlign.m_size, size ); // is this test correct?
				RED_UNUSED(sizeAlign);

				void* depthAddress = ((Uint8*)data.m_texture.getBaseAddress()) + offset;
				void* stencilAddress = nullptr;
				if (hasStencil)
				{
					data.m_memoryRegionSecondary = GPU_API_ALLOCATE_REGION( GpuMemoryPool_RenderTargets, memClass, stencilTargetSizeAlign.m_size, stencilTargetSizeAlign.m_align, lifeTimeHint );
					stencilAddress = data.m_memoryRegionSecondary.GetRawPtr();
					GPU_API_UNLOCK_REGION( GpuMemoryPool_RenderTargets, data.m_memoryRegionSecondary );
				}

				data.m_aliasedAsDepthStencils[mip].setAddresses(depthAddress, stencilAddress );

				if (htileSizeAlign.m_size > 0)
				{
					void* htileAddress = GPU_API_ALLOCATE( GpuMemoryPool_DefaultGarlic, MC_HTile, htileSizeAlign.m_size, htileSizeAlign.m_align );
					data.m_aliasedAsDepthStencils[mip].setHtileAddress(htileAddress);

					data.m_needsInitialClear = true;

					// disable HiStencil for now - it improves HiZ perf and is simpler to implement (dont have to track stencil clear values)
					data.m_aliasedAsDepthStencils[mip].setHtileStencilDisable( !hasStencil );
				}
			}

		}
		else if (desc.usage & TEXUSAGE_Dynamic)
		{
			// allocate double buffer for dynamic textures

			data.m_dynamicBuffers[0] = GPU_API_ALLOCATE( GpuMemoryPool_BuffersDynamic, MC_DynamicTextureBuffer, gnmSizeAlign.m_size, gnmSizeAlign.m_align );
			data.m_dynamicBuffers[1] = GPU_API_ALLOCATE( GpuMemoryPool_BuffersDynamic, MC_DynamicTextureBuffer, gnmSizeAlign.m_size, gnmSizeAlign.m_align );
			data.m_texture.setBaseAddress( data.m_dynamicBuffers[0] );
		}
		else if (desc.usage & TEXUSAGE_Staging)
		{
			void* baseAddress = GPU_API_ALLOCATE( GpuMemoryPool_BuffersDynamic, MC_StagingTextureBuffer, gnmSizeAlign.m_size, gnmSizeAlign.m_align );
			data.m_texture.setBaseAddress( baseAddress );
		}
		else
		{
			data.m_memoryRegion = GPU_API_ALLOCATE_REGION( GpuMemoryPool_Textures, MC_TextureData, gnmSizeAlign.m_size, gnmSizeAlign.m_align, Red::MemoryFramework::Region_Longlived );
			data.m_texture.setBaseAddress( data.m_memoryRegion.GetRawPtr() );
			GPU_API_UNLOCK_REGION( GpuMemoryPool_Textures, data.m_memoryRegion );	// These shouldn't be moved
		}


		if ( !desc.IsInPlace() && initData != nullptr )
		{
			// Now copy over any initial data into the texture's memory. We know the texture cannot be used yet, so we don't have to
			// worry about synchronization or anything like that.

			STextureData& texData = GetDeviceData().m_Textures.Data( texRef );

			// If we have fully cooked data, just copy it over.
			if ( initData->m_isCooked )
			{
				size_t totalSize;
				Uint32 align;
				sce::GpuAddress::computeTotalTiledTextureSize( &totalSize, &align, &texData.m_texture );
				Red::System::MemoryCopy( texData.m_texture.getBaseAddress(), initData->m_cookedData.GetRawPtr(), totalSize );
			}
			// Otherwise, copy each chunk, handling possible mix of cooked/uncooked.
			else
			{
				Uint32 arraySize = desc.type == TEXTYPE_2D ? 1 : desc.sliceNum;
				if ( desc.type == TEXTYPE_CUBE ) arraySize *= 6;

				for ( Uint32 slice = 0; slice < arraySize; ++slice )
				{
					for ( Uint32 mip = 0; mip < desc.initLevels; ++mip )
					{
						GPUAPI_ASSERT( mip < texData.m_Desc.CalcTargetLevels() );

						Uint32 dataIndex = mip + slice * desc.initLevels;

						Uint64 offset;
						Uint64 size;
						sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &offset, &size, &texData.m_texture, mip, slice );

						if ( initData->m_mipsInitData[dataIndex].m_isCooked )
						{
							Red::System::MemoryCopy( ((Uint8*)texData.m_texture.getBaseAddress()) + offset, initData->m_mipsInitData[dataIndex].m_data, size );
						}
						else
						{
							sce::GpuAddress::TilingParameters tilingParams;
							tilingParams.initFromTexture( &texData.m_texture, mip, slice );

							sce::GpuAddress::tileSurface( ((Uint8*)texData.m_texture.getBaseAddress()) + offset, initData->m_mipsInitData[dataIndex].m_data, &tilingParams );
						}
					}
				}
			}
		}


		// TODO: stats shouldn't be handled on final, but they're used for budeting
		// If this is a simple texture, count used memory
		if ( Utils::ShouldCountTextureStats( data.m_Desc ) )
		{
			const Uint32 textureSize = CalcTextureSize( texRef );
			dd.m_TextureStats.AddTexture( textureSize, group );
		}

		return texRef;
	}

	void GetTextureDescFromMemoryFile( const void *memoryFile, Uint32 fileSize, TextureDesc *outDesc /* = NULL */ )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return;
		}
#ifndef RED_PLATFORM_CONSOLE
		bool isCube = false;
		TextureDesc localDesc;
		{
			DirectX::TexMetadata metaData;
			HRESULT res = DirectX::GetMetadataFromDDSMemory(memoryFile, fileSize, DirectX::DDS_FLAGS_NONE, metaData);

			localDesc.initLevels = static_cast< GpuApi::Uint32 >( metaData.mipLevels );
			localDesc.format     = GpuApi::Map( metaData.format );
			localDesc.height     = static_cast< GpuApi::Uint32 >( metaData.height );
			localDesc.width      = static_cast< GpuApi::Uint32 >( metaData.width );
			localDesc.type       = GpuApi::TEXTYPE_2D;
			localDesc.usage      = GpuApi::TEXUSAGE_Samplable;
			isCube				 = ((DirectX::TEX_MISC_TEXTURECUBE & metaData.miscFlags) != 0);
		}

		if ( !IsDescSupported( localDesc ) )
		{
			return;
		}
		if ( outDesc )
		{
			*outDesc = localDesc;
		}
#endif
	}

	TextureRef CreateTextureFromMemoryFile( const void *memoryFile, Uint32 fileSize, eTextureGroup group, TextureDesc *outDesc )
	{
		if ( !IsInit() )
		{
			// ace_fix!!!!! przywrocic ten fatal error (teraz jest bo niektore shity sa tak wrzucone w silniku)		
			// GPUAPI_HALT( TXT( "Not init during attempt to create texture" ) );
			return TextureRef::Null();
		}

		return CreateDDSTextureFromMemory( (Uint8*)memoryFile, fileSize );
	}

	Uint32 CalcTextureSize( const TextureDesc &texDesc )
	{
		return CalculateCookedTextureSize( texDesc );
		/*Uint32 totalPixels = 0;

		// Calculate number of pixels in texture
		// dex_todo: this does not account for alignment, compression, etc....
		const Uint32 levelsCount = texDesc.CalcTargetLevels();
		for ( Uint32 lvl_i=0; lvl_i<levelsCount; ++lvl_i )
		{				
			const Uint32 mipWidth = Red::Math::NumericalUtils::Max< Uint32 >( texDesc.width >> lvl_i, 1 );
			const Uint32 mipHeight = Red::Math::NumericalUtils::Max< Uint32 >( texDesc.height >> lvl_i, 1 );
			totalPixels += mipWidth * mipHeight;
		}

		// Cubemap has 6 faces :)
		GPUAPI_ASSERT( TEXTYPE_2D == texDesc.type || TEXTYPE_CUBE == texDesc.type || TEXTYPE_ARRAY == texDesc.type );
		if ( TEXTYPE_CUBE == texDesc.type )
		{
			totalPixels *= 6;
		}
		if ( TEXTYPE_ARRAY == texDesc.type )
		{
			totalPixels *= texDesc.sliceNum;
		}

		// Calculate 
		const Uint32 totalBits = totalPixels * Utils::GetTextureFormatPixelSize( texDesc.format );
		return totalBits / 8;*/
	}

	Uint32 CalcTextureSize( const TextureRef &tex )
	{
		if ( tex )
		{
			Uint32 memory_size = GetDeviceData().m_Textures.Data( tex ).m_memorySize;
			return memory_size;
		}

		return 0;
	}


	Bool CopyTextureData( const TextureRef& destRef, Uint32 destMipLevel, Uint32 destArraySlice, const TextureRef& srcRef, Uint32 srcMipLevel, Uint32 srcArraySlice )
	{
		// we cannot perform this on GPU with a memcpy unless many preconditions are met (tiling, padding, format, etc)
		// so tell the Renderer it should fallback to using CS or PS
		return false;
	}

	extern Bool g_IsInsideRenderBlock;


	static void DoDMA( SDeviceData& dd, sce::Gnmx::GfxContext& gfxc, void* dst, void* src, Uint32 numBytes, Bool blocking )
	{
		GPUAPI_FATAL_ASSERT( g_IsInsideRenderBlock, "DMAMemory must be called between Begin/EndRender" );
		GPUAPI_FATAL_ASSERT( ( (Uint8*)src >= (Uint8*)dst + numBytes ) || ( (Uint8*)dst >= (Uint8*)src + numBytes ), "Overlapped copy!" );

		gfxc.copyData( dst, src, numBytes, blocking ? sce::Gnm::kDmaDataBlockingEnable : sce::Gnm::kDmaDataBlockingDisable );

		{
			volatile uint64_t* label = (volatile uint64_t*)dd.m_constantBufferMem.Allocate( sizeof(uint64_t), 16 );
			*label = 0x0;
			gfxc.m_dcb.writeAtEndOfPipe( sce::Gnm::kEopFlushCbDbCaches, sce::Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label), sce::Gnm::kEventWriteSource64BitsImmediate, 0x1, sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kCachePolicyLru );
			gfxc.m_dcb.waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 );
		}
	}

	// A general purpose memory move that leverages DMA requests inserted into draw command buffer
	void DMAMemory( void* dst, void* src, Uint32 numBytes, Bool isBlocking )
	{
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		DoDMA( GetDeviceData(), gfxc, dst, src, numBytes, isBlocking );
	}

	void BatchedDMAMemory( SDMABatchElement* batch, Uint32 batchCount, Bool isBlocking )
	{
		GPUAPI_ASSERT( batchCount > 0, TXT("zero length batch is not supported") );

		SDeviceData& dd = GetDeviceData();
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		for ( Uint32 i = 0; i < batchCount-1; ++i)
		{
			DoDMA( dd, gfxc, batch[i].dst, batch[i].src, batch[i].size, false );
		}

		DoDMA( dd, gfxc, batch[batchCount-1].dst, batch[batchCount-1].src, batch[batchCount-1].size, isBlocking );
	}

Uint32 copyRectWithCompute = 0;
	// MG: Copying rect with compute turned out to be slower, because we have to flush CB cache before and l1 and l2 after.
	// TODO: add graphics-pipe based implementation, because we don't have copying in the unittests now.
	Bool CopyRect( const TextureRef &sourceRef, const Rect& sourceRect, Uint32 sourceArraySlice, const TextureRef &destRef, Uint32 destX, Uint32 destY, Uint32 destArraySlice )
	{
		if ( copyRectWithCompute == 0 )
		{
			return false;
		}
		
		/*
		MG note: This function is an "adapted" copy paste from Gnm toolkit. Hopefully someday this shit will just be part of Gnmx. This code seems pretty good though, may be a good idea to 
		introduce this into other GpuApi-s.
		This function copies the texture with the use of compute, supporting many different formats, which is a step up compared to DX12's CopyTextureRegion (not saying we desperately need that ;))
		*/

		SDeviceData &dd = GetDeviceData();
		SSwapChainData& swapChain = dd.m_SwapChains.Data( dd.m_SwapChainRef );
		sce::Gnmx::GfxContext& gfxc = swapChain.backBuffer->context;

		STextureData& sourceTexData = dd.m_Textures.Data( sourceRef );
		STextureData& destTexData = dd.m_Textures.Data( destRef );

		const TextureDesc& sourceDesc = sourceTexData.m_Desc;
		const TextureDesc& destDesc = destTexData.m_Desc;

		Bool isFullSource = sourceRect.left == 0 && sourceRect.top == 0 && sourceRect.right == (Int32)sourceDesc.width && sourceRect.bottom == (Int32)sourceDesc.height;
		Bool isFullDest = destX == 0 && destY == 0 && sourceRect.right == (Int32)destDesc.width && sourceRect.bottom == (Int32)destDesc.height;

		if ( !( sourceDesc.width == destDesc.width && sourceDesc.height == destDesc.height )
	      || !( sourceDesc.sliceNum == destDesc.sliceNum && sourceDesc.initLevels == destDesc.initLevels )
		  || !( isFullSource && isFullDest ) )
		{

			return false;
		}

		sce::Gnm::Texture* textureSrc = &sourceTexData.m_texture;
		sce::Gnm::Texture* textureDst = &destTexData.m_texture;
		GPUAPI_ASSERT( textureDst );
		GPUAPI_ASSERT( textureSrc );

		// If the data formats of the two textures are identical, we use a different shader that loads and stores the raw pixel bits directly, without any format conversion.
		// This not only preserves precision, but allows some additional tricks (such as copying otherwise-unwritable block-compressed formats by "spoofing" them as writable formats with identical
		// per-pixel sizes).
		Bool copyRawPixels = (textureDst->getDataFormat().m_asInt == textureSrc->getDataFormat().m_asInt);

		sce::Gnm::Texture textureDstCopy = *textureDst;
		sce::Gnm::Texture textureSrcCopy = *textureSrc;

		if(copyRawPixels)
		{
			sce::Gnm::DataFormat dataFormat = textureDstCopy.getDataFormat();
			switch(dataFormat.getSurfaceFormat())
			{
			case sce::Gnm::kSurfaceFormatBc1: 
			case sce::Gnm::kSurfaceFormatBc4:
				dataFormat.m_bits.m_channelType = sce::Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = sce::Gnm::kSurfaceFormat32_32;
				textureDstCopy.setWidthMinus1((textureDstCopy.getWidth() + 3) / 4 - 1);
				textureDstCopy.setHeightMinus1((textureDstCopy.getHeight() + 3) / 4 - 1);
				textureSrcCopy.setWidthMinus1((textureSrcCopy.getWidth() + 3) / 4 - 1);
				textureSrcCopy.setHeightMinus1((textureSrcCopy.getHeight() + 3) / 4 - 1);
				break;
			case sce::Gnm::kSurfaceFormatBc2: 
			case sce::Gnm::kSurfaceFormatBc3:
			case sce::Gnm::kSurfaceFormatBc5:
			case sce::Gnm::kSurfaceFormatBc6:
			case sce::Gnm::kSurfaceFormatBc7:
				dataFormat.m_bits.m_channelType = sce::Gnm::kTextureChannelTypeUInt;
				dataFormat.m_bits.m_surfaceFormat = sce::Gnm::kSurfaceFormat32_32_32_32;
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

		ShaderRef shader;

		switch(textureDst->getTextureType())
		{
		case sce::Gnm::kTextureType1d:
		case sce::Gnm::kTextureType1dArray:
			GPUAPI_ASSERT(textureSrc->getTextureType() == sce::Gnm::kTextureType1d || textureSrc->getTextureType() == sce::Gnm::kTextureType1dArray,
				TXT("textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X)."), textureDst->getTextureType(), textureSrc->getTextureType());
			shader = copyRawPixels ? dd.m_embeddedShaders.cs_copyrawtexture1d_c() : dd.m_embeddedShaders.cs_copytexture1d_c();
			break;
		case sce::Gnm::kTextureTypeCubemap:
			// Spoof the cubemap textures as 2D texture arrays.
			textureDstCopy.initAs2dArray(textureDstCopy.getWidth(), textureDstCopy.getHeight(), textureDstCopy.getLastArraySliceIndex()+1, textureDstCopy.getLastMipLevel()+1, textureDstCopy.getDataFormat(), textureDstCopy.getTileMode(), textureDstCopy.getNumFragments(), false);
			textureSrcCopy.initAs2dArray(textureSrcCopy.getWidth(), textureSrcCopy.getHeight(), textureSrcCopy.getLastArraySliceIndex()+1, textureSrcCopy.getLastMipLevel()+1, textureSrcCopy.getDataFormat(), textureSrcCopy.getTileMode(), textureSrcCopy.getNumFragments(), false);
			textureDstCopy.setBaseAddress(textureDst->getBaseAddress());
			textureSrcCopy.setBaseAddress(textureSrc->getBaseAddress());
			// Intentional fall-through
		case sce::Gnm::kTextureType2d:
		case sce::Gnm::kTextureType2dArray:
			GPUAPI_ASSERT(textureDst->getHeight() == textureSrc->getHeight(), TXT("source and destination texture heights do not match (dest=%d, source=%d)."), textureDst->getHeight(), textureSrc->getHeight());
			GPUAPI_ASSERT(textureSrc->getTextureType() == sce::Gnm::kTextureType2d || textureSrc->getTextureType() == sce::Gnm::kTextureType2dArray || textureSrc->getTextureType() == sce::Gnm::kTextureTypeCubemap,
				TXT("textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X)."), textureDst->getTextureType(), textureSrc->getTextureType());
			shader = copyRawPixels ? dd.m_embeddedShaders.cs_copyrawtexture2d_c() : dd.m_embeddedShaders.cs_copytexture2d_c();
			break;
		case sce::Gnm::kTextureType3d:
			GPUAPI_ASSERT(textureDst->getHeight() == textureSrc->getHeight(), TXT("source and destination texture heights do not match (dest=%d, source=%d)."), textureDst->getHeight(), textureSrc->getHeight());
			GPUAPI_ASSERT(textureDst->getDepth() == textureSrc->getDepth(), TXT("source and destination texture depths do not match (dest=%d, source=%d)."), textureDst->getDepth(), textureSrc->getDepth());
			GPUAPI_ASSERT(textureSrc->getTextureType() == sce::Gnm::kTextureType3d,
				TXT("textureDst and textureSrc must have the same dimensionality (dst=0x%02X, src=0x%02X)."), textureDst->getTextureType(), textureSrc->getTextureType());
			shader = copyRawPixels ? dd.m_embeddedShaders.cs_copyrawtexture3d_c() : dd.m_embeddedShaders.cs_copytexture3d_c();
			break;
		default:
			break; // unsupported texture type -- handled below
		}
		if(shader == 0)
		{
			GPUAPI_FATAL( "textureDst's dimensionality (0x%02X) is not supported by this function.", textureDst->getTextureType() );
			return false;
		}

		DecompressIfRequired( sourceTexData );
		SynchronizeGraphicsToCompute( gfxc );

		gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );
		
		SShaderData& shaderData = dd.m_Shaders.Data( shader );
		internalSetCsShader( gfxc, &shaderData );
		dd.m_shadersChangedMask |= ( 1 << ComputeShader );

		textureDstCopy.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC); // The destination texture is GPU-coherent, because we will write to it.
		textureSrcCopy.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO); // The source texture is read-only, because we'll only ever read from it.

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

				gfxc.setTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureSrcCopy );
				gfxc.setRwTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureDstCopy );

				switch(textureDstCopy.getTextureType())
				{
				case sce::Gnm::kTextureType1d:
				case sce::Gnm::kTextureType1dArray:
					gfxc.dispatch( (mipWidth+63)/64, 1, 1);
					break;
				case sce::Gnm::kTextureTypeCubemap:
				case sce::Gnm::kTextureType2d:
				case sce::Gnm::kTextureType2dArray:
					gfxc.dispatch( (mipWidth+7)/8, (mipHeight+7)/8, 1);
					break;
				case sce::Gnm::kTextureType3d:
					gfxc.dispatch( (mipWidth+3)/4, (mipHeight+3)/4, (mipDepth+3)/4 );
					break;
				default:
					SCE_GNM_ASSERT(0); // This path should have been caught in the previous switch statement
					return false;
				}
			}
		}

		SynchronizeComputeToGraphics( gfxc );
		gfxc.setShaderType(sce::Gnm::kShaderTypeGraphics);

		return true;
	}
	


	void* LockLevel( const TextureRef& ref, Uint32 level, Uint32 slice, Uint32 lockFlags, Uint32& outPitch )
	{
		STextureData &data = GetDeviceData().m_Textures.Data(ref);
		if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		{
			if ( (data.m_Desc.usage & TEXUSAGE_Dynamic) && (lockFlags & GpuApi::BLF_Discard) )
			{
				// validate the texture has not been locked multiple times in a frame - this will not work if it has been used as a resource!
				if (data.m_lockedOnFrame == GpuApi::FrameIndex() && data.m_lockedOnBatch < GpuApi::BatchIndex())
				{
					GPUAPI_HALT("Cannot lock texture for discard multiple times in a frame! [%s]", GetProfilerMarkerString());
				}
				else if (data.m_lockedOnFrame < GpuApi::FrameIndex())
				{
					data.m_currentBuffer = (data.m_currentBuffer + 1) % 2;
					void* ptr = data.m_dynamicBuffers[ data.m_currentBuffer ];
					data.m_texture.setBaseAddress(ptr);
				}
			}

			// HACK : If DoNotWait, and we were locked within the last few frames, don't lock it.
			if ( lockFlags & BLF_DoNotWait )
			{
				if (data.m_lockedOnFrame >= GpuApi::FrameIndex() - 3)
				{
					return nullptr;
				}
			}

			data.m_lockedOnFrame = GpuApi::FrameIndex();
			data.m_lockedOnBatch = GpuApi::BatchIndex();

			Uint64 offset;
			Uint64 size;
			Int32 ret = sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &offset, &size, &data.m_texture, level, slice );

			if (ret == sce::GpuAddress::kStatusSuccess)
			{
				outPitch = CalculateTexturePitch( data.m_texture.getPitch(), data.m_Desc.format );
				return ((Uint8*)(data.m_texture.getBaseAddress())) + offset;
			}
		}
		return nullptr;
	}

	void UnlockLevel( const TextureRef& ref, Uint32 level, Uint32 slice )
	{
		// We don't have to do anything in the unlock

		//GPUAPI_HALT(TXT("NOT IMPLEMENTED"));

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		////dex++: enabled locks on staged textures
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		////dex--
		//{
		//	//HACK DX10 only mip0 now
		//	GPUAPI_ASSERT(level == 0);
		//	Uint32 subresource = D3D11CalcSubresource( level, 0, data.m_Desc.initLevels );
		//	GetDeviceContext()->Unmap( data.m_pTexture, subresource );
		//}
	}

	void* LockLevelRaw( const TextureRef& ref, Uint32 subresource, Uint32 lockFlags, Uint32& outPitch )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		//{
		//	D3D11_MAPPED_SUBRESOURCE mapped;
		//	GPUAPI_MUST_SUCCEED( GetDeviceContext()->Map( data.m_pTexture, subresource, (D3D11_MAP)MapBuffLockFlagsToD3D( lockFlags ), 0, &mapped ) );
		//	outPitch = mapped.RowPitch;
		//	return mapped.pData;
		//}
		//else
		{
			return NULL;
		}
	}

	void UnlockLevelRaw( const TextureRef& ref, Uint32 subresource )
	{
		GPUAPI_HALT("NOT IMPLEMENTED");

		//const STextureData &data = GetDeviceData().m_Textures.Data(ref);
		//if (data.m_Desc.usage & (TEXUSAGE_Dynamic | TEXUSAGE_Staging))
		//{
		//	GetDeviceContext()->Unmap( data.m_pTexture, subresource );
		//}
	}

	extern Bool g_deferWaitOnGraphicsWrites;

	// Decompress if HTILE is used, also wait for graphics writes if texture has been used as a rendertarget
	Bool DecompressIfRequired(STextureData& texData)
	{
		Bool didDecompress = false;

		sce::Gnmx::GfxContext& gfxc = GetSwapChainData().backBuffer->context;

		// Wait for previous writes to complete? 
		if (g_deferWaitOnGraphicsWrites && texData.WasBoundThisFrame(FrameIndex()) )
		{
			Uint32 rtSize = texData.m_aliasedAsRenderTargetsSize;
			Uint32 dtSize = texData.m_aliasedAsDepthStencilsSize;
			Uint32 mipSet = texData.DecodeBoundMipSet();

			while (mipSet)
			{
				int mipIndex = __builtin_ctz(mipSet);	// find the lowest set bit
				mipSet ^= (1 << mipIndex);				// clear the bit

				if (mipIndex < rtSize)
				{
					const sce::Gnm::RenderTarget& prevRT = texData.m_aliasedAsRenderTargets[mipIndex];
					gfxc.waitForGraphicsWrites( prevRT.getBaseAddress256ByteBlocks(), 
						prevRT.getSliceSizeInBytes() / 256, 
						sce::Gnm::kWaitTargetSlotAll, 
						sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
						sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
						sce::Gnm::kStallCommandBufferParserEnable);

				}
				else if (mipIndex < dtSize)
				{
					const sce::Gnm::DepthRenderTarget* prevDS = &texData.m_aliasedAsDepthStencils[mipIndex];
					gfxc.waitForGraphicsWrites( prevDS->getZWriteAddress256ByteBlocks(), 
						prevDS->getZSliceSizeInBytes() / 256, 
						sce::Gnm::kWaitTargetSlotDb, 
						sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
						sce::Gnm::kExtendedCacheActionFlushAndInvalidateDbCache,
						sce::Gnm::kStallCommandBufferParserEnable);
				}
				else
				{
					// probably bound a temp RT to do a StretchRect
					uint64_t surfaceOffset, surfaceSize;
					Int32 slice = 0;	// help!? what should this be?
					if (sce::GpuAddress::kStatusSuccess == sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&surfaceOffset, &surfaceSize, &texData.m_texture, mipIndex, (slice < 0) ? 0 : slice))
					{
						uint32_t baseAddress = ((uint64_t)texData.m_texture.getBaseAddress() + surfaceOffset) >> 8;
						uint32_t baseSize = (uint32_t)surfaceSize;
						gfxc.waitForGraphicsWrites( 
							baseAddress, 
							baseSize,
							sce::Gnm::kWaitTargetSlotAll, 
							sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
							sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
							sce::Gnm::kStallCommandBufferParserEnable);
					}

				}
			}
		}
		texData.m_wasBoundAsTarget = 0;

		if (texData.m_needsDecompress)
		{
			for (Int32 i = 0; i < texData.m_aliasedAsRenderTargetsSize; ++i)
			{
				if (texData.m_aliasedAsRenderTargets[i].getCmaskFastClearEnable())
				{
					gfxc.setCmaskClearColor(0, (Uint32*)&texData.m_clearColor);

					// save rt setup
					RenderTargetSetup originalRt = GetRenderTargetSetup();

					// clear RT setup
					RenderTargetSetup nullRt;
					SetupRenderTargets(nullRt);

					// cache the DrawContext settings
					eDrawContext prevContext	= GpuApi::GetDrawContext();
					Uint32 prevRefValue	= GpuApi::GetDrawContextRefValue();

					// set the draw context required for eliminateFastClear
					GpuApi::SetCustomDrawContext( GpuApi::DSSM_Max, GpuApi::RASTERIZERMODE_Max, GpuApi::BLENDMODE_Max );

					{
						gfxc.pushMarker("EliminateFastClear");
						//gfxc.waitForGraphicsWrites(texData.m_aliasedAsRenderTargets[i].getBaseAddress256ByteBlocks(), texData.m_aliasedAsRenderTargets[i].getSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotCb0,
						//	sce::Gnm::kCacheActionNone, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache, sce::Gnm::kStallCommandBufferParserDisable);
						EliminateFastClear( gfxc, &(texData.m_aliasedAsRenderTargets[i]) );
						gfxc.popMarker();
					}

					GetDeviceData().m_shadersChangedMask = 0xFF;
					GetDeviceData().m_lastPrimitiveType = PRIMTYPE_Invalid;
					GetDeviceData().m_lastIndexBCC = BCC_Invalid;

					// restore draw context
					GpuApi::SetDrawContext(prevContext, prevRefValue);

					// restore RT setup
					SetupRenderTargets( originalRt );

					didDecompress = true;
				}
			}

			for (Int32 i = 0; i < texData.m_aliasedAsDepthStencilsSize; ++i)
			{
				const sce::Gnm::DepthRenderTarget& dt = texData.m_aliasedAsDepthStencils[i];
				if (dt.getHtileAccelerationEnable())
				{
					gfxc.setDepthClearValue(texData.m_clearDepth);

					// save rt setup
					RenderTargetSetup originalRt = GetRenderTargetSetup();

					// clear RT setup
					RenderTargetSetup nullRt;
					SetupRenderTargets(nullRt);

					// cache the DrawContext settings
					eDrawContext prevContext	= GpuApi::GetDrawContext();
					Uint32 prevRefValue	= GpuApi::GetDrawContextRefValue();

					// set the draw context required for decompressDepthSurface
					GpuApi::SetCustomDrawContext(GpuApi::DSSM_NoStencilNoDepth, GpuApi::RASTERIZERMODE_DefaultNoCull, GpuApi::BLENDMODE_Set_0RTOnlyWrites);

					for (int i = 0; i < texData.m_Desc.sliceNum; ++i)
					{
						sce::Gnm::DepthRenderTarget dtCopy = dt;
						dtCopy.setArrayView(i, i);
						DecompressDepth( gfxc, &dtCopy );
					}

					GetDeviceData().m_shadersChangedMask = 0xFF;
					GetDeviceData().m_lastPrimitiveType = PRIMTYPE_Invalid;
					GetDeviceData().m_lastIndexBCC = BCC_Invalid;

					// restore draw context
					GpuApi::SetDrawContext(prevContext, prevRefValue);

					// restore RT setup
					SetupRenderTargets( originalRt );

					didDecompress = true;
				}
			}

			texData.m_needsDecompress = false;
		}

		return didDecompress;
	}


	void BindTextures( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage )
	{
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );

		SDeviceData &dd = GetDeviceData();

		for ( Uint32 i=0; i<numTextures; ++i )
		{
			if ( textures != nullptr )
			{
				TextureRef currRef = textures[i];
				// not a valid assert on PS4 - all textures are Samplable and we rely on this for texture copying
				//GPUAPI_ASSERT( !currRef || (dd.m_Textures.Data(currRef).m_Desc.usage & TEXUSAGE_Samplable) );
				if (!currRef.isNull())
				{
					STextureData& texData = dd.m_Textures.Data(currRef);
					if ( texData.m_Desc.IsInPlace() && texData.m_memoryRegion.IsValid() ) 
					{
						texData.m_texture.setBaseAddress( texData.m_memoryRegion.GetRawPtr() );
					}
					dd.m_texturesSet[ shaderStage ][startSlot + i] = texData.m_texture;

					// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
					DecompressIfRequired(texData);
				}
				else
				{
					dd.m_texturesSet[ shaderStage ][startSlot + i] = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ).m_texture;
				}
			}
			else
			{
                dd.m_texturesSet[ shaderStage ][startSlot + i] = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D) ).m_texture;

			}
		}
	}

	void BindTexturesFast( Uint32 startSlot, Uint32 numTextures, const TextureRef *textures, eShaderType shaderStage )
	{
		BindTextures(startSlot, numTextures, textures, shaderStage);
	}

	void BindTextureCubeMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		sce::Gnm::Texture& gnmTex = dd.m_texturesSet[ shaderStage ][slot];
		GPUAPI_ASSERT( !texture || (dd.m_Textures.Data(texture).m_Desc.usage & TEXUSAGE_Samplable) );
		if (!texture.isNull())
		{
			// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
			DecompressIfRequired(dd.m_Textures.Data(texture));

			Uint16 sliceBegin = sliceIndex * 8;		// 8 rather than 6 because it gets rounded up to POW2 (I think!)

			gnmTex = dd.m_Textures.Data( texture ).m_texture;
			gnmTex.setArrayView(sliceBegin, sliceBegin);
			gnmTex.setMipLevelRange(mipIndex, mipIndex);
		}
		else
		{
			gnmTex = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ).m_texture;
		}
	}


	void BindTextureCubeMipFace( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, Uint32 faceIndex, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		sce::Gnm::Texture& gnmTex = dd.m_texturesSet[shaderStage][slot];
		GPUAPI_ASSERT( !texture || (dd.m_Textures.Data(texture).m_Desc.usage & TEXUSAGE_Samplable) );
		if (!texture.isNull())
		{
			// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
			DecompressIfRequired(dd.m_Textures.Data(texture));

			const TextureDesc& desc = GetTextureDesc(texture);
			Uint16 slice = sliceIndex * 8 + faceIndex;	// 8 rather than 6 because it gets rounded up to POW2 (I think!)

			const sce::Gnm::Texture& srcTex = dd.m_Textures.Data( texture ).m_texture;

			// alias as non cubemap texture array so we can access the faces individually using setArrayView
			gnmTex.initAs2dArray(desc.width, desc.height, desc.sliceNum * 8, desc.initLevels, srcTex.getDataFormat(), srcTex.getTileMode(), srcTex.getNumFragments(), false);
			gnmTex.setBaseAddress(srcTex.getBaseAddress());
			gnmTex.setMipLevelRange(mipIndex, mipIndex);
			gnmTex.setArrayView(slice, slice);
		}
		else
		{
			gnmTex = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ).m_texture;
		}
	}


	void BindTextureMipLevel( Uint32 slot, const TextureRef &texture, Uint32 sliceIndex, Uint32 mipIndex, eShaderType shaderStage )
	{
		SDeviceData &dd = GetDeviceData();
		sce::Gnm::Texture& gnm_texture = dd.m_texturesSet[shaderStage][slot];
		if (!texture.isNull())
		{
			// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
			DecompressIfRequired(dd.m_Textures.Data(texture));

			const TextureDesc& desc = GetTextureDesc(texture);
			const sce::Gnm::Texture& srcTex = dd.m_Textures.Data( texture ).m_texture;

			gnm_texture.initAs2d(desc.width, desc.height, desc.initLevels, srcTex.getDataFormat(), srcTex.getTileMode(), srcTex.getNumFragments());

			gnm_texture.setBaseAddress(srcTex.getBaseAddress());
			gnm_texture.setMipLevelRange(mipIndex, mipIndex);
			gnm_texture.setArrayView(sliceIndex, sliceIndex);

		}
		else
		{
			gnm_texture = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ).m_texture;
		}
	}


	void BindTextureStencil( Uint32 slot, const TextureRef &texture, eShaderType shaderStage )
	{
		GPUAPI_ASSERT( slot <= GpuApi::MAX_PS_SAMPLERS );

		if ( texture )
		{
			Uint32 mip = 0;		// which mip? could we simply bind the texture which may provide access to all mips?

			SDeviceData &dd = GetDeviceData();
			STextureData &tdata = GetDeviceData().m_Textures.Data(texture);
			dd.m_texturesSet[ shaderStage ][ slot ].initFromStencilTarget( &tdata.m_aliasedAsDepthStencils[mip], sce::Gnm::kTextureChannelTypeUInt, false );

			// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
			DecompressIfRequired(tdata);
		}
	}

	// In D3D this is only possible for compute shaders so we want to keep compatibility
	void BindTextureUAVs( Uint32 startSlot, Uint32 numTextures, const TextureRef* textures )
	{
		GPUAPI_ASSERT( startSlot + numTextures <= GpuApi::MAX_PS_SAMPLERS );
		SDeviceData &dd = GetDeviceData();
		if ( textures != nullptr )
		{
			for ( Uint32 i=0; i<numTextures; ++i )
			{
				TextureRef currRef = textures[i];
				sce::Gnm::Texture& gnm_texture = dd.m_textureUAVsSet[startSlot + i];

				if (!currRef.isNull())
				{
					// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
					DecompressIfRequired(dd.m_Textures.Data(currRef));

					STextureData& texData = dd.m_Textures.Data( currRef );
					GPUAPI_ASSERT( texData.m_doCmask == false, TXT("CMASK with UAV seems to not be a good idea.") );

					gnm_texture = texData.m_texture;
					gnm_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
				}
				else
				{
					gnm_texture = dd.m_Textures.Data( GpuApi::GetInternalTexture( GpuApi::INTERTEX_Default2D) ).m_texture;
					gnm_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
				}
			}
		}
		else
		{
			for ( Uint32 i=startSlot; i<(numTextures + startSlot); ++i )
			{
				dd.m_textureUAVsSet[i].setBaseAddress(nullptr);
			}
		}
	}


	void BindTextureMipLevelUAV( Uint32 slot, const TextureRef& texture, Uint32 sliceIndex, Uint32 mipIndex )
	{
		SDeviceData &dd = GetDeviceData();
		sce::Gnm::Texture& gnm_texture = dd.m_textureUAVsSet[slot];	// get reference to GnmTexture struct in [slot]

		if (!texture.isNull())
		{
			// copy by value from m_Textures into uav slot
			GpuApi::STextureData& textureDesc = dd.m_Textures.Data( texture );
			gnm_texture = textureDesc.m_texture;

			// if this was bound as a RT and it has CMASK/HTILE then we need to decompress the surface before it can be used
			DecompressIfRequired(textureDesc);

			// modify the UAV specific bindings
			gnm_texture.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
			gnm_texture.setMipLevelRange(mipIndex, mipIndex);
			gnm_texture.setArrayView(sliceIndex, sliceIndex);
		}
		else
		{
			// clear the texture
			dd.m_textureUAVsSet[slot].setBaseAddress(nullptr);
		}

	}

	Bool IsDescSupported( const TextureDesc &desc )
	{
		Bool isSupported = false;
		Utils::TextureFactoryGNM( GetDeviceData().m_Caps, desc, &isSupported, nullptr, nullptr );
		return isSupported;
	}

	void LoadTextureData2D( const TextureRef& destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void* srcMemory, Uint32 srcPitch )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Textures.IsInUse( destTex ), TXT("Invalid texture reference") );
		if ( !GetDeviceData().m_Textures.IsInUse( destTex ) )
		{
			return;
		}

		STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
		GPUAPI_ASSERT( texData.m_texture.isTexture(), TXT("Destination texture doesn't exist") );
		if ( !texData.m_texture.isTexture() )
		{
			return;
		}

		Uint32 texLevels = texData.m_Desc.CalcTargetLevels();
		Uint32 texSlices = texData.m_Desc.CalcTargetSlices();

		GPUAPI_ASSERT( mipLevel < texLevels, TXT("Mip level out of range: %u >= %u"), mipLevel, texLevels );
		GPUAPI_ASSERT( arraySlice < texSlices, TXT("Array slice out of range: %u >= %u"), arraySlice, texSlices );
		if ( mipLevel >= texLevels || arraySlice >= texSlices )
		{
			return;
		}

		GPUAPI_ASSERT( !(texData.m_Desc.usage & TEXUSAGE_Immutable), TXT("Cannot load data into immutable texture") );
		if ( texData.m_Desc.usage & TEXUSAGE_Immutable )
		{
			return;
		}

		Uint64 offset;
		Uint64 size;
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &offset, &size, &texData.m_texture, mipLevel, arraySlice );

		if (texData.m_Desc.usage & TEXUSAGE_Dynamic)
		{
			// validate the texture has not been locked multiple times in a frame - this will not work if it has been used as a resource!
			if (texData.m_lockedOnFrame == GpuApi::FrameIndex() && texData.m_lockedOnBatch < GpuApi::BatchIndex())
			{
				GPUAPI_HALT("Cannot lock texture for discard multiple times in a frame! [%s]", GetProfilerMarkerString());
			}

			if (texData.m_lockedOnFrame < GpuApi::FrameIndex())
			{
				// toggle buffers if it's the first LOCK this frame
				Uint32 nextBufferIndex = (texData.m_currentBuffer + 1) % 2;

				// we first need to ensure this buffer has the most recent texture data in it
				{
					void* src = texData.m_dynamicBuffers[texData.m_currentBuffer];
					void* dst = texData.m_dynamicBuffers[nextBufferIndex];

					Uint64 totalSize = 0;
					sce::Gnm::AlignmentType totalAlign;
					Int32 ret = sce::GpuAddress::computeTotalTiledTextureSize(&totalSize, &totalAlign, &texData.m_texture);
					if (ret != sce::GpuAddress::kStatusSuccess)
					{
						GPUAPI_HALT("Failed to computeTotalTiledTextureSize! (%d)", ret);
						totalSize = size * texData.m_Desc.sliceNum;
					}

					Red::MemoryCopy(dst, src, totalSize);
				}

				// update the texture ptr
				void* ptr = texData.m_dynamicBuffers[ nextBufferIndex ];
				texData.m_texture.setBaseAddress(ptr);

				texData.m_currentBuffer = nextBufferIndex;
			}
		
			texData.m_lockedOnFrame = GpuApi::FrameIndex();
			texData.m_lockedOnBatch = GpuApi::BatchIndex();
		}


		sce::GpuAddress::TilingParameters tilingParams;
		tilingParams.initFromTexture( &texData.m_texture, mipLevel, arraySlice );

		if ( destRect == nullptr )
		{
			int ret = sce::GpuAddress::tileSurface( ((Uint8*)texData.m_texture.getBaseAddress()) + offset, srcMemory, &tilingParams );
			GPUAPI_ASSERT(ret == sce::GpuAddress::kStatusSuccess, TXT("sce::GpuAddress::tileSurfaceRegion failed!"));
			RED_UNUSED(ret);
		}
		else
		{
			sce::GpuAddress::SurfaceRegion surfaceRegion;
			
			surfaceRegion.m_left = destRect->left;
			surfaceRegion.m_top = destRect->top;
			surfaceRegion.m_right = destRect->right;
			surfaceRegion.m_bottom = destRect->bottom;
			surfaceRegion.m_front = 0;
			surfaceRegion.m_back = 1;

			Uint32 bitsPerEl = texData.m_texture.getDataFormat().getBitsPerElement();

			if ( texData.m_texture.getDataFormat().isBlockCompressedFormat() )
			{
				GPUAPI_ASSERT( ( surfaceRegion.m_left & 3 ) == 0 && ( surfaceRegion.m_right & 3 ) == 0 &&
					( surfaceRegion.m_top & 3 ) == 0 && ( surfaceRegion.m_bottom & 3 ) == 0, TXT("Setting compressed texture data with non-multiple-of-4 bounds") );

				// In a show of pure brilliance, for getBitsPerElement() elements are pixels always, but for pitchElems/sliceElems in
				// tileSurfaceRegion() elements are either pixels (for uncompressed) or 4x4 compressed blocks (for compressed textures).
				// So we need to adjust a bit... 16 pixels in each 4x4 block.
				bitsPerEl *= 16;

				// Also need to change region from pixels to blocks.
				surfaceRegion.m_left	/= 4;
				surfaceRegion.m_top		/= 4;
				surfaceRegion.m_right	/= 4;
				surfaceRegion.m_bottom	/= 4;
			}

			const Uint32 pitchElems = srcPitch * 8 / bitsPerEl;
			const Uint32 sliceElems = pitchElems * ( surfaceRegion.m_bottom - surfaceRegion.m_top );
			int ret = sce::GpuAddress::tileSurfaceRegion( ((Uint8*)texData.m_texture.getBaseAddress()) + offset, srcMemory, &tilingParams, &surfaceRegion, pitchElems, sliceElems );
			GPUAPI_ASSERT(ret == sce::GpuAddress::kStatusSuccess, TXT("sce::GpuAddress::tileSurfaceRegion failed!"));
			RED_UNUSED(ret);
		}
	}

	void LoadTextureData2DAsync( const TextureRef &destTex, Uint32 mipLevel, Uint32 arraySlice, const Rect* destRect, const void *srcMemory, Uint32 srcPitch, void* deferredContext )
	{
		if ( !destTex )
		{
			return;
		}

		// Get texture data
		STextureData& texData = GetDeviceData().m_Textures.Data( destTex );
		GPUAPI_ASSERT( mipLevel < texData.m_Desc.CalcTargetLevels() );
		GPUAPI_ASSERT( TEXTYPE_2D == texData.m_Desc.type );
		RED_UNUSED(texData);

		GPUAPI_HALT("NOT IMPLEMENTED");
//		Uint32 subresource = D3D11CalcSubresource(destTexLevel, arraySlice, texData.m_Desc.initLevels);
//		((ID3D11DeviceContext*)deferredContext)->UpdateSubresource( texData.m_pTexture, subresource, NULL, srcMemory, srcPitch, arraySlice );
	}


	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Uint8 *outDataRGBA, Uint32 stride, Bool forceFullAlpha )
	{
		GPUAPI_ASSERT( outDataRGBA );

		if ( !texture || grabWidth < 1 || grabHeight < 1 || stride < 4 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX >= td.m_Desc.width				||	// Checking for the top left side avoids misses
			 grabY >= td.m_Desc.height				||	// caused from integer overflow when adding the
			 grabX + grabWidth > td.m_Desc.width	||	// width and height
			 grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_R8G8B8A8 != td.m_Desc.format && TEXFMT_R8G8B8X8 != td.m_Desc.format) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		void* untiled = nullptr;
		if( td.m_texture.getTileMode() != sce::Gnm::TileMode::kTileModeDisplay_LinearAligned )
		{
			sce::GpuAddress::TilingParameters tilingParams;
			tilingParams.initFromTexture( &td.m_texture, 0, 0 );

			GpuApi::Uint64 size;
			sce::Gnm::AlignmentType alignmentType;
			sce::GpuAddress::computeUntiledSurfaceSize( &size, &alignmentType, &tilingParams );

			untiled = GPU_API_ALLOCATE( GpuMemoryPool_Misc, MC_TextureData, size, alignmentType );

			sce::GpuAddress::detileSurface( untiled, td.m_texture.getBaseAddress(), &tilingParams );
		}
		else
		{
			untiled = td.m_texture.getBaseAddress();
		}

		GpuApi::Uint32 pitch = CalculateTexturePitch( td.m_texture.getPitch(), td.m_Desc.format );

		const Uint8* data = (const Uint8*)((const Uint8*)untiled + grabY * pitch + grabX * 4 * sizeof(Uint8));

		// Copy data to pixel buffer
		if ( forceFullAlpha )
		{
			for ( Uint32 y=0; y<grabHeight; y++ )
			{
				const Uint8 *src  = data + y * pitch;
				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
				{
					dest[0] = src[0];
					dest[1] = src[1];
					dest[2] = src[2];
					dest[3] = 255;
				}
			}
		}
		else
		{
			for ( Uint32 y=0; y<grabHeight; y++ )
			{
				const Uint8 *src  = data + y * pitch;
				Uint8 *dest = outDataRGBA + stride * (y * grabWidth);
				for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=stride )
				{
					dest[0] = src[0];
					dest[1] = src[1];
					dest[2] = src[2];
					dest[3] = src[3];
				}
			}
		}

		// if we were untiling texture - free the untiled memory
		if( td.m_texture.getTileMode() != sce::Gnm::TileMode::kTileModeDisplay_LinearAligned )
		{
			GPU_API_FREE( GpuMemoryPool_Misc, MC_TextureData, untiled );
		}

		return true;
	}

	Bool GrabTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, Float *outData )
	{
		GPUAPI_ASSERT( outData );

		if ( !texture || grabWidth < 1 || grabHeight < 1 )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || (TEXFMT_Float_R16G16B16A16 != td.m_Desc.format ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		void* untiled = nullptr;
		if( td.m_texture.getTileMode() != sce::Gnm::TileMode::kTileModeDisplay_LinearAligned )
		{
			sce::GpuAddress::TilingParameters tilingParams;
			tilingParams.initFromTexture( &td.m_texture, 0, 0 );

			GpuApi::Uint64 size;
			sce::Gnm::AlignmentType alignmentType;
			sce::GpuAddress::computeUntiledSurfaceSize( &size, &alignmentType, &tilingParams );

			untiled = GPU_API_ALLOCATE( GpuMemoryPool_Misc, MC_TextureData, size, alignmentType );

			sce::GpuAddress::detileSurface( untiled, td.m_texture.getBaseAddress(), &tilingParams );
		}
		else
		{
			untiled = td.m_texture.getBaseAddress();
		}

		GpuApi::Uint32 pitch = CalculateTexturePitch( td.m_texture.getPitch(), td.m_Desc.format );

		switch( td.m_Desc.format )
		{
		case TEXFMT_Float_R16G16B16A16:
			{
				const Uint16* data = (const Uint16*)((const Uint8*)untiled + grabY * pitch + grabX * 4 * sizeof(Uint16));
			
				// Copy data to pixel buffer
				for ( Uint32 y=0; y<grabHeight; y++ )
				{	
					const Uint16* src = (const Uint16*)((Uint8*)data + y * pitch);
					Float *dest = outData + 4 * (y * grabWidth);
					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
					{
						dest[0] = Float16Compressor::Decompress( src[0] );
						dest[1] = Float16Compressor::Decompress( src[1] );
						dest[2] = Float16Compressor::Decompress( src[2] );
						dest[3] = 1.f;
					}
				}
			}
			break;
		default:
			GPUAPI_HALT( "Format not handled" );
			return false;
		}

		// if we were untiling texture - free the untiled memory
		if( td.m_texture.getTileMode() != sce::Gnm::TileMode::kTileModeDisplay_LinearAligned )
		{
			GPU_API_FREE( GpuMemoryPool_Misc, MC_TextureData, untiled );
		}
		
		return true;

//		// Create staging texture
//		ID3D11Texture2D *stagingTexture = NULL;
//		D3D11_TEXTURE2D_DESC texDesc;
//		texDesc.Usage = D3D11_USAGE_STAGING;
//		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//		texDesc.ArraySize = 1;
//		texDesc.BindFlags = 0;
//		texDesc.MipLevels = 1;
//		texDesc.MiscFlags = 0;
//		texDesc.SampleDesc.Count = 1;
//		texDesc.SampleDesc.Quality = 0;
//
//		texDesc.Format = Map(td.m_Desc.format);
//		texDesc.Width = td.m_Desc.width;
//		texDesc.Height = td.m_Desc.height;
//
//		HRESULT hr = GetDevice()->CreateTexture2D(&texDesc, 0, &stagingTexture);
//
//#ifdef GPU_API_DEBUG_PATH
//		const char* debugName = "tempTex";
//		stagingTexture->SetPrivateData( WKPDID_D3DDebugObjectName, 7, debugName );
//#endif
//
//		// Copy data
//		if ( td.m_pTexture == nullptr )
//		{
//			GPUAPI_HALT( TXT( "Source texture doesn't exist" ) );
//			return false;
//		}
//		GetDeviceContext()->CopyResource(stagingTexture, td.m_pTexture);
//
//		// Build lock rect
//		RECT realRect;
//		realRect.left   = grabX;
//		realRect.top    = grabY;
//		realRect.right  = grabX + grabWidth;
//		realRect.bottom = grabY + grabHeight;
//
//		// Lock destination surface
//		D3D11_MAPPED_SUBRESOURCE mappedTexture;
//		if ( !SUCCEEDED( GetDeviceContext()->Map( stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTexture ) ) )
//		{
//			GPUAPI_LOG_WARNING( TXT( "Texture pixels grab internal failure : lock rect failed." ) );
//			stagingTexture->Release();
//			return false;
//		}
//
//		// Grab the data
//		switch ( td.m_Desc.format )
//		{
//		case TEXFMT_Float_R16G16B16A16:
//			{
//				const Uint16* data = (const Uint16*)((const Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Uint16));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; y++ )
//				{	
//					const Uint16 *src  = (const Uint16*)((Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + 4 * (y * grabWidth);
//					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
//					{
//						dest[0] = Float16Compressor::Decompress( src[0] );
//						dest[1] = Float16Compressor::Decompress( src[1] );
//						dest[2] = Float16Compressor::Decompress( src[2] );
//						dest[3] = 1.f;
//					}
//				}
//			}
//			break;
//
//		case TEXFMT_Float_R32G32B32A32:
//			{
//				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * 4 * sizeof(Float));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; y++ )
//				{
//					Float *src  = (Float*)((Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + 4 * (y * grabWidth);
//					for ( Uint32 x=0; x<grabWidth; x++, src+=4, dest+=4 )
//					{
//						dest[0] = src[0];
//						dest[1] = src[1];
//						dest[2] = src[2];
//						dest[3] = 1.f;
//					}
//				}
//			}
//			break;
//
//		case TEXFMT_Float_R32:
//			{
//				Float* data = (Float*)((Uint8*)mappedTexture.pData + grabY * mappedTexture.RowPitch + grabX * sizeof(Float));
//
//				// Copy data to pixel buffer
//				for ( Uint32 y=0; y<grabHeight; ++y )
//				{
//					const Float *src  = (const Float*)((const Uint8*)data + y * mappedTexture.RowPitch);
//					Float *dest = outData + (y * grabWidth);
//					Red::System::MemoryCopy( dest, src, 4 * grabWidth );
//				}
//			}
//			break;
//
//		default:
//			GPUAPI_HALT( TXT( "Format not handled" ) );
//		}
//		
//		// Unlock and release
//		GetDeviceContext()->Unmap( stagingTexture, 0 );
//
//		stagingTexture->Release();
//
//		// Return :)
//		return true;
	}

	Bool SaveTexturePixels( const TextureRef &texture, Uint32 grabX, Uint32 grabY, Uint32 grabWidth, Uint32 grabHeight, const Char* fileName, eTextureSaveFormat format )
	{
#if 0
		//HACK DX10 no texture saving
		if ( !texture || grabWidth < 1 || grabHeight < 1 || !fileName )
		{
			return false;
		}

		// Get internal data
		SDeviceData  &dd = GetDeviceData();
		STextureData &td = dd.m_Textures.Data( texture );

		// Test if pixel grab is possible for given texture
		if ( grabX + grabWidth > td.m_Desc.width || grabY + grabHeight > td.m_Desc.height )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempted to grab data from invalid area." ) );
			return false;
		}		

		// Test whether we support grabbing pixels from given texture format
		if ( TEXTYPE_2D != td.m_Desc.type || ( TEXFMT_A8R8G8B8 != td.m_Desc.format && format != SAVE_FORMAT_DDS ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
			return false;
		}

		// Create software surface
		IDirect3DSurface9 *softSurf = NULL;
		D3DFORMAT softSurfFormat = D3DFMT_A8R8G8B8;
		
		if ( format == SAVE_FORMAT_DDS )
		{
			if ( td.m_Desc.format == TEXFMT_Float_R32 )
			{
				softSurfFormat = D3DFMT_R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_A32R32G32B32 || td.m_Desc.format == TEXFMT_Float_A16R16G16B16 || td.m_Desc.format == TEXFMT_Float_A2R10G10B10 )
			{
				softSurfFormat = D3DFMT_A32B32G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_Float_G16R16 )
			{
				softSurfFormat = D3DFMT_G32R32F;
			}
			else if ( td.m_Desc.format == TEXFMT_A8 )
			{
				softSurfFormat = D3DFMT_A8;
			}
			else if ( td.m_Desc.format == TEXFMT_L8 )
			{
				softSurfFormat = D3DFMT_L8;
			}
			else if ( td.m_Desc.format == TEXFMT_A8L8 )
			{
				softSurfFormat = D3DFMT_A8L8;
			}
			else
			{
				GPUAPI_LOG_WARNING( TXT( "Attempt to grab data from unsupported (for grabbing) format detected." ) );
				return false;
			}
		}

		// Build lock rect
		RECT realRect;
		realRect.left   = grabX;
		realRect.top    = grabY;
		realRect.right  = grabX + grabWidth;
		realRect.bottom = grabY + grabHeight;

		D3DXIMAGE_FILEFORMAT destFormat = Map( format );

		HRESULT hr = D3DXSaveSurfaceToFile( fileName, destFormat, td.m_pSurface, NULL, &realRect );

		// Return :)
		return SUCCEEDED( hr );
#else
		return false;
#endif
	}

	Bool SaveTextureToMemory( const Uint8* textureData, const size_t textureDataSize, const size_t width, size_t height, const eTextureFormat format, const size_t pitch, const eTextureSaveFormat saveFormat, void** buffer, size_t& size )
	{
		return true;
	}

	Bool SaveTextureToMemory( const TextureRef &texture, eTextureSaveFormat format, const Rect* sourceRect, void** buffer, Uint32& size )
	{
		// Done
		return true;
	}

#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
	Bool CompressBC6HBC7( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage )
	{
		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
		GetDevice()->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
		if ( !hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
		{
			GPUAPI_LOG_WARNING( TXT( "Sorry your driver and/or video card doesn't support DirectCompute 4.x" ) );
			return false;
		}

		EncoderBase* encoder = NULL;
		if ( compressedImage.format == TEXFMT_BC6H )
		{
			encoder = new CGPUBC6HEncoder();
		}
		else if ( compressedImage.format == TEXFMT_BC7 )
		{
			encoder = new CGPUBC7Encoder();
		}
		else
		{
			GPUAPI_HALT( TXT( "Should never get here." ) );
			return false;
		}

		GPUAPI_ASSERT( encoder );
		encoder->Initialize( GetDevice(), GetDeviceContext() );

		DirectX::Image image;
		image.format		= Map( srcImage.format );
		image.width			= srcImage.width;
		image.height		= srcImage.height;
		image.pixels		= *const_cast<Uint8**>( srcImage.data );
		image.rowPitch		= srcImage.rowPitch;
		image.slicePitch	= srcImage.slicePitch;

		DirectX::TexMetadata metadata;
		metadata.arraySize	= 1;
		metadata.depth		= 1;
		metadata.dimension	= DirectX::TEX_DIMENSION_TEXTURE2D;
		metadata.format		= Map( srcImage.format );
		metadata.height		= srcImage.height;
		metadata.width		= srcImage.width;
		metadata.mipLevels	= 1;
		metadata.miscFlags	= 0;

		ID3D11Texture2D* sourceTexture = NULL;
		HRESULT hr = DirectX::CreateTexture( GetDevice(), &image, 1, metadata, (ID3D11Resource**)&sourceTexture );
		GPUAPI_ASSERT( SUCCEEDED( hr ) );

		DirectX::Image* cImage = new DirectX::Image();
		hr = encoder->GPU_EncodeAndReturn( sourceTexture, Map( compressedImage.format ), cImage );
		if ( SUCCEEDED( hr ) )
		{
			Red::System::MemoryCopy( *compressedImage.data, cImage->pixels, cImage->slicePitch );
			compressedImage.width = cImage->width;
			compressedImage.height = cImage->height;
			compressedImage.rowPitch = cImage->rowPitch;
			compressedImage.slicePitch = cImage->slicePitch;
		}

		// cleanup
		free( cImage->pixels ); // memory was allocated inside GPU_EncodeAndReturn
		delete cImage;
		cImage = NULL;
		delete encoder;
		encoder = NULL;

		return SUCCEEDED( hr );
	}
#endif

	Bool CompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& compressedImage, EImageCompressionHint compressionHint, Float alphaThreshold /*= 0.5f*/ )
	{
#ifdef USE_COMPUTE_SHADER_FOR_BC7_COMPRESSION
		if ( compressedImage.format == TEXFMT_BC6H || compressedImage.format == TEXFMT_BC7 )
		{
			if ( CompressBC6HBC7( srcImage, compressedImage ) )
			{
				return true;
			}
		}
#endif
		return true;
	}

	Bool DecompressImage( /* in */ const TextureDataDesc& srcImage, /* in-out */ TextureDataDesc& decompressedImage )
	{
		return true;
	}

	TextureRef GetInternalTexture( eInternalTexture internalTexture )
	{
		if( INTERTEX_Max == internalTexture )
		{
			GPUAPI_HALT( "internal texture index invalid" );
			return TextureRef::Null();
		}
		const TextureRef &ref = GetDeviceData().m_InternalTextures[internalTexture];
		GPUAPI_ASSERT( ref, TXT( "Internal texture reference is invalid" ) );
		return ref;
	}

	void InitInternalTextures( bool assumeRefsPresent )
	{	
		SDeviceData &dd = GetDeviceData();

		// Pre check
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			Bool isPresent = !dd.m_InternalTextures[i].isNull();
			GPUAPI_ASSERT( isPresent == assumeRefsPresent );
			RED_UNUSED(isPresent);
		}

		// Create resources
		for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		{
			if ( !dd.m_InternalTextures[i] )
			{
				dd.m_InternalTextures[i] = TextureRef( dd.m_Textures.Create( 1 ) );
				GPUAPI_ASSERT( dd.m_InternalTextures[i] );
			}
		}

		// Create internal textures
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Blank2D] ),			GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::BlankTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_FlatNormal2D] ),		GPUAPI_BLANK2D_TEXTURE_SIZE,			Utils::FlatNormalTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_Default2D] ),		GPUAPI_DEFAULT2D_TEXTURE_SIZE,			Utils::DefaultTextureFill );
		Utils::InitInternalTextureDataCUBE( dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DefaultCUBE] ),		GPUAPI_DEFAULTCUBE_TEXTURE_SIZE,		Utils::DefaultCubeTextureFill );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_DissolvePattern] ),	GPUAPI_DISSOLVE_TEXTURE_SIZE,			Utils::GenerateDissolveTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_PoissonRotation] ),	GPUAPI_POISSON_ROTATION_TEXTURE_SIZE,	Utils::GeneratePoissonRotationTexture );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_SSAORotation] ),		GPUAPI_SSAO_ROTATION_TEXTURE_SIZE,		Utils::GenerateSSAORotationNoise );
		Utils::RefMipNoiseRandomGen().Seed( 0xf112 );
		Utils::InitInternalTextureData2D(   dd.m_Textures.Data( dd.m_InternalTextures[INTERTEX_MipNoise] ),			GPUAPI_MIP_NOISE_TEXTURE_SIZE,			Utils::GenerateMipNoise, true );

		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Blank2D], "BlankTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_FlatNormal2D], "FlatTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_Default2D], "DefaultTexture" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DefaultCUBE], "DefaultCube" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_DissolvePattern], "DissolvePattern" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_PoissonRotation], "PoissonRotation" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_SSAORotation], "SSAORotation" );
		GpuApi::SetTextureDebugPath( dd.m_InternalTextures[INTERTEX_MipNoise], "MipNoise" );

		//// Post check
		//for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		//{
		//	const STextureData &td = dd.m_Textures.Data( dd.m_InternalTextures[i] );
		//	GPUAPI_ASSERT( dd.m_InternalTextures[i] && "Not all internal texture were created!" );
		//	GPUAPI_ASSERT( NULL != td.m_pTexture );
		//	//GPUAPI_ASSERT( (NULL != td.m_pSurface) == (TEXTYPE_2D == td.m_Desc.type) );
		//}
	}

	void ShutInternalTextures( bool dropRefs )
	{
		SDeviceData &dd = GetDeviceData();

		// Release d3d resources

		GPUAPI_HALT("NOT IMPLEMENTED");

		//for ( Uint32 i=0; i<INTERTEX_Max; ++i )
		//{
		//	TextureRef ref = dd.m_InternalTextures[i];
		//	if ( !ref )
		//	{
		//		continue;
		//	}

		//	STextureData &data = dd.m_Textures.Data( ref );
		//	//if ( data.m_pSurface )
		//	//{
		//	//	data.m_pSurface->Release();
		//	//	data.m_pSurface = NULL;
		//	//}
		//	if ( data.m_pTexture )
		//	{
		//		data.m_pTexture->Release();
		//		data.m_pTexture = NULL;
		//	}
		//}

		// Drop resources

		if ( dropRefs )
		{
			for ( Uint32 i=0; i<INTERTEX_Max; ++i )
			{
				SafeRelease( dd.m_InternalTextures[i] );
			}
		}
	}

	//dex++: dynamic texture debug code
	Uint32 GetNumDynamicTextures()
	{
		return GetDeviceData().m_NumDynamicTextures;
	}

	const char* GetDynamicTextureName( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Name;
		}
		else
		{
			return NULL;
		}
	}

	TextureRef GetDynamicTextureRef( Uint32 index )
	{
		if ( index < GetDeviceData().m_NumDynamicTextures )
		{
			return GetDeviceData().m_DynamicTextures[index].m_Texture;
		}
		else
		{
			return TextureRef::Null();
		}
	}

	void AddDynamicTexture( TextureRef tex, const char* name )
	{
		if ( GetDeviceData().m_NumDynamicTextures < ARRAY_COUNT( GetDeviceData().m_DynamicTextures ) )
		{
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Name = name;
			GetDeviceData().m_DynamicTextures[ GetDeviceData().m_NumDynamicTextures ].m_Texture = tex;
			GetDeviceData().m_NumDynamicTextures += 1;
		}
	}

	void RemoveDynamicTexture( TextureRef tex )
	{
		for ( Uint32 i=0; i<GetDeviceData().m_NumDynamicTextures; ++i )
		{
			if ( GetDeviceData().m_DynamicTextures[ i ].m_Texture == tex )
			{
				// copy rest of the textures
				for ( Uint32 j=i+1; j<GetDeviceData().m_NumDynamicTextures; ++j )
				{
					GetDeviceData().m_DynamicTextures[j-1] = GetDeviceData().m_DynamicTextures[j];
				}
				
				// remove from list
				GetDeviceData().m_NumDynamicTextures -= 1;
				break;
			}
		}
	}
	//dex--


	Bool CalculateCookedTextureMipOffsetAndSize( const TextureDesc& texDesc, Uint32 mip, Uint32 slice, Uint32* outOffset, Uint32* outSize )
	{
		const Uint32 texMips = texDesc.CalcTargetLevels();
		const Uint32 texSlices = texDesc.CalcTargetSlices();

		if ( mip >= texMips || slice >= texSlices )
		{
			return false;
		}

		// Create temporary texture. We force the in-place flag, so that this won't actually allocate any extra memory, it
		// just sets up the Texture object with the info we need.
		TextureDesc tempDesc = texDesc;
		tempDesc.inPlaceType = INPLACE_Texture;

		sce::Gnm::Texture tempTex;
		sce::Gnm::SizeAlign tempSizeAlign;
		Utils::TextureFactoryGNM( GetDeviceData().m_Caps, tempDesc, nullptr, &tempTex, &tempSizeAlign );

		size_t offset, size;
		Int32 result = sce::GpuAddress::computeTextureSurfaceOffsetAndSize( &offset, &size, &tempTex, mip, slice );
		if ( result != sce::GpuAddress::kStatusSuccess )
		{
			return false;
		}

		if ( outOffset != nullptr )
		{
			*outOffset = offset;
		}

		if ( outSize != nullptr )
		{
			*outSize = size;
		}

		return true;
	}


	Uint32 CalculateCookedTextureSize( const TextureDesc& texDesc )
	{
		// Create temporary texture. We force the in-place flag, so that this won't actually allocate any extra memory, it
		// just sets up the Texture object with the info we need.
		TextureDesc tempDesc = texDesc;
		tempDesc.inPlaceType = INPLACE_Texture;

		sce::Gnm::SizeAlign sizeAlign;
		sce::Gnm::Texture tempTex;
		Utils::TextureFactoryGNM( GetDeviceData().m_Caps, tempDesc, nullptr, &tempTex, &sizeAlign );

		return sizeAlign.m_size;
	}


	// Texture streaming in-flight stats adjustment
	void IncrementInFlightTextureMemory( Uint32 textureSize )
	{
		GpuApi::GetDeviceData().m_TextureStats.IncrementTextureMemoryInFlight( textureSize );
	}

	void DecrementInFlightTextureMemory( Uint32 textureSize )
	{
		GpuApi::GetDeviceData().m_TextureStats.DecrementTextureMemoryInFlight( textureSize );
	}


	Red::MemoryFramework::MemoryRegionHandle GetTextureInPlaceMemory( const TextureRef& texture )
	{
#ifdef RED_ASSERTS_ENABLED
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Textures.IsInUse( texture ), TXT("Invalid texture ref") );
#endif

		const STextureData& texData = GetDeviceData().m_Textures.Data( texture );
		if ( !texData.m_Desc.IsInPlace() )
		{
			return Red::MemoryFramework::MemoryRegionHandle();
		}

		return texData.m_memoryRegion;
	}



	void STextureData::EncodeBoundData(Int32 frame, Int16 mip)
	{
		GPUAPI_ASSERT(mip < 16);
		Uint16 mipSet = 0;

		// clear mipSet if this is the first time it is bound this frame
		if (WasBoundThisFrame(frame))
		{
			mipSet = DecodeBoundMipSet();
		}

		mipSet |= 1 << mip;

		// pack lower 16 bits of frame and mip into U32
		Uint16 frameLow = (Uint32)frame & 0xFFFF;
		m_wasBoundAsTarget = frameLow | (mipSet << 16);
	}

	Bool STextureData::WasBoundThisFrame(Int32 frame)
	{
		return (m_wasBoundAsTarget & 0xFFFF) == (frame & 0xFFFF);
	}

	Uint16 STextureData::DecodeBoundMipSet()
	{
		Uint16 mipSet = (Uint16)(m_wasBoundAsTarget >> 16);
		return mipSet;
	}
}

