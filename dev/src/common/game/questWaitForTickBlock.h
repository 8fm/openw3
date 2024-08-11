/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraph.h"
#include "questTestBlock.h"

class CQuestWaitForTickBlock : public CQuestTestBlock
{
	DECLARE_ENGINE_CLASS( CQuestWaitForTickBlock, CQuestTestBlock, 0 )

private:
	Uint32			m_tick;

public:
	CQuestWaitForTickBlock()  { m_name = TXT( "Wait for tick" ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual String GetCaption() const { return TXT( "Wait for tick" ); }
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const
	{
		return graph && graph->IsTest();
	}

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnExecute( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestWaitForTickBlock )
	PARENT_CLASS( CQuestTestBlock )
	PROPERTY_EDIT( m_tick, TXT( "Tick number to wait for" ) )
	END_CLASS_RTTI()