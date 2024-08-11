/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questPhaseIOBlock.h"

class CQuestThread;

///////////////////////////////////////////////////////////////////////////////

class CQuestPhaseOutputBlock : public CQuestPhaseIOBlock
{
	DECLARE_ENGINE_CLASS( CQuestPhaseOutputBlock, CQuestPhaseIOBlock, 0 )

	// runtime data
	TInstanceVar< CQuestScopeBlock* >		i_listener;			//!< listener we need to inform about this block's activation
	TInstanceVar< TGenericPtr >				i_listenerData;		//!< instance data in context of which the listener operates

public:
	CQuestPhaseOutputBlock();

	//! CGraphBlock interface
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	void AttachListener( InstanceBuffer& data, const CQuestScopeBlock& listener, InstanceBuffer& listenerData ) const;
};

BEGIN_CLASS_RTTI( CQuestPhaseOutputBlock )
	PARENT_CLASS( CQuestPhaseIOBlock )
END_CLASS_RTTI()