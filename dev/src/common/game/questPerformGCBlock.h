/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questGraph.h"
#include "questTestBlock.h"

class CQuestPerformGCBlock : public CQuestTestBlock
{
	DECLARE_ENGINE_CLASS( CQuestPerformGCBlock, CQuestTestBlock, 0 )

protected:
	// runtime data
	TInstanceVar< Uint32 >							i_GCRequestId;
public:
	CQuestPerformGCBlock()  { m_name = TXT( "Perform GC" ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual String GetCaption() const { return TXT( "Perform GC" ); }
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const
	{
		return graph && graph->IsTest();
	}

#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestPerformGCBlock )
	PARENT_CLASS( CQuestTestBlock )
	END_CLASS_RTTI()