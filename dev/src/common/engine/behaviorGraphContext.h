/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behaviorIncludes.h"
#include "../engine/pose.h"
#include "Behavior/Tools/stackAllocator.h"
#include "poseHandle.h"

class IAnimatedObjectInterface;
class CSkeleton;
class CPoseProvider;

// All synced or not thread-safe actions should be moved to IAnimationPostAction
class IAnimationPostAction
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

public:
	IAnimationPostAction() {}
	virtual ~IAnimationPostAction() {}

	virtual void Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose ) = 0;
};

class CAnimationPostActionList
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

	TDynArray< IAnimationPostAction* > m_actions;

public:
	CAnimationPostActionList();
	~CAnimationPostActionList();

	// Add new post action
	void Add( IAnimationPostAction* action );

	// Process all actions and clear list
	void Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose );

	// Clear list
	void Clear();

	// Is list empty
	Bool Empty() const;
};

//////////////////////////////////////////////////////////////////////////

struct SBehaviorUpdateContext
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

	Uint32								m_updateID;
	EBehaviorLod						m_lod;
	CAnimationPostActionList			m_postActions;
	BehaviorGraphTools::StackAllocator	m_oneFrameAllocator;

	SBehaviorUpdateContext();

	RED_INLINE void SetLodLevel( EBehaviorLod lod ) { m_lod = lod; }

	void PrepareForUpdate();

	// Clear post actions
	void ClearPostActions() { m_postActions.Clear(); }

	// Process pose actions
	void ProcessPostActions( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose ) { ASSERT( ::SIsMainThread() ); m_postActions.Process( ac, pose ); }

public: // For users

	// Add post action
	void AddPostAction( IAnimationPostAction* action ) { m_postActions.Add( action ); }

	RED_INLINE EBehaviorLod GetLodLevel() const { return m_lod; }

	// One frame allocator - object exist only for one frame, and then are all deleted.
	// Allocation and deallocation is superfast.
	// WARNING! - when object are released, dtors are not called!
	BehaviorGraphTools::StackAllocator& GetOneFrameAllocator() { return m_oneFrameAllocator; }
};

//////////////////////////////////////////////////////////////////////////

struct SMimicPostProcessData
{
	//AnimQsTransform			m_mimicBones[ MIMIC_POSE_BONES_NUM ];
	Float					m_mimicAreas[ MIMIC_POSE_AREAS_NUM ];
	//Float					m_mimicLipsyncOffset;
};

struct SBehaviorSampleContext
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Animation, MC_Animation );

	struct SBoneCorrection
	{
		Int32					m_bone;
		AnimQsTransform			m_transform;
	};

public:
	SBehaviorSampleContext();
	~SBehaviorSampleContext();

	Bool Init( const IAnimatedObjectInterface* owner, CSkeleton* poseSkeleton, CSkeleton* mimicSkeleton );
	void Deinit();

	SBehaviorGraphOutput& ForcePoseLocalSpace( const SBehaviorGraphOutput& pose );

	// Prepare for sample
	SBehaviorGraphOutput& PrepareForSample( Bool forceTPose = true );

	// Prepare for sample constraints
	SBehaviorGraphOutput& PrepareForSampleConstraints( SBehaviorGraphOutput& output );

	// Get sampled pose (or constrained)
	SBehaviorGraphOutput& GetSampledPose();
	const SBehaviorGraphOutput& GetSampledPose() const;

	// Get main pose
	SBehaviorGraphOutput& GetMainPose();
	const SBehaviorGraphOutput& GetMainPose() const;

	// Get ore create frozen pose
	void CreateAndCacheFrozenPose();
	SBehaviorGraphOutput* GetFrozenPose();
	void ReleaseFrozenPose();
	Bool HasFrozenPose() const;

	// Complete sample
	void CompleteSample();

	// Complete sample constraints
	void CompleteSampleConstraints();

public:
	// Add post action
	void AddPostAction( IAnimationPostAction* action ) { m_postActions.Add( action ); }

	// Clear post actions
	void ClearPostActions() { m_postActions.Clear(); }

	// Process post actions
	void ProcessPostActions( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose ) { ASSERT( ::SIsMainThread() ); m_postActions.Process( ac, pose ); }

	// Get post actions
	const CAnimationPostActionList & GetPostActions() const { return m_postActions; }

