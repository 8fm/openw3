/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureStreamRequest.h"
#include "renderTextureBase.h"
#include "renderInterface.h"
#include "renderTextureStreaming.h"
#include "renderMesh.h"


namespace Config
{
	TConfigVar< Float > cvSoftTextureRequestTimeout( "Streaming/Textures", "SoftRequestTimeout", 10.0f );
	TConfigVar< Float > cvMaxTextureRequestTimeout( "Streaming/Textures", "MaxRequestTimeout", 15.0f );
}


CRenderTextureStreamRequest::CRenderTextureStreamRequest( Bool lockTextures )
	: m_isReady( true )
	, m_lockTextures( lockTextures )
	, m_isCancelled( false )
{
}

CRenderTextureStreamRequest::~CRenderTextureStreamRequest()
{
	// Release "Waiting Request" on each texture that's still pending (in case we were canceled)
	for ( auto texture : m_pendingTextures )
	{
		texture->ReleaseWaitingRequest();
	}

	// And release all textures.
	for ( auto texture : m_textures )
	{
		RED_FATAL_ASSERT( texture != nullptr, "Null texture in stream request! How can this be? We already checked it when adding!" );

		if ( m_lockTextures )
		{
			texture->UnlockStreaming();
		}
		texture->Release();
	}

	for ( auto mesh : m_meshes )
	{
		mesh->Release();
	}
}

void CRenderTextureStreamRequest::AddRenderTexture( IRenderResource* texture )
{
	if ( texture != nullptr )
	{
		// Not ready once we actually have textures.
		m_isReady.SetValue( false );

		if ( m_textures.Insert( static_cast< CRenderTextureBase* >( texture ) ) )
		{
			texture->AddRef();
		}
	}
}

void CRenderTextureStreamRequest::AddRenderMesh( IRenderResource* mesh )
{
	if ( mesh != nullptr )
	{
		if ( m_meshes.Insert( static_cast< CRenderMesh* >( mesh ) ) )
		{
			mesh->AddRef();
		}
	}
}


Bool CRenderTextureStreamRequest::IsReady() const
{
	if ( ( m_isReady.GetValue() && !GetRenderer()->IsStreamingMeshes() ) || GetRenderer()->GetTextureStreamingManager()->IsStopped() )
	{
		return true;
	}

	if ( m_timeout.GetTimePeriod() > Config::cvSoftTextureRequestTimeout.Get() )
	{
		ERR_RENDERER( TXT("CRenderTextureStreamRequest seems to be taking too long! Will timeout soon!") );
	}

	if ( m_timeout.GetTimePeriod() > Config::cvMaxTextureRequestTimeout.Get() )
	{
		ERR_RENDERER( TXT("CRenderTextureStreamRequest has timed out, waited for %0.1fs"), m_timeout.GetTimePeriod() );
		m_isReady.SetValue( true );
		return true;
	}

	return false;
}


void CRenderTextureStreamRequest::Start()
{
	// Not a perfect test, but just want something light that should generally catch any problems.
	RED_FATAL_ASSERT( !m_isCancelled.GetValue(), "Cannot start a canceled texture stream request" );

	THashSet< CRenderTextureBase* > locked;
	for ( auto texture : m_textures )
	{
		RED_FATAL_ASSERT( texture != nullptr, "Null texture in stream request! How can this be? We already checked it when adding!" );

		if ( ( m_lockTextures && texture->LockStreaming() ) || ( !m_lockTextures && texture->HasStreamingSource() ) )
		{
			locked.Insert( texture );

			// If the texture isn't already streamed, add it to the pending list.
			if ( !texture->HasHiResLoaded() )
			{
				texture->AddWaitingRequest();
				m_pendingTextures.Insert( texture );
			}
		}
		else
		{
			// Couldn't lock the texture, so we won't be tracking it.
			texture->Release();
		}
	}
	// Swap so we only have the textures we successfully locked.
	m_textures.Swap( locked );

	// If there are no textures for us to wait for, then we don't need to register with the streaming manager. We just
	// mark ourselves done.
	if ( m_pendingTextures.Empty() )
	{
		m_isReady.SetValue( true );
		return;
	}

	GetRenderer()->GetTextureStreamingManager()->RegisterTextureRequest( this );


	m_timeout.ResetTimer();
}

void CRenderTextureStreamRequest::Cancel()
{
	m_isCancelled.SetValue( true );

	// May be called while streaming update is in progress, which could potentially cause an OnTexture* callback below to be
	// called. To prevent that, we first unregister. This way, we know that after that point, we will not get any callback.
	GetRenderer()->GetTextureStreamingManager()->UnregisterTextureRequest( this );

	// Now safe to do things, since we won't get interrupted with a possible OnTexture* callback.

	for ( auto texture : m_pendingTextures )
	{
		RED_FATAL_ASSERT( texture != nullptr, "Null texture in stream request! How can this be? We already checked it when adding!" );
		texture->ReleaseWaitingRequest();
	}
	m_pendingTextures.Clear();

	for ( auto texture : m_textures )
	{
		RED_FATAL_ASSERT( texture != nullptr, "Null texture in stream request! How can this be? We already checked it when adding!" );

		if ( m_lockTextures )
		{
			texture->UnlockStreaming();
		}
		texture->Release();
	}
	m_textures.Clear();

	m_isReady.SetValue( true );
}


Bool CRenderTextureStreamRequest::OnTextureStreamed( CRenderTextureBase* texture )
{
	RED_ASSERT( texture != nullptr, TXT("Shouldn't be getting notification from null texture") );
	if ( texture == nullptr )
	{
		return m_pendingTextures.Empty();
	}

	// Can be called from the streaming update job, or from render thread. But, cannot be called from multiple places at once.
	// So, we don't need any explicit thread safety here.

	// Remove from the texture's we're watching.
	if ( m_pendingTextures.Erase( texture ) )
	{
		texture->ReleaseWaitingRequest();
	}

	// If there are no textures left, we're done.
	if ( m_pendingTextures.Empty() )
	{
		m_isReady.SetValue( true );
		return true;
	}

	return false;
}

Bool CRenderTextureStreamRequest::OnTextureCanceled( CRenderTextureBase* texture )
{
	RED_ASSERT( texture != nullptr, TXT("Shouldn't be getting notification from null texture") );
	if ( texture == nullptr )
	{
		return m_pendingTextures.Empty();
	}

	// Can be called from the streaming update job, or from render thread. But, cannot be called from multiple places at once.
	// So, we don't need any explicit thread safety here.


	// If a locked texture was canceled, then the situation is obviously pretty bad, so we'll just remove it from our list so
	// we don't sit waiting on something that is actually pushing over budget. We maintain a lock on it, to ensure it still has
	// high priority.

	if ( m_pendingTextures.Erase( texture ) )
	{
		texture->ReleaseWaitingRequest();
	}

	// If there are no textures left, we're done.
	if ( m_pendingTextures.Empty() )
	{
		m_isReady.SetValue( true );
		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////


IRenderTextureStreamRequest* CRenderInterface::CreateTextureStreamRequest( Bool lockTextures )
{
	return new CRenderTextureStreamRequest( lockTextures );
}
