/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bitmapTexture.h"
#include "renderCommands.h"
#include "textureCache.h"
#include "../core/dependencyMapper.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "renderFence.h"
#include "renderSettings.h"
#include "../core/importer.h"
#include "../core/diskFile.h"

IMPLEMENT_ENGINE_CLASS( CBitmapTexture );

IMPLEMENT_RTTI_ENUM( ETextureCompression );
IMPLEMENT_RTTI_ENUM( ETextureCategory );
IMPLEMENT_RTTI_ENUM( ETextureRawFormat );

CBitmapTexture::CBitmapTexture()
	: m_textureGroup( CNAME( Default ) )
	, m_sourceData( NULL )
	, m_pcDownscaleBias( 1 )
	, m_xboneDownscaleBias( 1 )
	, m_ps4DownscaleBias( 1 )
#ifndef NO_DEBUG_PAGES
	, m_resourceLoadError( false )
#endif
	, m_cookedData( GpuApi::INPLACE_Texture )
{
}

CBitmapTexture::~CBitmapTexture()
{
	// Release streaming source
	SAFE_RELEASE( m_streamingSource );

	// Release rendering data
	SAFE_RELEASE( m_renderResource );
}

void CBitmapTexture::SetTextureGroup( CName textureGroup )
{
#ifndef NO_RESOURCE_IMPORT
	CName oldTextureGroup = textureGroup;

	// We have import file, easy job
	if ( m_sourceData )
	{
		// Change texture group and compression
		m_textureGroup = textureGroup;
		m_compression = GetTextureGroup().m_compression;

		// Get platform-specific compression
		GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, false );

		// Generate mipmaps
		GenerateMipmaps();
	}
	else if ( GFileManager->GetFileSize( m_importFile ) > 0 )
	{
		// Reimport
		String importFileExt = CFilePath( m_importFile ).GetExtension();
		IImporter* importer = IImporter::FindImporter( ClassID< CBitmapTexture >(), importFileExt );
		if ( !importer )
		{
			GFeedback->ShowError( TXT("There is no importer to import source data from '%ls'. Make source source data is there."), m_importFile.AsChar() );
			return;
		}

		// Change texture group and compression
		m_textureGroup = textureGroup;
		m_compression = GetTextureGroup().m_compression;

		GpuApi::eTextureFormat oldPlatformSpecificCompression = m_platformSpecificCompression;
		GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, false );

		// Reimport file
		IImporter::ImportOptions options;
		options.m_existingResource = this;
		options.m_parentObject = GetParent();
		options.m_sourceFilePath = m_importFile;
		if ( !importer->DoImport( options ) )
		{
			m_textureGroup = oldTextureGroup;
			m_compression = GetTextureGroup().m_compression;
			m_platformSpecificCompression = oldPlatformSpecificCompression;
			GFeedback->ShowError( TXT("Unable to recompress texture '%ls' from '%ls'"), GetFriendlyName().AsChar(), m_importFile.AsChar() );
			return;
		}
	}
	else if ( m_importFile.Empty() )
	{
		GFeedback->ShowError( TXT("Texture has no source artist data. Unable to recompress.") );
	}
	else
	{
		GFeedback->ShowError( TXT("Unable to find source data in '%ls'"), m_importFile.AsChar() );
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBitmapTexture::SerializeMipMaps( IFile& file )
{
	const TextureGroup& textureGroup = GetTextureGroup();
	const Bool allowStreaming = textureGroup.m_hasMipchain && textureGroup.m_isStreamable;
	const Bool useTextureCache = IsCooked() && m_residentMipIndex > 0;

	// Do nothing for GC, mapper, and non-file-based files. We only need to do work here if we're saving to or loading from an actual file.
	if ( file.IsGarbageCollector() || file.IsMapper() || !file.IsFileBased() || ( !file.IsReader() && !file.IsWriter() ) )
	{
		return;
	}


	// We are writing, flush all streaming before that - it's needed so we don't read while writing at the same time
	if ( file.IsWriter() && GRender )
	{
		// Send the cancel streaming command
		( new CRenderCommand_CancelTextureStreaming() )->Commit();

		// Flush renderer
		CRenderFenceHelper flushHelper( GRender );
		flushHelper.Flush();
	}


	// Load the skip offset
	// TODO : Remove this next time version is bumped? Doesn't appear to be used for anything!
	Uint32 skipOffset = 0;
	file << skipOffset;


	if ( file.IsReader() )
	{
		// Load the mip count
		Uint32 mipCount = 0;
		file << mipCount;

		// Create mipmaps
		m_mips.Resize( mipCount );

		if ( !useTextureCache )
		{
			SAFE_RELEASE( m_streamingSource );

			// Create the streaming source data
			if ( allowStreaming )
			{
				IFileLatentLoadingToken* loadingToken = file.CreateLatentLoadingToken( file.GetOffset() );
				if ( loadingToken )
				{
					m_streamingSource = new CBitmapTextureStreamingSourcePC( (Uint16)m_width, (Uint16)m_height, (Uint8)GetMipCount(), loadingToken );
				}
			}
		}
	}
	else
	{
		// Save the mip chain size
		Uint32 mipCount = m_mips.Size();
		file << mipCount;

		// Writing to the same file
		if ( allowStreaming && !useTextureCache )
		{
			// Destroy previous streaming source
			SAFE_RELEASE( m_streamingSource );
				
			// Recreate loading token
			IFileLatentLoadingToken* loadingToken = file.CreateLatentLoadingToken( file.GetOffset() );
			if ( loadingToken )
			{
				// Recreate streaming source
				m_streamingSource = new CBitmapTextureStreamingSourcePC( (Uint16)m_width, (Uint16)m_height, (Uint8)GetMipCount(), loadingToken );
			}
		}
	}

	if (  ( (Int32)m_width < (1 << (GetMipCount() - 1)) ) && ( (Int32)m_height < (1 << (GetMipCount() - 1)) ) )
	{
		RED_LOG( RED_LOG_CHANNEL( "Texture" ), TXT("Texture mip level and size mismatch, it needs reimport: %ls"), GetFile()->GetFileName().AsChar() );
	}

	// Placeholder - calculate resident mipmap index
	const Int32 numResidentMips = Config::cvMaxResidentMipMap.Get();
	const Int32 firstMipToPreload = Max< Int32 >( 0, m_mips.SizeInt() - numResidentMips );

	// Load mips
	for ( Uint32 i=0; i<m_mips.Size(); i++ )
	{
		// Load mipmap info
		CBitmapTexture::MipMap& mipmap = m_mips[i];
		file << mipmap.m_width;
		file << mipmap.m_height;
		file << mipmap.m_pitch;

		// Get the file offset at the beginning of the mipmap. We use this later for setting up the streaming source.
		const Uint32 mipFileOffset = static_cast< Uint32 >( file.GetOffset() ) + sizeof( Uint32 );

		// Serialize mipmap data
		if ( !file.IsCooker() && !IsCooked() )
		{
			const Bool allowStreaming = textureGroup.m_hasMipchain;
			const Bool preload = !m_streamingSource || ( (Int32)i >= firstMipToPreload );
			mipmap.m_data.Serialize( file, allowStreaming, preload );
		}

		// Update the mip data size to load
		if ( m_streamingSource )
		{
			// TODO: move this to constructor of CBitmapTextureStreamingSourcePC
			SBitmapTextureStreamingMipInfo* mipInfo = const_cast< SBitmapTextureStreamingMipInfo* >( m_streamingSource->GetMipLoadingInfo( i ) );
			mipInfo->m_pitch = mipmap.m_pitch;
			mipInfo->m_offset = mipFileOffset;
			mipInfo->m_size = static_cast< Uint32 >( mipmap.m_data.GetSize() );
			mipInfo->m_isCooked = false;
		}
	}


	// Set up streaming source to read from texture cache.
	if ( file.IsReader() && useTextureCache && allowStreaming )
	{
		// If we're reading in a texture with cooked data in the texture cache, set up the streaming source to load from there.
		SAFE_RELEASE( m_streamingSource );

		m_streamingSource = new CTextureCacheStreamingSourcePC( m_textureCacheKey );
	}
#ifndef RED_FINAL_BUILD
	if ( m_streamingSource != nullptr )
	{
		m_streamingSource->SetDebugName( GetDepotPath() );
	}
#endif
}

void CBitmapTexture::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// No crap is done on mapping or on GC
	if ( file.IsMapper() || file.IsGarbageCollector() )
	{
		return;
	}

	// No texture group, use default
	if ( !m_textureGroup )
	{
		m_textureGroup = CNAME( Default );
	}

	if ( file.IsReader() )
	{
#ifdef USE_NEW_COMPRESSION
		Bool isOldVersion = file.GetVersion() < VER_ADDED_NEW_TEXTURE_COMPRESSION_FORMATS;
#else
		Bool isOldVersion = true;
#endif

		CBitmapTexture::GetCompressedFormat( m_format, m_compression, m_platformSpecificCompression, isOldVersion );
	}

	// Deal with the display mip maps
	SerializeMipMaps( file );

	// Serialize cooked data
	m_cookedData.Serialize( file );

	SAFE_RELEASE( m_renderResource );
}

