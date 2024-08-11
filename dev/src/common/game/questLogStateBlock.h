/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraph.h"
#include "questTestBlock.h"

class CQuestLogStateBlock : public CQuestTestBlock
{
	DECLARE_ENGINE_CLASS( CQuestLogStateBlock, CQuestTestBlock, 0 )

private:
	String			m_state;

public:
	CQuestLogStateBlock()  { m_name = TXT( "Log state" ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual String GetCaption() const { return TXT( "Log state" ); }
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const
	{
		return graph && graph->IsTest();
	}

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestLogStateBlock )
	PARENT_CLASS( CQuestTestBlock )
	PROPERTY_EDIT( m_state, TXT( "State name to put in logs" ) )
	END_CLASS_RTTI()