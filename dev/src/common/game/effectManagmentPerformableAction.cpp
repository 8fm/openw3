/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectManagmentPerformableAction.h"

IMPLEMENT_ENGINE_CLASS( IEffectManagmentPerformableAction )
IMPLEMENT_ENGINE_CLASS( CPlayEffectPerformableAction )
IMPLEMENT_ENGINE_CLASS( CStopAllEffectsPerformableAction )
IMPLEMENT_ENGINE_CLASS( CStopEffectPerformableAction )


void CPlayEffectPerformableAction::PerformOnEntity( CEntity* entity )
{
	entity->PlayEffect( m_effectName, m_boneName );
}

void CStopAllEffectsPerformableAction::PerformOnEntity( CEntity* entity )
{
	entity->StopAllEffects();
}

void CStopEffectPerformableAction::PerformOnEntity( CEntity* entity )
{
	entity->StopEffect( m_effectName );
}