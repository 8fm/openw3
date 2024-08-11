

#pragma once
#include "virtualAnimation.h"

#include "skeletalAnimationEntry.h"

class TCrInstance;

#ifndef NO_EDITOR
class VirtualAnimationMixerEdListener
{
public:
	virtual void OnAnimationTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) = 0;
	virtual void OnFKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) = 0;
	virtual void OnIKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) = 0;
	virtual void OnAllTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) = 0;
};
#endif

class VirtualAnimationMixer
{
public:
	struct ControlRigSetup
	{
		const TCrDefinition*	m_definition;
		TCrInstance*			m_instance;

		ControlRigSetup() : m_instance( NULL ), m_definition( NULL ) {}
	};

private:
	const IVirtualAnimationContainer*	m_container;

#ifndef NO_EDITOR
	VirtualAnimationMixerEdListener*	m_listener;
#endif

public:
	VirtualAnimationMixer( const IVirtualAnimationContainer* c );

	void CalcMinMaxTime( Float& min, Float& max ) const;
	AnimQsTransform GetMovementAtTime( Float time ) const;
	AnimQsTransform GetMovementBetweenTime( Float startTime, Float endTime ) const;

	Bool Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut, ControlRigSetup* csSetup ) const;
	Bool Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut, ControlRigSetup* csSetup ) const;

	Bool CalcBlendingParams( Float time, const VirtualAnimation& animation, Float& animTime, Float& animWeight ) const;
	Bool CalcBlendingParamsClamped( Float time, const VirtualAnimation& animation, Float& animTime, Float& animWeight ) const;

#ifndef NO_EDITOR
	void ConnectEditorListener( VirtualAnimationMixerEdListener* listener ) { m_listener = listener; }
#endif

private:
	void CalcMinMaxTime( Float& min, Float& max, EVirtualAnimationTrack track ) const;

	RED_INLINE Bool IsAnimValid( const VirtualAnimation& anim ) const				{ return anim.m_cachedAnimation != NULL && anim.m_cachedAnimation->GetAnimation() != NULL; }
	RED_INLINE Bool CanUseAnim( const VirtualAnimation& anim, Float time ) const	{ return anim.m_time <= time && anim.m_time + anim.GetDuration() - time >= -0.001f; }
	RED_INLINE Bool IsAnimInside( Float start, Float end, const VirtualAnimation& anim ) const { return start <= anim.m_time && end - anim.m_time + anim.GetDuration() >= -0.001f; }

	RED_INLINE Bool ShouldExtractBone( const VirtualAnimation& anim, Uint32 num ) const		{ return anim.m_boneToExtract != -1 && anim.m_boneToExtract < (Int32)num; }

	void ExtractBoneFromPose( Int32 boneToExtract, AnimQsTransform* bones, Uint32 num ) const;

	void BlendPosesAdditive( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight ) const;
	void BlendPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight ) const;

	void BlendPartialPosesAdditive( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones ) const;
	void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones ) const;
	void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones, const TDynArray< Float >& boneWeights ) const;

	Bool FindFkPoses( Float time, const VirtualAnimationPoseFK*& poseA, const VirtualAnimationPoseFK*& poseB, Float& weight ) const;
	void BlendFKPoses( AnimQsTransform* bonesOut, Uint32 boneNumIn, const VirtualAnimationPoseFK* poseA, const VirtualAnimationPoseFK* poseB, Float weight ) const;

	Bool FindIkPoses( Float time, const VirtualAnimationPoseIK*& poseA, const VirtualAnimationPoseIK*& poseB, Float& weight ) const;
	void BlendIKPoses( AnimQsTransform* bonesOut, Uint32 boneNumIn, const VirtualAnimationPoseIK* poseA, const VirtualAnimationPoseIK* poseB, Float weight, ControlRigSetup* csSetup ) const;
};
