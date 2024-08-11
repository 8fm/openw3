/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "textureArray.h"
#include "textureCache.h"
#include "../core/dataError.h"
#include "../core/dependencyMapper.h"
#include "../core/depot.h"
#include "../core/directory.h"
#include "renderer.h"
#include "drawableComponent.h"
#include "renderResource.h"
#include "material.h"

IMPLEMENT_ENGINE_CLASS( CTextureArrayEntry );
IMPLEMENT_ENGINE_CLASS( CTextureArray );

CTextureArray::~CTextureArray()
{
	SAFE_RELEASE( m_renderResource );
	SAFE_RELEASE( m_streamingSource );
}

void CTextureArray::GetAdditionalInfo( TDynArray< String >& info ) const
{
	TBaseClass::GetAdditionalInfo( info );
}

void CTextureArray::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	CreateRenderResource();
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

CTextureArray* CTextureArray::Create( const FactoryInfo& data )
{
	// Create new bitmap object
	CTextureArray* obj = data.CreateResource();

#ifndef NO_TEXTURECACHE_COOKER
	// Setup initial data
	obj->m_compression = data.m_compression;
#endif

	// Done
	return obj;
}

#ifndef NO_TEXTURECACHE_COOKER
Bool CTextureArray::IsTextureValidForArray( const CBitmapTexture* texture, const CTextureArray* array, String& errorMessage /*out*/ )
{
	ASSERT( texture );
	ASSERT( array );

	static const String sizeError( TXT("Texture '%ls' [%dx%d] has different size than one of the others in the array: '%ls' [%dx%d] - it cannot be added.") );
	static const String mipmapError( TXT("Texture '%ls' [mipchain size: %d] has different mipmap chain than one of the others in the array: '%ls' [mipchain size: %d] - it cannot be added.") );
	static const String compressionError( TXT("Texture '%ls' [compression: '%ls'] has different compression than one of the others in the array: '%ls' [compression: '%ls'] - it cannot be added.") );
	static const String downscaleBiasError( TXT("Texture '%ls' has different bias than one of the others in the array: '%ls' - it cannot be added.") );

	String texName = texture->GetDepotPath();

	CBitmapTexture* firstNonMatchingTexture = NULL;
	if ( !array->CheckIfSizeFits( texture, &firstNonMatchingTexture ) )
	{
		errorMessage = String::Printf( sizeError.AsChar(), texName.AsChar(), texture->GetWidth(), texture->GetHeight(), firstNonMatchingTexture->GetDepotPath().AsChar(), firstNonMatchingTexture->GetWidth(), firstNonMatchingTexture->GetHeight() );
		return false;
	}
	if ( !array->CheckIfMipMapLevelsFit( texture, &firstNonMatchingTexture ) )
	{
		errorMessage = String::Printf( mipmapError.AsChar(), texName.AsChar(), texture->GetMipCount(), firstNonMatchingTexture->GetDepotPath().AsChar(), firstNonMatchingTexture->GetMipCount() );
		return false;
	}
	if ( !array->CheckIfCompressionFits( texture, &firstNonMatchingTexture ) )
	{
		CEnum* compressionEnum = SRTTI::GetInstance().FindEnum( CName( TXT("ETextureCompression" ) ) );
		ASSERT( compressionEnum );

		CName textureComporessionName;
		RED_VERIFY( compressionEnum->FindName( (Int32)texture->GetCompression(), textureComporessionName ) );
		CName nonMatchingComporessionName;
		RED_VERIFY( compressionEnum->FindName( (Int32)firstNonMatchingTexture->GetCompression(), nonMatchingComporessionName ) );
		
		errorMessage = String::Printf( compressionError.AsChar(), texName.AsChar(), textureComporessionName.AsString().AsChar(), firstNonMatchingTexture->GetDepotPath().AsChar(), nonMatchingComporessionName.AsString().AsChar() );
		return false;
	}
	if ( !array->CheckIfDownscaleBiasFits( texture, &firstNonMatchingTexture ) )
	{
		errorMessage = String::Printf( downscaleBiasError.AsChar(), texName.AsChar(), firstNonMatchingTexture->GetDepotPath().AsChar() );
		return false;
	}

	return true;
}
#endif 

