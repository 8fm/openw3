/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureBase.h"
#include "renderHelpers.h"
#include "../engine/renderSettings.h"
#include "../engine/texture.h"
#include "renderTextureStreaming.h"

IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( CRenderTextureBase );

Bool GDoNotCreateRenderResources = false;

void CRenderTextureBase::CalcMipStreaming( const CName textureGroupName, Uint32 width, Uint32 height, Uint32 mipCount, Uint8* inoutResidentMip, Uint8* inoutMaxLoadMip )
{
	const TextureGroup* textureGroup = SRenderSettingsManager::GetInstance().GetTextureGroup( textureGroupName );
	if ( textureGroup == nullptr )
	{
		WARN_RENDERER( TXT("Unknown texture group: %s"), textureGroupName.AsChar() );
		return;
	}

	// We cannot resize this texture or has no mipmaps, always use what the texture has to begin with.
	const Bool mipsAllowed = textureGroup->m_hasMipchain;
	const Bool canResize = textureGroup->m_isResizable;
	if ( !canResize || mipCount == 1 || !mipsAllowed )
	{
		return;
	}

	// Shrink texture by the specified ratio
	Uint32 mipShift = Config::cvTextureDownscale.Get();
	Uint32 maxSize = Config::cvMaxTextureSize.Get();
	if ( textureGroup->m_isDetailMap )
	{
		// This is a detail texture so use another scale
		mipShift = Config::cvDetailTextureDownscale.Get();
	}
	else if ( textureGroup->m_isAtlas )
	{
		// This is an atlas texture so use another scale
		mipShift = Config::cvAtlasTextureDownscale.Get();
		maxSize  = Config::cvMaxAtlasTextureSize.Get();
	}

	// Shrink even more if still larger than allowed
	Uint32 shrinkWidth = width >> mipShift;
	Uint32 shrinkHeight = height >> mipShift;
	while ( shrinkWidth > maxSize || shrinkHeight > maxSize )
	{
		shrinkWidth /= 2;
		shrinkHeight /= 2;
		++mipShift;
	}

	// But do not let textures to be too small. We want to be able to fill the resident mip size.
	const Uint32 maxNumResident = Config::cvMaxResidentMipMap.Get();
	const Uint32 maxMipShift = Max( maxNumResident, mipCount ) - maxNumResident;

	mipShift = Min( mipShift, maxMipShift );

	if ( inoutResidentMip != nullptr )
	{
		// Calculate mip index to load when streaming
		if ( textureGroup->m_isStreamable )
		{
			// We can stream this texture so keep only the resident part in the memory
			*inoutResidentMip = (Uint8)maxMipShift;
		}
		else
		{
			// We cannot stream this texture, so load it whole at once
			*inoutResidentMip = (Uint8)mipShift;
		}
	}

	if ( inoutMaxLoadMip != nullptr )
	{
		*inoutMaxLoadMip = (Uint8)mipShift;
	}
}

CRenderTextureBase::CRenderTextureBase( const GpuApi::TextureRef &texture )
	: m_samplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapAnisoMip )
	, m_streamingSource( nullptr )
	, m_streamingTask( nullptr )
	, m_streamingMipIndex( -1 )
	, m_residentMipIndex( -1 )
	, m_pendingMipIndex( -1 )
	, m_texture( texture )
	, m_lastBindDistance( FLT_MAX )
	, m_textureCategory( eTextureCategory_Generic )
	, m_streamingKey( 0 )
	, m_streamingLocks( 0 )
	, m_waitingRequests( 0 )
#ifndef RED_FINAL_BUILD
	, m_texUsedMemory( 0 )
	, m_hiResTexUsedMemory( 0 )
#endif // RED_FINAL_BUILD	
{	
	m_approxSize = texture ? GpuApi::CalcTextureSize( m_texture ) : 0; // Cache memory size of the texture, this is used in the texture streaming
}

CRenderTextureBase::~CRenderTextureBase()
{
	// Release textures
	GpuApi::SafeRelease( m_texture );
	GpuApi::SafeRelease( m_hiResTexture );

	CancelStreaming();

	// Release streaming source if was used
	SAFE_RELEASE( m_streamingSource );
}

