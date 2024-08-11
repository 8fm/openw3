/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cubeTexture.h"
#include "textureCache.h"
#include "../core/dependencyMapper.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "renderResource.h"
#include "material.h"

#ifndef NO_CUBEMAP_GENERATION

// Cube map gen
#include "../../../external/libcubemapgen/CCubeMapProcessor.h"

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment ( lib, "../../../external/libcubemapgen/CubeMapGenLib-x64-debug-MT.lib" )
	#else
		#pragma comment ( lib, "../../../external/libcubemapgen/CubeMapGenLib-x64-MT.lib" )
	#endif
#else
	#ifdef _DEBUG
		#pragma comment ( lib, "../../../external/libcubemapgen/CubeMapGenLib-x86-debug-MT.lib" )
	#else
		#pragma comment ( lib, "../../../external/libcubemapgen/CubeMapGenLib-x86-MT.lib" )
	#endif
#endif

#endif
#include "renderer.h"
#include "game.h"
#include "drawableComponent.h"

IMPLEMENT_ENGINE_CLASS( CCubeTexture );
IMPLEMENT_ENGINE_CLASS( CubeFace );
IMPLEMENT_RTTI_ENUM( ECubeGenerationStrategy );


CCubeTexture::~CCubeTexture()
{
	SAFE_RELEASE( m_renderResource );
	SAFE_RELEASE( m_streamingSource );
}

void CCubeTexture::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

#ifndef NO_CUBEMAP_GENERATION
	if (    property->GetName() == TXT("front")
		 || property->GetName() == TXT("back")
		 || property->GetName() == TXT("top")
		 || property->GetName() == TXT("bottom")
		 || property->GetName() == TXT("left")
		 || property->GetName() == TXT("right")
		 || property->GetName() == TXT("compression")
		)
	{
		DeterminePlatformSpecificCompression( m_platformSpecificCompression );
	}
	GenerateCachedData( m_cookedData );
#endif
	CreateRenderResource();
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

void CCubeTexture::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}


	if ( file.GetVersion() < VER_TEXTURE_COOKING )
	{
		// Device data
		DataBuffer deviceData, systemData;
		deviceData.Serialize( file );
		systemData.Serialize( file );

		// Convert to new format
		UpgradeOldCachedData( deviceData, systemData );
	}
	else
	{
		file.Serialize( &m_cookedData.m_header, sizeof( m_cookedData.m_header ) );
		m_cookedData.m_deviceData.Serialize( file );
	}


	if ( file.IsReader() )
	{
		m_platformSpecificCompression = m_cookedData.m_header.GetTextureFormat();

		CreateStreamingSource();
	}
}


void CCubeTexture::CreateStreamingSource()
{
	SAFE_RELEASE( m_streamingSource );

	// If we are fully resident, no streaming.
	if ( m_cookedData.m_header.m_residentMip == 0 )
	{
		return;
	}

	m_streamingSource = new CTextureCacheStreamingSourcePC( m_cookedData.m_header.m_textureCacheKey );
#ifndef RED_FINAL_BUILD
	m_streamingSource->SetDebugName( GetDepotPath() );
#endif
}


void CCubeTexture::OnSave()
{
#ifndef NO_EDITOR
	CreateRenderResource();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	IMaterial::RecompileMaterialsUsingCube( this );
#endif // NO_RUNTIME_MATERIAL_COMPILATION

	CDrawableComponent::RecreateProxiesOfRenderableComponents();
#endif
}

void CCubeTexture::CreateRenderResource()
{
	// Release previous cube
	SAFE_RELEASE( m_renderResource );

	if ( GRender && !GRender->IsDeviceLost() )
	{
		// Create renderable version of cubemap
		m_renderResource = GRender->UploadCube( this );
	}
}

void CCubeTexture::ReleaseRenderResource()
{
	// Release previous cube
	SAFE_RELEASE( m_renderResource );
}


CCubeTexture::CCubeTexture()
{
#ifndef NO_EDITOR
	m_strategy = ECGS_BlurMips;
	m_targetFaceSize = 0;
	m_platformSpecificCompression = GpuApi::TEXFMT_R8G8B8A8;
#endif
}

IRenderResource* CCubeTexture::GetRenderResource() const
{
	if ( !m_renderResource )
	{
		const_cast<CCubeTexture*>( this )->CreateRenderResource();
	}

	return m_renderResource;
}


#ifndef NO_CUBEMAP_GENERATION

