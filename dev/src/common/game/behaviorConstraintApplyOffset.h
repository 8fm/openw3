/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintApplyOffset;

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintApplyOffset : public CBehaviorGraphPoseConstraintNode
									   , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintApplyOffset, CBehaviorGraphPoseConstraintNode, "Constraints", "Apply offset (from animation proxy)" );

protected:
	TInstanceVar< Float > i_timeDelta;

public:
	CBehaviorConstraintApplyOffset();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Apply offset (from animation proxy)" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintApplyOffset );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintCalcAdSetOffsetForPelvis	: public CBehaviorGraphBaseNode
													, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintCalcAdSetOffsetForPelvis, CBehaviorGraphBaseNode, "Constraints", "Calc and set offset for pelvis (from animation proxy)" );

protected:
	String					m_pelvisBoneName;

protected:
	TInstanceVar< Int32 >	i_pelvisIdx;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Calc and set offset for pelvis WS (from animation proxy)" ); }
#endif

public:
	CBehaviorConstraintCalcAdSetOffsetForPelvis();;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintCalcAdSetOffsetForPelvis );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_pelvisBoneName, TXT("Pelvis bone name"), TXT("BehaviorBoneSelection") );
END_CLASS_RTTI();
