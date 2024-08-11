/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTexture.h"
#include "renderHelpers.h"
#include "../../common/engine/textureCache.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../core/fileLatentLoadingToken.h"

#include "../engine/swfTexture.h"
#include "../engine/baseEngine.h"

CRenderTexture::CRenderTexture( const GpuApi::TextureRef &texture )
	: CRenderTextureBase( texture )
	, m_width( 1.f )
	, m_height( 1.f )
{
	if ( m_texture )
	{
		GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( m_texture );
		m_width = (Float)desc.width;
		m_height = (Float)desc.height;
	}
}

CRenderTexture::~CRenderTexture()
{
}

CName CRenderTexture::GetCategory() const
{
	return RED_NAME( RenderTexture );
}

void CRenderTexture::GetSize( Float& width, Float& height ) const
{
	width = m_width;
	height = m_height;
}

void CRenderTexture::GetSizeVector( Vector& sizeVector ) const
{
	sizeVector = Vector( m_width, m_height, 1.0f / m_width, 1.0f / m_height );
}

void CRenderTexture::GetInvSizeVector( Vector& sizeVector ) const
{
	sizeVector = Vector( 1.0f / m_width, 1.0f / m_height, 0.0f, 0.0f );
}


void CRenderTexture::OnStreamingChanged()
{
	// Get whichever is the best texture for us to use.
	GpuApi::TextureRef texToUse;
	if ( m_hiResTexture )
	{
		texToUse = m_hiResTexture;
	}
	else if ( m_texture )
	{
		texToUse = m_texture;
	}

	// If we have a valid texture, update our cached size from it.
	if ( texToUse )
	{
		GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( texToUse );
		m_width = (Float)desc.width;
		m_height = (Float)desc.height;
	}
	else
	{
		m_width = 1.f;
		m_height = 1.f;
	}
}


static GpuApi::TextureRef LoadFromCookedEngineTexture( const CBitmapTexture* engineTexture, GpuApi::eTextureGroup textureGroup )
{
	const CBitmapTexture::MipArray& mips = engineTexture->GetMips();

	GpuDataBuffer& nonConstBuffer = const_cast< GpuDataBuffer& >( engineTexture->GetCookedData() );
	if ( !nonConstBuffer.GetDataHandle().IsValid() )
	{
		WARN_RENDERER( TXT("Cooked data is not loaded. Loading now. %ls"), engineTexture->GetFriendlyName().AsChar() );
		nonConstBuffer.Load();
	}

	Uint8 residentMip = engineTexture->GetResidentMipIndex();

	// Declare texture object info
	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_2D;
	desc.initLevels		= static_cast< Uint16 >( engineTexture->GetMipCount() - residentMip );
	desc.width			= mips[ residentMip ].m_width;
	desc.height			= mips[ residentMip ].m_height;
	desc.usage			= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.format			= engineTexture->GetPlatformSpecificCompression();
	desc.inPlaceType	= GpuApi::INPLACE_Texture;

	GpuApi::TextureInitData initData;
	initData.m_isCooked = true;
	initData.m_cookedData = nonConstBuffer.GetDataHandle();

	// Create texture
	GpuApi::TextureRef newTexture = GpuApi::CreateTexture( desc, textureGroup, &initData );
	if ( newTexture )
	{
		nonConstBuffer.UnlinkDataHandle();
	}
	else
	{
		WARN_RENDERER( TXT("Failed to create texture %ix%i, %ls"), desc.width, desc.height, engineTexture->GetFriendlyName().AsChar() );
	}

	return newTexture;
}