Bool CCubeTexture::DeterminePlatformSpecificCompression( GpuApi::eTextureFormat& format )
{
#ifdef USE_NEW_COMPRESSION
	Bool isOldVersion = false;//file.GetVersion() < VER_ADDED_NEW_TEXTURE_COMPRESSION_FORMATS;
#else
	Bool isOldVersion = true;
#endif
	if ( m_compression != TCM_None )
	{
		// it's a compressed format, so the raw format is irrelevant
		CBitmapTexture::GetCompressedFormat( TRF_TrueColor, m_compression, format, isOldVersion );
		return true;
	}

	// determine format from cube faces
	const CubeFace* faces[6] = { &m_left, &m_right, &m_front, &m_back, &m_top, &m_bottom };
	for ( Uint32 i = 0; i < 6; ++i )
	{
		if ( faces[ i ]->m_texture )
		{
			CBitmapTexture::GetCompressedFormat( faces[ i ]->m_texture->GetFormat(), m_compression, format, false );
			return true;
		}
	}

	format = GpuApi::TEXFMT_R8G8B8A8;
	return false;
}

#endif // !NO_CUBEMAP_GENERATION


// --------------------------------------------------------------------------


CCubeTexture::CookedDataHeader::CookedDataHeader()
	: m_residentMip( 0 )
	, m_encodedFormat( ITexture::EncodeTextureFormat( GpuApi::TEXFMT_R8G8B8A8 ) )
	, m_edgeSize( 0 )
	, m_mipCount( 0 )
{
}

GpuApi::eTextureFormat CCubeTexture::CookedDataHeader::GetTextureFormat() const
{
	return ITexture::DecodeTextureFormat( m_encodedFormat & 0x7fff );
}

Bool CCubeTexture::CookedDataHeader::IsCooked() const
{
	return ( m_encodedFormat & 0x8000 ) != 0;
}


#ifndef NO_CUBEMAP_GENERATION

Bool CCubeTexture::CookedDataHeader::SetTextureFormat( GpuApi::eTextureFormat format )
{
	Uint16 encoded = ITexture::EncodeTextureFormat( format );
	if ( encoded == 0 )
	{
		return false;
	}

	m_encodedFormat &= 0x8000;
	m_encodedFormat |= encoded;
	return true;
}

void CCubeTexture::CookedDataHeader::SetIsCooked( Bool cooked )
{
	if ( cooked )
	{
		m_encodedFormat |= 0x8000;
	}
	else
	{
		m_encodedFormat &= 0x7fff;
	}
}

#endif



#ifndef NO_CUBEMAP_GENERATION


static void ErrorOutputCallback( UniChar *msg, UniChar *title)
{
	ERR_ENGINE(TXT("%ls %ls"), msg, title );
}

static void FlipCubeFace( Uint32 dimension, Uint32 pitch, void *dataRGBA, Uint32 pixelByteSize, Bool faceFlipX, Bool faceFlipY, Bool faceRotate )
{
	if ( !faceFlipX && !faceFlipY && !faceRotate )
	{
		return;
	}

	// Build transformed image
	TDynArray< Uint8 > tempBuffer ( pitch * dimension );
	for ( Uint32 y=0; y<dimension; y++ )
	{
		for ( Uint32 x=0; x<dimension; x++ )
		{
			// Rotate
			Uint32 destX = faceRotate ? y : x;
			Uint32 destY = faceRotate ? x : y;

			// Flip
			destX = faceFlipX ? ( (dimension-1) - destX ) : destX;
			destY = faceFlipY ? ( (dimension-1) - destY ) : destY;

			// Copy
			const Uint8* readPtr = (const Uint8*)dataRGBA + x * pixelByteSize + y * pitch;
			Uint8* writePtr = &(tempBuffer[ destX * pixelByteSize + destY * pitch ]);
			Red::System::MemoryCopy( writePtr, readPtr, pixelByteSize );
		}
	}

	// Copy our image back
	Red::System::MemoryCopy( dataRGBA, tempBuffer.Data(), tempBuffer.DataSize() );
}