void CBitmapTexture::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Create rendering data
	CreateRenderResource();
}

#ifndef NO_DATA_VALIDATION
void CBitmapTexture::OnCheckDataErrors() const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors();

	// Texture is empty
	if ( !m_width || !m_height || !m_mips.Size() )
	{
		DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Texture is empty and has no data") );
	}

	// Source data is empty
	if ( !m_sourceData && GetTextureGroup().m_hasMipchain && (GetFile() != NULL ) )
	{
		DATA_HALT( DES_Tiny, this, TXT("Rendering"), TXT("Texture has no source data") );
	}

	// Texture is not assigned to any texture group
	if ( !m_mips.Empty() && ( m_textureGroup == CNAME( Default ) || !m_textureGroup ) )
	{
		const Uint32 currentMemory = static_cast< Uint32 >( m_mips[0].m_data.GetInternalMemSize() );
		const Uint32 projectedMemory = currentMemory / 4;
		const Float wastedMemory = (Float)( currentMemory - projectedMemory ) * 1.33f / 1024.0f;
		if ( wastedMemory > 1024.0f )
		{
			DATA_HALT( DES_Minor, this, TXT("Rendering"), TXT("Texture has no texture group assigned. Around %1.2fMB of GPU memory is wasted."), wastedMemory / 1024.0f );
		}
		else
		{
			DATA_HALT( DES_Minor, this, TXT("Rendering"), TXT("Texture has no texture group assigned. Around %1.2fKB of GPU memory is wasted."), wastedMemory );
		}
	}

	// Failed to create rendering resource
	if ( !m_renderResource && !GIsCooker )
	{
		DATA_HALT( DES_Uber, this, TXT("Rendering"), TXT("Texture was not uploaded to GPU. Ask programmers.") );
	}
}
#endif // NO_DATA_VALIDATION