static GpuApi::TextureRef LoadFromEngineTexture( const CBitmapTexture* engineTexture, Uint32 residentMip, GpuApi::eTextureGroup textureGroup )
{
	// Get display format
	GpuApi::eTextureFormat displayFormat = engineTexture->GetPlatformSpecificCompression();

	// Get mipmaps
	const CBitmapTexture::MipArray& mips = engineTexture->GetMips();

	// Get the size of the texture to upload
	const Uint32 width = mips[ residentMip ].m_width;
	const Uint32 height = mips[ residentMip ].m_height;
	const Uint16 residentMipCount = static_cast<Uint16>( mips.Size() - residentMip );

	// Declare texture object info
	GpuApi::TextureDesc desc;
	desc.type		= GpuApi::TEXTYPE_2D;
	desc.initLevels	= residentMipCount;
	desc.width		= width;
	desc.height		= height;
	desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.format		= displayFormat;

	TDynArray< GpuApi::TextureLevelInitData > initMipData( residentMipCount );

	for ( Uint16 mip_i = 0; mip_i < residentMipCount; ++mip_i )
	{
		// Get mipmap data
		CBitmapTexture::MipMap& mip = const_cast< CBitmapTexture::MipMap& >( mips[ mip_i + residentMip ] );

		if ( !mip.m_data.Load() )
		{
			RED_HALT( "Failed to load mip %d, %ls", mip_i + residentMip, engineTexture->GetFriendlyName().AsChar() );
			continue;
		}

		const void* data = mip.m_data.GetData();
		if ( data == nullptr )
		{
			RED_HALT( "NULL???" );
			continue;
		}

		initMipData[mip_i].m_isCooked = false;
		initMipData[mip_i].m_data = data;
	}


	GpuApi::TextureInitData initData;
	initData.m_isCooked = false;
	initData.m_mipsInitData = initMipData.TypedData();

	// Create texture
	GpuApi::TextureRef newTexture = GpuApi::CreateTexture( desc, textureGroup, &initData );
	if ( !newTexture )
	{
		WARN_RENDERER( TXT("Failed to create texture %ix%i, %ls"), width, height, engineTexture->GetFriendlyName().AsChar() );
	}

	return newTexture;
}

CRenderTexture* CRenderTexture::Create( const GpuApi::TextureRef &texture, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderTexture > renderTexture ( partialRegistrationHash );

	renderTexture.InitResource( new CRenderTexture( texture ) );

	return renderTexture.RetrieveSuccessfullyCreated();
}

extern Bool GDoNotCreateRenderResources;
CRenderTexture* CRenderTexture::Create( const CBitmapTexture* engineTexture, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderTexture > texture ( partialRegistrationHash );

	RED_WARNING( engineTexture != nullptr, "Can't create a render texture when we don't have an engine texture!" );
	if ( engineTexture == nullptr )
	{
		return nullptr;
	}

	if ( !GpuApi::IsInit() )
	{
		RED_LOG( GpuApi, TXT("Tried to create a render texture before GpuApi has been initialized! [%ls] Returning NULL."), engineTexture->GetFriendlyName().AsChar() );
		return nullptr;
	}

	// Creating of the rendering resources is suppressed
	if ( GDoNotCreateRenderResources )
	{
		return nullptr;
	}


	const CBitmapTexture::MipArray& mips = engineTexture->GetMips();
	RED_WARNING( !mips.Empty(), "Texture '%ls' has no mips, cannot create render resource", engineTexture->GetFriendlyName().AsChar() );
	// There are no mipmaps in the texture
	if ( mips.Empty() )
	{
		return nullptr;
	}

	texture.InitResource( new CRenderTexture( GpuApi::TextureRef::Null() ) );	
	
	GpuApi::TextureRef gpuTexture;

	Uint8 residentMip = engineTexture->GetResidentMipIndex();
	Uint8 maxStreamMip = 0;

	if ( engineTexture->IsCooked() )
	{
#ifndef RED_PLATFORM_CONSOLE
		// Get max streaming mip. Don't change resident mip, since the cooked data contains exactly what we need.
		// Don't need to do this for consoles, since they don't have multiple profile settings.
		CalcMipStreaming( engineTexture->GetTextureGroupName(), mips[0].m_width, mips[0].m_height, mips.Size(), nullptr, &maxStreamMip );
#endif

		const GpuApi::eTextureGroup textureGroup = engineTexture->IsA< CSwfTexture >() ? GpuApi::TEXG_UI : GpuApi::TEXG_Generic;
		gpuTexture = LoadFromCookedEngineTexture( engineTexture, textureGroup );
	}
	else
	{
		// Get texture resident and max stream mip
		maxStreamMip = ( Uint8 ) engineTexture->GetDownscaleBiasForCurrentPlatform();
		CalcMipStreaming( engineTexture->GetTextureGroupName(), mips[0].m_width, mips[0].m_height, mips.Size(), &residentMip, &maxStreamMip );

		const GpuApi::eTextureGroup textureGroup = engineTexture->IsA< CSwfTexture >() ? GpuApi::TEXG_UI : GpuApi::TEXG_Generic;
		gpuTexture = LoadFromEngineTexture( engineTexture, residentMip, textureGroup );
	}

	RED_WARNING( !gpuTexture.isNull(), "Failed to create render resource for texture '%ls'", engineTexture->GetFriendlyName().AsChar() );
	if ( gpuTexture.isNull() )
	{
		return nullptr;
	}

	// Init gpu texture
	texture->InitWithGpuTexture( gpuTexture );

	// Create new texture wrapper
	texture->SetDepotPath( engineTexture->GetDepotPath() );
	texture->SetTextureGroup( engineTexture->GetTextureGroupName() );

	Bool doInitStreaming = true;
	// m_umbraId is calculated based on path in SetDepotPath
	static Uint32 brokenResourcePathHash = GetHash(String(TXT("dlc\\bob\\data\\characters\\models\\common\\textures\\patterns\\pattern_cloth_008.xbm")));
	if ( texture->m_umbraId == brokenResourcePathHash )
	{
		if (engineTexture->GetStreamingSource() && !engineTexture->GetStreamingSource()->IsReady())
		{
			texture->InitStreaming( residentMip, maxStreamMip, nullptr );
			doInitStreaming = false;
		}
	}
	if ( doInitStreaming )
	{
		texture->InitStreaming( residentMip, maxStreamMip, engineTexture->GetStreamingSource() );
	}

	// We are done
	return texture.RetrieveSuccessfullyCreated();
}


