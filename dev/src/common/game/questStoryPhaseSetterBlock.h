/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CQuestStoryPhaseSetterBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestStoryPhaseSetterBlock, CQuestGraphBlock, 0 )

private:
	// block data
	TDynArray< IQuestSpawnsetAction* >		m_spawnsets;		//!< Community spawnsets this block activates

public:
	CQuestStoryPhaseSetterBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetClientColor() const { return Color( 48, 48, 48 ); }
	virtual void OnRebuildSockets();
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

	// Collect resources
	virtual void CollectContent( IQuestContentCollector& collector ) const override;
};

BEGIN_CLASS_RTTI( CQuestStoryPhaseSetterBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_spawnsets, TXT( "Community spawnsets this block activates" ) )
END_CLASS_RTTI()