IRenderResource* CBitmapTexture::GetRenderResource() const
{
	// No render resource created
	if ( !m_renderResource && !GRender->IsDeviceLost() )
	{
		// Recreate rendering resource
		CBitmapTexture* nonConstThis = const_cast<CBitmapTexture*>( this );
		nonConstThis->CreateRenderResource();
	}

	// Return created resource
	return m_renderResource;
}

void CBitmapTexture::CreateRenderResource()
{ 
	// Create driver version of texture
	if ( GRender && !GRender->IsDeviceLost() )
	{
		// Release previous version
		SAFE_RELEASE( m_renderResource );

		// Create rendering resource
		m_renderResource = GRender->UploadTexture( this );

		// Evict all data from memory
		for ( Uint32 i=0; i<m_mips.Size(); i++ )
		{
			CBitmapTexture::MipMap& mip = m_mips[i];
			mip.m_data.Unload();
		}
	}
}

void CBitmapTexture::ReleaseRenderResource()
{
	SAFE_RELEASE( m_renderResource );
}

void CBitmapTexture::GetAdditionalInfo( TDynArray< String >& info ) const
{
	// General info
	{
		String text = String::Printf( TXT("%ix%i, %i mipmap(s)"), m_width, m_height, m_mips.Size() );
		info.PushBack( text );
	}

	// Texture group
	{
		info.PushBack( m_textureGroup.AsString() );
	}
}