void CRenderTextureBase::InitWithGpuTexture( const GpuApi::TextureRef& texture )
{
	RED_FATAL_ASSERT( !m_texture, "Already init?" );
	m_texture = texture;
	m_approxSize = texture ? GpuApi::CalcTextureSize( texture ) : 0; // Cache memory size of the texture, this is used in the texture streaming
}

void CRenderTextureBase::SetSamplerStatePreset( GpuApi::eSamplerStatePreset preset )
{
	m_samplerStatePreset = preset;
}

void CRenderTextureBase::SetTextureGroup( const CName& group )
{
	// Texture group name
	m_textureGroupName = group;

	// Texture content category
	const TextureGroup* textureGroup = SRenderSettingsManager::GetInstance().GetTextureGroup( m_textureGroupName );
	if ( textureGroup != nullptr )
	{
		m_textureCategory = textureGroup->m_category;
	}

	// Sample settings, most textures use default one
	GpuApi::eSamplerStatePreset sampleState = GpuApi::SAMPSTATEPRESET_WrapAnisoMip;
	if ( group == RED_NAME( TerrainSpecial ) || group == RED_NAME( TerrainColor ) )
	{
		sampleState = GpuApi::SAMPSTATEPRESET_ClampLinearNoMip;
	}
	else if ( group == RED_NAME( TerrainMaskMap ) )
	{
		sampleState = GpuApi::SAMPSTATEPRESET_ClampPointNoMip;
	}
	else if ( group == RED_NAME( TerrainDiffuseAtlas ) || group == RED_NAME( TerrainNormalAtlas ) )
	{
		sampleState = GpuApi::SAMPSTATEPRESET_AtlasLinearMip;
	}
	else if ( group == RED_NAME( PostFxMap ) )
	{
		sampleState = GpuApi::SAMPSTATEPRESET_ClampPointNoMip;
	}

	// Set sampler state
	SetSamplerStatePreset( sampleState );
}

void CRenderTextureBase::SetDepotPath( const String& path )
{
	if ( m_texture )
	{
		GpuApi::SetTextureDebugPath( m_texture, UNICODE_TO_ANSI( path.AsChar() ) );
	}
	m_umbraId = GetHash( path );

#ifndef RED_FINAL_BUILD
	m_debugDepotPath = path;
#endif
}

Uint32 CRenderTextureBase::GetUsedVideoMemory() const
{
	Uint32 size = GpuApi::CalcTextureSize( m_texture );
	size += GpuApi::CalcTextureSize( m_hiResTexture );
	return size;
}

#ifndef RED_FINAL_BUILD
Uint32 CRenderTextureBase::GetCachedUsedVideoMemory()
{
	if ( m_texture )
	{
		if ( m_texUsedMemory == 0 )
		{
			m_texUsedMemory = GpuApi::CalcTextureSize( m_texture );
		}
	}
	else
	{
		m_texUsedMemory = 0;
	}

	if ( m_hiResTexture )
	{
		if ( m_hiResTexUsedMemory == 0 )
		{
			m_hiResTexUsedMemory = GpuApi::CalcTextureSize( m_hiResTexture );
		}
	}
	else
	{
		m_hiResTexUsedMemory = 0;
	}

	return m_texUsedMemory + m_hiResTexUsedMemory;
}
#endif

void CRenderTextureBase::UpdateStreamingExpiration()
{
	const Float maxStreamingDistance = GetRenderer()->GetTextureStreamingManager()->GetMaxStreamingDistance( m_textureCategory );

	// Even if a texture was bound at distance 0, we want it to be possible to eventually be unstreamed. It should be re-bound to
	// keep it alive, anyways.
	m_lastBindDistance += Max< Float >( m_lastBindDistance, 0.01f ) / 100.f;

	if ( HasStreamingLock() )
	{
		// When a texture is locked, we limit the distance to just inside the max streaming distance, so it keeps higher priority.
		m_lastBindDistance = Min( m_lastBindDistance, maxStreamingDistance * 0.95f );
	}
	else
	{
		if ( m_lastBindDistance > maxStreamingDistance )
		{
			m_lastBindDistance = FLT_MAX;
		}
	}
}