void CTextureArray::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}

	if ( file.GetVersion() < VER_TEXTURE_COOKING )
	{
		// Skip old cooked/system data, as it was unused and the cooked data has changed. LatentDataBuffer, so it doesn't actually have
		// to alloc and read it in right now, it'll just skip the block.
		DataBuffer dummyData;
		dummyData.Serialize( file );
		dummyData.Serialize( file );
	}
	else
	{
		file.Serialize( &m_cookedData.m_header, sizeof( CookedDataHeader ) );
		m_cookedData.m_deviceData.Serialize( file );
	}


	if ( file.IsReader() )
	{
		if ( IsCooked() )
		{
			CreateCookedStreamingSource();
		}
		else
		{
			// Get data from child bitmaps.
			CreateUncookedStreamingSource();
		}

#ifndef RED_FINAL_BUILD
		if ( m_streamingSource != nullptr )
		{
			m_streamingSource->SetDebugName( GetDepotPath() );
		}
#endif

		// If we have cooked data, then we can just get the texture format from there.
		if ( IsCooked() )
		{
			m_platformSpecificCompression = m_cookedData.m_header.GetTextureFormat();
		}
#ifndef NO_TEXTURECACHE_COOKER
		// Otherwise, decide on a format based on our textures.
		else
		{
			ETextureCompression compression = TCM_None;
			// determine compression from bitmaps
			for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
			{
				if ( m_bitmaps[i].m_texture.Get() )
				{
					compression = m_bitmaps[i].m_texture.Get()->GetCompression();
					break;
				}
			}
			for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
			{
				if ( m_bitmaps[i].m_texture.Get() )
				{
					ETextureCompression otherCompression = m_bitmaps[i].m_texture.Get()->GetCompression();
#ifndef NO_EDITOR
					if ( compression != otherCompression )
					{
						CEnum* compressionEnum = SRTTI::GetInstance().FindEnum( CName( TXT("ETextureCompression") ) );
						ASSERT( compressionEnum );

						CName compressionName;
						RED_VERIFY( compressionEnum->FindName( compression, compressionName ) );
						CName otherCompressionName;
						RED_VERIFY( compressionEnum->FindName( otherCompression, otherCompressionName ) );

						if ( !file.IsCooker() )
						{
							DATA_HALT( DES_Uber, this, TXT("TextureArray"), TXT( "Compression mismatch ('%ls' != '%ls') in texture array entries" ), compressionName.AsString().AsChar(), otherCompressionName.AsString().AsChar() );
						}
					}
#else // !NO_EDITOR
					if ( compression != otherCompression )
					{
						DATA_HALT( DES_Uber, this, TXT("TextureArray"), TXT( "Compression mismatch in texture array entries" ) );
					}
#endif // NO_EDITOR
				}
			}
			m_compression = compression;

#ifdef USE_NEW_COMPRESSION
			Bool isOldVersion = file.GetVersion() < VER_ADDED_NEW_TEXTURE_COMPRESSION_FORMATS;
#else
			Bool isOldVersion = true;
#endif
			if ( m_compression != TCM_None )
			{
				// compressed format, it's enough to determine platform specific compression
				CBitmapTexture::GetCompressedFormat( TRF_TrueColor, m_compression, m_platformSpecificCompression, isOldVersion );
			}
			else
			{
				for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
				{
					if ( m_bitmaps[i].m_texture.Get() )
					{
						// all of the bitmap entries have the same parameters
						CBitmapTexture::GetCompressedFormat( m_bitmaps[i].m_texture.Get()->GetFormat(), m_compression, m_platformSpecificCompression, isOldVersion );
						return;
					}
				}

				ASSERT( false, TXT("Undetermined TextureArray platform specific compression") );
				m_platformSpecificCompression = GpuApi::TEXFMT_R8G8B8A8;
			}
		}


		// Get texture group from our bitmaps.
		if ( file.GetVersion() < VER_TEXTURE_COOKING )
		{
			UpdateTextureGroup();
		}
#endif 
	}
}


void CTextureArray::CreateCookedStreamingSource()
{
	SAFE_RELEASE( m_streamingSource );

	// If we are fully resident, no streaming.
	if ( m_cookedData.m_header.m_residentMip == 0 )
	{
		return;
	}

	m_streamingSource = new CTextureCacheStreamingSourcePC( m_cookedData.m_header.m_textureCacheKey );
}


void CTextureArray::CreateUncookedStreamingSource()
{
	SAFE_RELEASE( m_streamingSource );

	const Uint32 numBitmaps = m_bitmaps.Size();

	if ( numBitmaps == 0 )
	{
		return;
	}

	Uint32 nonNull = 0;
	while ( nonNull < numBitmaps && m_bitmaps[nonNull].m_texture.Get() == nullptr )
	{
		++nonNull;
	}
	RED_WARNING( nonNull < numBitmaps, "Texture array has no non-null textures, %s", GetFriendlyName().AsChar() );
	if ( nonNull >= numBitmaps )
	{
		return;
	}


	CBitmapTexture* firstNonNullTex = m_bitmaps[nonNull].m_texture.Get();
	const Uint32 width = firstNonNullTex->GetWidth();
	const Uint32 height = firstNonNullTex->GetHeight();
	const Uint32 mipCount = firstNonNullTex->GetMipCount();
	const GpuApi::eTextureFormat firstFormat = firstNonNullTex->GetPlatformSpecificCompression();

	TDynArray< IFileLatentLoadingToken* > loadingTokens( numBitmaps );
	for ( Uint32 i = 0; i < numBitmaps; ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();

		if ( tex == nullptr )
		{
			DATA_HALT( DES_Minor, this, TXT("Rendering"), TXT("Texture array has null texture %d"), i );
			continue;
		}

		if ( tex->GetWidth() != width || tex->GetHeight() != height )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Texture array has textures with different sizes: %ux%u != %ux%u"), tex->GetWidth(), tex->GetHeight(), width, height );
			continue;
		}

		if ( tex->GetMipCount() != mipCount )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Texture array has textures with different numbers of mip levels: %u != %u"), tex->GetMipCount(), mipCount );
			continue;
		}

		if ( tex->GetPlatformSpecificCompression() != firstFormat )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Texture array has textures with different formats: %") RED_PRIWas TXT(" != %") RED_PRIWas,
				GpuApi::GetTextureFormatName( tex->GetPlatformSpecificCompression() ), GpuApi::GetTextureFormatName( firstFormat ) );
			continue;
		}

		IBitmapTextureStreamingSource* streamingSource = tex->GetStreamingSource();
		RED_WARNING( streamingSource != nullptr, "Could not get streaming source for texture %s", tex->GetFriendlyName().AsChar() );
		if ( streamingSource != nullptr )
		{
			IFileLatentLoadingToken* token = streamingSource->CreateLoadingToken();
			RED_WARNING( token != nullptr, "Could not create loading token for texture %s", tex->GetFriendlyName().AsChar() );

			loadingTokens[i] = token;
		}
	}

	m_streamingSource = new CTextureArrayStreamingSourcePC( (Uint16)width, (Uint16)height, (Uint8)mipCount, loadingTokens );

	// Update the mip data size to load
	if ( m_streamingSource )
	{
		for ( Uint32 tex_i = 0; tex_i < numBitmaps; ++tex_i )
		{
			CBitmapTexture* tex = m_bitmaps[ tex_i ].m_texture.Get();

			if ( tex == nullptr )
			{
				DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("One of textures from texture array was not found") );
				continue;
			}

			IBitmapTextureStreamingSource* bitmapStreamingSource = tex->GetStreamingSource();
			RED_WARNING( bitmapStreamingSource != nullptr, "No streaming source for texture '%ls'", tex->GetDepotPath().AsChar() );
			if ( bitmapStreamingSource == nullptr )
			{
				continue;
			}

			const CBitmapTexture::MipArray& mips = tex->GetMips();
			for ( Uint32 mip_i = 0; mip_i < mips.Size(); ++mip_i )
			{
				const CBitmapTexture::MipMap& mip = mips[mip_i];

				const SBitmapTextureStreamingMipInfo* bitmapInfo = bitmapStreamingSource->GetMipLoadingInfo( mip_i );
				RED_WARNING( bitmapInfo != nullptr, "Could not get mip info for texture %s, mip %u", tex->GetFriendlyName().AsChar(), mip_i );

				SBitmapTextureStreamingMipInfo* mipInfo = const_cast< SBitmapTextureStreamingMipInfo* >( m_streamingSource->GetMipLoadingInfo( mip_i, tex_i ) );
				RED_WARNING( mipInfo != nullptr, "Could not get mip info for texture array %s, tex %u, mip %u", GetFriendlyName().AsChar(), tex_i, mip_i );

				if ( bitmapInfo == nullptr || mipInfo == nullptr )
				{
					continue;
				}

				*mipInfo = *bitmapInfo;
			}
		}
	}
}

