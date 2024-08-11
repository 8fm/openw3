/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"
#include "behaviorConstraintPullStirrupsToLegs.h"
#include "../engine/behaviorIkTwoBones.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintMoveHandsByOffset : public CBehaviorGraphPoseConstraintNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintMoveHandsByOffset, CBehaviorGraphPoseConstraintNode, "Constraints", "Move hands by offset" );

protected:
	STwoBonesIKSolverData m_leftHand;
	STwoBonesIKSolverData m_rightHand;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< Float > i_weight;
	TInstanceVar< Float > i_leftHandOffset;
	TInstanceVar< Float > i_rightHandOffset;
	TInstanceVar< STwoBonesIKSolver > i_leftHand;
	TInstanceVar< STwoBonesIKSolver > i_rightHand;

public:
	CBehaviorConstraintMoveHandsByOffset();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Move hands by offset" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	
public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintMoveHandsByOffset );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_leftHand, TXT("") );
	PROPERTY_EDIT( m_rightHand, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
