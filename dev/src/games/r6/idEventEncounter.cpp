/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventEncounter.h"
#include "idEventSenderDataStructs.h"
#include "../../common/game/encounter.h"
#include "eventRouterComponent.h"
#include "../../common/engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CIdEventEncounter )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventEncounter::Activate( CIDTopicInstance* topicInstance )
{
	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_data.m_encounterTag );
		if ( entity && entity->IsA< CEncounter >() )
		{
			CEncounter* encounter = static_cast< CEncounter* >( entity );

			switch( m_data.m_enableMode )
			{
			case EAM_Enable :
				encounter->EnterArea();
				break;
			case EAM_Disable :
				encounter->LeaveArea();
				break;
			case EAM_ChangePhase :
				encounter->SetSpawnPhase( m_data.m_encounterSpawnPhase );
				break;
			}
		}
	}
}

