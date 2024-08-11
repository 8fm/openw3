/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "behaviorGraphAnimationManualSlot.h"

class CBehaviorGraphMimicManualSlotNode : public CBehaviorGraphAnimationManualSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicManualSlotNode, CBehaviorGraphAnimationManualSlotNode, "Slots.Mimic", "Manual slot" )

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual void OnRebuildSockets();
#endif

public:	
	virtual Bool IsMimic() const { return true; }

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
	virtual void PerformPoseCorrection( SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicManualSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationManualSlotNode );
END_CLASS_RTTI();