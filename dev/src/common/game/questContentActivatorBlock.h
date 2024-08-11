/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

///////////////////////////////////////////////////////////////////////////////

class CQuestContentActivatorBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestContentActivatorBlock, CQuestGraphBlock, 0 )

public:
	CQuestContentActivatorBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! CGraphBlock interface
	virtual String GetCaption() const { return TXT("Activate Content"); }
	virtual String GetBlockName() const { return TXT("Activate Content"); }
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 255, 255, 0 ); }
	virtual String GetBlockCategory() const { return TXT( "PlayGo" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;

private:
	CName										m_playGoChunk;
};

BEGIN_CLASS_RTTI( CQuestContentActivatorBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_playGoChunk, TXT("PlayGo chunk"), TXT("PlayGoChunkSelector") );
END_CLASS_RTTI()