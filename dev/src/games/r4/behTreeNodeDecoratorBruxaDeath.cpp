/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDecoratorBruxaDeath.h"

#include "../../common/game/behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorBruxaDeathDefinition )

RED_DEFINE_STATIC_NAME( Death )


CBehTreeNodeDecoratorBruxaDeathInstance::CBehTreeNodeDecoratorBruxaDeathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_guardAreaDataPtr( owner )
	, m_saveLock( CGameSessionManager::GAMESAVELOCK_INVALID )
{

}

Bool CBehTreeNodeDecoratorBruxaDeathInstance::Activate()
{
	if ( !Super::Activate() )
	{
		return false;
	}

	static const String SAVE_LOCK_REASON( TXT("BruxaDeath") );

	SGameSessionManager::GetInstance().CreateNoSaveLock( SAVE_LOCK_REASON ,m_saveLock );

	return true;
}

void CBehTreeNodeDecoratorBruxaDeathInstance::Deactivate()
{
	if ( m_saveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
		m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	Super::Deactivate();
}

Bool CBehTreeNodeDecoratorBruxaDeathInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( Death ) )
	{
		if ( m_saveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
		{
			SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
			m_saveLock = CGameSessionManager::GAMESAVELOCK_INVALID;
		}
	}
	return Super::OnEvent( e );
}