CRenderTexture* CRenderTexture::Create( const CTextureCacheQuery& cacheQuery, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderTexture > texture ( partialRegistrationHash );

	if ( !cacheQuery )
	{
		WARN_RENDERER( TXT("Can't create a render texture when we don't have a texture cache entry!") );
		return nullptr;
	}

	if ( !GpuApi::IsInit() )
	{
		RED_LOG( GpuApi, TXT("Tried to create a render texture before GpuApi has been initialized! [%ls] Returning NULL."), cacheQuery.GetPath().AsChar() );
		return nullptr;
	}

	// Creating of the rendering resources is suppressed
	if ( GDoNotCreateRenderResources )
	{
		return nullptr;
	}


	// There are no mipmaps in the texture
	if ( cacheQuery.GetEntry().m_info.m_mipCount == 0 )
	{
		WARN_RENDERER( TXT("Texture '%ls' has no mips, cannot create render resource"), cacheQuery.GetPath().AsChar() );
		return nullptr;
	}

	if ( cacheQuery.GetEntry().m_isCube || cacheQuery.GetEntry().m_info.m_sliceCount > 1 )
	{
		WARN_RENDERER( TXT("Texture from cache has wrong type (either cube or array, but looking for 2d) [%ls] Returning NULL."), cacheQuery.GetPath().AsChar() );
		return nullptr;
	}
	
	// Create new texture wrapper
	texture.InitResource( new CRenderTexture( GpuApi::TextureRef::Null() ) );
	
	GpuApi::TextureRef gpuTexture;

	Uint8 residentMip = 0;
	Uint8 maxStreamMip = 0;

	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_2D;
	desc.width			= cacheQuery.GetEntry().m_info.m_baseWidth;
	desc.height			= cacheQuery.GetEntry().m_info.m_baseHeight;
	desc.initLevels		= cacheQuery.GetEntry().m_info.m_mipCount;
	desc.sliceNum		= cacheQuery.GetEntry().m_info.m_sliceCount;
	desc.usage			= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.format			= ITexture::DecodeTextureFormat( cacheQuery.GetEntry().m_encodedFormat );
	desc.inPlaceType	= GpuApi::INPLACE_Texture;

	Uint32 size = cacheQuery.GetEntry().m_info.m_uncompressedSize;

	Red::MemoryFramework::MemoryRegionHandle data = GpuApi::AllocateInPlaceMemoryRegion( GpuApi::INPLACE_Texture, size, GpuApi::MC_TextureData, cacheQuery.GetEntry().m_info.m_baseAlignment );
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
	data.SetDebugString( cacheQuery.GetPath().AsChar() );
#endif

	// TODO : Could maybe drop mips for lower settings? Not sure how speedtree texture mips would look up-close.

	ETextureCacheLoadResult result = cacheQuery.LoadData( 0, data.GetRawPtr(), size );
	if ( result != TCLR_Success )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, data );
		WARN_RENDERER( TXT("Failed to load texture from cache '%ls'"), cacheQuery.GetPath().AsChar() );
		return nullptr;
	}

	GpuApi::TextureInitData initData;
	initData.m_isCooked = true;
	initData.m_cookedData = data;

	gpuTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );
	GpuApi::UnlockInPlaceMemoryRegion( GpuApi::INPLACE_Texture, data );

	RED_WARNING( !gpuTexture.isNull(), "Failed to create render resource for texture '%ls'", cacheQuery.GetPath().AsChar() );
	if ( gpuTexture.isNull() )
	{
		GpuApi::ReleaseInPlaceMemoryRegion( GpuApi::INPLACE_Texture, data );
		return nullptr;
	}

	texture->InitWithGpuTexture( gpuTexture );
	texture->SetDepotPath( cacheQuery.GetPath() );
	texture->SetTextureGroup( RED_NAME( Default ) );
	texture->InitStreaming( residentMip, maxStreamMip, nullptr );

	return texture.RetrieveSuccessfullyCreated();
}