static Bool GetFaceData( const CubeFace& face, Uint32 targetSize, TDynArray<Uint8>& outData, ETextureRawFormat& outDataFormat )
{
	Bool didLoadTexture = false;
	CBitmapTexture* faceBitmapTexture = face.m_texture;

#ifndef NO_RESOURCE_COOKING
	// If this face's texture has been cooked, try to load an original non-cooked version. This should only happen during the
	// cooking process, if the face texture is being included, and was processed first.
	if ( faceBitmapTexture != nullptr && faceBitmapTexture->IsCooked() )
	{
		// Texture has already been cooked, so we need to reload the original one...
		CDiskFile* diskFile = faceBitmapTexture->GetFile();
		if ( diskFile == nullptr )
		{
			ERR_ENGINE( TXT("Couldn't get CDiskFile for bitmap %ls"), face.m_texture->GetFriendlyName().AsChar() );
			return false;
		}

		IFile* reader = diskFile->CreateReader();
		if ( reader == nullptr )
		{
			ERR_ENGINE( TXT("Couldn't create reader for bitmap %ls"), face.m_texture->GetFriendlyName().AsChar() );
			return false;
		}

		CDependencyLoader loader( *reader, nullptr );

		DependencyLoadingContext loadingContext;
		loadingContext.m_parent = faceBitmapTexture;
		if ( !loader.LoadObjects( loadingContext ) )
		{
			delete reader;
			ERR_ENGINE( TXT("Couldn't load bitmap %ls"), face.m_texture->GetFriendlyName().AsChar() );
			return false;
		}

		delete reader;

		if ( loadingContext.m_loadedRootObjects.Size() != 1 )
		{
			ERR_ENGINE( TXT("Unexpected number of loaded objects %u"), loadingContext.m_loadedRootObjects.Size() );
			return false;
		}

		faceBitmapTexture = Cast< CBitmapTexture >( loadingContext.m_loadedRootObjects[0] );
		if ( faceBitmapTexture == nullptr )
		{
			ERR_ENGINE( TXT("Couldn't get re-loaded bitmap %ls"), face.m_texture->GetFriendlyName().AsChar() );
			return false;
		}

		didLoadTexture = true;
	}
#endif


	// If this face has built-in source data, or has a texture with source data, use that directly.
	if ( face.m_sourceTexture != nullptr || faceBitmapTexture->GetSourceData() != nullptr )
	{
		CSourceTexture* faceSourceTexture = face.m_sourceTexture.IsValid() ? face.m_sourceTexture.Get() : faceBitmapTexture->GetSourceData();
		outDataFormat = faceSourceTexture->GetFormat();
		outData.Resize( (CBitmapTexture::GetPixelSize( outDataFormat ) / 8) * targetSize * targetSize );

		if ( TRF_HDR == outDataFormat )
		{
			faceSourceTexture->FillBufferHDR( (Float*)outData.Data(), targetSize, targetSize );
		}
		else if ( TRF_TrueColor == outDataFormat )
		{
			faceSourceTexture->FillBufferTrueColor( outData.TypedData(), targetSize, targetSize );
		}
		else
		{
			RED_HALT( "Unexpected source data format: %u", outDataFormat );
			return false;
		}
	}
	else
	{
		outDataFormat = TRF_TrueColor;
		outData.Resize( (CBitmapTexture::GetPixelSize( outDataFormat ) / 8) * targetSize * targetSize );

		const CBitmapTexture::MipMap& baseMip = faceBitmapTexture->GetMips()[0];

		Bool unloadMip = false;

		if ( !const_cast< CBitmapTexture::MipMap& >( baseMip ).m_data.Load() )
		{
			RED_HALT( "Texture with no data at mip0: %ls", face.m_texture->GetFriendlyName().AsChar() );
			return false;
		}

		CSourceTexture* sourceTexture = CreateObject< CSourceTexture >( (CObject*)NULL );
		sourceTexture->Init( baseMip.m_width, baseMip.m_height, faceBitmapTexture->GetFormat() );
		sourceTexture->CreateFromMipCompressed( baseMip, faceBitmapTexture->GetCompression() );
		sourceTexture->FillBufferTrueColor( outData.TypedData(), targetSize, targetSize );
		sourceTexture->Discard();

		if ( unloadMip )
		{
			const_cast< CBitmapTexture::MipMap& >( baseMip ).m_data.Unload();
		}
	}

	if ( didLoadTexture )
	{
		faceBitmapTexture->Discard();
	}

	return true;
}


