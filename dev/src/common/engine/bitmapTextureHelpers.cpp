#include "build.h"
#include "bitmapTexture.h"
#include "../gpuApiUtils/gpuApiInterface.h"
#ifndef NO_TEXTURE_EDITING
#include "../gpuApiDX10/gpuApiBase.h"
#endif

#include "renderCommands.h"
#include "renderFence.h"
#include "textureGroup.h"
#include "material.h"


namespace DXTUtils
{
	void CopyMemSlice			( Uint8 * out, Uint32 dstPitch, const Rect & dstRct, const Uint8 * in, Uint32 srcPitch, const Rect & srcRct, Uint32 bytesPerPixel );

	void SwizzleNormalsHigh		( Uint8 * out, const Uint8 * in, Uint32 width, Uint32 height );
	void DeswizzleNormalsHigh	( Uint8 * out, const Uint8 * in, Uint32 width, Uint32 height );

	Bool IsSwizzled				( ETextureRawFormat format, ETextureCompression compression );

	Bool SwizzleStep			( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression );
	Bool DeswizzleStep			( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression );

	Uint32 GetNumRows			( Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression );
	Uint32 GetPitch				( Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression );
}


namespace
{
	//** ************************************
	//
	Bool IsValidDX11Format( Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression )
	{
		if( width < 4 || height < 4 )
		{
			Bool bDTX = compression == TCM_DXTNoAlpha || compression == TCM_DXTAlpha || compression == TCM_DXTAlphaLinear || compression == TCM_Normals || compression == TCM_NormalsHigh || compression == TCM_NormalsGloss || compression == TCM_QualityR || compression == TCM_QualityRG || compression == TCM_QualityColor;
			return !bDTX;
		}

		return true;
	}

} //namespace


void CBitmapTexture::GenerateMipmaps()
{
#ifndef NO_TEXTURE_EDITING
	Bool hasMipchain = GetTextureGroup().m_hasMipchain;

	if ( m_sourceData )
	{
		if ( !m_sourceData->m_dataBuffer.Load() )
		{
			ASSERT( !"Unable to load data buffer to generate mipmaps" );
		}

		// Generate mipmap chain
		const Bool generateFullMipChain = hasMipchain;
		m_sourceData->CreateMips( m_mips, m_width, m_height, GetCompression(), generateFullMipChain );

		m_sourceData->m_dataBuffer.Unload();
	}
	else if ( hasMipchain )
	{
		// No source data :( Generate from mip chain

		MarkModified();

		CSourceTexture* sourceTexture = CreateObject< CSourceTexture >( (CObject*) NULL );
		sourceTexture->Init( m_width, m_height, m_format );
		sourceTexture->CreateFromMipCompressed( m_mips[0], m_compression );

		sourceTexture->CreateMips( m_mips, m_width, m_height, m_compression, true );

		sourceTexture->Discard();

		// Upload to card
		if ( m_renderResource )
		{
			CreateRenderResource();
		}
	}
#endif
}


Bool CBitmapTexture::InitFromSourceData( CSourceTexture* sourceTexture, CName textureGroup, Bool silent /*= false*/ )
{
#ifndef NO_TEXTURE_EDITING
	ASSERT( sourceTexture->m_width > 0 && sourceTexture->m_height > 0 );

	if ( !( sourceTexture->m_width > 0 && sourceTexture->m_height > 0 ) )
	{
		return false;
	}

	if ( !MarkModified() )
	{
		return false;
	}

	// Setup initial data
	m_width = sourceTexture->m_width;
	m_height = sourceTexture->m_height;
	m_format = sourceTexture->m_format;

	if ( textureGroup != CName::NONE )
	{
		m_textureGroup = textureGroup;
	}

	m_mips.Clear();

	// Set initial compression from texture group we are using
	const TextureGroup& group = GetTextureGroup();
	m_compression = group.m_compression;

	// Get platform-specific compression
	GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, false );

	// Delete existing artist data
	if ( m_sourceData )
	{
		m_sourceData->Discard();
		m_sourceData = NULL;
	}

	m_sourceData = sourceTexture;
	m_sourceData->SetParent( this );

	if ( !m_sourceData->m_dataBuffer.Load() )
	{
		return false;
	}

	m_sourceData->CreateMips( m_mips, m_width, m_height, GetCompression(), group.m_hasMipchain, false, false, false, silent );

	m_sourceData->m_dataBuffer.Unload();

	// Cancel streaming
	if ( !silent && GRender )
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// Upload to card
	CreateRenderResource();

	// Recompile materials
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	IMaterial::RecompileMaterialsUsingTexture( this );
#endif

	//CDrawableComponent::RecreateProxiesOfRenderableComponents();

