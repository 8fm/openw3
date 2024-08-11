
#pragma once

#include "behaviorGraphAnimationNode.h"

struct SMimicPostProcessData;

class CBehaviorGraphMimicsBoneAnimationNode : public CBehaviorGraphAnimationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsBoneAnimationNode, CBehaviorGraphAnimationNode, "Mimic.Animation", "Bone Animation" );

public:
	CBehaviorGraphMimicsBoneAnimationNode();

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override;
#endif

protected:
	virtual Bool ShouldDoPoseCorrection() const override;

private:
	void FillMimicData( CBehaviorGraphInstance& instance, SMimicPostProcessData* mimicData, const SBehaviorGraphOutput &pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsBoneAnimationNode );
	PARENT_CLASS( CBehaviorGraphAnimationNode );
END_CLASS_RTTI();
