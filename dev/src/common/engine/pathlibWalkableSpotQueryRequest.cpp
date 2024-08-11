/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibWalkableSpotQueryRequest.h"

#include "pathlibWalkableSpotQueryTask.h"
#include "pathlibWorld.h"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// IWalkableSpotQueryCallback
///////////////////////////////////////////////////////////////////////////////
IWalkableSpotQueryCallback::IWalkableSpotQueryCallback()
	: m_refCount( 0 )
{

}
IWalkableSpotQueryCallback::~IWalkableSpotQueryCallback()
{

}

void IWalkableSpotQueryCallback::AddNextCallback( Ptr callback )
{
	m_nextCallback = callback;
}

void IWalkableSpotQueryCallback::OnQueryCompleted( CWalkableSpotQueryRequest* request )
{
	Callback( request );

	if ( m_nextCallback )
	{
		m_nextCallback->OnQueryCompleted( request );
	}
}

///////////////////////////////////////////////////////////////////////////////
// IWalkableSpotQueryRequest
///////////////////////////////////////////////////////////////////////////////
CWalkableSpotQueryRequest::CWalkableSpotQueryRequest()
	: m_refCount( 0 )
	, m_queryState( STATE_DISPOSED )
{}


void CWalkableSpotQueryRequest::Setup( CPathLibWorld& pathlib, const Box& testBox, const Vector3& destinationPos, const Vector3& sourcePos, Float personalSpace, Float maxDist, Flags flags, CollisionFlags collisionFlags, NodeFlags forbiddenNodeFlags, Uint32 taskPriority )
{
	SetQueryState( STATE_SETUP );

	m_callback.Clear();
	m_testBox = testBox;
	m_destinationPos = destinationPos;
	m_sourcePos = sourcePos;
	m_personalSpace = personalSpace;
	m_maxDist = maxDist;
	m_category = pathlib.GetGlobalSettings().ComputePSCategory( personalSpace );
	m_taskPriority = taskPriority;
	m_collisionFlags = collisionFlags;
	m_forbiddenNodeFlags = forbiddenNodeFlags;
	m_flags = flags;

}

void CWalkableSpotQueryRequest::Submit( CPathLibWorld& pathlib )
{
	ASSERT( GetQueryState() == STATE_SETUP || IsQueryCompleted() );
	SetQueryState( STATE_ONGOING );

	CTaskManager* taskManager = pathlib.GetTaskManager();
	CWalkableSpotQueryTask* task = new CWalkableSpotQueryTask( *taskManager, this );
	taskManager->AddTask( task );
	task->Release();
}
void CWalkableSpotQueryRequest::AddCallback( IWalkableSpotQueryCallback::Ptr callback )
{
	IWalkableSpotQueryCallback::Ptr prevCallback = m_callback;
	m_callback = callback;
	if ( prevCallback )
	{
		callback->AddNextCallback( prevCallback );
	}
	m_flags |= FLAG_SYNCHRONOUS_COMPLETION_CALLBACK;
}

Bool CWalkableSpotQueryRequest::AcceptPosition( const Vector3& nodePos )
{
	return true;
}

void CWalkableSpotQueryRequest::CompletionCallback()
{
	if ( m_callback )
	{
		m_callback->OnQueryCompleted( this );
	}
}

};			// namespace PathLib