void CTextureArray::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	CreateRenderResource();
}

void CTextureArray::OnSave()
{
#ifndef NO_TEXTURECACHE_COOKER
	if ( !m_dirty )
	{
		return;
	}

	CreateRenderResource();
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	IMaterial::RecompileMaterialsUsingTextureArray( this );
#endif // NO_RUNTIME_MATERIAL_COMPILATION
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
#endif 
}


#ifndef NO_DATA_VALIDATION
void CTextureArray::OnCheckDataErrors() const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors();

	// If it's cooked, it must have already been checked
	if ( IsCooked() )
	{
		return;
	}

	// Texture is empty
	if ( !m_bitmaps.Size() )
	{
		DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("TextureArray is empty and has no data") );
	}

	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		if ( !m_bitmaps[i].m_texture.Get() )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("TextureArray could not load %-th texture"), i );
		}
	}

	// check consistency
	TDynArray< Uint32 > widths;
	TDynArray< Uint32 > heights;
	TDynArray< ETextureCompression > compressions;
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex )
		{
			widths.PushBackUnique( tex->GetWidth() );
			heights.PushBackUnique( tex->GetHeight() );
			compressions.PushBackUnique( tex->GetCompression() );
		}
	}
	
	if ( widths.Size() > 1 )
	{
		String error( TXT("Textures of different widths found in TextureArray $path$: ") );
		for ( Uint32 i = 0; i < widths.Size(); ++i )
		{
			error += String::Printf( TXT( "%d, " ), widths[i] );
		}
		DATA_HALT( DES_Major, this, TXT("Rendering"), error.AsChar() );
	}
	if ( heights.Size() > 1 )
	{
		String error( TXT("Textures of different heights found in TextureArray $path$: ") );
		for ( Uint32 i = 0; i < widths.Size(); ++i )
		{
			error += String::Printf( TXT( "%d, " ), widths[i] );
		}
		DATA_HALT( DES_Major, this, TXT("Rendering"), error.AsChar() );
	}
	if ( compressions.Size() > 1 )
	{
		String error( TXT("More than one compression type found in TextureArray $path$: ") );

		CEnum* compressionEnum = SRTTI::GetInstance().FindEnum( CName( TXT("ETextureCompression") ) );
		ASSERT( compressionEnum );
		for ( Uint32 i = 0; i < compressions.Size(); ++i )
		{
			CName compressionName;
			RED_VERIFY( compressionEnum->FindName( compressions[ i ], compressionName ) );
			error += String::Printf( TXT( "%s, " ), compressionName.AsString().AsChar() );
		}
		DATA_HALT( DES_Major, this, TXT("Rendering"), error.AsChar() );
	}

	// Failed to create rendering resource
	if ( !m_renderResource && !GIsCooker )
	{
		DATA_HALT( DES_Uber, this, TXT("Rendering"), TXT("TextureArray was not uploaded to GPU. Ask programmers.") );
	}
}
#endif // NO_DATA_VALIDATION

void CTextureArray::CreateRenderResource()
{
	// Release previous resource
	SAFE_RELEASE( m_renderResource );

	if ( GRender && !GRender->IsDeviceLost() )
	{
		m_renderResource = GRender->UploadTextureArray( this );
	}
}

void CTextureArray::ReleaseRenderResource()
{
	// Release previous texture array
	SAFE_RELEASE( m_renderResource );
}

CTextureArray::CTextureArray()
	: m_textureGroup( RED_NAME( Default ) )
	, m_pcDownscaleBias( 1 )
	, m_xboneDownscaleBias( 2 )
	, m_ps4DownscaleBias( 2 )
#ifndef NO_TEXTURECACHE_COOKER
	, m_dirty( false )
#endif
{
}