static Bool BuildCubeMips( const CubeFace* (&faces)[6], Uint32 inputSize, Uint32 outputSize, ECubeGenerationStrategy strategy, ETextureCompression compression, GpuApi::eTextureFormat targetFormat, const String& debugName, CBitmapTexture::MipArray (&outMipMaps)[6] )
{
	SetErrorMessageCallback( ErrorOutputCallback );

	const Uint32 numOutputMips = MLog2( outputSize ) + 1;

	CCubeMapProcessor cubeMapProcessor;
	cubeMapProcessor.Init( inputSize, outputSize, numOutputMips, 4 );
	cubeMapProcessor.m_NumFilterThreads = 4;

	Uint32 facesInputHDRMask = 0;
	for ( Uint32 face_i = 0; face_i < 6; ++face_i )
	{
		const CubeFace& face = *faces[face_i];

		// Skip invalid face
		if ( face.m_texture == nullptr && face.m_sourceTexture == nullptr )
		{
			WARN_ENGINE( TXT("Texture with no data for face %i at mip0: %ls"), (Int32)face_i, debugName.AsChar() );
			continue;
		}

		// Load face data. This will resample the source data if it does not match what the cube processor is expecting.
		TDynArray<Uint8> sourceData;
		ETextureRawFormat sourceDataFormat = TRF_TrueColor;


		if ( !GetFaceData( face, inputSize, sourceData, sourceDataFormat ) )
		{
			WARN_ENGINE( TXT("Couldn't get data for face %i: %ls"), (Int32)face_i, debugName.AsChar() );
			continue;
		}


		const Uint32 sourceDataPixelByteSize = CBitmapTexture::GetPixelSize( sourceDataFormat ) / 8;
		FlipCubeFace( face.GetWidth(), face.GetWidth() * sourceDataPixelByteSize, sourceData.Data(),  sourceDataPixelByteSize, face.m_flipX, face.m_flipY, face.m_rotate );

		const Bool isFaceInputHDR = TRF_HDR == sourceDataFormat;
		facesInputHDRMask |= isFaceInputHDR ? FLAG(face_i) : 0;

		cubeMapProcessor.SetInputFaceData(
			face_i,												// a_FaceIdx, 
			isFaceInputHDR ? CP_VAL_FLOAT32 : CP_VAL_UNORM8,	// a_SrcType, 
			4,													// a_SrcNumChannels, 
			inputSize * sourceDataPixelByteSize,				// a_SrcPitch,
			sourceData.TypedData(),								//*a_SrcDataPtr, 
			isFaceInputHDR ? 999.f : 1.0f,						// a_MaxClamp, 
			1.0f,												// a_Degamma, 
			1.0f												// a_Scale 
			);
	}

	Float baseFilterAngle = 0.0f, initialMipAngle = 0.0f, mipAnglePerLevelScale = 0.0f;
	Int32 filterType = CP_FILTER_TYPE_COSINE, fixupType = CP_FIXUP_AVERAGE_HERMITE, fixupWidth = 3;
	Uint8 useSolidAngle = false;

	switch ( strategy )
	{
	case ECGS_SharpAll:
		baseFilterAngle = 0.0f;
		initialMipAngle = 0.0f;
		mipAnglePerLevelScale = 0.0f;
		break;

	case ECGS_Preblur:
		baseFilterAngle = 3.0f;
		initialMipAngle = 1.0f;
		mipAnglePerLevelScale = 2.0f;
		break;

	case ECGS_PreblurStrong:
		baseFilterAngle = 22.0f;
		initialMipAngle = 2.0f;
		mipAnglePerLevelScale = 2.0f;
		break;

	case ECGS_PreblurDiffuse:
		baseFilterAngle = 90.0f;
		initialMipAngle = 2.0f;
		mipAnglePerLevelScale = 2.0f;
		break;

	case ECGS_BlurMips:
	default:
		baseFilterAngle = 0.5f;
		initialMipAngle = 0.5f;
		mipAnglePerLevelScale = 2.0f;
		break;
	}

	cubeMapProcessor.InitiateFiltering(
		baseFilterAngle,			//float32 a_BaseFilterAngle, 
		initialMipAngle,			//float32 a_InitialMipAngle, 
		mipAnglePerLevelScale,		//float32 a_MipAnglePerLevelScale, 
		filterType,					//int32 a_FilterType, 
		fixupType,					//int32 a_FixupType, 
		fixupWidth,					//int32 a_FixupWidth, 
		useSolidAngle				//bool8 a_bUseSolidAngle );
		);

	const Bool enableProgressBar = !(GGame && GGame->IsActive());

	if ( enableProgressBar )
	{
		GFeedback->BeginTask( TXT("Generating cube map"), false );
	}

	LOG_ENGINE( TXT("Processing cube map '%ls'"), debugName.AsChar() );

	while ( cubeMapProcessor.GetStatus() == CP_STATUS_PROCESSING )
	{
		// Not exact, just an approximation...
		Uint32 intProgress = static_cast< Uint32 >( cubeMapProcessor.m_ThreadProgress[0].m_FractionCompleted * 1000 );
		GFeedback->UpdateTaskProgress( intProgress, 1000 );
		Sleep( 50 );
	}

	if ( enableProgressBar )
	{
		GFeedback->EndTask();
	}

	const Bool isCompressedFormat = GpuApi::IsTextureFormatDXT( targetFormat );

	TDynArray<Uint8> mipData;

	for ( Uint32 face_i = 0; face_i < 6; ++face_i )
	{
		const Bool isFaceHDR = 0 != (facesInputHDRMask & FLAG(face_i));
		const ETextureRawFormat faceRawFormat = isFaceHDR ? TRF_HDR : TRF_TrueColor;
		const Uint32 facePixelByteSize = CBitmapTexture::GetPixelSize( faceRawFormat ) / 8;

		const Bool useFloatPrecisionInput =
			isFaceHDR &&
			(GpuApi::TEXFMT_Float_R16G16B16A16	== targetFormat ||
			GpuApi::TEXFMT_Float_R32G32B32A32	== targetFormat ||
			GpuApi::TEXFMT_BC6H					== targetFormat);

		outMipMaps[face_i].Resize( numOutputMips );

		for ( Uint32 mip_i = 0; mip_i < numOutputMips; ++mip_i )
		{
			const Uint32 mipSize = outputSize >> mip_i;
			// Size of final image. Compressed images are minimum 4x4.
			const Uint32 clampedSize = Max< Uint32 >( mipSize, isCompressedFormat ? 4 : 1 );

			CImageSurface& surface = cubeMapProcessor.m_OutputSurface[mip_i][face_i];

			RED_ASSERT( (Int32)mipSize == surface.m_Width && (Int32)mipSize == surface.m_Height );

			mipData.ResizeFast( mipSize * mipSize * facePixelByteSize );

			surface.GetImageData( isFaceHDR ? CP_VAL_FLOAT32 : CP_VAL_UNORM8, 4, mipSize * facePixelByteSize, mipData.Data() );

			CBitmapTexture::MipMap& mip = outMipMaps[face_i][mip_i];

			if ( !CBitmapTexture::CreateMip( mip, clampedSize, clampedSize, faceRawFormat, compression ) )
			{
				WARN_ENGINE( TXT("GpuApi: Failed to create mip chain for cube %ix%i, %ls"), outputSize, outputSize, debugName.AsChar() );
				return false;
			}

			if ( !CBitmapTexture::ConvertBuffer( mipSize, mipSize, mipSize * facePixelByteSize, 
				useFloatPrecisionInput ? GpuApi::TEXFMT_Float_R32G32B32A32 : GpuApi::TEXFMT_R8G8B8A8, 
				mipData.Data(),
				mipData.DataSize(),
				targetFormat, 
				mip.m_data.GetData(),
				GetImageCompressionHint( compression ) ) )
			{
				// No compression, direct copy
				Red::System::MemoryCopy( mip.m_data.GetData(), mipData.TypedData(), mipSize * mipSize * facePixelByteSize );
			}
		}
	}

	return true;
}


