/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorPoseConstraintNode.h"
#include "../engine/behaviorIkTwoBones.h"
#include "behaviorConstraintPullStirrupsToLegs.h"

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintUprightSpineBonesData;
struct SBehaviorConstraintUprightSpineBones;
class CBehaviorConstraintUprightSpine;

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintUprightSpineBonesData
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintUprightSpineBonesData );

	CName m_boneName;
	Float m_weight;
	Float m_weightMatchEntity;
	Float m_weightMatchEntityFullSpeed;
	Int32 m_boneCount;

	SBehaviorConstraintUprightSpineBonesData();
};

BEGIN_CLASS_RTTI( SBehaviorConstraintUprightSpineBonesData );
PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Bone"), TXT( "BehaviorBoneSelection" ) );
PROPERTY_EDIT( m_weight, TXT("") );
PROPERTY_EDIT( m_boneCount, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorConstraintUprightSpineBones
{
public:
	DECLARE_RTTI_STRUCT( SBehaviorConstraintUprightSpineBones );

	TDynArray< Int32 > m_boneIndices;
	Float m_weightPerBone;
	Float m_weightMatchEntityPerBone;
	Float m_weightMatchEntityFullSpeedPerBone;

	void Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintUprightSpineBonesData & data );
};

BEGIN_CLASS_RTTI( SBehaviorConstraintUprightSpineBones );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorConstraintUprightSpine : public CBehaviorGraphPoseConstraintNode
									  , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorConstraintUprightSpine, CBehaviorGraphPoseConstraintNode, "Constraints", "Keep spine upright" );

protected:
	SBehaviorConstraintUprightSpineBonesData m_bones;
	STwoBonesIKSolverData m_leftHandIK;
	STwoBonesIKSolverData m_rightHandIK;
	Float m_matchEntityFullSpeed;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< THandle<CEntity> > i_matchEntity;
	TInstanceVar< Vector > i_matchEntityLocation;
	TInstanceVar< Float > i_matchEntityUseFullSpeed;
	TInstanceVar< SBehaviorConstraintUprightSpineBones > i_bones;
	TInstanceVar< STwoBonesIKSolver > i_leftHandIK;
	TInstanceVar< STwoBonesIKSolver > i_rightHandIK;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_handIKTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_leftHandIKTargetWeightHandler;
	TInstanceVar< SBehaviorConstraintTargetWeightHandler > i_rightHandIKTargetWeightHandler;
	TInstanceVar< Float > i_handIKWeight;
	TInstanceVar< Float > i_leftHandIKWeight;
	TInstanceVar< Float > i_rightHandIKWeight;

public:
	CBehaviorConstraintUprightSpine();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Keep spine upright" ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorConstraintUprightSpine );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY_EDIT( m_bones, TXT("") );
	PROPERTY_EDIT( m_leftHandIK, TXT("") );
	PROPERTY_EDIT( m_rightHandIK, TXT("") );
	PROPERTY_EDIT( m_matchEntityFullSpeed, TXT("") );
END_CLASS_RTTI();

