#include "build.h"
#include "heightmapUtils.h"

#ifndef NO_HEIGHTMAP_EDIT


// GpuApi will pull in the DirectXTex libs.
#include "../../../external/DirectXTex/DirectXTex/DirectXTex.h"


static String GetLowerExtension( const String& absolutePath )
{
	size_t lastDotIndex;
	if ( !absolutePath.FindCharacter( TXT('.'), lastDotIndex, true ) )
	{
		HALT( "Error parsing resource filename." )
			return nullptr;
	}
	String lowerExtension = absolutePath.MidString( lastDotIndex + 1 );
	lowerExtension.MakeLower();

	return lowerExtension;
}


static Bool LoadScratchImage( const String& absolutePath, DirectX::ScratchImage& scratchImage )
{
	String lowerExtension = GetLowerExtension( absolutePath );
	
	HRESULT loadRes = S_FALSE;

	if ( lowerExtension == TXT("tga") )
	{
		loadRes = DirectX::LoadFromTGAFile( absolutePath.AsChar(), nullptr, scratchImage );
	}
	else if ( lowerExtension == TXT("dds") )
	{
		loadRes = DirectX::LoadFromDDSFile( absolutePath.AsChar(), DirectX::DDS_FLAGS_FORCE_RGB, nullptr, scratchImage );
	}
	else
	{
		loadRes = DirectX::LoadFromWICFile( absolutePath.AsChar(), DirectX::WIC_FLAGS_FORCE_RGB, nullptr, scratchImage );
	}

	if ( FAILED( loadRes ) )
	{
		HALT( "Heightmap Utils: loading texture '%ls' failed.", absolutePath.AsChar() );
		return false;
	}
	return true;
}


static Bool SaveImage( const String& absolutePath, const DirectX::Image& image )
{
	String lowerExtension = GetLowerExtension( absolutePath );

	HRESULT saveRes = S_FALSE;

	if ( lowerExtension == TXT("tga") )
	{
		saveRes = DirectX::SaveToTGAFile( image, absolutePath.AsChar() );
	}
	else if ( lowerExtension == TXT("dds") )
	{
		saveRes = DirectX::SaveToDDSFile( image, DirectX::DDS_FLAGS_NONE, absolutePath.AsChar() );
	}
	else if ( lowerExtension == TXT("png") )
	{
		saveRes = DirectX::SaveToWICFile( image, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec( DirectX::WIC_CODEC_PNG ), absolutePath.AsChar() );
	}
	else if ( lowerExtension == TXT("jpg") || lowerExtension == TXT("jpeg") )
	{
		saveRes = DirectX::SaveToWICFile( image, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec( DirectX::WIC_CODEC_JPEG ), absolutePath.AsChar() );
	}
	else if ( lowerExtension == TXT("tif") || lowerExtension == TXT("tiff") )
	{
		saveRes = DirectX::SaveToWICFile( image, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec( DirectX::WIC_CODEC_TIFF ), absolutePath.AsChar() );
	}
	/*
	Other WIC formats, probably don't need to worry about them...
	WIC_CODEC_GIF,              // Graphics Interchange Format  (.gif)
	WIC_CODEC_WMP,              // Windows Media Photo / HD Photo / JPEG XR (.hdp, .jxr, .wdp)
	WIC_CODEC_ICO,              // Windows Icon (.ico)
	*/

	if ( FAILED( saveRes ) )
	{
		WARN_ENGINE( TXT("Saving image failed.") );
		return false;
	}
	return true;
}


SHeightmapImageEntry<Uint16>::SHeightmapImageEntry()
{
	m_data = NULL;
}

SHeightmapImageEntry<Uint32>::SHeightmapImageEntry()
{
	m_data = NULL;
}

SHeightmapImageEntry<Uint16>::~SHeightmapImageEntry() 
{ 
	RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_data );
	m_data = NULL;
}

SHeightmapImageEntry<Uint32>::~SHeightmapImageEntry() 
{ 
	RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_data );
	m_data = NULL;
}