#endif

	return true;

}

Bool CBitmapTexture::InitEmpty( Uint32 width, Uint32 height, ETextureRawFormat format, CName textureGroup, Uint8 fillByte/*=0 */ )
{
#ifndef NO_TEXTURE_EDITING
	// Release current rendering resource
	ReleaseRenderResource();

	// Delete streaming info
	SAFE_RELEASE( m_streamingSource );

	// Setup initial data
	m_width = width;
	m_height = height;
	m_format = format;
	m_textureGroup = textureGroup;

	m_sourceData = NULL;

	// Set initial compression from texture group we are using
	const TextureGroup& group = GetTextureGroup();
	m_compression = group.m_compression;

	// Create the uncompressed data
	MipMap uncompressedMip;
	uncompressedMip.m_width = width;
	uncompressedMip.m_height = height;
	uncompressedMip.m_pitch = width * ( GetPixelSize( format ) / 8 );
	uncompressedMip.m_data.Allocate( uncompressedMip.m_pitch * height );
	Red::System::MemorySet( uncompressedMip.m_data.GetData(), fillByte, uncompressedMip.m_pitch * height );

	// Compress the mipmap
	const Bool generateMips = group.m_hasMipchain;
	if ( !CompressMipChain( uncompressedMip, format, m_mips, m_compression, generateMips ) )
	{
		ASSERT( "Unable to compress mip chain" );
		return false;
	}

#endif 

	return true;
}

Bool CBitmapTexture::InitFromMip( const MipMap& mip, CName textureGroupName, ETextureRawFormat textureFormat )
{
#ifndef NO_TEXTURE_EDITING
	ASSERT( mip.m_width > 0 && mip.m_height > 0 );

	if ( !( mip.m_width > 0 && mip.m_height > 0 ) )
	{
		return false;
	}

	if ( !MarkModified() )
	{
		return false;
	}

	// Setup initial data
	m_width = mip.m_width;
	m_height = mip.m_height;
	m_format = textureFormat;

	if ( textureGroupName != CName::NONE )
	{
		m_textureGroup = textureGroupName;
	}

	m_mips.Clear();

	// Set initial compression from texture group we are using
	const TextureGroup& group = GetTextureGroup();
	m_compression = group.m_compression;

	// Get platform-specific compression
	GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, false );

	// Delete existing artist data
	if ( m_sourceData )
	{
		m_sourceData->Discard();
		m_sourceData = NULL;
	}

	// Generate mipmap chain
	const Bool generateFullMipChain = group.m_hasMipchain;
	CompressMipChain( mip, GetFormat(), m_mips, GetCompression(), generateFullMipChain );

	// Cancel streaming
	if ( GRender )
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// Upload to card
	CreateRenderResource();

	// Recompile materials
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	IMaterial::RecompileMaterialsUsingTexture( this );
#endif

	CDrawableComponent::RecreateProxiesOfRenderableComponents();

#endif

	return true;
}


Bool CBitmapTexture::InitFromCompressedMip( const MipMap& mip, CName textureGroupName, ETextureRawFormat textureFormat )
{
#ifndef NO_TEXTURE_EDITING
	ASSERT( mip.m_width > 0 && mip.m_height > 0 );

	if ( !( mip.m_width > 0 && mip.m_height > 0 ) )
	{
		ERR_ENGINE(TXT("CBitmapTexture::InitFromCompressedMip: invalid mip dimensions w:%u, h:%u"), mip.m_width, mip.m_height);
		return false;
	}

	if ( !MarkModified() )
	{
		ERR_ENGINE(TXT("CBitmapTexture::InitFromCompressedMip: failed to mark resource modified!"));
		return false;
	}

	// Setup initial data
	m_width = mip.m_width;
	m_height = mip.m_height;
	m_format = textureFormat;

	if ( textureGroupName != CName::NONE )
	{
		m_textureGroup = textureGroupName;
	}

	m_mips.Clear();

	// Set initial compression from texture group we are using
	const TextureGroup& group = GetTextureGroup();
	m_compression = group.m_compression;

	// Get platform-specific compression
	GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, false );
	
	// Delete existing artist data
	if ( m_sourceData )
	{
		m_sourceData->Discard();
		m_sourceData = NULL;
	}

	m_mips.ClearFast();
	m_mips.PushBack( mip );
	GenerateMipmaps();

	// Cancel streaming
	if ( GRender )
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}

	// Upload to card
	CreateRenderResource();

	// Recompile materials
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	IMaterial::RecompileMaterialsUsingTexture( this );
#endif

	CDrawableComponent::RecreateProxiesOfRenderableComponents();

