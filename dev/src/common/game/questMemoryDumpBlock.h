/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraph.h"
#include "questTestBlock.h"

class CQuestMemoryDumpBlock : public CQuestTestBlock
{
	DECLARE_ENGINE_CLASS( CQuestMemoryDumpBlock, CQuestTestBlock, 0 )

private:
	String			m_tag;

public:
	CQuestMemoryDumpBlock()  { m_name = TXT( "Dump memory" ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual String GetCaption() const { return TXT( "Dump memory" ); }
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

BEGIN_CLASS_RTTI( CQuestMemoryDumpBlock )
	PARENT_CLASS( CQuestTestBlock )
	PROPERTY_EDIT( m_tag, TXT( "Tag for dump" ) )
	END_CLASS_RTTI()