void CCubeTexture::GenerateCachedData( CCubeTexture::CookedData &outData )
{
	const CubeFace* faces[6] = { &m_left, &m_right, &m_front, &m_back, &m_top, &m_bottom };

	// Make sure we have some data to generate from.
	{
		Bool missingData = false;
		for ( Uint32 face_i = 0; face_i < 6; ++face_i )
		{
			if ( faces[face_i]->m_texture == nullptr && faces[face_i]->m_sourceTexture == nullptr )
			{
				missingData = true;
				break;
			}
		}
		if ( missingData )
		{
			DATA_HALT( DES_Minor, this, TXT("Rendering"), TXT("Cube texture is missing data in one or more faces.") );
		}
	}

	// Determine size. The final edge size will be the largest dimension from all faces, rounded up to a power of two.
	Uint32 cubeSize = 1;
	cubeSize = Max( cubeSize, m_front.GetWidth(),	m_front.GetHeight() );
	cubeSize = Max( cubeSize, m_back.GetWidth(),	m_back.GetHeight() );
	cubeSize = Max( cubeSize, m_left.GetWidth(),	m_left.GetHeight() );
	cubeSize = Max( cubeSize, m_right.GetWidth(),	m_right.GetHeight() );
	cubeSize = Max( cubeSize, m_top.GetWidth(),		m_top.GetHeight() );
	cubeSize = Max( cubeSize, m_bottom.GetWidth(),	m_bottom.GetHeight() );

	cubeSize = RoundUpToPow2( cubeSize );

	const Uint32 cubeSizeFromMip0 = cubeSize;

	// If we have an override for face size, use that instead.
	cubeSize = m_targetFaceSize > 0 ? m_targetFaceSize : cubeSize;

#ifndef NO_DATA_ASSERTS
	// Verify that the face textures are a proper size. We'll resize them as necessary, but it's best if the source
	// images are the same size.
	for ( Uint32 face_i = 0; face_i < 6; ++face_i )
	{
		const CubeFace& face = *faces[face_i];
		if ( face.GetWidth() != cubeSize || face.GetHeight() != cubeSize )
		{
			DATA_HALT( DES_Minor, this, TXT("Rendering"), TXT("Cube face is not the expected size. It'll be resized to work, but you should keep all faces the same size. Expected: %ux%u, Actual: %ux%u"),
				cubeSize, cubeSize, face.GetWidth(), face.GetHeight() );
		}
	}
#endif // !NO_DATA_ASSERTS


	// Determine dest format
	GpuApi::eTextureFormat destTexFormat = GetPlatformSpecificCompression();

	// mip chains for 6 faces
	CBitmapTexture::MipArray mipMaps[6];
	if ( !BuildCubeMips( faces, cubeSizeFromMip0, cubeSize, m_strategy, m_compression, destTexFormat, GetFriendlyName(), mipMaps ) )
	{
		RED_HALT( "Failed to create cube mip chain, %ls", GetFriendlyName().AsChar() );
		return;
	}

	const Uint16 numMips = static_cast< Uint16 >( mipMaps[0].Size() );

	// Fill in header
	outData.m_header.m_edgeSize = static_cast< Uint16 >( cubeSize );
	outData.m_header.SetTextureFormat( destTexFormat );
	outData.m_header.SetIsCooked( false );
	outData.m_header.m_mipCount = static_cast< Uint16 >( mipMaps[0].Size() );
	outData.m_header.m_residentMip = 0;

	// And copy the data. Since this is not cooked data, we just pack them in order.
	// Total size of the data is the size of one face's mip chain * 6
	Uint32 dataSize = 0;
	for ( Uint16 mip_i = 0; mip_i < numMips; ++mip_i )
	{
		dataSize += mipMaps[0][mip_i].m_data.GetSize();
	}
	dataSize *= 6;

	outData.m_deviceData.Allocate( dataSize );
	void* outDataPtr = outData.m_deviceData.GetDataHandle().GetRawPtr();
	for ( Uint16 face_i = 0; face_i < 6; ++face_i )
	{
		for ( Uint16 mip_i = 0; mip_i < numMips; ++mip_i )
		{
			void* mipData = mipMaps[face_i][mip_i].m_data.GetData();
			const size_t mipSize = mipMaps[face_i][mip_i].m_data.GetSize();

			Red::MemoryCopy( outDataPtr, mipData, mipSize );

			outDataPtr = OffsetPtr( outDataPtr, mipSize );
		}
	}
}


