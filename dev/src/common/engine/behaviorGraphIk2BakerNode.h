#pragma once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"
#include "../core/engineQsTransform.h"

class CBehaviorGraphIk2BakerNode	: public CBehaviorGraphBaseNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphIk2BakerNode, CBehaviorGraphBaseNode, "Constraints.Simple", "Ik2 Baker" );

protected:
	String m_endBoneName;

	Float m_blendInDuration;
	Float m_blendOutDuration;

	CName m_animEventName;
	Float m_defaultEventStartTime;
	Float m_defaultEventEndTime;

	EAxis m_hingeAxis;

	Bool m_enforceEndPosition;
	Bool m_bonePositionInWorldSpace;
	Bool m_enforceEndRotation;

protected:
	TInstanceVar< Int32 >	i_zeroBoneIdx; // firstBone's parent
	TInstanceVar< Int32 >	i_firstBoneIdx; // secondBone's parent
	TInstanceVar< Int32 >	i_secondBoneIdx; // endBone's parent
	TInstanceVar< Int32 >	i_endBoneIdx; // effector bone

	TInstanceVar< Float >	i_blendInDuration;
	TInstanceVar< Float >	i_blendOutDuration;

	TInstanceVar< Bool >	i_isAnimEventActive;
	TInstanceVar< Float >	i_currentAnimEventTime;
	TInstanceVar< Float >	i_animEventDuration;

	TInstanceVar< Bool >	i_needToRecalculateAdditiveTransforms;
	
	TInstanceVar< Float >	i_weight;

	TInstanceVar< Vector >		i_targetPos;
	TInstanceVar< EulerAngles >	i_targetRot;

	TInstanceVar< EngineQsTransform > i_firstBoneAdditiveTransform;
	TInstanceVar< EngineQsTransform > i_secondBoneAdditiveTransform;
	TInstanceVar< EngineQsTransform > i_endBoneAdditiveTransform;

protected:
	CBehaviorGraphValueNode*		m_cachedValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetPosNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetRotNode;

public:
	CBehaviorGraphIk2BakerNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets() override;
	virtual String GetCaption() const override { return TXT( "Ik2 Baker" ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const override;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void CacheConnections() override;

private:
	void CheckBones( CBehaviorGraphInstance& instance ) const;
	void CheckBlendTimes( CBehaviorGraphInstance& instance ) const;
	void UpdateAdditiveTransforms( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	Float CalculateBlendWeight( CBehaviorGraphInstance& instance ) const;
	Bool AreEqual( Float arg1, Float arg2, Float epsilon = NumericLimits< Float >::Epsilon() ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphIk2BakerNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedValueNode );
	PROPERTY( m_cachedTargetPosNode );
	PROPERTY( m_cachedTargetRotNode );
	PROPERTY_CUSTOM_EDIT( m_endBoneName, TXT( "End bone name" ), TXT( "BehaviorBoneSelection" ) );
	PROPERTY_EDIT_RANGE( m_blendInDuration, TXT( "Blend-in duration" ), 0.f, FLT_MAX );
	PROPERTY_EDIT_RANGE( m_blendOutDuration, TXT( "Blend-out duration" ), 0.f, FLT_MAX );
	PROPERTY_EDIT( m_animEventName, TXT( "Anim event name" ) );
	PROPERTY_EDIT( m_defaultEventStartTime, TXT( "Default event start time" ) );
	PROPERTY_EDIT( m_defaultEventEndTime, TXT( "Default event start time" ) );
	PROPERTY_EDIT( m_hingeAxis, TXT( "Hinge axis" ) );
	PROPERTY_EDIT( m_enforceEndPosition, TXT( "Enforce end position" ) );
	PROPERTY_EDIT( m_bonePositionInWorldSpace, TXT( "Bone position in world space" ) );
	PROPERTY_EDIT( m_enforceEndRotation, TXT( "Enforce end rotation" ) );
END_CLASS_RTTI();