const TextureGroup& CBitmapTexture::GetTextureGroup() const
{
	// Find texture group by name
	if ( GRender )
	{
		// Use the texture group settings
		const TextureGroup* group = SRenderSettingsManager::GetInstance().GetTextureGroup( m_textureGroup );
		if ( group )
		{
			return *group;
		}
	}

	// Use default settings
	static TextureGroup defaultSettings;
	return defaultSettings;
}

Uint32 CBitmapTexture::GetDownscaleBiasForCurrentPlatform() const 
{
#if defined(RED_PLATFORM_WINPC)
	return 0;
#elif defined(RED_PLATFORM_DURANGO)
	return GetXBoneDownscaleBias() - GetPCDownscaleBias();
#elif defined(RED_PLATFORM_ORBIS)
	return GetPS4DownscaleBias() - GetPCDownscaleBias();
#else
	return 0;
#endif
}

CBitmapTexture::SStats CBitmapTexture::GetStats() const
{
	SStats stats;
	{
		stats.m_width = m_width;					// Width of the bitmap
		stats.m_height = m_height;					// Height of the bitmap
		stats.m_mips = m_mips;						// Mipmaps
		stats.m_sourceData = m_sourceData;			// Source artist data
		stats.m_format = m_format;					// Artist data format
		
#ifndef NO_RESOURCE_IMPORT
		stats.m_importFile = m_importFile;
#endif
	};
	return stats;
}



#ifndef NO_RESOURCE_COOKING

