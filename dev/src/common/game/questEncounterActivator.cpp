#include "build.h"
#include "questEncounterActivator.h"

#include "encounter.h"
#include "questGraphSocket.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestEncounterActivator )


	CQuestEncounterActivator::CQuestEncounterActivator() 
	: m_deactivateEncounter( false )
{
	m_name = TXT( "Encounter manual activation" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

EGraphBlockShape CQuestEncounterActivator::GetBlockShape() const						{ return GBS_Rounded; }
Color CQuestEncounterActivator::GetClientColor() const									{ return Color( 48, 48, 48 ); }
String CQuestEncounterActivator::GetBlockCategory() const								{ return TXT( "Gameplay" ); }
Bool CQuestEncounterActivator::CanBeAddedToGraph( const CQuestGraph* graph ) const		{ return true; }


void CQuestEncounterActivator::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif


void CQuestEncounterActivator::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_encounterTag );
		if ( entity && entity->IsA< CEncounter >() )
		{
			CEncounter* encounter = static_cast< CEncounter* >( entity );
			
			if( m_deactivateEncounter )
			{
				encounter->LeaveArea();
			}
			else
			{
				encounter->EnterArea();
			}
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