#endif // !NO_CUBEMAP_GENERATION



// --------------------------------------------------------------------------


// This still exists for loading old cube textures.
namespace CubeCookingUtils
{

	struct SCookedCubeHeader
	{
		enum
		{
			TAG_VALUE = 0x98abc666
		};

		Uint32	tag;
		Uint32	reserved;
		Uint32	formatEncoded;
		Uint32	edgeSize;
		Uint16	mipCount;
		Uint32	totalDataSize;

		Bool IsValidCubeHeader() const
		{
			return
				TAG_VALUE == tag &&
				ITexture::DecodeTextureFormat( (Uint16)formatEncoded ) != GpuApi::TEXFMT_Max &&
				edgeSize >= 4 &&
				IsPow2(edgeSize) &&
				mipCount > 0 && 
				(Uint32)(1<<(mipCount-1)) <= edgeSize &&
				totalDataSize > sizeof(SCookedCubeHeader);
		}
	};

	struct SCookedCubeMipInfo
	{
		Uint32 dataOffset;
		Uint32 dataSize;
	};


	static const SCookedCubeHeader* GetHeader( const void* buffer )
	{
		return static_cast< const SCookedCubeHeader* >( buffer );
	}

	static const SCookedCubeMipInfo* GetMipInfo( const void* buffer, Uint16 face, Uint16 mip )
	{
		const SCookedCubeHeader* header = GetHeader( buffer );
		const SCookedCubeMipInfo* mipInfos = static_cast< const SCookedCubeMipInfo* >( OffsetPtr( buffer, sizeof( SCookedCubeHeader ) ) );
		return &mipInfos[ mip + face * header->mipCount ];
	}

	static const void* GetResidentMipData( const void* buffer, Uint16 face, Uint16 mip )
	{
		const SCookedCubeMipInfo* mipInfo = GetMipInfo( buffer, face, mip );
		if ( mipInfo == nullptr )
		{
			return nullptr;
		}

		return OffsetPtr( buffer, mipInfo->dataOffset );
	}
}


void CCubeTexture::UpgradeOldCachedData( const DataBuffer& deviceData, const DataBuffer& /*systemData*/ )
{
	const CubeCookingUtils::SCookedCubeHeader* header = CubeCookingUtils::GetHeader( deviceData.GetData() );

	// If we don't have any old cooked data, or it's invalid, just generate it again.
	if ( header == nullptr || !header->IsValidCubeHeader() )
	{
#ifndef NO_CUBEMAP_GENERATION
		DeterminePlatformSpecificCompression( m_platformSpecificCompression );
		GenerateCachedData( m_cookedData );
#else
		WARN_ENGINE( TXT("Cube Texture '%ls' does not have valid cached data, and cube generation is disabled. Cannot load!"), GetFriendlyName().AsChar() );
#endif
		return;
	}

	m_cookedData.m_header.m_edgeSize = static_cast< Uint16 >( header->edgeSize );
#ifndef NO_EDITOR
	m_cookedData.m_header.SetTextureFormat( ITexture::DecodeTextureFormat( (Uint16)header->formatEncoded ) );
	m_cookedData.m_header.SetIsCooked( false );
#endif	
	m_cookedData.m_header.m_mipCount = header->mipCount;
	m_cookedData.m_header.m_residentMip = 0;


	// Total size of the data is the size of one face's mip chain * 6
	Uint32 dataSize = 0;
	for ( Uint16 mip_i = 0; mip_i < header->mipCount; ++mip_i )
	{
		dataSize += CubeCookingUtils::GetMipInfo( deviceData.GetData(), 0, mip_i )->dataSize;
	}
	dataSize *= 6;

	m_cookedData.m_deviceData.Allocate( dataSize );
	void* outDataPtr = m_cookedData.m_deviceData.GetDataHandle().GetRawPtr();
	for ( Uint16 face_i = 0; face_i < 6; ++face_i )
	{
		for ( Uint16 mip_i = 0; mip_i < header->mipCount; ++mip_i )
		{
			const void* mipData = CubeCookingUtils::GetResidentMipData( deviceData.GetData(), face_i, mip_i );
			const size_t mipSize = CubeCookingUtils::GetMipInfo( deviceData.GetData(), 0, mip_i )->dataSize;

			Red::MemoryCopy( outDataPtr, mipData, mipSize );

			outDataPtr = OffsetPtr( outDataPtr, mipSize );
		}
	}
}


