/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playPropertyAnimationAction.h"

IMPLEMENT_ENGINE_CLASS( CPlayPropertyAnimationAction )

void CPlayPropertyAnimationAction::PerformOnEntity( CEntity* entity )
{
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity );
	if ( gameplayEntity )
	{
		gameplayEntity->PlayPropertyAnimation( m_animationName, m_loopCount, m_lengthScale, m_mode );
	}
}