/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibStreamingItem.h"

#include "pathlibStreamingManager.h"
#include "baseEngine.h"


namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// IStreamingItem::CStreamingRequest
///////////////////////////////////////////////////////////////////////////////
IStreamingItem::CStreamingRequest::CStreamingRequest( IStreamingItem* item, CStreamingManager& manager )
	: m_item( item )
	, m_manager( &manager )
{
	m_item->AddRef( m_manager );
}

IStreamingItem::CStreamingRequest::CStreamingRequest()
	: m_item( NULL )
	, m_manager( NULL )
{

}
IStreamingItem::CStreamingRequest::CStreamingRequest( const CStreamingRequest& req )
	: m_item( req.m_item )
	, m_manager( req.m_manager )
{
	if ( m_item )
	{
		m_item->AddRef( m_manager );
	}
}
IStreamingItem::CStreamingRequest::CStreamingRequest( CStreamingRequest&& req )
	: m_item( req.m_item )
	, m_manager( req.m_manager )
{
	req.m_item = NULL;
	req.m_manager = NULL;
}

IStreamingItem::CStreamingRequest::~CStreamingRequest()
{
	if ( m_item )
	{
		m_item->DelRef( m_manager );
	}
}

void IStreamingItem::CStreamingRequest::operator=( const CStreamingRequest& req )
{
	if ( m_item != req.m_item )
	{
		Release();

		m_item = req.m_item;
		m_manager = req.m_manager;

		if ( m_item )
		{
			m_item->AddRef( m_manager );
		}
	}
}
void IStreamingItem::CStreamingRequest::operator=( CStreamingRequest&& req )
{
	Release();

	m_item = req.m_item;
	m_manager = req.m_manager;

	req.m_item = NULL;
	req.m_manager = NULL;
}

void IStreamingItem::CStreamingRequest::Release()
{
	if ( m_item )
	{
		m_item->DelRef( m_manager );
		m_item = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// IStreamingItem
///////////////////////////////////////////////////////////////////////////////
IStreamingItem::IStreamingItem()
	: m_streamingRequestsCount( 0 )
	, m_streamingState( STREAMING_STATE_UNLOADED )
	, m_unstreamRequestTime( EngineTime::ZERO )
	, m_isBeingLoadedNow( false )
{

}

IStreamingItem::~IStreamingItem()
{
	//PATHLIB_ASSERT( m_streamingRequestsCount == 0 );
}
void IStreamingItem::RequestLoad( CStreamingManager* manager )
{
	switch( m_streamingState )
	{
	default:
		ASSUME( false );
	case STREAMING_STATE_PENDING_TO_UNLOAD:
		SetStreamingState( STREAMING_STATE_LOADED );
		manager->GetLoadedItems().ListInsert( *this );
		break;
	case STREAMING_STATE_UNLOADED:
		manager->AddPendingToLoadItem( *this );
		//manager->GetPendingToLoadItems().ListInsert( *this );
		SetStreamingState( STREAMING_STATE_PENDING_TO_LOAD );
		break;
	case STREAMING_STATE_PENDING_TO_LOAD:
	case STREAMING_STATE_LOADING:
	case STREAMING_STATE_LOADED:
	case STREAMING_STATE_UNLOADING:
		break;
	}
}
void IStreamingItem::RequestUnload( CStreamingManager* manager )
{
	switch( m_streamingState )
	{
	default:
		ASSUME( false );
	case STREAMING_STATE_PENDING_TO_LOAD:
		manager->GetUnloadedItems().ListInsert( *this );
		SetStreamingState( STREAMING_STATE_UNLOADED );
		break;
	case STREAMING_STATE_LOADED:
		m_unstreamRequestTime = GEngine->GetRawEngineTime() + 11.666f;
		manager->AddPendingToUnloadItem( *this );
		//manager->GetPendingToUnloadItems().ListInsert( *this );
		SetStreamingState( STREAMING_STATE_PENDING_TO_UNLOAD );
		break;
	case STREAMING_STATE_LOADING:
	case STREAMING_STATE_UNLOADED:
	case STREAMING_STATE_UNLOADING:
	case STREAMING_STATE_PENDING_TO_UNLOAD:
		break;
	}
}
void IStreamingItem::AddRef( CStreamingManager* manager )
{
	//PATHLIB_ASSERT( SIsMainThread() );
	if ( ++m_streamingRequestsCount == 1 )
	{
		RequestLoad( manager );
	}
}
void IStreamingItem::DelRef( CStreamingManager* manager )
{
	//PATHLIB_ASSERT( SIsMainThread() );
	if ( --m_streamingRequestsCount == 0 )
	{
		RequestUnload( manager );
	}
}
void IStreamingItem::CheckStreamingState( CStreamingManager* manager )
{
	if ( m_streamingRequestsCount > 0 )
	{
		RequestLoad( manager );
	}
	else
	{
		RequestUnload( manager );
	}
}

void IStreamingItem::NeverUnstream( CStreamingManager* manager )
{
	// Artificial
	AddRef( manager );
}

void IStreamingItem::PreLoad()
{
	PATHLIB_ASSERT( SIsMainThread() && ( m_streamingState == STREAMING_STATE_UNLOADED || m_streamingState == STREAMING_STATE_PENDING_TO_LOAD ) );
	SetStreamingState( STREAMING_STATE_LOADING );
	m_isBeingLoadedNow.SetValue( true );
}
void IStreamingItem::Load()
{
	// asynchronous loading
	PATHLIB_ASSERT( m_streamingState == STREAMING_STATE_LOADING );
}

void IStreamingItem::PostLoad()
{
	m_isBeingLoadedNow.SetValue( false );
	// asynchronous post-loading processing
	PATHLIB_ASSERT( m_streamingState == STREAMING_STATE_LOADING );
}

void IStreamingItem::PostLoadInterconnection()
{
	// asynchronous post-post-loading
	PATHLIB_ASSERT( m_streamingState == STREAMING_STATE_LOADING );
}

void IStreamingItem::Attach( CStreamingManager* manager )
{
	PATHLIB_ASSERT( SIsMainThread() && m_streamingState == STREAMING_STATE_LOADING );
	SetStreamingState( STREAMING_STATE_LOADED );
	CheckStreamingState( manager );
}
void IStreamingItem::PreUnload()
{
	PATHLIB_ASSERT( SIsMainThread() && ( m_streamingState ==  STREAMING_STATE_LOADED || m_streamingState == STREAMING_STATE_PENDING_TO_UNLOAD ) );

	SetStreamingState( STREAMING_STATE_UNLOADING );
}
void IStreamingItem::Unload()
{
	ASSERT( m_streamingState == STREAMING_STATE_UNLOADING );
}
void IStreamingItem::Detach( CStreamingManager* manager )
{
	PATHLIB_ASSERT( SIsMainThread() && ( m_streamingState == STREAMING_STATE_UNLOADING ) );
	SetStreamingState( STREAMING_STATE_UNLOADED );
	CheckStreamingState( manager );
}


};				// namespace PathLib