#endif

	return true;
}

//** ************************************
//
void CBitmapTexture::DropMipmaps( Int32 numMipmapsToDrop )
{
#ifndef NO_TEXTURE_EDITING
	
#ifdef RED_FINAL_BUILD
	const Bool final = true;
#else
	const Bool final = false;
#endif

	// Ultra nice mips only on final builds
	if ( GetSourceData() && final )
	{
		m_width = m_width >> numMipmapsToDrop;
		m_height = m_height >> numMipmapsToDrop;

		m_sourceData->CreateMips( m_mips, m_width, m_height, m_compression, true );
	}
	else
	{
		// No source data :( 

		// Remove mipmaps
		while ( (numMipmapsToDrop--) && ( m_mips.Size() > 1 ) )
		{
			m_mips.Erase( m_mips.Begin() );
		}

		// Update size
		m_width = m_mips[0].m_width;
		m_height = m_mips[0].m_height;
	}
#endif
}


//** ************************************
//
Uint32 CBitmapTexture::GetPixelSize( ETextureRawFormat rawFormat )
{
	switch ( rawFormat )
	{
		case TRF_TrueColor: return 32;
		case TRF_Grayscale: return 8;
		case TRF_Grayscale_Font: return 8;
		case TRF_HDR: return 128;
		case TRF_AlphaGrayscale: return 16;
		case TRF_HDRGrayscale: return 32;
	}

	ASSERT( !"Unknown texture raw format" );
	return 32;
}

//** ************************************
//
Uint32 CBitmapTexture::GetNumChannels( ETextureRawFormat rawFormat )
{
	switch ( rawFormat )
	{
		case TRF_TrueColor: return 4;
		case TRF_Grayscale: return 1;
		case TRF_Grayscale_Font: return 1;
		case TRF_HDR: return 4;
		case TRF_AlphaGrayscale: return 2;
		case TRF_HDRGrayscale: return 1;
	}

	ASSERT( !"Unknown texture raw format" );
	return 4;
}

//** ************************************
//
Bool CBitmapTexture::CreateMip( MipMap & destData, Uint32 width, Uint32 height, ETextureRawFormat destFormat, ETextureCompression destCompression )
{
#ifndef NO_TEXTURE_EDITING
	// No data in source texture
	if ( width == 0 || height == 0 )
	{
		ERR_ENGINE( TXT("CreateMip: empty source data, unable to compress.") );

		return false;
	}

	// This is the case which causes DX9 to fail - let's leave it failing
	if( !IsValidDX11Format( width, height, destFormat, destCompression ) )
	{
		// Since it's ok to "leave it failing", I'll comment this out to avoid flooding ModKit logs.
		//ERR_ENGINE(TXT("CreateMip: !IsValidDX11Format"));
		return false;
	}

	// Create mipmap
	destData.m_width	= width;
	destData.m_height	= height;
	destData.m_pitch	= DXTUtils::GetPitch( width, height, destFormat, destCompression );

	Uint32 numRows		= DXTUtils::GetNumRows( width, height, destFormat, destCompression );

	// Calculate required data size
	Uint32 dataSize = numRows * destData.m_pitch;

	// Unlink any streaming
	destData.m_data.Unlink();

	// Copy data
	destData.m_data.Allocate( dataSize );

#endif

	// Done
	return true;
}

