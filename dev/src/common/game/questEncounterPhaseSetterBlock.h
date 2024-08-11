#pragma once


class IEncounterPhaseNamesGetter
{
public:
	virtual void GetEncounterPhaseNames( IProperty *property, TDynArray< CName >& outPhaseNames ) = 0;
};

class CQuestEncounterPhaseBlock : public CQuestGraphBlock, public IEncounterPhaseNamesGetter
{
	DECLARE_ENGINE_CLASS( CQuestEncounterPhaseBlock, CQuestGraphBlock, 0 )

protected:
	CName								m_encounterTag;
	CName								m_encounterSpawnPhase;

public:
	CQuestEncounterPhaseBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const override;
	virtual Color GetClientColor() const override;
	virtual void OnRebuildSockets() override;
	virtual String GetBlockCategory() const override;
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const override;

#endif

	void GetEncounterPhaseNames( IProperty *property, TDynArray< CName >& outPhaseNames ) override;

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
};

BEGIN_CLASS_RTTI( CQuestEncounterPhaseBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_encounterTag, TXT( "Encounter to effect" ), TXT("EncounterEntitySelector") )
	PROPERTY_CUSTOM_EDIT( m_encounterSpawnPhase, TXT("Spawn phase name to activate"), TXT("EncounterPhasesEditor") )
END_CLASS_RTTI()
