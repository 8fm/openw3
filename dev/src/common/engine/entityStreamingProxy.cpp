/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entity.h"
#include "entityStreamingProxy.h"
#include "../core/configVar.h"

namespace Config
{
	TConfigVar< Bool >		cvEntityStreamingAsyncPrefetch( "Streaming/Entity", "StreamingAsyncPrefetch", true );
}

//--------------------

CEntityStreamingData::CEntityStreamingData()
{
}

//--------------------

CEntityStreamingJob::CEntityStreamingJob( CEntity* entity, EJobPriority priority /* = JP_StreamingObject */ )
	: ILoadJob( priority, true )
	, m_entity( entity )
{
}

CEntityStreamingJob::~CEntityStreamingJob()
{
}

EJobResult CEntityStreamingJob::Process()
{
	// entity got deleted but the streaming job was not canceled
	if ( !m_entity )
		return JR_Failed;

	m_entity->PrecacheStreamedComponents( m_data );
	return JR_Finished;
}

//--------------------

CEntityStreamingProxy::CEntityStreamingProxy( CEntity* entity, const Uint32 gridHash, const Uint32 id )
	: m_entity( entity )
	, m_gridHash( gridHash )
	, m_id( id )
	, m_job( nullptr )
{
}

CEntityStreamingProxy::~CEntityStreamingProxy()
{
	if ( m_job )
	{
		PC_SCOPE( EntityStreamingProxyJobSync );

		// cancel the resource prefetch job
		m_job->Cancel();
		m_job->Release();
		m_job = nullptr;
	}
}

Bool CEntityStreamingProxy::StartStreaming()
{
	RED_ASSERT( m_entity.IsValid(), TXT("Invalid entity in streaming proxy - proxy was not unregistered after entity was destroyed ?") );
	RED_ASSERT( m_job == nullptr, TXT("Called StartStreaming twice") );

	if ( m_entity )
	{
		if ( Config::cvEntityStreamingAsyncPrefetch.Get() )
		{
			// create asynchronous streaming job that will create entity components
			m_job = new CEntityStreamingJob( m_entity, m_entity->GetStreamingPriority() );
			SJobManager::GetInstance().Issue( m_job );
		}
		else
		{
			// create the components synchronously
			m_entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			return (m_entity->IsStreamedIn() == true);
		}
	}

	return true;
}

Bool CEntityStreamingProxy::UpdateStreaming( Bool wait )
{
	if ( m_job )
	{
		RED_ASSERT( m_entity->IsStreamedIn() != true, TXT("Entity got streamed in before the job succeeded") );

		// wait for the job
		if ( wait )
		{
			PC_SCOPE( EntityStreamingProxyJobSync );

			while ( !m_job->HasEnded() )
			{
				Red::Threads::YieldCurrentThread();
			}
		}

		// job not yet ready
		if ( !m_job->HasEnded() )
			return false;

		// finalize components
		CEntityStreamingData& data = m_job->GetData();
		m_entity->CreateStreamedComponents( SWN_DoNotNotifyWorld, true, &data );

		// release the job (it's finished)
		m_job->Release();
		m_job = nullptr;

		// should be valid
		RED_ASSERT( m_entity->IsStreamedIn() == true, TXT("Failed to stream in components that were loaded asynchronously") );
		return true;
	}

	return true;
}

Bool CEntityStreamingProxy::Unstream()
{
	RED_ASSERT( m_entity.IsValid(), TXT("Invalid entity in streaming proxy - proxy was not unregistered after entity was destroyed ?") );

#ifndef NO_EDITOR
	// Check if the entity can be unstreamed
	if ( m_entity.IsValid() && !m_entity.Get()->CanUnstream() )
	{
		return false;
	}
#endif

	// cancel the streaming job
	if ( m_job )
	{
		m_job->Cancel();
		m_job->Release();
		m_job = nullptr;
	}

	// destroy already created components
	if ( m_entity )
	{
		m_entity->DestroyStreamedComponents( SWN_DoNotNotifyWorld );
		return (m_entity->IsStreamedIn() == false);
	}

	return true;
}

//--------------------
