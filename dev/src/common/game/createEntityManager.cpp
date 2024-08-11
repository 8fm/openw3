#include "build.h"
#include "createEntityManager.h"
#include "createEntityHelper.h"
#include "../game/commonGame.h"
#include "../engine/dynamicLayer.h"
#include "../engine/jobSpawnEntity.h"

//////////////////////////////////////////////////////////////
// CCreateEntityManager
CCreateEntityManager::CCreateEntityManager()
{
}

CCreateEntityManager::~CCreateEntityManager()
{
	ASSERT( m_createEntityHelperList.Empty() );
}

void CCreateEntityManager::AddProcessingItem( CCreateEntityHelper *const createEntityHelper )
{
	m_createEntityHelperList.PushBack( createEntityHelper );
	createEntityHelper->BeginProcessing();
}

Bool CCreateEntityManager::CreateEntityAsync( CCreateEntityHelper *const createEntityHelper, EntitySpawnInfo && entitySpawnInfo )
{
	if ( createEntityHelper->StartSpawnJob( Move( entitySpawnInfo ) ) )
	{
		AddProcessingItem( createEntityHelper );
		return true;
	}
	return false;
}

void CCreateEntityManager::OnWorldEnd()
{
	for ( const THandle< CCreateEntityHelper >& helper : m_createEntityHelperList )
	{
		helper->Discard( this );
	}
	m_createEntityHelperList.ClearFast();
}

void CCreateEntityManager::Update()
{
	for ( Int32 i = m_createEntityHelperList.Size() - 1; i >= 0; --i )
	{
		const THandle< CCreateEntityHelper > & createEntityHelperHandle	= m_createEntityHelperList[ i ];
		CCreateEntityHelper *const createEntityHelper					= createEntityHelperHandle.Get();
		if ( createEntityHelper == nullptr )
		{
			ASSERT( false, TXT( "createEntityHelper is null this shouldn't happen!" ) );
			m_createEntityHelperList.RemoveAt( i );
			continue;
		}
		if ( createEntityHelper->Update( this ) )
		{
			createEntityHelper->Discard( this );
			// Garbage collection magic will take care of clearing pointer
			m_createEntityHelperList.RemoveAtFast( i );
		}
	}
}
