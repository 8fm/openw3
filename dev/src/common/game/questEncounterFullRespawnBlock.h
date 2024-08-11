#pragma once

class CQuestEncounterFullRespawn : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestEncounterFullRespawn, CQuestGraphBlock, 0 )

protected:
	CName				m_encounterTag;	

public:
						CQuestEncounterFullRespawn();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! CGraphBlock interface
	EGraphBlockShape	GetBlockShape() const override									{ return GBS_Rounded; }
	Color				GetClientColor() const override									{ return Color( 48, 48, 48 ); }
	String				GetBlockCategory() const override								{ return TXT( "Gameplay" ); }
	Bool				CanBeAddedToGraph( const CQuestGraph* graph ) const override	{ return true; }
	void				OnRebuildSockets() override;

#endif

	void				OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const override;
};

BEGIN_CLASS_RTTI( CQuestEncounterFullRespawn )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_encounterTag, TXT( "Encounter to effect" ), TXT("EncounterEntitySelector") )
END_CLASS_RTTI()