IRenderResource* CTextureArray::GetRenderResource() const
{
	if ( !m_renderResource )
	{
		const_cast<CTextureArray*>( this )->CreateRenderResource();
	}

	return m_renderResource;
}


Uint16 CTextureArray::GetMipCount() const
{
	if ( IsCooked() )
	{
		return m_cookedData.m_header.m_mipCount;
	}

#ifndef NO_TEXTURECACHE_COOKER
	// Find the minimum number of mips found in a texture.
	Uint16 numMips = 0;
	Bool foundOne = false;
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex != nullptr )
		{
			const Uint16 texMips = (Uint16)tex->GetMipCount();
			if ( texMips < numMips || !foundOne )
			{
				numMips = texMips;
				foundOne = true;
			}
		}
	}
	if ( foundOne )
	{
		return numMips;
	}
#endif

	return 0;
}

Uint16 CTextureArray::GetResidentMipIndex() const
{
	if ( IsCooked() )
	{
		return m_cookedData.m_header.m_residentMip;
	}

	Uint16 residentMip = 0;

#ifndef NO_TEXTURECACHE_COOKER
	// Find the maximum resident mip index in our textures.
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex != nullptr )
		{
			residentMip = Max< Uint16 >( residentMip, tex->GetResidentMipIndex() );
		}
	}
#endif

	return residentMip;
}

Uint32 CTextureArray::GetWidth() const
{
	if ( IsCooked() )
	{
		return m_cookedData.m_header.m_width;
	}

#ifndef NO_TEXTURECACHE_COOKER
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex != nullptr )
		{
			return tex->GetWidth();
		}
	}
#endif

	return 0;
}

Uint32 CTextureArray::GetHeight() const
{
	if ( IsCooked() )
	{
		return m_cookedData.m_header.m_height;
	}

#ifndef NO_TEXTURECACHE_COOKER
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex != nullptr )
		{
			return tex->GetHeight();
		}
	}
#endif

	return 0;
}

void CTextureArray::GetTextures( TDynArray< CBitmapTexture* >& textures ) const
{
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();
		if ( tex )
		{
			textures.PushBack( tex );
		}
	}
}

#ifndef NO_TEXTURECACHE_COOKER

void CTextureArray::UpdateTextureGroup()
{
	m_textureGroup = RED_NAME( Default );
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[i].m_texture.Get();
		if ( tex != nullptr )
		{
			m_textureGroup = tex->GetTextureGroupName();
			break;
		}
	}
}


Bool CTextureArray::CheckIfSizeFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture /*=NULL*/ ) const
{
	Uint32 width = texture->GetWidth();
	Uint32 height = texture->GetHeight();

	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();
		if ( tex != nullptr && ( tex->GetWidth() != width || tex->GetHeight() != height ) )
		{
			if ( firstNonMatchingTexture )
			{
				*firstNonMatchingTexture = tex;
			}
			return false;
		}
	}
	return true;
}

Bool CTextureArray::CheckIfMipMapLevelsFit( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture /*=NULL*/ ) const
{
	Uint32 mipMapLevels = texture->GetMipCount();

	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();
		if ( tex != nullptr && tex->GetMipCount() != mipMapLevels )
		{
			if ( firstNonMatchingTexture )
			{
				*firstNonMatchingTexture = tex;
			}
			return false;
		}
	}
	return true;
}

Bool CTextureArray::CheckIfCompressionFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture /*=NULL*/ ) const
{
	ETextureCompression compression = texture->GetCompression();

	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();
		if ( tex != nullptr && tex->GetCompression() != compression )
		{
			if ( firstNonMatchingTexture )
			{
				*firstNonMatchingTexture = tex;
			}
			return false;
		}
	}
	return true;
}

Bool CTextureArray::CheckIfDownscaleBiasFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture /*=NULL*/ ) const
{
	Uint32 pcbias = texture->GetPCDownscaleBias();
	Uint32 xbonebias = texture->GetXBoneDownscaleBias();
	Uint32 ps4bias = texture->GetPS4DownscaleBias();
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* tex = m_bitmaps[ i ].m_texture.Get();
		if ( tex != nullptr ) 
		{	 
			if ( tex->GetPCDownscaleBias() != pcbias || tex->GetXBoneDownscaleBias() != xbonebias || tex->GetPS4DownscaleBias() != ps4bias )
			{
				if ( firstNonMatchingTexture )
				{
					*firstNonMatchingTexture = tex;
				}
				return false;
			}
		}
	}
	return true;
}

Bool CTextureArray::Contains( const CBitmapTexture* texture ) const
{
	const String path = texture->GetDepotPath();

	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		if ( m_bitmaps[i].m_texture.GetPath() == path )
		{
			return true;
		}
	}
	return false;
}

void CTextureArray::AddTexture( CBitmapTexture* texture )
{
	if ( m_bitmaps.Empty() )
	{
		m_compression = texture->GetCompression();
		CBitmapTexture::GetCompressedFormat( texture->GetFormat(), m_compression, m_platformSpecificCompression, false );
	}

	m_bitmaps.PushBack( CTextureArrayEntry( texture ) );

	UpdateTextureGroup();
}

void CTextureArray::InsertTexture( CBitmapTexture* texture, Uint32 index )
{
	ASSERT( index >= 0 );
	m_bitmaps.Insert( index, CTextureArrayEntry( texture ) );
	UpdateTextureGroup();
}

void CTextureArray::RemoveTextureAt( Uint32 index )
{
	ASSERT( index >= 0 );
	ASSERT( index < m_bitmaps.Size() );
	m_bitmaps.RemoveAt( index );
	UpdateTextureGroup();
}
#endif 

