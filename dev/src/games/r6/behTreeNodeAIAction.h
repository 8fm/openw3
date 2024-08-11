/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeAtomicAction.h"
#include "r6behTreeVars.h"

class CBehTreeNodeAIActionInstance;

class CBehTreeNodeAIActionDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAIActionDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAIActionInstance, AIAction )
	DECLARE_AS_R6_ONLY

protected:
	CBehTreeValAIAction m_action;

public:
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAIActionDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
	PROPERTY_INLINED( m_action, TXT("AIAction definition.") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAIActionInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	CAIAction*	m_actionInstance;

public:
	typedef CBehTreeNodeAIActionDefinition Definition;

	CBehTreeNodeAIActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	virtual ~CBehTreeNodeAIActionInstance();

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	virtual Bool IsAvailable();

	////////////////////////////////////////////////////////////////////
	//! action interface
	void OnStopAIAction( CAIAction* actionBeingStopped );

};