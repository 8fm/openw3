/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "communityArea.h"
#include "communitySystem.h"
#include "../engine/triggerAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( CCommunityArea );
IMPLEMENT_ENGINE_CLASS( CCommunityAreaTypeDefault );
IMPLEMENT_ENGINE_CLASS( CCommunityAreaTypeSpawnRadius );
IMPLEMENT_ENGINE_CLASS( CCommunityAreaType );

CCommunityArea::CCommunityArea()
	: m_triggerArea( NULL )
	, m_communityAreaType( NULL )
{

}

void CCommunityArea::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	m_triggerArea = FindComponent< CTriggerAreaComponent >();
}

void CCommunityArea::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( !activator->GetEntity()->IsA< CPlayer >() ) return;

	CCommunitySystem *communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem )
	{
		if ( m_communityAreaType )
		{
			m_communityAreaType->OnEnter( m_triggerArea );
		}
	}
}

void CCommunityArea::OnAreaExit( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( !activator->GetEntity()->IsA< CPlayer >() ) return;

	CCommunitySystem *communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem )
	{
		if ( m_communityAreaType )
		{
			m_communityAreaType->OnExit();
		}
	}
}

void CCommunityArea::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	CCommunitySystem *communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem && m_communityAreaType )
	{
		CAreaComponent *visibilityArea = communitySystem->GetVisibilityArea();
		if ( visibilityArea && visibilityArea == m_triggerArea )
		{
			m_communityAreaType->OnExit();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CCommunityAreaTypeDefault::OnEnter( CTriggerAreaComponent *triggerArea )
{
	CCommunitySystem *communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	communitySystem->SetVisibilityArea( triggerArea );
	
	communitySystem->SetVisibilityAreaSpawnRadius( m_areaSpawnRadius );
	communitySystem->SetVisibilityAreaDespawnRadius( m_areaDespawnRadius );

	if ( m_spawnRadius > 0.0f && m_despawnRadius > 0.0f )
	{
		communitySystem->GetPrevVisibilityRadius( m_prevSpawnRadius, m_prevDespawnRadius );
		communitySystem->SetVisibilityRadius( m_spawnRadius, m_despawnRadius );
	}
}

void CCommunityAreaTypeDefault::OnExit()
{
	CCommunitySystem *communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	communitySystem->ResetVisibilityArea();


	if ( !m_dontRestore )
	{
		communitySystem->SetVisibilityRadius( m_prevSpawnRadius, m_prevDespawnRadius );
	}

	communitySystem->UpdatePrevVisibilityRadius();
}

void CCommunityAreaTypeDefault::GetAreaData( Float &areaSpawnRadius, Float &areaDespawnRadius, Float &spawnRadius, Float &despawnRadius, Bool &dontRestore, Float &prevSpawnRadius, Float &prevDespawnRadius )
{
	areaSpawnRadius = m_areaSpawnRadius;
	areaDespawnRadius = m_areaDespawnRadius;
	spawnRadius = m_spawnRadius;
	despawnRadius = m_despawnRadius;
	dontRestore = m_dontRestore;

	prevSpawnRadius = m_prevSpawnRadius;
	prevDespawnRadius = m_prevDespawnRadius;
}

//////////////////////////////////////////////////////////////////////////

void CCommunityAreaTypeSpawnRadius::OnEnter( CTriggerAreaComponent *triggerArea )
{
	CCommunitySystem *comm = GCommonGame->GetSystem< CCommunitySystem >();
	ASSERT( comm );
	
	comm->GetPrevVisibilityRadius( m_prevSpawnRadius, m_prevDespawnRadius );
	comm->SetVisibilityRadius( m_spawnRadius, m_despawnRadius );
}

void CCommunityAreaTypeSpawnRadius::OnExit()
{
	CCommunitySystem *comm = GCommonGame->GetSystem< CCommunitySystem >();
	ASSERT( comm );

	if ( !m_dontRestore )
	{
		comm->SetVisibilityRadius( m_prevSpawnRadius, m_prevDespawnRadius );
	}

	comm->UpdatePrevVisibilityRadius();
}

void CCommunityAreaTypeSpawnRadius::GetAreaData( Float &spawnRadius, Float &despawnRadius, Bool &dontRestore, Float &prevSpawnRadius, Float &prevDespawnRadius )
{
	spawnRadius = m_spawnRadius;
	despawnRadius = m_despawnRadius;
	dontRestore = m_dontRestore;

	prevSpawnRadius = m_prevSpawnRadius;
	prevDespawnRadius = m_prevDespawnRadius;
}