// --------------------------------------------------------------------------


Uint32 CubeFace::GetWidth() const
{
	if ( m_sourceTexture ) return m_sourceTexture->GetWidth(); 
	if ( m_texture ) return m_texture->GetWidth(); 
	return 0;
}

Uint32 CubeFace::GetHeight() const
{
	if ( m_sourceTexture ) return m_sourceTexture->GetHeight(); 
	if ( m_texture ) return m_texture->GetHeight(); 
	return 0;
}

Bool CubeFace::GetSize( Uint32& width, Uint32& height ) const
{
	if ( m_sourceTexture )
	{
		width = m_sourceTexture->GetWidth();
		height = m_sourceTexture->GetHeight();
		return true;
	}
	if ( m_texture )
	{
		width = m_texture->GetWidth();
		height = m_texture->GetHeight();
		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////


#ifndef NO_RESOURCE_COOKING

void CCubeTexture::OnCook( ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

#ifndef NO_TEXTURECACHE_COOKER
	if ( GTextureCacheCooker == nullptr )
	{
		RED_HALT( "No texture cache cooker" );
		return;
	}

	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( cooker.GetPlatform() );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return;
	}

	const Uint32 residentMipMap = GetResidentMipForCookingPlatform( cooker.GetPlatform() );

	CCubeTexture::CookedData fullCachedCube;
	if ( ShouldRecreateForCook() )
	{
		// We need to generate the full mip chain, not just the resident portion. We're not just doing a straightforward downsample, there
		// may be some extra blurring happening at each level, so we need the full chain for consistency.
		GenerateCachedData( fullCachedCube );
	}
	else
	{
		fullCachedCube = GetCookedData();
	}


	// Cook just the resident data.
	CTextureBakerSourceCubeTexture residentSource( fullCachedCube, residentMipMap );
	CTextureBakerOutput residentCooked;
	if ( !CookFunction( residentSource, residentCooked ) )
	{
		RED_HALT( "Failed to cook resident data for %s", GetFriendlyName().AsChar() );
		return;
	}

	// Resident data header.
	m_cookedData.m_header = fullCachedCube.m_header;
	m_cookedData.m_header.m_textureCacheKey = CalcTextureCacheKey();
	m_cookedData.m_header.SetIsCooked( true );
	m_cookedData.m_header.m_residentMip = residentMipMap;

	m_cookedData.m_deviceData.Allocate( residentCooked.GetTotalDataSize() );
	Red::System::MemoryCopy( m_cookedData.m_deviceData.GetDataHandle().GetRawPtr(), residentCooked.GetData(), residentCooked.GetTotalDataSize() );
#endif
}

Bool CCubeTexture::ShouldRecreateForCook() const
{
	// If we don't have any cached data, we need to recreate.
	if ( !GetCookedData().IsValid() )
	{
		return true;
	}

	// Prefer recreating the cube mip chain. But, if we're missing source data we'll use whatever we have.
	for ( Uint16 face_i = 0; face_i < 6; ++face_i )
	{
		const CubeFace& face = GetFace( face_i );
		if ( face.m_sourceTexture == nullptr && face.m_texture == nullptr )
		{
			return false;
		}
	}

	return true;
}

Uint32 CCubeTexture::GetMipCountForCookingPlatform( ECookingPlatform platform ) const
{
	// Determine size. The final edge size will be the largest dimension from all faces, rounded up to a power of two.
	Uint32 cubeSize = 1;
	cubeSize = Max( cubeSize, m_front.GetWidth(),	m_front.GetHeight() );
	cubeSize = Max( cubeSize, m_back.GetWidth(),	m_back.GetHeight() );
	cubeSize = Max( cubeSize, m_left.GetWidth(),	m_left.GetHeight() );
	cubeSize = Max( cubeSize, m_right.GetWidth(),	m_right.GetHeight() );
	cubeSize = Max( cubeSize, m_top.GetWidth(),		m_top.GetHeight() );
	cubeSize = Max( cubeSize, m_bottom.GetWidth(),	m_bottom.GetHeight() );
	cubeSize = RoundUpToPow2( cubeSize );

	const Uint32 cubeSizeFromMip0 = cubeSize;

	// If we have an override for face size, use that instead.
	cubeSize = m_targetFaceSize > 0 ? m_targetFaceSize : cubeSize;

	const Uint32 numMips = MLog2( cubeSize ) + 1;

	return numMips;
}

Uint32 CCubeTexture::GetResidentMipForCookingPlatform( ECookingPlatform platform ) const
{
	const Uint16 numMips = GetMipCountForCookingPlatform( platform );

	// Determine first mipmap to use for texture
	Uint16 residentMipMap = 0;
	// up to 32x32 will be kept resident
	const Uint16 maxResidentMips = 6;

	//const TextureGroup& texGroup = texture->GetTextureGroup();
	if ( /*texGroup.m_isStreamable &&*/ numMips > maxResidentMips  )
	{
		residentMipMap = numMips - maxResidentMips;
	}

	return residentMipMap;
}

#endif // !NO_RESOURCE_COOKING


#ifndef NO_TEXTURECACHE_COOKER

CTextureBakerSourceCubeTexture::CTextureBakerSourceCubeTexture( const CCubeTexture::CookedData& cookedData, Uint16 baseMip )
	: m_cookedData( cookedData )
	, m_baseMip( baseMip )
{
	RED_WARNING( !m_cookedData.m_header.IsCooked(), "CookedData is already in a cooked format!" );

	m_format = m_cookedData.m_header.GetTextureFormat();

	// Figure out how much data we can skip from the start of a cube face's mip chain to m_baseMip. This is the same value for all faces.
	m_offsetToBaseMip = 0;
	for ( Uint16 mip_i = 0; mip_i < m_baseMip; ++mip_i )
	{
		const Uint16 mipEdge = m_cookedData.m_header.m_edgeSize >> mip_i;
		const Uint32 mipSize = GpuApi::CalculateTextureSize( mipEdge, mipEdge, m_format );
		m_offsetToBaseMip += mipSize;
	}
}

Uint16 CTextureBakerSourceCubeTexture::GetSliceCount() const
{
	return 6;
}

Uint16 CTextureBakerSourceCubeTexture::GetMipCount() const
{
	return m_cookedData.m_header.m_mipCount - m_baseMip;
}

const void* CTextureBakerSourceCubeTexture::GetMipData( Uint16 mip, Uint16 slice ) const
{
	Uint16 mipCount = GetMipCount();

	// Need to manually skip ahead to the proper spot.
	const void* ptr = m_cookedData.m_deviceData.GetDataHandle().GetRawPtr();
	for ( Uint16 tex_i = 0; tex_i < 6; ++tex_i )
	{
		// Skip ahead to m_baseMip.
		ptr = OffsetPtr( ptr, m_offsetToBaseMip );

		for ( Uint16 mip_i = 0; mip_i < mipCount; ++mip_i )
		{
			if ( tex_i == slice && mip_i == mip )
			{
				return ptr;
			}

			ptr = OffsetPtr( ptr, GetMipDataSize( mip_i, tex_i ) );
		}
	}
	return nullptr;
}

Uint32 CTextureBakerSourceCubeTexture::GetMipDataSize( Uint16 mip, Uint16 /*slice*/ ) const
{
	const Uint16 mipSize = GetMipEdgeLength( mip );
	return GpuApi::CalculateTextureSize( mipSize, mipSize, m_format );
}

Uint32 CTextureBakerSourceCubeTexture::GetMipPitch( Uint16 mip, Uint16 /*slice*/ ) const
{
	const Uint16 mipSize = GetMipEdgeLength( mip );
	return GpuApi::CalculateTexturePitch( mipSize, m_format );
}

Uint16 CTextureBakerSourceCubeTexture::GetBaseWidth() const
{
	return GetMipEdgeLength( 0 );
}

Uint16 CTextureBakerSourceCubeTexture::GetBaseHeight() const
{
	return GetMipEdgeLength( 0 );
}

GpuApi::eTextureFormat CTextureBakerSourceCubeTexture::GetTextureFormat() const
{
	return m_format;
}

GpuApi::eTextureType CTextureBakerSourceCubeTexture::GetTextureType() const
{
	return GpuApi::TEXTYPE_CUBE;
}

Uint16 CTextureBakerSourceCubeTexture::GetMipEdgeLength( Uint16 mip ) const
{
	return m_cookedData.m_header.m_edgeSize >> ( mip + m_baseMip );
}

#endif // !NO_TEXTURECACHE_COOKER