void CTextureArray::GetTextureNames( TDynArray< CName >& textureNames ) const
{
#ifndef NO_TEXTURECACHE_COOKER
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		//may be slow
		textureNames.PushBack( CName( m_bitmaps[i].m_texture.GetPath() ) );
		//may be slow
	}
#endif
}

#ifndef NO_TEXTURECACHE_COOKER
void CTextureArray::SetTextures( const TDynArray< CBitmapTexture* >& textures )
{
	m_bitmaps.ClearFast();
	for ( Uint32 i = 0; i < textures.Size(); ++i )
	{
		AddTexture( textures[i] );
	}
}
#endif


//////////////////////////////////////////////////////////////////////////


GpuApi::eTextureFormat CTextureArray::CookedDataHeader::GetTextureFormat() const
{
	return ITexture::DecodeTextureFormat( m_encodedFormat );
}

#ifndef NO_TEXTURECACHE_COOKER

Bool CTextureArray::CookedDataHeader::SetTextureFormat( GpuApi::eTextureFormat format )
{
	Uint16 encoded = ITexture::EncodeTextureFormat( format );
	if ( encoded == 0 )
	{
		return false;
	}

	m_encodedFormat = encoded;
	return true;
}

#endif 




#ifndef NO_RESOURCE_COOKING

Bool CTextureArray::CanCook( String& outReason, const TDynArray< CBitmapTexture* >* bitmapOverrides /*= nullptr*/ ) const
{
	// No textures, or no mips: do not process it
	if ( GetTextureCount() == 0 )
	{
		outReason = String::Printf( TXT("Texture array '%ls' has no textures."), GetDepotPath().AsChar() );
		return false;
	}
	if ( GetMipCount() == 0 )
	{
		outReason = String::Printf( TXT("Texture array '%ls' has no mipmaps."), GetDepotPath().AsChar() );
		return false;
	}


	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		if ( !m_bitmaps[i].m_texture.Get() && ( !bitmapOverrides || !(*bitmapOverrides)[i] ) )
		{
			outReason = String::Printf( TXT("Texture array '%ls' has null bitmap at index %u."), GetDepotPath().AsChar(), i );
			return false;
		}
	}


	// Check consistent mip count, etc. We already know we have no null bitmaps, so we can just compare each one against bitmap 0.
	// TODO : This should take into account downscale bias on each texture.
	CBitmapTexture* bmp0 = m_bitmaps[0].m_texture.Get();
	for ( Uint32 i = 1; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* bmpi;
		if ( bitmapOverrides && (*bitmapOverrides)[i] )
		{
			bmpi = (*bitmapOverrides)[i];
		}
		else
		{
			bmpi = m_bitmaps[i].m_texture.Get();
		}

		if ( bmpi->GetWidth() != bmp0->GetWidth() || bmpi->GetHeight() != bmp0->GetHeight() )
		{
			outReason = String::Printf( TXT("Texture array '%ls' has size mismatch %ux%u != %ux%u."), GetDepotPath().AsChar(), bmpi->GetWidth(), bmpi->GetHeight(), bmp0->GetWidth(), bmp0->GetHeight() );
			return false;
		}
		if ( bmpi->GetMipCount() != bmp0->GetMipCount() )
		{
			outReason = String::Printf( TXT("Texture array '%ls' has mip count mismatch %u != %u."), GetDepotPath().AsChar(), bmpi->GetMipCount(), bmp0->GetMipCount() );
			return false;
		}
		if ( bmpi->GetPlatformSpecificCompression() != bmp0->GetPlatformSpecificCompression() )
		{
			outReason = String::Printf( TXT("Texture array '%ls' has format mismatch %") RED_PRIWas TXT(" != %") RED_PRIWas, GetDepotPath().AsChar(),
				GpuApi::GetTextureFormatName( bmpi->GetPlatformSpecificCompression() ), GpuApi::GetTextureFormatName( bmp0->GetPlatformSpecificCompression() ) );
			return false;
		}
		if ( bmpi->GetTextureGroupName() != bmp0->GetTextureGroupName() )
		{
			outReason = String::Printf( TXT("Texture array '%ls' has texture group mismatch %ls != %ls"), GetDepotPath().AsChar(),
				bmpi->GetTextureGroupName().AsChar(), bmp0->GetTextureGroupName().AsChar() );
			return false;
		}
	}

	return true;
}

