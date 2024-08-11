/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "allocatedBehaviorGraphOutput.h"
#include "behaviorIncludes.h"
#include "behaviorGraphPoseSlotListener.h"

class CBehaviorGraphPoseSlotNode : public CBehaviorGraphBaseNode
								 , public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseSlotNode, CBehaviorGraphBaseNode, "Slots", "Pose" );

protected:
	CName					m_slotName;
	String					m_firstBone;
	Bool					m_worldSpace;
	EInterpolationType		m_interpolation;
	Bool					m_blendFloatTracks;
	Bool					m_ignoreZeroFloatTracks;

protected:
	TInstanceVar< Bool >	i_running;
	TInstanceVar< Float	>	i_blendTime;
	TInstanceVar< Float	>	i_blendTimer;
	TInstanceVar< Int32 >		i_blendType; // -1 - invalid, 0 - blend in , 1 - blend out
	TInstanceVar< Int32 >		i_firstBoneIndex;
	TInstanceVar< Matrix >	i_firstBoneWS;
	TInstanceVar< TGenericPtr >	i_listener;
	TInstanceVar< CAllocatedBehaviorGraphOutput > i_pose;

public:
	CBehaviorGraphPoseSlotNode();

	virtual void OnPropertyPostChange( IProperty* property );

	virtual String GetCaption() const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	void SetPose( CBehaviorGraphInstance& instance, const CAnimatedComponent* componentWithPose, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL ) const;
#ifdef USE_HAVOK_ANIMATION
	void SetPose( CBehaviorGraphInstance& instance, const TDynArray< hkQsTransform >&poseLS, const TDynArray< Float >& floatTracks, const Matrix& localToWorld, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL ) const;
#else
	void SetPose( CBehaviorGraphInstance& instance, const TDynArray< RedQsTransform >&poseLS, const TDynArray< Float >& floatTracks, const Matrix& localToWorld, Float blendTime, EBlendType type, IPoseSlotListener* l = NULL ) const;
#endif
	void ResetPose( CBehaviorGraphInstance& instance ) const;

	Bool IsSlotActive( const CBehaviorGraphInstance& instance ) const;

	const CName& GetSlotName() const;

protected:
	void CreatePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
	void CachePose( CBehaviorGraphInstance& instance, const CAnimatedComponent* componentWithPoseLS ) const;
#ifdef USE_HAVOK_ANIMATION
	void CachePose( CBehaviorGraphInstance& instance, const TDynArray< hkQsTransform >&poseLS, const TDynArray< Float >& floatTracks ) const;
#else
	void CachePose( CBehaviorGraphInstance& instance, const TDynArray< RedQsTransform >&poseLS, const TDynArray< Float >& floatTracks ) const;
#endif
	IPoseSlotListener* GetListener( CBehaviorGraphInstance& instance ) const;
	void SetListener( CBehaviorGraphInstance& instance, IPoseSlotListener* l ) const;

	void FinishBlending( CBehaviorGraphInstance& instance ) const;

	Float GetWeight( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	void BlendBoneInWS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &pose, Int32 bone, const hkQsTransform& boneBWS, Float weight ) const;
#else
	void BlendBoneInWS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &pose, Int32 bone, const RedQsTransform& boneBWS, Float weight ) const;
#endif
#ifdef USE_HAVOK_ANIMATION
	RED_INLINE void BlendBoneInLS( hkQsTransform& boneA, hkQsTransform& boneB, Float weight ) const
	{
		boneA.setInterpolate4( boneA, boneB, weight );
	}
#else
	RED_INLINE void BlendBoneInLS( RedQsTransform& boneA, RedQsTransform& boneB, Float weight ) const
	{
		boneA.Lerp( boneA, boneB, weight );
	}
#endif

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseSlotNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_RO( m_slotName, TXT("Slot name - use for set slot pose") );
	PROPERTY_CUSTOM_EDIT( m_firstBone, TXT("First bone - default bone with 0 index"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_worldSpace, TXT("Blend first bone in world space") );
	PROPERTY_EDIT( m_interpolation, TXT("Interpolation type") );
	PROPERTY_EDIT( m_blendFloatTracks, TXT("Blend float tracks") );
	PROPERTY_EDIT( m_ignoreZeroFloatTracks, TXT("Zero float track will be ignored in blending process") );
END_CLASS_RTTI();
