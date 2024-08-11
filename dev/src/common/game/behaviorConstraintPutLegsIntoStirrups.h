/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"
#include "../engine/behaviorIkTwoBones.h"
#include "behaviorConstraintPullStirrupsToLegs.h"

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPutLegIntoStirrupData;
struct SBehaviorConstraintPutLegIntoStirrup;
class CBehaviorConstraintPutLegsIntoStirrups;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPutLegIntoStirrupData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPutLegIntoStirrupData );

	CName m_footStoreName;
	CName m_stirrupStoreName;
	CName m_stirrupFinalStoreName;
	STwoBonesIKSolverData m_ik;
	Vector m_additionalSideDirForIKMS;
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPutLegIntoStirrupData );
	PROPERTY_EDIT( m_footStoreName, TXT("Foot store name") );
	PROPERTY_EDIT( m_stirrupStoreName, TXT("Stirrup store name") );
	PROPERTY_EDIT( m_stirrupFinalStoreName, TXT("Stirrup final store name") );
	PROPERTY_EDIT( m_ik, TXT("") );
	PROPERTY_EDIT( m_additionalSideDirForIKMS, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintPutLegIntoStirrup
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintPutLegIntoStirrup );

	Float m_weight;
	AnimQsTransform m_footTransformMS;
	AnimQsTransform m_stirrupTransformMS;
	AnimQsTransform m_stirrupFinalTransformMS;
	STwoBonesIKSolver m_ik;
	AnimVector4 m_additionalSideDirForIKMS;

	SBehaviorConstraintPutLegIntoStirrup();

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPutLegIntoStirrupData & data );

	void UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPutLegIntoStirrupData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & stirrupTargetWeightHandler );

	Bool IsActive() const { return m_weight != 0.0f; }
};

BEGIN_CLASS_RTTI( SBehaviorConstraintPutLegIntoStirrup );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintPutLegsIntoStirrups : public CBehaviorGraphPoseConstraintNode
											 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintPutLegsIntoStirrups, CBehaviorGraphPoseConstraintNode, "Constraints", "Put legs relatively to stirrups" );

protected:
	SBehaviorConstraintPutLegIntoStirrupData m_leftLeg;
	SBehaviorConstraintPutLegIntoStirrupData m_rightLeg;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< SBehaviorConstraintPutLegIntoStirrup > i_leftLeg;
	TInstanceVar< SBehaviorConstraintPutLegIntoStirrup > i_rightLeg;
	TInstanceVar< THandle<CEntity> > i_mountEntity;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_stirrupsOffTargetWeightHandler;

public:
	CBehaviorConstraintPutLegsIntoStirrups();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Put legs relatively to stirrups" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintPutLegsIntoStirrups );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_leftLeg, TXT("") );
	PROPERTY_EDIT( m_rightLeg, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
