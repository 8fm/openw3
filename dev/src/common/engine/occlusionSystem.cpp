/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "occlusionSystem.h"
#include "renderCommands.h"

#ifdef USE_UMBRA

IMPLEMENT_ENGINE_CLASS( COcclusionQueryPtr );

///////////////////////////////////////////////////////////////////////////////
// COcclusionSystem
///////////////////////////////////////////////////////////////////////////////

COcclusionQueryPtr::COcclusionQueryPtr( UmbraQueryPtr&& query )
	: m_query( Move( query ) )
{
}

COcclusionQueryPtr::COcclusionQueryPtr( COcclusionQueryPtr&& ptr )
	: m_query( Move( ptr.m_query ) )
{
}

COcclusionQueryPtr::COcclusionQueryPtr( const COcclusionQueryPtr& ptr )
	: m_query( ptr.m_query )
{
}

//////////////////////////////////////////////////////////////////////////

COcclusionSystem::COcclusionSystem()
{

}

COcclusionQueryPtr COcclusionSystem::AddQuery( const Box& bbox )
{
	return COcclusionQueryPtr( m_pendingQueue.AddQuery( bbox ) );
}

void COcclusionSystem::Init()
{
	UmbraQuery::InitializeQueriesPool( 1024 );

}

void COcclusionSystem::Deinit()
{
	UmbraQuery::ShutdownQueriesPool();

}

void COcclusionSystem::Tick()
{
	if ( !m_pendingQueue.IsEmpty() )
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( world )
		{
			// start job or render command here
			( new CRenderCommand_PerformVisibilityQueries( world->GetRenderSceneEx(), Move( m_pendingQueue ) ) )->Commit();
		}
		
	}
}

#endif

