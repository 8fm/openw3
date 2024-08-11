/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "textureCookerNonResource.h"
#include "../../common/engine/textureCache.h"
#include "../../common/core/depot.h"

// GpuApi will pull in the DirectXTex libs.
#include "../../../external/DirectXTex/DirectXTex/DirectXTex.h"




// HACK : Ewww.... need texture format mapping, but don't want to clutter ::GpuApi with dx-specific stuff. Of course, this will only
// work for static/inline functions.
namespace GpuApiDxHack
{
	// Resolve some otherwise ambiguous symbols.
	typedef Red::System::Bool Bool;
	typedef Red::System::Uint32 Uint32;
	typedef Red::System::Float Float;
	// Bring in GpuApi for enum types and such.
	using namespace GpuApi;
	// And bring in mapping functions.
#include "../../common/gpuApiDX10/gpuApiMapping.h"
}



class CDirectXTextureSource : public ITextureBakerSource
{
private:
	DirectX::ScratchImage*	m_texture;
	Uint16					m_startMip;

public:
	CDirectXTextureSource( DirectX::ScratchImage* texture, Uint16 startMip = 0 )
		: m_texture( texture )
		, m_startMip( startMip )
	{}

	virtual Uint16 GetSliceCount() const
	{
		return (Uint16)m_texture->GetMetadata().arraySize;
	}

	virtual Uint16 GetMipCount() const
	{
		return (Uint16)m_texture->GetMetadata().mipLevels - m_startMip;
	}

	virtual const void* GetMipData( Uint16 mip, Uint16 slice ) const
	{
		return m_texture->GetImage( mip, slice, 0 )->pixels;
	}

	virtual Uint32 GetMipDataSize( Uint16 mip, Uint16 slice ) const
	{
		return (Uint32)m_texture->GetImage( mip, slice, 0 )->slicePitch;
	}

	virtual Uint32 GetMipPitch( Uint16 mip, Uint16 slice ) const
	{
		return (Uint32)m_texture->GetImage( mip, slice, 0 )->rowPitch;
	}

	virtual Uint16 GetBaseWidth() const
	{
		return (Uint16)m_texture->GetMetadata().width;
	}

	virtual Uint16 GetBaseHeight() const
	{
		return (Uint16)m_texture->GetMetadata().height;
	}

	virtual GpuApi::eTextureFormat GetTextureFormat() const
	{
		return GpuApiDxHack::GpuApi::Map( m_texture->GetMetadata().format );
	}

	virtual GpuApi::eTextureType GetTextureType() const
	{
		if ( ( m_texture->GetMetadata().miscFlags & DirectX::TEX_MISC_TEXTURECUBE ) != 0 )
		{
			return GpuApi::TEXTYPE_CUBE;
		}
		else if ( m_texture->GetMetadata().arraySize > 1 )
		{
			return GpuApi::TEXTYPE_ARRAY;
		}
		else
		{
			return GpuApi::TEXTYPE_2D;
		}
	}

	virtual Bool IsLooseFileTexture() const { return true; }
};


// If we ran into some sort of error during cook, we'll generate some fake data, so we can still have something.
// So, the generated texture is 1x1, RGBA8, filled with pink.
class CNonResourceTextureErrorSource : public ITextureBakerSource, public Red::System::NonCopyable
{
private:
	Uint32 m_dummyData;

public:
	CNonResourceTextureErrorSource()
		: m_dummyData( 0xffff00ff )
	{
	}

	virtual Uint16 GetSliceCount() const { return 1; }
	virtual Uint16 GetMipCount() const { return 1; }
	virtual const void* GetMipData( Uint16 /*mip*/, Uint16 /*slice*/ ) const { return &m_dummyData; }
	virtual Uint32 GetMipDataSize( Uint16 /*mip*/, Uint16 /*slice*/ ) const { return sizeof( m_dummyData ); }
	virtual Uint32 GetMipPitch( Uint16 /*mip*/, Uint16 /*slice*/ ) const { return sizeof( m_dummyData ); }
	virtual Uint16 GetBaseWidth() const { return 1; }
	virtual Uint16 GetBaseHeight() const { return 1; }
	virtual GpuApi::eTextureFormat GetTextureFormat() const { return GpuApi::TEXFMT_R8G8B8A8; }
	virtual GpuApi::eTextureType GetTextureType() const { return GpuApi::TEXTYPE_ARRAY; }
	virtual Bool IsLooseFileTexture() const { return true; }
};


//////////////////////////////////////////////////////////////////////////


static Bool HandleError( const String& depotPath, CAsyncTextureBaker::CookFunctionPtr cookFunction )
{
	// Save the entire thing into texture cache. It's stored as a non-resource texture so we can look it up by name, since we can't really
	// store a cache index anywhere.

	CNonResourceTextureErrorSource source;
	if ( !GTextureCacheCooker->StoreNonResourceTextureData( depotPath.AsChar(), source, cookFunction ) )
	{
		ERR_WCC( TXT("Failed to store '%ls' in texture cache"), depotPath.AsChar() );
		return false;
	}

	return true;
}