CRenderTexture* CRenderTexture::Create( IBitmapTextureStreamingSource* streamingSource, const CName& groupName, Uint64 partialRegistrationHash, GpuApi::eInternalTexture fallback /*= GpuApi::INTERTEX_Blank2D*/ )
{
	TScopedRenderResourceCreationObject< CRenderTexture > texture ( partialRegistrationHash );

	RED_ASSERT( streamingSource );
	if ( !GpuApi::IsInit() )
	{
#ifndef RED_FINAL_BUILD
		RED_LOG( GpuApi, TXT("Tried to create a render texture before GpuApi has been initialized! [%ls] Returning NULL."), streamingSource->GetDebugName().AsChar() );
#endif // RED_FINAL_BUILD
		return nullptr;
	}

	// Creating of the rendering resources is suppressed
	if ( GDoNotCreateRenderResources )
	{
		return nullptr;
	}

	// There are no mipmaps in the texture
	if ( streamingSource->GetNumMipmaps() == 0 )
	{
#ifndef RED_FINAL_BUILD
		WARN_RENDERER( TXT("Texture '%ls' has no mips, cannot create render resource"), streamingSource->GetDebugName().AsChar() );
#endif
		return nullptr;
	}

	// Create new texture wrapper
	// NOTE: THIS ASSUMES WE USE THE "Create( IBitmapTextureStreamingSource* streamingSource ..." FUNCTION WITH 2D TEXUTRES ONLY
	GpuApi::TextureRef defaultTex = GpuApi::GetInternalTexture( fallback ); 
	GpuApi::AddRef( defaultTex );
	
	texture.InitResource( new CRenderTexture( defaultTex ) );
	
#ifndef RED_FINAL_BUILD
	texture->SetDepotPath( streamingSource->GetDebugName() ); 
#endif

	texture->SetTextureGroup( groupName );
	texture->InitStreaming( 15/*high but within 6 signed bits*/, 0, streamingSource );

	return texture.RetrieveSuccessfullyCreated();
}

void CRenderTexture::RecalculateMipStreaming()
{
	if( m_streamingSource == nullptr )	// If there is no streaming for this texture, skip it
	{
		return;
	}

	const CName& textureGroupName = m_textureGroupName;
	const Uint32 width = (Uint32)m_streamingSource->GetBaseWidth();
	const Uint32 height = (Uint32)m_streamingSource->GetBaseHeight();
	const Uint32 mipCount = m_streamingSource->GetNumMipmaps();
	Uint8 maxStreamMip = 0;

	CalcMipStreaming( textureGroupName, width, height, mipCount, nullptr, &maxStreamMip );
	InitStreaming( m_residentMipIndex, maxStreamMip, m_streamingSource );
}

IRenderResource* CRenderInterface::UploadTexture( const CBitmapTexture* texture )
{
	RED_WARNING( !IsDeviceLost(), "Unable to create new render resources when device is lost" );

	if ( IsDeviceLost() )
	{
		return nullptr;
	}

	PC_SCOPE( UploadTexture );

	CRenderTexture* renderTexture = nullptr;

	if ( CanUseResourceCache() )
	{		
		const Uint64 hash = CRenderTexture::CalcResourceHash( texture );
		if ( CRenderTexture::ResourceCacheRequestPartialCreate( hash, renderTexture ) )
		{
			renderTexture = CRenderTexture::Create( texture, hash );
		}
	}
	else
	{		
		renderTexture = CRenderTexture::Create( texture, 0 );
	}

	return renderTexture;
}
