/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/bitmapTexture.h"

// Simple thumbnail generator for CBitmapTextures
class CBitmapThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CBitmapThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;
};

DEFINE_SIMPLE_RTTI_CLASS(CBitmapThumbnailGenerator,IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CBitmapThumbnailGenerator);

TDynArray<CThumbnail*> CBitmapThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	// Not a bitmap texture
	CBitmapTexture* texture = Cast< CBitmapTexture >( resource );
	if ( !texture )
	{
		return TDynArray<CThumbnail*>();
	}

	// Get bitmap size
	Uint32 width = texture->GetWidth();
	Uint32 height = texture->GetHeight();
	if ( !width || !height )
	{
		LOG_IMPORTER( TXT("Texture '%s' has invalid size. Unable to grab thumbnail."), texture->GetFriendlyName().AsChar() );
		return TDynArray<CThumbnail*>();
	}

	// Get the base mip
	if ( !texture->GetMips().Size() )
	{
		LOG_IMPORTER( TXT("Texture '%s' has no mipmaps. Unable to grab thumbnail."), texture->GetFriendlyName().AsChar() );
		return TDynArray<CThumbnail*>();
	}

	// Get number of mipmaps in the bitmap
	const Uint32 numMips = texture->GetMipCount();
	ASSERT( numMips );

	// Determine mipmap to grab
	Uint32 mipToGrab = 0;
	if ( numMips > 1 )
	{
		// Go down
		while ( width>256 || height>256 )
		{
			width = Max< Uint32 >( width>>1, 1 );
			height = Max< Uint32 >( height>>1, 1 );
			mipToGrab++;
		}

		// If out of mips, grab last one
		if ( mipToGrab >= numMips )
		{
			mipToGrab = numMips - 1;
		}
	}

	TDynArray<CThumbnail*> res;

	const CBitmapTexture::MipMap& sourceMip = texture->GetMips()[ mipToGrab ];
	if ( !const_cast< CBitmapTexture::MipMap& >( sourceMip ).m_data.Load() )
	{
		LOG_IMPORTER( TXT("Texture '%s' mip %i has no data. Unable to grab thumbnail."), texture->GetFriendlyName().AsChar(), mipToGrab );
		return res;
	}

	void* srcData = sourceMip.m_data.GetData();
	size_t srcDataSize = sourceMip.m_data.GetSize();

	void* data = nullptr;
	size_t dataSize = 0;
	
	GpuApi::eTextureFormat format = texture->GetPlatformSpecificCompression();

	// grab texture data
	if ( GpuApi::SaveTextureToMemory( static_cast< Uint8* >( srcData ), srcDataSize, sourceMip.m_width, sourceMip.m_height, format, sourceMip.m_pitch, GpuApi::SAVE_FORMAT_PNG, &data, dataSize ) )
	{
		RED_WARNING( data != nullptr && dataSize != 0, "Failed to save thumbnail for texture '%s'", texture->GetFriendlyName().AsChar() );
		if ( data != nullptr || dataSize > 0 )
		{
			CThumbnail* thumb = new CThumbnail( data, static_cast< Uint32 >( dataSize ) );
			res.PushBack( thumb );
			GpuApi::FreeTextureData( data );
		}
	}

	// Unload source data
	const_cast< CBitmapTexture::MipMap& >( sourceMip ).m_data.Unload();
	return res;
}
