/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/game/behTree.h"
#include "../../common/game/behTreeInstance.h"
#include "aiAction.h"

class CR6BehTreeInstance : public CBehTreeInstance
{
	DECLARE_ENGINE_CLASS( CR6BehTreeInstance, CBehTreeInstance, 0 )

protected:
	TDynArray< CAIAction* >	m_actionInstances;

public:
	CR6BehTreeInstance();
	RED_INLINE Bool IsBound() const { return nullptr != m_treeDefinition; }

	CAIAction* SpawnAIActionInstance( const CAIAction* templ );
	void RemoveAIActionInstance( CAIAction* inst );

	Bool CheckAIActionAvailability( CAIAction* action );
	EAIActionStatus PerformAIAction( CAIAction* actionToPerform );
	EAIActionStatus StopAIAction( CAIAction* actionToStop );
};

BEGIN_CLASS_RTTI( CR6BehTreeInstance )
	PARENT_CLASS( CBehTreeInstance )
	PROPERTY( m_actionInstances )
END_CLASS_RTTI()