void CRenderTextureBase::BindNoSampler( Uint32 textureIndex, ERenderShaderType shaderStage /*= RST_PixelShader */, Float distance /* = 0.f */ )
{
	// Streaming switch
	if ( m_hiResTexture )
	{
		// Use hires texture
		GpuApi::BindTextures( textureIndex, 1, &m_hiResTexture, Map( shaderStage ) );
	}
	else if ( m_texture )
	{
		// Use resident texture
		GpuApi::BindTextures( textureIndex, 1, &m_texture, Map( shaderStage ) );
	}

	// Update render distance
	UpdateLastBindDistance( distance );
}

void CRenderTextureBase::BindNoSamplerFast( Uint32 textureIndex, ERenderShaderType shaderStage /*= RST_PixelShader */, Float distance /* = 0.f */ )
{
	// Streaming switch
	if ( m_hiResTexture )
	{
		// Use hires texture
		GpuApi::BindTexturesFast( textureIndex, 1, &m_hiResTexture, Map( shaderStage ) );
	}
	else if ( m_texture )
	{
		// Use resident texture
		GpuApi::BindTexturesFast( textureIndex, 1, &m_texture, Map( shaderStage ) );
	}

	// Update render distance
	UpdateLastBindDistance( distance );
}

void CRenderTextureBase::Bind( Uint32 samplerIndex, ERenderShaderType shaderStage, Float distance /* = 0.f */ )
{
	Bind( samplerIndex, m_samplerStatePreset, shaderStage, distance );
}

void CRenderTextureBase::Bind( Uint32 samplerIndex, GpuApi::eSamplerStatePreset forcedSamplerStatePreset, ERenderShaderType shaderStage, Float distance /* = 0.f */ )
{
	// Assert that sampler state is valid
	RED_ASSERT( forcedSamplerStatePreset < GpuApi::SAMPSTATEPRESET_Max );

	if ( m_hiResTexture || m_texture )
	{
		// Set sampler states
		GpuApi::SetSamplerStatePreset( samplerIndex, forcedSamplerStatePreset, Map( shaderStage ) );
	}

	BindNoSampler( samplerIndex, shaderStage, distance );
}


void CRenderTextureBase::OnDeviceLost()
{
	// Release textures
	GpuApi::SafeRelease( m_texture );
	GpuApi::SafeRelease( m_hiResTexture );

	// Cancel streaming
	CancelStreaming();

	// Release streaming source if was used
	SAFE_RELEASE( m_streamingSource );
}

void CRenderTextureBase::OnDeviceReset()
{
	// Nothing, need to recreate stuff on engine side
}


void CRenderTextureBase::UpdateApproximateSize()
{
	if ( m_streamingSource && m_streamingSource->IsReady() )
	{
		// Calculate new approximate size, based on the fully streamed texture.
		GpuApi::TextureDesc fullDesc = GpuApi::GetTextureDesc( m_texture );

		if ( m_streamingSource->GetEncodedFormat() != 0 )
		{
			// Overwrite the format if we know it
			fullDesc.format	= ITexture::DecodeTextureFormat( m_streamingSource->GetEncodedFormat() );
		}
		fullDesc.initLevels	= m_streamingSource->GetNumMipmaps();
		fullDesc.width		= m_streamingSource->GetBaseWidth();
		fullDesc.height		= m_streamingSource->GetBaseHeight();

		m_approxSize		= GpuApi::CalcTextureSize( fullDesc );
	}
}


Uint8 CRenderTextureBase::GetMaxMipCount() const
{
	// If we can be streamed, get the number of mips from there. Take into account maxStreamingMipIndex, in case we've got a
	// reduced texture quality setting.
	if ( m_streamingSource && m_streamingSource->IsReady() )
	{
		return m_streamingSource->GetNumMipmaps() - m_maxStreamingMipIndex;
	}

	// If we have a resident texture, give the number of mips there.
	if ( m_texture )
	{
		return (Uint8)GpuApi::GetTextureDesc( m_texture ).initLevels;
	}

	return 0;
}


