#pragma once

class CQuestEncounterActivator : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestEncounterActivator, CQuestGraphBlock, 0 )

protected:
	CName								m_encounterTag;	
	Bool								m_deactivateEncounter;

public:
	CQuestEncounterActivator();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const override;
	virtual Color GetClientColor() const override;
	virtual void OnRebuildSockets() override;
	virtual String GetBlockCategory() const override;
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const override;

#endif

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
};

BEGIN_CLASS_RTTI( CQuestEncounterActivator )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_encounterTag, TXT( "Encounter to effect" ), TXT("EncounterEntitySelector") )
	PROPERTY_EDIT(  m_deactivateEncounter, TXT("If encounter should be deactivated or activated") );
END_CLASS_RTTI()