public:
	// Cache pose from prev sampling
	void CachePoseFromPrevSampling();

	// Get pose from prev sampling
	const SBehaviorGraphOutput& GetPoseFromPrevSampling() const;

	// Cache pose for last full update and sample - used for rare ticking, stores prev as last
	void CachePoseForLastFullUpdateAndSample();

	// Forces prev to be same as current last
	void ForcePrevToBeLastFullUpdateAndSample();

	// Restore pose from last full update and sample - used for rare ticking
	SBehaviorGraphOutput& RestorePoseFromLastFullUpdateAndSample();

	// Get pose from last full update and sample - used for rare ticking
	const SBehaviorGraphOutput& GetPoseFromLastFullUpdateAndSample() const;

	// Get pose from prev full update and sample - used for rare ticking
	const SBehaviorGraphOutput& GetPoseFromPrevFullUpdateAndSample() const;

	// Poses from cache
	CPoseHandle					GetPose();
	CPoseHandle					GetMimicPose();
	const SBehaviorGraphOutput& GetMimicFace( Uint32 num );

	// Poses allocator
	CPoseProvider*				GetPoseProvider() const { return m_poseAlloc; }
	CPoseProvider*				GetMimicProvider() const { return HasMimic()? m_mimicAlloc : nullptr; }

	SMimicPostProcessData*		GetMimicPostProcessData();

	void ForceTPose();
	void SetTPose( SBehaviorGraphOutput* pose, Bool mimic ) const;

public:
	// Is main pose TPose
	Bool IsMainPoseTPose() const;

	// Has animation event occurred
	Bool HasAnimationEventOccurred( const CName& animEventName ) const;

	// Get bone in local space
	Matrix			GetBoneMatrixLocalSpace( Uint32 boneIndex ) const;
	AnimQsTransform	GetBoneTransformLocalSpace( Uint32 boneIndex ) const;
	Bool			GetBoneTransformLocalSpaceRef( Uint32 boneIndex, AnimQsTransform& transform ) const;

	// Get float tracks
	Uint32  GetFloatTrackNum() const;
	Float GetFloatTrack( Uint32 trackIndex ) const;

	Uint32 GetAnimEventsNum() const;

	Bool HasMimic() const;
	Bool IsValid() const;

public:
	RED_INLINE EBehaviorLod	GetLodLevel() const { return m_lod; }
	RED_INLINE void			SetLodLevel( EBehaviorLod lod ) { m_lod = lod; }

	RED_INLINE Uint32			GetSampledID() const { return m_sampledID; }

public:
	RED_INLINE Bool ShouldCorrectPose() const	{ return m_boneCorrections.Size() > 0; }

	void SetPoseCorrection( SBehaviorGraphOutput& pose ) const;
	void SetPoseCorrectionIdentity( SBehaviorGraphOutput& pose ) const;
	void SetupBoneCorrection( const CAnimatedComponent* parent, const BoneMappingContainer& bones );
	void SetupBoneCorrection( const SBehaviorGraphOutput& parentPose, const BoneMappingContainer& bones );
	void ResetBoneCorrection();

	// One frame allocator - object exist only for one frame, and then are all deleted.
	// Allocation and deallocation is superfast.
	// WARNING! - when object are released, dtors are not called!
	BehaviorGraphTools::StackAllocator& GetOneFrameAllocator() { return m_oneFrameAllocator; }

protected:
	void ResetPose( SBehaviorGraphOutput* pose, Bool forceTPose = true ) const;

	void CreateMimicPostProcessData();
	void ResetMimicPostProcessData();

protected:
	EBehaviorLod						m_lod;
	Uint32								m_sampledID;

	CPose*								m_pose;

	//SBehaviorGraphOutput*				m_mainPose;
	CPoseHandle							m_rememberedPose;
	CPoseHandle							m_constraintedPose;
	CPoseHandle							m_lastFullUpdateAndSamplePose; // last is last done
	CPoseHandle							m_prevFullUpdateAndSamplePose; // prev is the one that was done just before last
	CPoseHandle							m_frozenPose;

	const IAnimatedObjectInterface*		m_owner;

	CSkeleton*							m_poseSkeleton;
	CSkeleton*							m_mimicSkeleton;

	CPoseProvider*						m_poseAlloc;
	CPoseProvider*						m_mimicAlloc;

	SMimicPostProcessData*				m_mimicPostProcessData;

	TDynArray< SBoneCorrection >		m_boneCorrections;

	Bool								m_isConstrainted;

	CAnimationPostActionList			m_postActions;

	BehaviorGraphTools::StackAllocator	m_oneFrameAllocator;
};

//////////////////////////////////////////////////////////////////////////
// TODO: move this to separate file!
class CAnimationEffectAction : public IAnimationPostAction
{
public:
	CName	m_effect;
	Float	m_time;

	CAnimationEffectAction( const CName& effect, Float time );
	CAnimationEffectAction() : m_time(0), m_effect() {}

	virtual void Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose );
};

//////////////////////////////////////////////////////////////////////////

class CAnimationMorphAction : public IAnimationPostAction
{
public:
	Float	m_value;

	CAnimationMorphAction();

	virtual void Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose );
};

//////////////////////////////////////////////////////////////////////////