Uint16 CRenderTextureBase::GetMaxWidth() const
{
	// If we can be streamed, get the number of mips from there. Take into account maxStreamingMipIndex, in case we've got a
	// reduced texture quality setting.
	if ( m_streamingSource && m_streamingSource->IsReady() )
	{
		return m_streamingSource->GetBaseWidth() >> m_maxStreamingMipIndex;
	}

	// If we have a resident texture, give the number of mips there.
	if ( m_texture )
	{
		return GpuApi::GetTextureDesc( m_texture ).width;
	}

	return 0;
}

Uint16 CRenderTextureBase::GetMaxHeight() const
{
	// If we can be streamed, get the number of mips from there. Take into account maxStreamingMipIndex, in case we've got a
	// reduced texture quality setting.
	if ( m_streamingSource && m_streamingSource->IsReady() )
	{
		return m_streamingSource->GetBaseHeight() >> m_maxStreamingMipIndex;
	}

	// If we have a resident texture, give the number of mips there.
	if ( m_texture )
	{
		return GpuApi::GetTextureDesc( m_texture ).height;
	}

	return 0;
}



void CRenderTextureBase::InitStreaming( Uint8 residentMip, Uint8 maxStreamMip, IBitmapTextureStreamingSource* streamingSource )
{
	// Install the streaming source
	if ( maxStreamMip < residentMip )
	{
		SAFE_COPY( m_streamingSource, streamingSource );
	}

	m_residentMipIndex		= (Int8)residentMip;
	m_streamingMipIndex		= (Int8)residentMip;
	m_maxStreamingMipIndex	= (Int8)maxStreamMip;

	if ( m_streamingSource != nullptr && !m_streamingSource->IsReady() )
	{
		m_streamingSource->TryToMakeReady();
		if ( !m_streamingSource->IsReady() )
		{
#ifndef RED_FINAL_BUILD
			WARN_RENDERER( TXT("Streaming source '%ls' is not ready when creating render texture. Has correct texture cache been loaded already?"), m_streamingSource->GetDebugName().AsChar() );
#endif
		}
	}

	UpdateApproximateSize();


	if ( m_streamingSource != nullptr && m_streamingSource->IsReady() )
	{
		if ( m_streamingKey != 0 )
		{
			GetRenderer()->GetTextureStreamingManager()->UpdateTexture( m_streamingKey );
		}
		else
		{
			m_streamingKey = GetRenderer()->GetTextureStreamingManager()->RegisterTexture( this );
		}
	}
}


void CRenderTextureBase::OnNewTextureCache()
{
	if ( m_streamingSource != nullptr && !m_streamingSource->IsReady() )
	{
		m_streamingSource->TryToMakeReady();
		if ( m_streamingSource->IsReady() )
		{
			UpdateApproximateSize();

			if ( m_streamingKey != 0 )
			{
				GetRenderer()->GetTextureStreamingManager()->UpdateTexture( m_streamingKey );
			}
			else
			{
				m_streamingKey = GetRenderer()->GetTextureStreamingManager()->RegisterTexture( this );
			}
		}
	}
}


#ifndef NO_DEBUG_WINDOWS
Int8 CRenderInterface::GetTextureStreamedMipIndex( const IRenderResource* resource )
{
	RED_ASSERT( resource != nullptr, TXT("Trying to get streamed mip index from null resource!") );
	if ( resource == nullptr )
	{
		return -1;
	}
	RED_ASSERT( resource->IsRenderTexture(), TXT("Trying to get streamed mip index from a render resource that isn't a texture!") );
	if ( !resource->IsRenderTexture() )
	{
		return -1;
	}

	const CRenderTextureBase* texture = static_cast< const CRenderTextureBase* >( resource );
	return texture->GetStreamingMipIndex();
}
#endif
