/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;


/*
* Roll Constraint
* Special roll control for human skeleton.
*/
class CBehaviorGraphConstraintNodeRoll	: public CBehaviorGraphBaseNode
										, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeRoll, CBehaviorGraphBaseNode, "Constraints", "Roll deformation" );

protected:
	Float				m_weight;

	enum EBoneIndex		{	BI_Pelvis,
							BI_Shoulder_Left,			BI_Shoulder_Right,
							BI_Bicep_Left,				BI_Bicep_Right,
							BI_Forearm_Left,			BI_Forearm_Right,
							BI_Hand_Left,				BI_Hand_Right,
							BI_ElbowRoll_Left,			BI_ElbowRoll_Right,
							BI_ForearmRoll1_Left,		BI_ForearmRoll1_Right,
							BI_ForearmRoll2_Left,		BI_ForearmRoll2_Right,
							BI_HandRoll_Left,			BI_HandRoll_Right,
							BI_Bicep2_Left,				BI_Bicep2_Right,
							BI_ShoulderRoll_Left,		BI_ShoulderRoll_Right,
							BI_Thigh_Left,				BI_Thigh_Right,
							BI_Shin_Left,				BI_Shin_Right,
							BI_KneeRoll_Left,			BI_KneeRoll_Right,
							BI_LegRoll2_Left,			BI_LegRoll2_Right,
							BI_LegRoll_Left,			BI_LegRoll_Right,
							BI_Last
						};

protected:
	TInstanceVar< Float >				i_controlValue;
	TInstanceVar< TDynArray< Int32 > >	i_boneIndex;

protected:
	CBehaviorGraphValueNode*			m_cachedControlValueNode;

public:
	CBehaviorGraphConstraintNodeRoll();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void CacheConnections();
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CacheBoneIndex( CBehaviorGraphInstance& instance ) const;
	Bool CheckBones( CBehaviorGraphInstance& instance ) const;

	Float CalcShoulderRollWeight( const TDynArray< Int32 >& boneIndex, const SBehaviorGraphOutput& pose, EBoneIndex bicepBone, Float mulFactor ) const;

	void SetBoneRotation( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, SBehaviorGraphOutput& pose, EBoneIndex boneOut, EBoneIndex boneA, EBoneIndex boneB, Float weight ) const;
	void SetBoneRotationWithOffset( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponen, SBehaviorGraphOutput& pose, EBoneIndex boneOut, EBoneIndex boneA, EBoneIndex boneB, Float angleOffset, Float weight ) const;
	void SetBoneRotation( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, SBehaviorGraphOutput& pose, const CSkeleton* refSkeleton, EBoneIndex boneOut, EBoneIndex boneIn, Float weight ) const;
	void SetBoneRoll( const TDynArray< Int32 >& boneIndex, SBehaviorGraphOutput& pose, EBoneIndex boneOut, EBoneIndex boneIn, Float weight ) const;
	void SetBoneRotationEqualInMS( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, SBehaviorGraphOutput& pose, EBoneIndex boneOut, EBoneIndex boneRef ) const;

	void DisplayAxisForBone( Uint32 bone, const CAnimatedComponent* animatedComponent, CRenderFrame* frame ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeRoll );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_weight, TXT("") );
	PROPERTY( m_cachedControlValueNode );
END_CLASS_RTTI();