Bool CookNonResourceTexture( const String& depotPath, ECookingPlatform platform )
{
	if ( GTextureCacheCooker == nullptr )
	{
		ERR_WCC( TXT("Cannot cook non-resource texture '%ls' without GTextureCacheCooker"), depotPath.AsChar() );
		return false;
	}

	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( platform );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return false;
	}


	CFilePath filePath( depotPath );

	String absolutePath = GDepot->CDirectory::GetAbsolutePath() + depotPath;

	String lowerExtension = filePath.GetExtension().ToLower();


	DirectX::ScratchImage scratchImage;
	Bool shouldTryCompress = false;

	HRESULT loadRes = S_FALSE;
	if ( lowerExtension == TXT("tga") )
	{
		loadRes = DirectX::LoadFromTGAFile( absolutePath.AsChar(), nullptr, scratchImage );
		shouldTryCompress = true;
	}
	else if ( lowerExtension == TXT("dds") )
	{
		loadRes = DirectX::LoadFromDDSFile( absolutePath.AsChar(), DirectX::DDS_FLAGS_FORCE_RGB, nullptr, scratchImage );
		// We won't try to compress DDS, since the creator already has the option for what format to use.
	}
	else
	{
		loadRes = DirectX::LoadFromWICFile( absolutePath.AsChar(), DirectX::WIC_FLAGS_FORCE_RGB, nullptr, scratchImage );
		shouldTryCompress = true;
	}

	if ( FAILED( loadRes ) )
	{
		ERR_WCC( TXT("Failed to load non-resource texture from '%ls'"), absolutePath.AsChar() );
		return HandleError( depotPath, CookFunction );
	}


	const DirectX::TexMetadata& metadata = scratchImage.GetMetadata();

	if ( scratchImage.GetImageCount() == 0 )
	{
		ERR_WCC( TXT("Image file '%ls' has no images!"), depotPath.AsChar() );
		return HandleError( depotPath, CookFunction );
	}
	if ( metadata.arraySize == 0 )
	{
		ERR_WCC( TXT("Image file '%ls' has 0 array size"), depotPath.AsChar() );
		return HandleError( depotPath, CookFunction );
	}
	if ( metadata.mipLevels == 0 )
	{
		ERR_WCC( TXT("Image file '%ls' has 0 mip levels"), depotPath.AsChar() );
		return HandleError( depotPath, CookFunction );
	}
	if ( metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D )
	{
		ERR_WCC( TXT("Image file '%ls' is 1D or 3D texture, which are not supported"), depotPath.AsChar() );
		return HandleError( depotPath, CookFunction );
	}
	if ( GpuApiDxHack::GpuApi::Map( metadata.format ) == GpuApi::TEXFMT_Max )
	{
		ERR_WCC( TXT("Image file '%ls' uses unsupported D3D format %u"), depotPath.AsChar(), metadata.format );
		return HandleError( depotPath, CookFunction );
	}
	if ( metadata.width > 4096 || metadata.height > 4096 )
	{
		ERR_WCC( TXT("Image file '%ls' is too big (%ux%u)"), depotPath.AsChar(), metadata.width, metadata.height );
		return HandleError( depotPath, CookFunction );
	}


	DirectX::ScratchImage* finalImage = &scratchImage;
	DirectX::ScratchImage compressedImage;
	if ( shouldTryCompress )
	{
		DXGI_FORMAT targetFormat = DXGI_FORMAT_BC1_UNORM;
		// TODO : What about PNGs that are saved with alpha channel, but still fully opaque? worry about that, or leave for artists to fix?
		if ( DirectX::HasAlpha( metadata.format ) )
		{
			targetFormat = DXGI_FORMAT_BC3_UNORM;
		}
		Uint32 flags = DirectX::TEX_COMPRESS_DITHER | DirectX::TEX_COMPRESS_PARALLEL;
		Float alpha = 0.5f;
		if ( SUCCEEDED( DirectX::Compress( scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, targetFormat, flags, alpha, compressedImage ) ) )
		{
			finalImage = &compressedImage;
		}
		else
		{
			WARN_WCC( TXT("Failed to compress %ls %ux%u"), depotPath.AsChar(), metadata.width, metadata.height );
		}
	}


	// Save the entire thing into texture cache. It's stored as a non-resource texture so we can look it up by name, since we can't really
	// store a cache index anywhere.

	CDirectXTextureSource source( finalImage );
	if ( !GTextureCacheCooker->StoreNonResourceTextureData( depotPath.AsChar(), source, CookFunction ) )
	{
		ERR_WCC( TXT("Failed to store '%ls' in texture cache"), depotPath.AsChar() );
		return false;
	}

	return true;
}
