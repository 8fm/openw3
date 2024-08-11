/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Unlock the NPC for quests
class CQuestUnlockNPCBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestUnlockNPCBlock, CQuestGraphBlock, 0 )

private:
	TagList		m_npcsTag;

public:
	CQuestUnlockNPCBlock();

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
};

BEGIN_CLASS_RTTI( CQuestUnlockNPCBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_npcsTag, TXT( "Tags of the NPCs to lock" ) );
END_CLASS_RTTI()

