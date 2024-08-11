/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintRiderInSaddle;

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintRiderInSaddle : public CBehaviorGraphPoseConstraintNode
									   , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintRiderInSaddle, CBehaviorGraphPoseConstraintNode, "Constraints", "Rider in saddle (match other entity's pelvis offset)" );

protected:
	CName m_bone;
	Float m_blendTime;
	Float m_blendRotation;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< THandle<CEntity> > i_matchEntity;
	TInstanceVar< Float > i_atTargetWeight;
	TInstanceVar< Vector > i_currentOffset;
	TInstanceVar< Vector > i_currentRotation;
	TInstanceVar< Int32 > i_bone;
	TInstanceVar< Int32 > i_parentBone;

public:
	CBehaviorConstraintRiderInSaddle();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Rider in saddle (match other entity's pelvis offset)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintRiderInSaddle );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_blendTime, TXT("Blend time of weight") );
	PROPERTY_EDIT( m_blendRotation, TXT("Blend rotation") );
END_CLASS_RTTI();