//** ************************************
//
Bool CBitmapTexture::CopyRect( const MipMap& srcData, const Rect* srcRect, MipMap& destData, const Rect* destRect, ETextureRawFormat format, ETextureCompression compression )
{
#ifndef NO_TEXTURE_EDITING
	// Fill destination rect
	Rect dstMipRect;
	if ( destRect )
	{
		dstMipRect.m_left	= destRect->m_left;
		dstMipRect.m_top	= destRect->m_top;
		dstMipRect.m_right	= destRect->m_right;
		dstMipRect.m_bottom	= destRect->m_bottom;
	}
	else 
	{
		dstMipRect.m_left		= 0;
		dstMipRect.m_top		= 0;
		dstMipRect.m_right	= destData.m_width;
		dstMipRect.m_bottom	= destData.m_height;
	}

	// Fill source rect
	Rect srcMipRect;
	if ( srcRect )
	{
		srcMipRect.m_left		= srcRect->m_left;
		srcMipRect.m_top		= srcRect->m_top;
		srcMipRect.m_right	= srcRect->m_right;
		srcMipRect.m_bottom	= srcRect->m_bottom;
	}
	else 
	{
		srcMipRect.m_left		= 0;
		srcMipRect.m_top		= 0;
		srcMipRect.m_right	= srcData.m_width;
		srcMipRect.m_bottom	= srcData.m_height;
	}

	// Create tables
	TDynArray< Rect > srcRects;
	srcRects.PushBack( srcMipRect );
	TDynArray< Rect > destRects;
	destRects.PushBack( dstMipRect );

	// Copy
	return CopyRects( srcData, srcRects, destData, destRects, format, compression );
#else
	return false;
#endif
}

//** ************************************
//
Bool CBitmapTexture::CopyBitmap( CBitmapTexture* destBitmap, const Rect* destRect, const CBitmapTexture* srcBitmap, const Rect* srcRect )
{
#ifndef NO_TEXTURE_EDITING
	// No bitmaps
	if ( !destBitmap || !srcBitmap )
	{
		ERR_ENGINE( TXT("CopyBitmap: no bitmaps") );
		return false;
	}

	// Check if bitmaps are empty
	if ( !destBitmap->GetMipCount() || !srcBitmap->GetMipCount() )
	{
		ERR_ENGINE( TXT("CopyBitmap: empty bitmaps") );
		return false;
	}

	// Invalid format
	if ( destBitmap->GetFormat() != srcBitmap->GetFormat() || destBitmap->GetCompression() != srcBitmap->GetCompression() )
	{
		ERR_ENGINE( TXT("CopyBitmap: incompatible bitmap formats") );
		return false;
	}

	// Copy data
	const CBitmapTexture::MipMap& srcMip = srcBitmap->GetMips()[0];
	CBitmapTexture::MipMap& destMip = const_cast<CBitmapTexture::MipMap&>( destBitmap->GetMips()[0] );

	// Copy data
	if ( !CopyRect( srcMip, srcRect, destMip, destRect, srcBitmap->GetFormat(), srcBitmap->GetCompression() ) )
	{
		ERR_ENGINE( TXT("CopyRect: copy rect failed") );
		return false;
	}

	// Done !
	destBitmap->GenerateMipmaps();

	return true;

#else
	return false;
#endif
}

//** ************************************
//
Bool CBitmapTexture::CopyRect( const MipMap & srcData, ETextureRawFormat format, ETextureCompression srcCompression, MipMap & destData, ETextureCompression destCompression )
{
#ifndef NO_TEXTURE_EDITING
	CSourceTexture* sourceTexture = CreateObject< CSourceTexture >( (CObject*)NULL );
	sourceTexture->Init( srcData.m_width, srcData.m_height, format );

	sourceTexture->CreateFromMipCompressed( srcData, srcCompression );

	MipArray mips;
	sourceTexture->CreateMips( mips, destData.m_width, destData.m_height, destCompression, false );

	if ( mips.Size() > 0 )
	{
		destData = mips[0];

		return true;
	}

#endif

	return false;
}



//** ************************************
//
Bool CBitmapTexture::CompressMipChain( const CBitmapTexture::MipMap & srcData, ETextureRawFormat srcFormat, MipArray & destMips, ETextureCompression destCompression, Bool generateAllMips )
{
#ifndef NO_TEXTURE_EDITING
	CSourceTexture* sourceTexture = CreateObject< CSourceTexture >( (CObject*) NULL );

	sourceTexture->Init( srcData.m_width, srcData.m_height, srcFormat );
	sourceTexture->CreateFromMip( srcData );

	sourceTexture->CreateMips( destMips, srcData.m_width, srcData.m_height, destCompression, generateAllMips );

	sourceTexture->Discard();

#endif

	return true;
}



