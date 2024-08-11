#include "build.h"
#include "questEncounterPhaseSetterBlock.h"

#include "encounter.h"
#include "questGraphSocket.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestEncounterPhaseBlock )


CQuestEncounterPhaseBlock::CQuestEncounterPhaseBlock() 
{
	m_name = TXT( "Encounter phase setter" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

EGraphBlockShape CQuestEncounterPhaseBlock::GetBlockShape() const						{ return GBS_Rounded; }
Color CQuestEncounterPhaseBlock::GetClientColor() const									{ return Color( 48, 48, 48 ); }
String CQuestEncounterPhaseBlock::GetBlockCategory() const								{ return TXT( "Gameplay" ); }
Bool CQuestEncounterPhaseBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const		{ return true; }


void CQuestEncounterPhaseBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif


void CQuestEncounterPhaseBlock::GetEncounterPhaseNames( IProperty *property, TDynArray< CName >& outPhaseNames )
{
	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_encounterTag );
		if ( entity && entity->IsA< CEncounter >() )
		{
			CEncounter* encounter = static_cast< CEncounter* >( entity );
			encounter->GetSpawnPhases( outPhaseNames );
		}
	}
}

void CQuestEncounterPhaseBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_encounterTag );
		if ( entity && entity->IsA< CEncounter >() )
		{
			CEncounter* encounter = static_cast< CEncounter* >( entity );
			encounter->SetSpawnPhase( m_encounterSpawnPhase );
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}
