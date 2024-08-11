/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventAbortInteraction.h"
/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventEncounter.h"
#include "idEventSenderDataStructs.h"
#include "../../common/game/encounter.h"
#include "eventRouterComponent.h"
#include "../../common/engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CIdEventAbortInteraction )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventAbortInteraction::Activate( CIDTopicInstance* topicInstance )
{
	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_entityTag );
		if( entity )
		{
			entity->CallEvent( CNAME( OnAbortInteraction ) );
		}
	}
}