void CTextureArray::OnCook( ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

#ifndef NO_TEXTURECACHE_COOKER
	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( cooker.GetPlatform() );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return;
	}


	// Copy the original m_bitmaps array. We're going to modify it temporarily to make sure we have only non-cooked sources.
	TDynArray< CBitmapTexture* > bitmapOverrides( m_bitmaps.Size() );

	// Build array of the actual CBitmapTextures. If any have already been cooked, we need to load them from their original file.
	for ( Uint32 i = 0; i < m_bitmaps.Size(); ++i )
	{
		CBitmapTexture* bmpi = m_bitmaps[i].m_texture.Get();
		if ( bmpi->IsCooked() )
		{
			// Texture has already been cooked, so we need to reload the original one...
			CDiskFile* diskFile = bmpi->GetFile();
			if ( diskFile == nullptr )
			{
				ERR_ENGINE( TXT("Couldn't get CDiskFile for bitmap %ls"), bmpi->GetFriendlyName().AsChar() );
				continue;
			}

			IFile* reader = diskFile->CreateReader();
			if ( reader == nullptr )
			{
				ERR_ENGINE( TXT("Couldn't create reader for bitmap %ls"), bmpi->GetFriendlyName().AsChar() );
				continue;
			}

			CDependencyLoader loader( *reader, nullptr );

			DependencyLoadingContext loadingContext;
			loadingContext.m_parent = this;
			if ( !loader.LoadObjects( loadingContext ) )
			{
				delete reader;
				ERR_ENGINE( TXT("Couldn't load bitmap %ls"), bmpi->GetFriendlyName().AsChar() );
				continue;
			}

			delete reader;

			if ( loadingContext.m_loadedRootObjects.Size() != 1 )
			{
				ERR_ENGINE( TXT("Unexpected number of loaded objects %u"), loadingContext.m_loadedRootObjects.Size() );
				continue;
			}

			bmpi = Cast< CBitmapTexture >( loadingContext.m_loadedRootObjects[0] );
			if ( bmpi == nullptr )
			{
				ERR_ENGINE( TXT("Couldn't get re-loaded bitmap %ls"), bmpi->GetFriendlyName().AsChar() );
				continue;
			}

			bitmapOverrides[ i ] = bmpi;
		}
	}



	// Check for any data errors. If there's some reason we can't cook (inconsistent or something), generate error data.
	// This way we at least have something.
	{
		String failReason;
		if ( !CanCook( failReason, &bitmapOverrides ) )
		{
			ERR_ENGINE( TXT("%ls"), failReason.AsChar() );
			HandleCookError( CookFunction );
			return;
		}
	}

	const Uint32 residentMipMap	= GetResidentMipForCookingPlatform( cooker.GetPlatform() );

	// Cook just the resident data.
	CTextureBakerSourceTextureArray residentSource( this, residentMipMap, &bitmapOverrides );
	CTextureBakerOutput residentCooked;
	if ( !CookFunction( residentSource, residentCooked ) )
	{
		ERR_ENGINE( TXT("Failed to cook resident data for %ls"), GetFriendlyName().AsChar() );
		return;
	}

	// Resident data header.
	m_cookedData.m_header.m_textureCacheKey	= CalcTextureCacheKey();
	m_cookedData.m_header.m_residentMip		= (Uint16)residentMipMap;
	m_cookedData.m_header.m_width			= (Uint16)GetWidth();
	m_cookedData.m_header.m_height			= (Uint16)GetHeight();
	m_cookedData.m_header.m_mipCount		= GetMipCount();
	m_cookedData.m_header.m_textureCount	= (Uint16)GetTextureCount();
	m_cookedData.m_header.SetTextureFormat( GetPlatformSpecificCompression() );
	m_cookedData.m_deviceData.Allocate( residentCooked.GetTotalDataSize() );
	Red::System::MemoryCopy( m_cookedData.m_deviceData.GetDataHandle().GetRawPtr(), residentCooked.GetData(), residentCooked.GetTotalDataSize() );


	// Now go through the textures we have, and destroy any that we manually reloaded
	for ( Uint32 i = 0; i < bitmapOverrides.Size(); ++i )
	{
		// Discard any bitmap overrides. These were loaded manually by us, so let's clean up and help avoid unnecessary GCing.
		if ( bitmapOverrides[ i ] != nullptr )
		{
			bitmapOverrides[ i ]->Discard();
		}
	}
#endif
}

// Hacky helper for functions not linked in some projects. Copy-pasted from DX10, as this is only
// supposed to be used when uncooking x64 builds. Types & in lines come from gpuApiInterface.h.
namespace TextureArrayUncookingHelper
{
	struct TextureDesc
	{
		TextureDesc ( ) { }

		Uint32 CalcTargetSlices() const { return type == GpuApi::TEXTYPE_CUBE ? sliceNum * 6 : sliceNum; }
		Uint32 CalcTargetLevels() const { return initLevels > 0 ? initLevels : (1 + Red::Math::MLog2( Red::Math::NumericalUtils::Max( width, height ) )); }

		GpuApi::eTextureType type;
		Uint32 width;
		Uint32 height;
		Uint16 initLevels;
		Uint16 sliceNum;
		GpuApi::eTextureFormat format;
	};

	Bool CalculateCookedTextureMipOffsetAndSize( const TextureDesc& texDesc, Uint32 mip, Uint32 slice, Uint32* outOffset, Uint32* outSize )
	{
		const Uint32 texMips = texDesc.CalcTargetLevels();
		const Uint32 texSlices = texDesc.CalcTargetSlices();

		if ( mip >= texMips || slice >= texSlices )
		{
			return false;
		}

		if ( outOffset != nullptr )
		{
			Uint32 offset = 0;

			for ( Uint32 i = 0; i <= mip; ++i )
			{
				Uint32 mipWidth		= GpuApi::CalculateTextureMipDimension( texDesc.width, i, texDesc.format );
				Uint32 mipHeight	= GpuApi::CalculateTextureMipDimension( texDesc.height, i, texDesc.format );
				Uint32 mipSize		= GpuApi::CalculateTextureSize( mipWidth, mipHeight, texDesc.format );

				if ( i < mip )
				{
					offset += mipSize * texSlices;
				}
				else
				{
					offset += mipSize * slice;
				}
			}

			*outOffset = offset;
		}

		if ( outSize != nullptr )
		{
			const Uint32 mipWidth	= GpuApi::CalculateTextureMipDimension( texDesc.width, mip, texDesc.format );
			const Uint32 mipHeight	= GpuApi::CalculateTextureMipDimension( texDesc.height, mip, texDesc.format );
			*outSize				= GpuApi::CalculateTextureSize( mipWidth, mipHeight, texDesc.format );
		}

		return true;
	}
}

