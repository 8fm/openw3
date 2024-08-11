/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeComposite.h"

class CBehTreeNodePlayerStateSelectorInstance;

////////////////////////////////////////////////////////////////////////
// Sequence that can be checked for availability and evaluated
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerStateSelectorDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayerStateSelectorDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodePlayerStateSelectorInstance, PlayerStateSelector );
	DECLARE_AS_R6_ONLY
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodePlayerStateSelectorDefinition );
	PARENT_CLASS( IBehTreeNodeCompositeDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerStateSelectorInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;

public:
	typedef CBehTreeNodePlayerStateSelectorDefinition Definition;

	CBehTreeNodePlayerStateSelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	Bool Activate() override;
	void Update() override;
	void OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;

private:
	Bool Think();
	Uint32 SelectChild();
};