Bool CHeightmapUtils::LoadHeightmap( const String& absolutePath, /*out*/SHeightmapImageEntry<Uint16>& entry, /*out*/Uint32* width /*= NULL*/, /*out*/Uint32* height /*= NULL*/ )
{
	ASSERT( !absolutePath.Empty() );

	DirectX::ScratchImage scratchImage;
	if ( !LoadScratchImage( absolutePath, scratchImage ) )
	{
		return false;
	}

	const DirectX::Image* image = scratchImage.GetImage( 0, 0, 0 );

	Uint32 imageWidth		= ( Uint32 )image->width;
	Uint32 imageHeight		= ( Uint32 )image->height;
	Uint32 imageBpp			= ( Uint32 )image->rowPitch / imageWidth;
	DXGI_FORMAT imageFormat	= image->format;

	if ( imageBpp != 2 )
	{
		ERR_ENGINE( TXT("Heightmap Utils: texture's '%ls' bit depth should be 2 but is [%d], import is impossible"), absolutePath.AsChar(), imageBpp );
		return false;
	}

	if ( imageFormat != DXGI_FORMAT_R16_UNORM && imageFormat != DXGI_FORMAT_R16_UINT )
	{
		ERR_ENGINE( TXT("Heightmap Utils: texture '%ls', 16-bit format should be 'DXGI_FORMAT_R16_UNORM' [%d], but is [%d], import is impossible" ), absolutePath.AsChar(), DXGI_FORMAT_R16_UNORM, imageFormat );
		return false;
	}

	// Make a copy of the data, since scratchImage will go out of scope and destroy everything.
	entry.m_data = (Uint16*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, image->rowPitch * imageHeight );
	Red::System::MemoryCopy( entry.m_data, (Uint16*)image->pixels, image->rowPitch * imageHeight );

	if ( entry.m_params.normalizeImage )
	{
		Uint32 area = imageWidth * imageHeight;
		Uint16 minVal = NumericLimits< Uint16 >::Max();
		for ( Uint32 i = 0; i < area; ++i )
		{
			minVal = Min< Uint16 >( minVal, entry.m_data[ i ] );
		}
		for ( Uint32 i = 0; i < area; ++i )
		{
			entry.m_data[ i ] -= minVal;
		}
	}

	if ( width )
	{
		*width = imageWidth;
	}
	if ( height )
	{
		*height = imageHeight;
	}

	return true;
}


Bool CHeightmapUtils::LoadColor( const String& absolutePath, /*out*/SHeightmapImageEntry<Uint32>& entry, /*out*/Uint32* width /*= NULL*/, /*out*/Uint32* height /*= NULL*/ )
{
	ASSERT( !absolutePath.Empty() );

	DirectX::ScratchImage scratchImage;
	if ( !LoadScratchImage( absolutePath, scratchImage ) )
	{
		return false;
	}

	const DirectX::Image* image = scratchImage.GetImage( 0, 0, 0 );

	Uint32 imageWidth		= ( Uint32 )image->width;
	Uint32 imageHeight		= ( Uint32 )image->height;
	Uint32 imageBpp			= ( Uint32 )image->rowPitch / imageWidth;
	DXGI_FORMAT imageFormat	= image->format;

	if ( imageBpp != 4 )
	{
		ERR_ENGINE( TXT("Heightmap Utils: texture's '%ls' bit depth should be 4 but is [%d], import is impossible"), absolutePath.AsChar(), imageBpp );
		return false;
	}

	if ( imageFormat != DXGI_FORMAT_R8G8B8A8_UNORM )
	{
		ERR_ENGINE( TXT("Heightmap Utils: texture '%ls', format should be 'DXGI_FORMAT_R8G8B8A8_UNORM' [%d], but is [%d], import is impossible" ), absolutePath.AsChar(), DXGI_FORMAT_R8G8B8A8_UNORM, imageFormat );
		return false;
	}

	// Make a copy of the data, since scratchImage will go out of scope and destroy everything.
	entry.m_data = (Uint32*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, image->rowPitch * imageHeight );
	Red::System::MemoryCopy( entry.m_data, (Uint32*)image->pixels, image->rowPitch * imageHeight );

	if ( width )
	{
		*width = imageWidth;
	}
	if ( height )
	{
		*height = imageHeight;
	}

	return true;
}



