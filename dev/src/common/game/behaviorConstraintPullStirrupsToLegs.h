/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintTargetWeightHandler;
struct SBehaviorConstraintPullStirrupToLegData;
struct SBehaviorConstraintPullStirrupToLeg;
class CBehaviorConstraintPullStirrupsToLegs;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPullStirrupToLegData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPullStirrupToLegData );

	CName m_stirrupBoneName;
	CName m_footBoneName;
	CName m_toeBoneName;

	Vector m_stirrupContactPoint;
	Vector m_footContactPoint;

	Vector m_toeAlignStirrupSideDir;
	Vector m_toeAlignStirrupRotationAxisDir;
	Float m_alignToToeWeight;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPullStirrupToLegData );
	PROPERTY_EDIT( m_stirrupBoneName, TXT("Stirrup bone name") );
	PROPERTY_EDIT( m_stirrupContactPoint, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_footBoneName, TXT("Foot bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_footContactPoint, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_toeBoneName, TXT("Toe bone name"), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT( m_toeAlignStirrupSideDir, TXT("") );
	PROPERTY_EDIT( m_toeAlignStirrupRotationAxisDir, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPullStirrupToLeg
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPullStirrupToLeg );

	Float m_weight;
	Float m_distanceTargetWeight;
	Int32 m_stirrupBoneIdx;
	Int32 m_stirrupParentBoneIdx;
	Int32 m_footBoneIdx;
	Int32 m_toeBoneIdx;

	SBehaviorConstraintPullStirrupToLeg();

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPullStirrupToLegData & data );

	void UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPullStirrupToLegData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & stirrupTargetWeightHandler );

	Bool IsActive() const { return m_weight != 0.0f; }
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPullStirrupToLeg );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintTargetWeightHandler : public IEventHandler< CAnimationEventFired >
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintTargetWeightHandler );

	Float m_targetWeight;
	CName m_eventName;
	Bool m_eventSwitchesOn;

	void Setup( CBehaviorGraphInstance& instance, const CName & eventName, Bool eventSwitchesOn = false );

	void OnActivated( CBehaviorGraphInstance& instance );
	void OnDeactivated( CBehaviorGraphInstance& instance );

	void ReadyForNextFrame();

	virtual	void HandleEvent( const CAnimationEventFired &event ) override;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintTargetWeightHandler );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintPullStirrupsToLegs : public CBehaviorGraphPoseConstraintNode
											, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintPullStirrupsToLegs, CBehaviorGraphPoseConstraintNode, "Constraints", "Pull stirrups to legs" );

protected:
	SBehaviorConstraintPullStirrupToLegData m_leftLeg;
	SBehaviorConstraintPullStirrupToLegData m_rightLeg;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< SBehaviorConstraintPullStirrupToLeg > i_leftLeg;
	TInstanceVar< SBehaviorConstraintPullStirrupToLeg > i_rightLeg;
	TInstanceVar< THandle<CEntity> > i_mountEntity;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_stirrupsOffTargetWeightHandler;

public:
	CBehaviorConstraintPullStirrupsToLegs();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Pull stirrups to legs" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintPullStirrupsToLegs );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_leftLeg, TXT("") );
	PROPERTY_EDIT( m_rightLeg, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
