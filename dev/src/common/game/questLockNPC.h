/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Lock the NPC for quests
class CQuestLockNPCBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestLockNPCBlock, CQuestGraphBlock, 0 )

private:
	TagList		m_npcsTag;

public:
	CQuestLockNPCBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual String GetCaption() const { return TXT("Lock NPC"); }

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 200, 200, 80 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestLockNPCBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_npcsTag, TXT( "Tags of the NPCs to lock" ) );
END_CLASS_RTTI()

