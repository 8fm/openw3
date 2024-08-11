/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questEncounterManagerBlock.h"

#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

#include "encounter.h"
#include "questGraphSocket.h"

IMPLEMENT_ENGINE_CLASS( CQuestEncounterManagerBlock )


CQuestEncounterManagerBlock::CQuestEncounterManagerBlock() 
	: m_enableEncounter( true )
	, m_forceDespawnDetached( false )
{
	m_name = TXT( "Encounter manager" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

EGraphBlockShape CQuestEncounterManagerBlock::GetBlockShape() const							{ return GBS_Rounded; }
Color CQuestEncounterManagerBlock::GetClientColor() const									{ return Color( 135, 40, 156 ); }
String CQuestEncounterManagerBlock::GetBlockCategory() const								{ return TXT( "Gameplay" ); }
Bool CQuestEncounterManagerBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const		{ return true; }


void CQuestEncounterManagerBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif		// NO_EDITOR_GRAPH_SUPPORT


void CQuestEncounterManagerBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		CEntity* entity = world->GetTagManager()->GetTaggedEntity( m_encounterTag );
		if ( entity && entity->IsA< CEncounter >() )
		{
			CEncounter* encounter = static_cast< CEncounter* >( entity );
			encounter->EnableEncounter( m_enableEncounter );	
			if ( !m_encounterSpawnPhase.Empty() )
			{
				encounter->SetSpawnPhase( m_encounterSpawnPhase );
			}
			if ( m_forceDespawnDetached )
			{
				encounter->ForceDespawnDetached();
			}
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}

Bool CQuestEncounterManagerBlock::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( TBaseClass::OnPropertyMissing( propertyName, readValue ) )
	{
		return true;
	}
	if ( propertyName.AsString() == TXT("forceFullDespawn") )
	{
		Bool b;
		if ( readValue.AsType< Bool >( b ) )
		{
			m_forceDespawnDetached = b;
		}
	}
	return false;
}



void CQuestEncounterManagerBlock::GetEncounterPhaseNames( IProperty *property, TDynArray< CName >& outPhaseNames )
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