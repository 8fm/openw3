/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "enableEncounterPerformableAction.h"

#include "encounter.h"

IMPLEMENT_ENGINE_CLASS( CEnableEncounterAction );

void CEnableEncounterAction::PerformOnEntity( CEntity* entity )
{
	CEncounter* encounter = Cast< CEncounter >( entity );
	if ( encounter )
	{
		encounter->EnableEncounter( m_enable );
	}
}