Bool CTextureArray::UncookData( const TextureCacheEntry& textureEntry, BufferHandle textureData, const String& sourceFile, const String& depotPath )
{
	Uint8* dataPtr = static_cast< Uint8* >( textureData->GetData( ) );
	Uint32 dataSize = textureData->GetSize( );

	// Texarray descriptor.
	TextureArrayUncookingHelper::TextureDesc desc;
	desc.type = GpuApi::TEXTYPE_ARRAY;
	desc.format = m_cookedData.m_header.GetTextureFormat( );
	desc.width = GpuApi::CalculateTextureMipDimension( m_cookedData.m_header.m_width, 0, desc.format );
	desc.height = GpuApi::CalculateTextureMipDimension( m_cookedData.m_header.m_height, 0, desc.format );
	desc.initLevels = 0;
	desc.sliceNum = m_cookedData.m_header.m_textureCount;

	// Rebuild m_bitmaps.
	for( Uint32 i = 0; i < m_cookedData.m_header.m_textureCount; ++i )
	{
		// Create new texture and initialize it from mip.
		CBitmapTexture* newTexture = ::CreateObject< CBitmapTexture >( );
		CBitmapTexture::MipMap newMip( m_cookedData.m_header.m_width, m_cookedData.m_header.m_height, 0 );
		newMip.m_data = BitmapMipLatentDataBuffer( dataPtr, dataSize );
		newTexture->InitFromMip( newMip, GetTextureGroupName( ), TRF_TrueColor );

		// Set up basic stuff (placeholder data, though).
		BufferHandle textureMipData( new BufferProxy( static_cast< void* >( dataPtr ), dataSize, [ ] ( void* ptr ) { } ) );
		newTexture->UncookData( textureEntry, textureMipData );

		// Set up proper data pointers (stored as tex0mip0, tex1mip0, tex2mip0, tex0mip1, tex1mip1, etc..).
		CBitmapTexture::MipArray& mips = newTexture->GetMips( );
		for( Uint32 mip = 0; mip < mips.Size( ); ++mip )
		{
			mips[ mip ].m_data = BitmapMipLatentDataBuffer( );

			Uint32 mipOffset = 0, mipSize = 0;
			if( TextureArrayUncookingHelper::CalculateCookedTextureMipOffsetAndSize( desc, mip, i, &mipOffset, &mipSize ) )
			{
				if( CBitmapTexture::CreateMip( mips[ mip ], mips[ mip ].m_width, mips[ mip ].m_height, newTexture->GetFormat( ), newTexture->GetCompression( ) ) )
				{
					mips[ mip ].m_data = BitmapMipLatentDataBuffer( dataPtr + mipOffset, mipSize );
				}
			}
		}

		// Not sure if this is needed at all.
		newTexture->SetResidentMipIndex( 0 );

		// Give the resource a proper path (I don't like it, damn you RedEngine for giving me no alternative).
		const CFilePath filePath( sourceFile.ToLower( ) );
		const String newFileName = String::Printf( TXT( "%ls.texture_%d.xbm" ), filePath.GetFileNameWithExt( ).AsChar( ), i );
		const String dirPathName = filePath.GetPathString( true ).ToLower( ).StringAfter( depotPath );
		CDirectory* directory = GDepot->CreatePath( dirPathName );
		newTexture->SaveAs( directory, newFileName, true );

		// Register the texture within the texarray resource.
		AddTexture( newTexture );
	}

	m_cookedData = CookedData( );

	ClearFlag( OF_WasCooked );

	return true;
}

#ifndef NO_TEXTURECACHE_COOKER
void CTextureArray::HandleCookError( CAsyncTextureBaker::CookFunctionPtr cookFunction )
{
	// If we ran into some sort of error during cook, we'll generate some fake data, so we can still have something.
	// So, the generated array is 1x1, 1 slice, RGBA8, filled with pink.

	class CTextureArrayErrorSource : public ITextureBakerSource, public Red::System::NonCopyable
	{
	private:
		Uint32 m_dummyData;

	public:
		CTextureArrayErrorSource()
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
		virtual Bool IsLooseFileTexture() const { return false; }
	};


	CTextureArrayErrorSource residentSource;
	CTextureBakerOutput residentCooked;
	if ( !cookFunction( residentSource, residentCooked ) )
	{
		RED_HALT( "Failed to generate error data for %ls", GetFriendlyName().AsChar() );
		return;
	}

	m_cookedData.m_header.m_textureCacheKey	= CalcTextureCacheKey();
	m_cookedData.m_header.m_residentMip		= 0;
	m_cookedData.m_header.m_width			= residentSource.GetBaseWidth();
	m_cookedData.m_header.m_height			= residentSource.GetBaseHeight();
	m_cookedData.m_header.m_mipCount		= residentSource.GetMipCount();
	m_cookedData.m_header.m_textureCount	= residentSource.GetSliceCount();
	m_cookedData.m_header.SetTextureFormat( residentSource.GetTextureFormat() );
	m_cookedData.m_deviceData.Allocate( residentCooked.GetTotalDataSize() );
	Red::System::MemoryCopy( m_cookedData.m_deviceData.GetDataHandle().GetRawPtr(), residentCooked.GetData(), residentCooked.GetTotalDataSize() );
}
#endif

Uint32 CTextureArray::GetMipCountForCookingPlatform( ECookingPlatform platform ) const
{
	return GetMipCount() - GetHighestMipForCookingPlatform( platform );
}

