/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "questPhaseIOBlock.h"

class CQuestPhaseInputBlock : public CQuestPhaseIOBlock
{
	DECLARE_ENGINE_CLASS( CQuestPhaseInputBlock, CQuestPhaseIOBlock, 0 )

public:
	CQuestPhaseInputBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

};

BEGIN_CLASS_RTTI( CQuestPhaseInputBlock )
	PARENT_CLASS( CQuestPhaseIOBlock )
END_CLASS_RTTI()