//** ************************************
//
Bool CBitmapTexture::CopyRects( const MipMap & srcData, const TDynArray< Rect > & srcRects, MipMap & destData, const TDynArray< Rect > & destRects, ETextureRawFormat format, ETextureCompression compression )
{
#ifndef NO_TEXTURE_EDITING
	// Invalid rect count
	if ( !srcRects.Size() || srcRects.Size() != destRects.Size() )
	{
		return false;
	}

	ASSERT( compression == TCM_None );

	for( Uint32 i = 0; i < destRects.Size(); ++i )
	{
		// Setup dest rect
		Rect dstMipRect = destRects[ i ];

		// Setup src rect
		Rect srcMipRect = srcRects[ i ];

		// Check target surface
		ASSERT( dstMipRect.m_left >= 0 );
		ASSERT( dstMipRect.m_top >= 0 );
		ASSERT( dstMipRect.m_right <= (Int32) destData.m_width );
		ASSERT( dstMipRect.m_bottom <= (Int32) destData.m_height );

		// Check source surface
		ASSERT( srcMipRect.m_left >= 0 );
		ASSERT( srcMipRect.m_top >= 0 );
		ASSERT( srcMipRect.m_right <= (Int32) srcData.m_width );
		ASSERT( srcMipRect.m_bottom <= (Int32) srcData.m_height );

		if ( ( ( dstMipRect.m_right - dstMipRect.m_left ) == ( srcMipRect.m_right - srcMipRect.m_left ) ) && 
			( ( dstMipRect.m_bottom - dstMipRect.m_top ) == ( srcMipRect.m_bottom - srcMipRect.m_top ) ) )
		{
			DXTUtils::CopyMemSlice( (Uint8*)destData.m_data.GetData(), destData.m_pitch, dstMipRect, (Uint8*)srcData.m_data.GetData(), srcData.m_pitch, srcMipRect, GetPixelSize( format ) / 8 );
		}
		else
		{
			ASSERT( !"Not supported, shouldn't happen - ask programmers!" );	
		}
	}

	return true;
#else
	return false;
#endif
}

//** ************************************

Bool CBitmapTexture::GetCompressedFormat( ETextureRawFormat format, ETextureCompression compression, GpuApi::eTextureFormat &outFormat, Bool oldVersion /* = true */ )
{
	switch ( compression )
	{
	case TCM_None:
		switch ( format )
		{
		case TRF_TrueColor:			outFormat = GpuApi::TEXFMT_R8G8B8A8;			return true;
		case TRF_Grayscale: 		outFormat = GpuApi::TEXFMT_L8;					return true;
		case TRF_Grayscale_Font:	outFormat = GpuApi::TEXFMT_A8;					return true;
		case TRF_HDR:				outFormat = GpuApi::TEXFMT_Float_R32G32B32A32;	return true;
		case TRF_AlphaGrayscale: 	outFormat = GpuApi::TEXFMT_A8L8;				return true;
		case TRF_HDRGrayscale: 		outFormat = GpuApi::TEXFMT_Float_R32;			return true;
		}
		ASSERT( !"Unknown source format" );
		break;

	case TCM_DXTNoAlpha:			outFormat = GpuApi::TEXFMT_BC1;					return true;
	case TCM_DXTAlpha:				
		{
#ifdef USE_NEW_COMPRESSION
			outFormat = oldVersion ? GpuApi::TEXFMT_BC3 : GpuApi::TEXFMT_BC7;		return true;
#else
			outFormat = GpuApi::TEXFMT_BC3; return true;
#endif
		}
	case TCM_RGBE:					outFormat = GpuApi::TEXFMT_Float_R32G32B32A32;	return true;
	case TCM_Normals: 				outFormat = GpuApi::TEXFMT_BC1;					return true;
	case TCM_NormalsHigh:
		{
#ifdef USE_NEW_COMPRESSION
			outFormat = oldVersion ? GpuApi::TEXFMT_BC3 : GpuApi::TEXFMT_BC7;		return true;
#else
			outFormat = GpuApi::TEXFMT_BC3;											return true;
#endif
		}

	case TCM_NormalsGloss:			outFormat = GpuApi::TEXFMT_BC3;					return true;
	case TCM_QualityR:				outFormat = GpuApi::TEXFMT_BC4;					return true;
	case TCM_QualityRG:				outFormat = GpuApi::TEXFMT_BC5;					return true;
	case TCM_QualityColor:			outFormat = GpuApi::TEXFMT_BC7;					return true;
	}

	RED_HALT( "Unknown compression/format: %i/%i", compression, format );
	return false;
}

//** ************************************