Uint32 CTextureArray::GetHighestMipForCookingPlatform( ECookingPlatform platform ) const
{
	Uint32 numMips = GetMipCount();
	Uint32 firstMip = 0;
	if ( numMips > 1 )
	{
		// Based on platform, calculate number of mipmaps to drop
		Int32 downscaleBias = 0;

		if ( platform == PLATFORM_XboxOne )
		{
			downscaleBias = m_xboneDownscaleBias - m_pcDownscaleBias;
		}
		else if ( platform == PLATFORM_PS4 )
		{
			downscaleBias = m_ps4DownscaleBias - m_pcDownscaleBias;
		}

		firstMip = ( Uint32 )Clamp< Int32 >( downscaleBias, 0, numMips - 1 );
	}

	return firstMip;
}

Uint32 CTextureArray::GetResidentMipForCookingPlatform( ECookingPlatform platform ) const
{
#ifdef HANDS_ON_DEMO_HACK
	// HANDS-ON HACK -- Doing terrain update in SetupInitialFrame seems to cause problems on PS4, so we'll just
	// keep texture arrays resident so we don't need it. Terrain textures shouldn't be streamed anyways, and any
	// other arrays... well... hopefully good enough for this...
	return 0;
#else

	const Uint32 highestMip = GetHighestMipForCookingPlatform( platform );
	const Uint32 numMips = GetMipCount() - highestMip;

	// Determine first mipmap to use for texture
	Uint32 residentMipMap = 0;
	// up to 32x32 will be kept resident
	const Uint32 maxResidentMips = 6;

	//const TextureGroup& texGroup = texture->GetTextureGroup();
	if ( /*texGroup.m_isStreamable &&*/ numMips > maxResidentMips  )
	{
		residentMipMap = numMips - maxResidentMips;
	}

	return residentMipMap;

#endif
}


#endif // !NO_RESOURCE_COOKING




#ifndef NO_TEXTURECACHE_COOKER

CTextureBakerSourceTextureArray::CTextureBakerSourceTextureArray( const CTextureArray* texture, Uint16 baseMip, const TDynArray< CBitmapTexture* >* bitmapOverrides /*= nullptr*/ )
	: m_texture( texture )
	, m_baseMip( baseMip )
{
	RED_WARNING( m_texture != nullptr, "No texture!" );
	if ( m_texture == nullptr ) { return; }

	m_format = m_texture->GetPlatformSpecificCompression();
	m_mipCount = (Uint16)m_texture->GetMipCount();

	// TODO : m_texture->GetTextures() is a bit broken, it just returns the non-null textures. This is a quick workaround
	// so that cooking can work in the face of an array with a null texture, but GetTextures() should really be fixed.
	// Of course, that'll also require going through everywhere that uses it, to make sure they handle nulls reasonably.
	m_bitmaps.Resize( m_texture->GetTextureCount() );
	for ( Uint32 i = 0; i < m_texture->GetTextureCount(); ++i )
	{
		THandle< CBitmapTexture > tex = m_texture->GetTexture( i );

		if ( bitmapOverrides && (*bitmapOverrides)[i] )
		{
			tex = (*bitmapOverrides)[i];
		}

		if ( tex == nullptr )
		{
			ERR_ENGINE( TXT("Failed to open %ls for %ls slice %u"), m_texture->GetTexturePath( i ).AsChar(), m_texture->GetFriendlyName().AsChar(), i );
		}
		if ( tex->IsCooked() )
		{
			ERR_ENGINE( TXT("Got a pre-cooked texture %ls for slice %u"), m_texture->GetTexturePath( i ).AsChar(), m_texture->GetFriendlyName().AsChar(), i );
		}

		m_bitmaps[i] = tex;
	}
}

Uint16 CTextureBakerSourceTextureArray::GetSliceCount() const
{
	return (Uint16)m_texture->GetTextureCount();
}

Uint16 CTextureBakerSourceTextureArray::GetMipCount() const
{
	return m_mipCount - m_baseMip;
}

const void* CTextureBakerSourceTextureArray::GetMipData( Uint16 mip, Uint16 slice ) const
{
	if ( !m_bitmaps[slice] )
	{
		return nullptr;
	}

	const LatentDataBuffer* buffer = CTextureBakerSourceBitmapTexture::GetBitmapMipDataBuffer( m_bitmaps[slice], mip + m_baseMip );
	return buffer != nullptr ? buffer->GetData() : nullptr;
}

Uint32 CTextureBakerSourceTextureArray::GetMipDataSize( Uint16 mip, Uint16 slice ) const
{
	if ( !m_bitmaps[slice] )
	{
		return 0;
	}

	const LatentDataBuffer* buffer = CTextureBakerSourceBitmapTexture::GetBitmapMipDataBuffer( m_bitmaps[slice], mip + m_baseMip );
	return buffer != nullptr ? buffer->GetSize() : 0;
}

Uint32 CTextureBakerSourceTextureArray::GetMipPitch( Uint16 mip, Uint16 slice ) const
{
	if ( !m_bitmaps[slice] )
	{
		return 0;
	}

	return m_bitmaps[slice]->GetMips()[mip + m_baseMip].m_pitch;
}

Uint16 CTextureBakerSourceTextureArray::GetBaseWidth() const
{
	return (Uint16)Max( m_texture->GetWidth() >> m_baseMip, 1u );
}

Uint16 CTextureBakerSourceTextureArray::GetBaseHeight() const
{
	return (Uint16)Max( m_texture->GetHeight() >> m_baseMip, 1u );
}

GpuApi::eTextureFormat CTextureBakerSourceTextureArray::GetTextureFormat() const
{
	return m_format;
}

GpuApi::eTextureType CTextureBakerSourceTextureArray::GetTextureType() const
{
	return GpuApi::TEXTYPE_ARRAY;
}

#endif // !NO_TEXTURECACHE_COOKER
