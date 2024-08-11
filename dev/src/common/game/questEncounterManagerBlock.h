/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "questEncounterPhaseSetterBlock.h"

class CQuestEncounterManagerBlock : public CQuestGraphBlock, public IEncounterPhaseNamesGetter
{
	DECLARE_ENGINE_CLASS( CQuestEncounterManagerBlock, CQuestGraphBlock, 0 )

protected:
	CName								m_encounterTag;	
	Bool								m_enableEncounter;
	Bool								m_forceDespawnDetached;
	CName								m_encounterSpawnPhase;

public:
	CQuestEncounterManagerBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	EGraphBlockShape GetBlockShape() const override;
	Color GetClientColor() const override;
	void OnRebuildSockets() override;
	String GetBlockCategory() const override;
	Bool CanBeAddedToGraph( const CQuestGraph* graph ) const override;

#endif

	void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;

	Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	// IEncounterPhaseNamesGetter interface
	void GetEncounterPhaseNames( IProperty *property, TDynArray< CName >& outPhaseNames ) override;
};

BEGIN_CLASS_RTTI( CQuestEncounterManagerBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_encounterTag, TXT( "Encounter to effect" ), TXT("EncounterEntitySelector") )
	PROPERTY_EDIT( m_enableEncounter, TXT("If encounter should be deactivated or activated") )
	PROPERTY_EDIT( m_forceDespawnDetached, TXT("Force current encounter creature population despawn") )
	PROPERTY_CUSTOM_EDIT( m_encounterSpawnPhase, TXT("Spawn phase name to activate"), TXT("EncounterPhasesEditor") )
END_CLASS_RTTI()