Bool CBitmapTexture::ConvertBuffer( Uint32 width, Uint32 height, Uint32 sourcePitch, GpuApi::eTextureFormat sourceFormat, const void* sourceData, size_t sourceDataSize, GpuApi::eTextureFormat targetFormat, void *outTargetData, GpuApi::EImageCompressionHint compressionHint, Float alphaThreshold /*= 0.5f*/ )
{
#ifndef NO_TEXTURE_EDITING
	if ( width == 0 || height == 0 || sourcePitch == 0 || nullptr == sourceData || nullptr == outTargetData )
	{
		RED_HALT( "Invalid parameters to ConvertBuffer" );
		return false;
	}

	// Same format, just do a pitched copy.
	if ( sourceFormat == targetFormat )
	{
		const Uint32 targetPitch = GpuApi::CalculateTexturePitch( width, targetFormat );
		const Uint32 rows = GpuApi::IsTextureFormatDXT( sourceFormat ) ? Max( height / 4, 1u ) : height;
		for ( Uint32 j = 0; j < rows; ++j )
		{
			const void* src = OffsetPtr( sourceData, j * sourcePitch );
			void* dst = OffsetPtr( outTargetData, j * targetPitch );
			Red::System::MemoryCopy( dst, src, targetPitch );
		}
		return true;
	}

	// decompression
	if ( GpuApi::IsTextureFormatDXT( sourceFormat ) && GpuApi::TEXFMT_R8G8B8A8 == targetFormat )
	{
		GpuApi::TextureDataDesc imageToDecompress;
		Uint8* pRawData = (Uint8*)sourceData;
		imageToDecompress.data = &pRawData;
		imageToDecompress.slicePitch = sourceDataSize;
		imageToDecompress.width = width;
		imageToDecompress.height = height;
		imageToDecompress.rowPitch = sourcePitch;
		imageToDecompress.format = sourceFormat;

		// required data: buffer to be filled and format used for compression
		GpuApi::TextureDataDesc decompressedImage;
		GpuApi::Uint8* pOutDecompressedData = (Uint8*)outTargetData;
		decompressedImage.data = &pOutDecompressedData;
		decompressedImage.format = targetFormat;

		ASSERT( GpuApi::CIH_None == compressionHint );
		return GpuApi::DecompressImage( imageToDecompress, decompressedImage );
	}

	// compression from ARGB
	if ( GpuApi::IsTextureFormatDXT( targetFormat ) && (GpuApi::TEXFMT_R8G8B8A8 == sourceFormat || GpuApi::TEXFMT_Float_R32G32B32A32 == sourceFormat) )
	{
		GpuApi::TextureDataDesc imageToCompress;
		Uint8* pRawData = (Uint8*)sourceData;
		imageToCompress.data = &pRawData;
		imageToCompress.slicePitch = sourceDataSize;
		imageToCompress.width = width;
		imageToCompress.height = height;
		imageToCompress.rowPitch = sourcePitch;
		imageToCompress.format = sourceFormat;
		
		// required data: buffer to be filled and format used for compression
		GpuApi::TextureDataDesc compressedImage;
		GpuApi::Uint8* pOutCompressedData = (Uint8*)outTargetData;
		compressedImage.data = &pOutCompressedData;
		compressedImage.format = targetFormat;

		return GpuApi::CompressImage( imageToCompress, compressedImage, compressionHint, alphaThreshold );
	}

	// compression from grayscale
	if ( GpuApi::TEXFMT_A8L8 == sourceFormat && GpuApi::TEXFMT_BC3 == targetFormat )
	{
		// BC3 needs full color channels, we have to create a proper buffer first
		TDynArray< Uint8 > argb( 4 * width * height );
		const Uint32 argbPitch = 4 * width;
		for ( Uint32 j = 0; j < height; ++j )
		{
			for ( Uint32 i = 0; i < width; ++i )
			{
				const Uint8 *src = (const Uint8*)sourceData + j * sourcePitch + 2 * i;
				Uint8 *dst = argb.TypedData() + j * argbPitch + 4 * i;
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[1];
				dst[3] = src[1];
			}
		}

		GpuApi::TextureDataDesc imageToCompress;
		Uint8* pData = (Uint8*)argb.TypedData();
		imageToCompress.data = &pData;
		imageToCompress.slicePitch = argb.DataSize();
		imageToCompress.width = width;
		imageToCompress.height = height;
		// Has been converted to RGBA, so need correct pitch/format.
		imageToCompress.rowPitch = 4 * width;
		imageToCompress.format = GpuApi::TEXFMT_R8G8B8A8;

		// required data: buffer to be filled and format used for compression
		GpuApi::TextureDataDesc compressedImage;
		Uint8* pOutCompressedData = (Uint8*)outTargetData;
		compressedImage.data = &pOutCompressedData;
		compressedImage.format = targetFormat;
		
		ASSERT( GpuApi::CIH_None == compressionHint );
		return GpuApi::CompressImage( imageToCompress, compressedImage, compressionHint, alphaThreshold );
	}

#endif

	return false;
}

