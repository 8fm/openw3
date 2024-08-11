#include "build.h"
#include "questEncounterFullRespawnBlock.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "encounter.h"

IMPLEMENT_ENGINE_CLASS( CQuestEncounterFullRespawn )

CQuestEncounterFullRespawn::CQuestEncounterFullRespawn()
{
	m_name = TXT( "Encounter full respawn" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestEncounterFullRespawn::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif


void CQuestEncounterFullRespawn::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		if ( CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_encounterTag ) )
		{
			if ( CEncounter* encounter = Cast< CEncounter >( entity ) )
			{
				encounter->DoFullRespawn();
			}
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