Bool CHeightmapUtils::ResizeImage( Uint16* data, Uint32 currentWidth, Uint32 currentHeight, Uint32 desiredWidth, Uint32 desiredHeight, /*out*/SHeightmapImageEntry<Uint16>& entry )
{
	DirectX::Image sourceImage;
	sourceImage.format = DXGI_FORMAT_R16_UNORM;
	sourceImage.width = currentWidth;
	sourceImage.height = currentHeight;
	sourceImage.rowPitch = currentWidth * 2;
	sourceImage.slicePitch = sourceImage.rowPitch * currentHeight;
	sourceImage.pixels = ( Uint8* )data;


	DirectX::ScratchImage destScratchImage;
	if ( FAILED( DirectX::Resize( sourceImage, desiredWidth, desiredHeight, DirectX::TEX_FILTER_LINEAR, destScratchImage ) ) )
	{
		ERR_ENGINE( TXT("Heightmap Utils: resizing image to [%d x %d] failed."), desiredWidth, desiredHeight );
		return false;
	}

	const DirectX::Image* destImage = destScratchImage.GetImage( 0, 0, 0 );

	RED_ASSERT( destImage->rowPitch == desiredWidth * 2 );

	// Make a copy of the data, since scratchImage will go out of scope and destroy everything.
	entry.m_data = (Uint16*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, desiredWidth * desiredHeight * 2 );
	Red::System::MemoryCopy( entry.m_data, destImage->pixels, desiredWidth * desiredHeight * 2 );

	return true;
}

Bool CHeightmapUtils::ResizeImage( Uint32* data, Uint32 currentWidth, Uint32 currentHeight, Uint32 desiredWidth, Uint32 desiredHeight, /*out*/SHeightmapImageEntry<Uint32>& entry )
{
	DirectX::Image sourceImage;
	sourceImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sourceImage.width = currentWidth;
	sourceImage.height = currentHeight;
	sourceImage.rowPitch = currentWidth * 4;
	sourceImage.slicePitch = sourceImage.rowPitch * currentHeight;
	sourceImage.pixels = ( Uint8* )data;

	DirectX::ScratchImage destScratchImage;
	if ( FAILED( DirectX::Resize( sourceImage, desiredWidth, desiredHeight, DirectX::TEX_FILTER_LINEAR, destScratchImage ) ) )
	{
		ERR_ENGINE( TXT("Heightmap Utils: resizing image to [%d x %d] failed."), desiredWidth, desiredHeight );
		return false;
	}

	const DirectX::Image* destImage = destScratchImage.GetImage( 0, 0, 0 );

	RED_ASSERT( destImage->rowPitch == desiredWidth * 4 );

	// Make a copy of the data, since scratchImage will go out of scope and destroy everything.
	entry.m_data = (Uint32*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, desiredWidth * desiredHeight * 4 );
	Red::System::MemoryCopy( entry.m_data, destImage->pixels, desiredWidth * desiredHeight * 4 );

	return true;
}


Bool CHeightmapUtils::SaveImage( const String& absolutePath, Uint16* data, Uint32 width, Uint32 height )
{
	ASSERT( data );
	ASSERT( !absolutePath.Empty() );

	DirectX::Image img;
	img.format = DXGI_FORMAT_R16_UNORM;
	img.width = width;
	img.height = height;
	img.rowPitch = width * 2;
	img.slicePitch = img.rowPitch * img.height;
	img.pixels = ( Uint8* )data;

	if ( !::SaveImage( absolutePath, img ) )
	{
		return false;
	}

	return true;
}

Bool CHeightmapUtils::SaveImage( const String& absolutePath, Uint32* data, Uint32 width, Uint32 height )
{
	ASSERT( data );
	ASSERT( !absolutePath.Empty() );

	DirectX::Image img;
	img.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	img.width = width;
	img.height = height;
	img.rowPitch = width * 4;
	img.slicePitch = img.rowPitch * img.height;
	img.pixels = ( Uint8* )data;

	if ( !::SaveImage( absolutePath, img ) )
	{
		return false;
	}

	return true;
}

Bool CHeightmapUtils::GetImageSize( const String& absolutePath, Uint32& width, Uint32& height )
{
	DirectX::TexMetadata metadata;
	String lowerExtension = GetLowerExtension( absolutePath );

	HRESULT loadRes = S_FALSE;

	if ( lowerExtension == TXT("tga") )
	{
		loadRes = DirectX::GetMetadataFromTGAFile( absolutePath.AsChar(), metadata );
	}
	else if ( lowerExtension == TXT("dds") )
	{
		loadRes = DirectX::GetMetadataFromDDSFile( absolutePath.AsChar(), DirectX::DDS_FLAGS_FORCE_RGB, metadata );
	}
	else
	{
		loadRes = DirectX::GetMetadataFromWICFile( absolutePath.AsChar(), DirectX::WIC_FLAGS_FORCE_RGB, metadata );
	}

	if ( FAILED( loadRes ) )
	{
		return false;
	}

	width	= ( Uint32 )metadata.width;
	height	= ( Uint32 )metadata.height;

	return true;
}

#endif //!NO_HEIGHTMAP_EDIT