Bool CBitmapTexture::SwizzlingStep( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat srcFormat, ETextureCompression srcCompression, ETextureRawFormat dstFormat, ETextureCompression dstCompression )
{
	// Swizzle before compressing (if necessary)
	Bool bSwizzle = DXTUtils::SwizzleStep( buf, width, height, dstFormat, dstCompression );

	// FIXME: Old API conformance ASSERT
	if( bSwizzle )
	{
		ASSERT( srcFormat == TRF_TrueColor && srcCompression == TCM_None );
	}

	// Deswizzle raw data (if necessary)
	Bool bDeswizzle = DXTUtils::DeswizzleStep( buf, width, height, srcFormat, srcCompression );

	// FIXME: Old API conformance ASSERT
	if( bDeswizzle )
	{
		ASSERT( !bSwizzle );
	}

	return bSwizzle || bDeswizzle;
}

//** ************************************
//
Bool DXTUtils::SwizzleStep		( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression )
{
	// Swizzle (if necessary)
	if( DXTUtils::IsSwizzled( format, compression ) )
	{
		if ( TCM_NormalsHigh == compression )
		{
			DXTUtils::SwizzleNormalsHigh( buf, buf, width, height );
		}
		else
		{
			RED_HALT( "Invalid Compression" );
		}

		return true;
	}

	return false;
}

//** ************************************
//
Bool	DXTUtils::DeswizzleStep	( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression )
{
	// Deswizzle (if necessary)
	if( DXTUtils::IsSwizzled( format, compression ) )
	{
		if ( compression == TCM_NormalsHigh )
		{
			DXTUtils::DeswizzleNormalsHigh( buf, buf, width, height );
		}
		else
		{
			RED_HALT( "Invalid Compression" );
		}

		return true;
	}

	return false;
}

//** ************************************
// 
void DXTUtils::CopyMemSlice	( Uint8 * out, Uint32 dstPitch, const Rect & dstRct, const Uint8 * in, Uint32 srcPitch, const Rect & srcRct, Uint32 bytesPerPixel )
{
	ASSERT( ( dstRct.m_right - dstRct.m_left ) == ( srcRct.m_right - srcRct.m_left ) );
	ASSERT( ( dstRct.m_bottom - dstRct.m_top ) == ( srcRct.m_bottom - srcRct.m_top ) );

	if ( ((dstRct.m_right - dstRct.m_left) != (srcRct.m_right - srcRct.m_left)) || (( dstRct.m_bottom - dstRct.m_top ) != ( srcRct.m_bottom - srcRct.m_top )) )
	{
		return;
	}

	in	= &in[ srcRct.m_top * srcPitch + srcRct.m_left * bytesPerPixel ];
	out	= &out[ dstRct.m_top * dstPitch + dstRct.m_left * bytesPerPixel ];
	
	Uint32 size	= ( dstRct.m_right - dstRct.m_left ) * bytesPerPixel;
	
	for( Int32 y = srcRct.m_top; y < srcRct.m_bottom; ++y, in += srcPitch, out += dstPitch )
	{
		Red::System::MemoryCopy( out, in, size );
	}
}

Float CalculateToksvigFactorForPixel( Uint8 *buf, Uint32 width, Uint32 height, Uint32 x, Uint32 y, Float glossiness )
{
	// ace_optimize

	struct Local	
	{
		static Float CalcGaussianWeight( Int32 i, Int32 j )
		{
			const Float sigma = 1.f; //< filter width [0..1]
			const Float v = 2.f * sigma * sigma;
			return exp( -(i*i + j*j) / v ) / ( M_PI*v );
		}

		static Vector SampleToksvigNormal( Uint8 *buf, Uint32 width, Uint32 height, Int32 i, Int32 j )
		{
			ASSERT( width > 0 && height > 0 );
			i = (i + width) % width;
			j = (j + height) % height;
			Uint32 off = 4 * (i + j * width);
			ASSERT( off < 4 * width * height );
			Float x = buf[off+0] / 127.5f - 0.5f;
			Float y = buf[off+1] / 127.5f - 0.5f;
			Float z = buf[off+2] / 127.5f - 0.5f;
			Vector v ( x, y, z, 1.f );
			v.Normalize3();
			return v;
		}
	};

	Vector n ( 0, 0, 0, 0 );

	Int32 range = 1;
	for ( Int32 oi=-range; oi<=range; ++oi )
	{
		for ( Int32 oj=-range; oj<=range; ++oj )
		{
			const Float weight = Local::CalcGaussianWeight( oi, oj );
			n += Local::SampleToksvigNormal( buf, width, height, (Int32)x + oi, (Int32)y + oj ) * weight;
		}
	}

	n /= n.W;

	Float len = Max( 0.f, n.Mag3() );
	Float ft = len / ( len + glossiness * (1.f - len) );

	return ft;
}

