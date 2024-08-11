/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeSequence.h"

class CBehTreeNodeSequenceCheckAvailabilityR6Instance;

////////////////////////////////////////////////////////////////////////
// Sequence that can be checked for availability and evaluated
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSequenceCheckAvailabilityR6Definition : public CBehTreeNodeSequenceCheckAvailabilityDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSequenceCheckAvailabilityR6Definition, CBehTreeNodeSequenceCheckAvailabilityDefinition, CBehTreeNodeSequenceCheckAvailabilityR6Instance, SequenceCheckAvailabilityR6 );
	DECLARE_AS_R6_ONLY
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeSequenceCheckAvailabilityR6Definition );
	PARENT_CLASS( CBehTreeNodeSequenceCheckAvailabilityDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSequenceCheckAvailabilityR6Instance : public CBehTreeNodeSequenceCheckAvailabilityInstance
{
	typedef CBehTreeNodeSequenceCheckAvailabilityInstance Super;
public:
	typedef CBehTreeNodeSequenceCheckAvailabilityR6Definition Definition;

	CBehTreeNodeSequenceCheckAvailabilityR6Instance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeSequenceCheckAvailabilityInstance( def, owner, context, parent )
	{
	}

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;
};