Uint32 CBitmapTexture::GetHighestMipForCookingPlatform( ECookingPlatform platform ) const
{
	Uint32 numMips = m_mips.Size();
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

Uint32 CBitmapTexture::GetResidentMipForCookingPlatform( ECookingPlatform platform ) const
{
	const Uint32 highestMip = GetHighestMipForCookingPlatform( platform );
	const Uint32 numMips = m_mips.Size() - highestMip;


	// Determine first mipmap to use for texture
	Uint32 residentMipMap = 0;

	// up to 32x32 will be kept resident
	const Uint32 maxResidentMips = 6;

	const TextureGroup& texGroup = GetTextureGroup();

	if ( texGroup.m_isStreamable && numMips > maxResidentMips  )
	{
		residentMipMap = numMips - maxResidentMips;
	}

	return residentMipMap;
}

void CBitmapTexture::OnCook( ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

#ifndef NO_TEXTURECACHE_COOKER
	CAsyncTextureBaker::CookFunctionPtr CookFunction = GTextureCacheCooker->GetDefaultCookFunction( cooker.GetPlatform() );
	if ( CookFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return;
	}

	const Uint32 firstMip = GetHighestMipForCookingPlatform( cooker.GetPlatform() );
	const Uint32 residentMip = GetResidentMipForCookingPlatform( cooker.GetPlatform() );

	// Remove high mips
	if ( firstMip > 0 )
	{
		DropMipmaps( firstMip );
	}

	CTextureBakerSourceBitmapTexture residentSource( this, residentMip );
	CTextureBakerOutput residentCooked;
	if ( !CookFunction( residentSource, residentCooked ) )
	{
		RED_HALT( "Failed to cook resident data for %s", GetFriendlyName().AsChar() );
		return;
	}

	m_cookedData = GpuDataBuffer( GpuApi::INPLACE_Texture, residentCooked.GetTotalDataSize(), residentCooked.GetData(), residentCooked.GetDataAlignment() );

	m_residentMipIndex = residentMip;

	m_textureCacheKey = CalcTextureCacheKey();
#endif
}

Bool CBitmapTexture::UncookData( const TextureCacheEntry& textureEntry, BufferHandle textureData )
{
	Uint8* dataPtr = static_cast< Uint8* >( textureData->GetData( ) );
	for( Uint32 i = 0; i < m_mips.Size( ); ++i )
	{
		// Let the helper determine data sizes for us.
		if( CreateMip( m_mips[i], m_mips[i].m_width, m_mips[i].m_height, m_format, GetCompression( ) ) )
		{
			// Store the mip data compressed in current platform's format.
			Uint32 srcBytes = m_mips[i].m_data.GetSize( );
			Red::MemoryCopy( m_mips[i].m_data.GetData( ), dataPtr, srcBytes );
			dataPtr += srcBytes;
		}
		else
		{
			// No data (zero width or zero height) or invalid texture format.
			RED_LOG_WARNING( CBitmapTexture, TXT( "Invalid or missing data for mip level %d!" ), i );
		}
	}

	m_residentMipIndex = 0;
	m_textureCacheKey = 0;

	ClearFlag( OF_WasCooked );

	return true;
}

#endif // !NO_RESOURCE_COOKING

#ifndef NO_EDITOR
Uint32 CBitmapTexture::CalcTextureDataSize()
{
	Uint32 rawData = 0;

	const CBitmapTexture::MipArray& mips = this->GetMips();
	for ( Uint32 i=0; i<mips.Size(); i++ )
	{
		rawData += mips[i].m_data.GetSize();
	}

	return rawData;
}
#endif // !NO_EDITOR

#ifndef NO_TEXTURECACHE_COOKER

CTextureBakerSourceBitmapTexture::CTextureBakerSourceBitmapTexture( CBitmapTexture* texture, Uint16 startMip /*= 0*/ )
	: m_texture( texture )
	, m_startMip( startMip )
{}

Uint16 CTextureBakerSourceBitmapTexture::GetSliceCount() const
{
	return 1;
}

Uint16 CTextureBakerSourceBitmapTexture::GetMipCount() const
{
	return (Uint16)m_texture->GetMipCount() - m_startMip;
}

const void* CTextureBakerSourceBitmapTexture::GetMipData( Uint16 mip, Uint16 /*slice*/ ) const
{
	const LatentDataBuffer* buffer = GetBitmapMipDataBuffer( m_texture, mip + m_startMip );
	return buffer != nullptr ? buffer->GetData() : nullptr;
}

Uint32 CTextureBakerSourceBitmapTexture::GetMipDataSize( Uint16 mip, Uint16 /*slice*/ ) const
{
	const LatentDataBuffer* buffer = GetBitmapMipDataBuffer( m_texture, mip + m_startMip );
	return buffer != nullptr ? buffer->GetSize() : 0;
}

Uint32 CTextureBakerSourceBitmapTexture::GetMipPitch( Uint16 mip, Uint16 /*slice*/ ) const
{
	return m_texture->GetMips()[mip + m_startMip].m_pitch;
}

Uint16 CTextureBakerSourceBitmapTexture::GetBaseWidth() const
{
	return (Uint16)m_texture->GetMips()[m_startMip].m_width;
}

Uint16 CTextureBakerSourceBitmapTexture::GetBaseHeight() const
{
	return (Uint16)m_texture->GetMips()[m_startMip].m_height;
}

GpuApi::eTextureFormat CTextureBakerSourceBitmapTexture::GetTextureFormat() const
{
	return m_texture->GetPlatformSpecificCompression();
}

GpuApi::eTextureType CTextureBakerSourceBitmapTexture::GetTextureType() const
{
	return GpuApi::TEXTYPE_2D;
}


const LatentDataBuffer* CTextureBakerSourceBitmapTexture::GetBitmapMipDataBuffer( CBitmapTexture* texture, Uint32 mip )
{
	const CBitmapTexture::MipArray& mips = texture->GetMips();
	if ( mip >= mips.Size() )
	{
		return nullptr;
	}

	BitmapMipLatentDataBuffer& data = const_cast< BitmapMipLatentDataBuffer& >( mips[mip].m_data );
	if ( data.GetData() == nullptr )
	{
		if ( !data.Load() )
		{
			ERR_ENGINE( TXT("Failed to load mip %d of texture '%s'"), mip, texture->GetFriendlyName().AsChar() );
			return nullptr;
		}
	}

	return &data;
}

#endif // !NO_TEXTURECACHE_COOKER