//** ************************************
// Channel swizzling for high quality normal mode with DXT compression. Works for four channel data only
void DXTUtils::SwizzleNormalsHigh( Uint8 * out, const Uint8 * in, Uint32 width, Uint32 height )
{
	const Uint32 numPixels = width * height;

	for ( Uint32 i = 0; i < 4 * numPixels; i += 4 )
	{
		out[ i + 2 ] = in[ i + 3 ];	// B = A (currently used on detail normalmaps)
		out[ i + 3 ] = in[ i + 0 ]; // A = R
		out[ i + 0 ] = 255;			// R = 1
		out[ i + 1 ] = in[ i + 1 ]; // G = G
	}
}

//** ************************************
// Channel swizzling for high quality normal mode with DXT compression. Works for four channel data only
void DXTUtils::DeswizzleNormalsHigh( Uint8 * out, const Uint8 * in, Uint32 width, Uint32 height )
{
	const Uint32 numPixels = width * height;

	for ( Uint32 i = 0; i < 4 * numPixels; i += 4 )
	{
		// Decompress pack
		Float x = ( Float )( in[ i + 3 ] / 255.0f ) * 2.0f - 1.0f;
		Float y = ( Float )( in[ i + 1 ] / 255.0f ) * 2.0f - 1.0f;
		Float z = sqrtf( 1.0f - x * x - y * y );

		// Deswizzle
		out[ i + 0 ] = in[ i + 3 ]; 
		out[ i + 1 ] = in[ i + 1 ];
		out[ i + 3 ] = in[ i + 2 ];
		out[ i + 2 ] = ( Uint8 ) Clamp< Float >( 127.0f + 127.0f * z, 0, 255 );
	}
}

//** ************************************
//
Bool DXTUtils::IsSwizzled	( ETextureRawFormat format, ETextureCompression compression )
{
	return compression == TCM_NormalsHigh;
}

//** ************************************
// 
Uint32 DXTUtils::GetNumRows( Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression )
{
	Uint32 rows = height;

	if( !( TCM_None == compression || TCM_TileMap == compression || TCM_RGBE == compression ) )
	{
		rows = Max< Uint32 >( 1, height / 4 );
	}

	return rows;
}

//** ************************************
//
Uint32 DXTUtils::GetPitch( Uint32 width, Uint32 height, ETextureRawFormat format, ETextureCompression compression )
{
	Uint32 pitch = width * ( CBitmapTexture::GetPixelSize( format ) / 8 );

	if( !( TCM_None == compression || TCM_TileMap == compression || TCM_RGBE == compression ) )
	{
		if( TCM_DXTNoAlpha == compression || TCM_Normals == compression )
		{
			// BC1
			pitch = Max< Uint32 >( 8, width * 2 );
		}
		else if( TCM_DXTAlpha == compression || TCM_NormalsHigh == compression )
		{
			// BC3 / BC7
			pitch = Max< Uint32 >( 16, width * 4 );
		}
		else if ( TCM_NormalsGloss == compression )
		{
			// BC3
			pitch = Max< Uint32 >( 16, width * 4 );
		}
		else if ( TCM_QualityR == compression )
		{
			// BC4
			pitch = Max< Uint32 >( 8, width * 2 );
		}
		else if ( TCM_QualityRG == compression )
		{
			// BC5
			pitch = Max< Uint32 >( 16, width * 4 );
		}
		else if( TCM_DXTAlphaLinear == compression )
		{
			// DXT5 linear
			pitch = Max< Uint32 >( 16, width * 4 );
		}
		else if( TCM_QualityColor == compression )
		{
			// BC7
			pitch = Max< Uint32 >( 16, width * 4 );
		}
	}

	// FIXME: OLD API conformance
	return Max< Uint32 >( 4, pitch );
}
