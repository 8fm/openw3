/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"
#include "behaviorConstraintPullStirrupsToLegs.h"

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPullReinToHandData;
struct SBehaviorConstraintPullReinToHand;
class CBehaviorConstraintPullReinsToHands;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPullReinToHandData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPullReinToHandData );

	CName m_reinBoneName;
	CName m_handBoneName;

	Vector m_reinContactPoint;
	Vector m_handContactPoint;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPullReinToHandData );
	PROPERTY_EDIT( m_reinBoneName, TXT("Rein bone name") );
	PROPERTY_EDIT( m_reinContactPoint, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_handBoneName, TXT("Hand bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_handContactPoint, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPullReinToHand
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPullReinToHand );

	Float m_weight;
	Float m_distanceTargetWeight;
	Int32 m_reinBoneIdx;
	Int32 m_reinParentBoneIdx;
	Int32 m_handBoneIdx;

#ifndef NO_EDITOR
	AnimQsTransform m_handTMS;
	AnimVector4 m_requestedContactPointMS;
	AnimQsTransform m_reinTMS;
	AnimVector4 m_reinContactPointMS;
	AnimQsTransform m_postReinTMS;
	AnimVector4 m_postReinContactPointMS;
#endif

	SBehaviorConstraintPullReinToHand();

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPullReinToHandData & data );

	void UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPullReinToHandData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & reinsTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & reinTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & forceReinsOnTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & forceReinOnTargetWeightHandler );

	Bool IsActive() const { return m_weight != 0.0f; }
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPullReinToHand );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintPullReinsToHands : public CBehaviorGraphPoseConstraintNode
										  , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintPullReinsToHands, CBehaviorGraphPoseConstraintNode, "Constraints", "Pull reins to hands" );

protected:
	SBehaviorConstraintPullReinToHandData m_leftHand;
	SBehaviorConstraintPullReinToHandData m_rightHand;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< SBehaviorConstraintPullReinToHand > i_leftHand;
	TInstanceVar< SBehaviorConstraintPullReinToHand > i_rightHand;
	TInstanceVar< THandle<CEntity> > i_mountEntity;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_reinsOffTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_leftReinOffTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_rightReinOffTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_forceReinsOnTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_forceLeftReinOnTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_forceRightReinOnTargetWeightHandler;

public:
	CBehaviorConstraintPullReinsToHands();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Pull reins to hands" ); }
#endif
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintPullReinsToHands );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_leftHand, TXT("") );
	PROPERTY_EDIT( m_rightHand, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
