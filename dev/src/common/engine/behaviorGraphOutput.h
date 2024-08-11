/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

//#define TPOSE_DETECTOR

//#define DEBUG_ANIMS

//#define USE_DEBUG_ANIM_POSES

#ifdef USE_DEBUG_ANIM_POSES

void DAP_CallMyPlaceForBreakpoint( const Char* msg, const Char* msgFile, const Uint32 lineNum );

#define DEBUG_ANIM_POSES( pose )			{ Bool isOk( true ); for ( Uint32 i=0; i<pose.m_numBones; ++i ) { if ( !pose.m_outputPose[i].IsOk() ) { isOk = false; break; } } if ( !isOk ) DAP_CallMyPlaceForBreakpoint( TXT("Anim pose is not ok"), MACRO_TXT( __FILE__ ), __LINE__ ); }
#define DEBUG_ANIM_TRANSFORM( transform )	if ( !transform.IsOk() ) { DAP_CallMyPlaceForBreakpoint( TXT("Anim transform is not ok"), MACRO_TXT( __FILE__ ), __LINE__ ); }
#define DEBUG_ANIM_MATRIX( matrix )			if ( !matrix.IsOk() ) { DAP_CallMyPlaceForBreakpoint( TXT("Anim matrix is not ok"), MACRO_TXT( __FILE__ ), __LINE__ ); }
#define DEBUG_ANIM_VECTOR( vec )			if ( !vec.IsOk() ) { DAP_CallMyPlaceForBreakpoint( TXT("Anim vector is not ok"), MACRO_TXT( __FILE__ ), __LINE__ ); }
#define DEBUG_ANIM_FLOAT( value )			if ( Red::Math::NumericalUtils::IsNan( value ) ) { DAP_CallMyPlaceForBreakpoint( TXT("Anim float is NaN"), MACRO_TXT( __FILE__ ), __LINE__ ); }

#else
#define DEBUG_ANIM_POSES( pose ) ;
#define DEBUG_ANIM_TRANSFORM( transform ) ;
#define DEBUG_ANIM_MATRIX( matrix ) ;
#define DEBUG_ANIM_VECTOR( vec ) ;
#define DEBUG_ANIM_FLOAT( value ) ;
#endif

class CStackAllocator;

struct SBehaviorUsedAnimationData
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_UsedAnimData );
public:
	const CSkeletalAnimationSetEntry*	m_animation;
	Float								m_currTime;
	Float								m_weight; // total weight (result of blending)
	Float								m_playbackSpeed;
	Bool								m_looped;
	Bool								m_firstUpdate;

public:
	SBehaviorUsedAnimationData()
	{ /* it may contain uninitialized data */ }

	SBehaviorUsedAnimationData(const CSkeletalAnimationSetEntry* animation, Float currTime, Float weight = 1.0f, Float playbackSpeed = 1.0f, Bool looped = false, Bool firstUpdate = false)
	:	m_animation( animation )
	,	m_currTime( currTime )
	,	m_weight( weight )
	,	m_playbackSpeed( playbackSpeed )
	,	m_looped( looped )
	,	m_firstUpdate( firstUpdate )
	{}

	Bool CanMergeWith( const SBehaviorUsedAnimationData& usedAnim ) const
	{
		return m_animation == usedAnim.m_animation;
	}

	void MergeWith( const SBehaviorUsedAnimationData& usedAnim, Float weight )
	{
		m_weight = ::Min<Float>( 1.0f, m_weight + usedAnim.m_weight * weight );
	}
};

struct SBehaviorUsedAnimationDataSet
{
	SBehaviorUsedAnimationDataSet( Uint32 slotsToReserve );

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_UsedAnimData );

	TDynArray< SBehaviorUsedAnimationData, MC_UsedAnimData, MemoryPool_Animation > m_usedAnims;

	Uint32 GetMemSize() const	{ return m_usedAnims.Capacity() * sizeof( SBehaviorUsedAnimationData ); }
	Uint32 GetNum() const		{ return m_usedAnims.Size(); }
	const SBehaviorUsedAnimationData* GetUsedData() const	{ return m_usedAnims.TypedData(); }

	SBehaviorUsedAnimationDataSet& operator=( const SBehaviorUsedAnimationDataSet& uads );
	Bool AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	void MergeUsedAnims( const SBehaviorUsedAnimationDataSet &a, const SBehaviorUsedAnimationDataSet &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnims( const SBehaviorUsedAnimationDataSet &a, Float aAnimWeightMul=1.0f );
	void ClearUsedAnims();
	const SBehaviorUsedAnimationData* FindWithHighestWeight() const;
};

struct SBehaviorUsedAnimations
{
	static const Uint32 k_usedAnimationDataToReserve = 0;		// Use this if the number of allocations from growing the arrays gets too high

	SBehaviorUsedAnimations( Uint32 usedAnimSlotsToReserve = k_usedAnimationDataToReserve )
		: m_anims( usedAnimSlotsToReserve )
		, m_overlayAnims( usedAnimSlotsToReserve )
		, m_additiveAnims( usedAnimSlotsToReserve )
	{
	}

	SBehaviorUsedAnimationDataSet	m_anims;
	SBehaviorUsedAnimationDataSet	m_overlayAnims;
	SBehaviorUsedAnimationDataSet	m_additiveAnims;

	Uint32 GetMemSize() const;

	bool IsUsingOnlyNormalAnims() const { return m_overlayAnims.GetNum() == 0 && m_additiveAnims.GetNum(); }

	SBehaviorUsedAnimations& operator=( const SBehaviorUsedAnimations& uads );
	Bool AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	Bool AppendUsedOverlayAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	Bool AppendUsedAdditiveAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	void MergeUsedAnims( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnims( const SBehaviorUsedAnimations &a, Float aAnimWeightMul=1.0f );
	void MergeUsedAnimsAsOverlays( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnimsAsOverlays( const SBehaviorUsedAnimations &a, Float aAnimWeightMul=1.0f );
	void MergeUsedAnimsAsAdditives( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnimsAsAdditives( const SBehaviorUsedAnimations &a, Float aAnimWeightMul=1.0f );
	void ClearUsedAnims();
};

const Uint32 c_eventFiredDefaultCount = 16;

struct SBehaviorGraphOutputParameter
{
	Uint32 boneCount;
	Uint32 floatTracksCount;
	AnimQsTransform* poseMemory;
	AnimFloat* floatTrackMemory;
	CAnimationEventFired * eventFiredMemory;
	Bool createEvents;
};

struct SBehaviorGraphOutput
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Animation, MC_Pose, __alignof( SBehaviorGraphOutput ) );

public:
	enum EFloatTrackType 
	{ 
		FTT_FOV, 
		FTT_DOF_Override, 
		FTT_DOF_FocusDistFar, 
		FTT_DOF_BlurDistFar, 
		FTT_DOF_Intensity,
		FTT_DOF_FocusDistNear,
		FTT_DOF_BlurDistNear,
		FFT_Last,
	};

	AnimQsTransform*					m_outputPose;
	AnimFloat*							m_floatTracks;
	CAnimationEventFired*				m_eventsFired;

	Uint32								m_numBones;
	Uint32								m_numFloatTracks;
	Uint32								m_numEventsFired;

	AnimQsTransform						m_deltaReferenceFrameLocal;

	SBehaviorUsedAnimations				m_usedAnims;

	static const Uint32					NUM_CUSTOM_FLOAT_TRACKS = 5;
	Float								m_customFloatTracks[ NUM_CUSTOM_FLOAT_TRACKS ];

	Red::Threads::CAtomic< Int32 >		m_refCount;

	Bool								m_ownPoseMemory;
	Bool								m_ownFloatTrackMemory;
	Bool								m_ownEventFiredMemory;
	Bool								m_tPose;

public:
	SBehaviorGraphOutput( Uint32 usedAnimSlotsToReserve = SBehaviorUsedAnimations::k_usedAnimationDataToReserve );
	RED_MOCKABLE ~SBehaviorGraphOutput();

	// DO NOT USE. For CPoseHandle use only.
	void AddRef();
	Int32 Release();

	RED_MOCKABLE void Init( Uint32 numBones, Uint32 numFloatTracks, Bool createEvents = true );
	void Init( const SBehaviorGraphOutputParameter & param );

	void Deinit();

	Bool IsTouched() const
	{
		return !m_tPose;
	}

	void Touch()
	{
		m_tPose = false;
	}

	Bool IsValid() const
	{
		return m_outputPose && m_eventsFired;
	}

	void SetInterpolate( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, float alpha );
	void SetInterpolateWithoutME( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, float alpha );

	void SetAddMul( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float weight );
	void SetAddMul( const SBehaviorGraphOutput &b, Float weight );

	void SetMulInv( const SBehaviorGraphOutput &a );

	void SetIdentity();
	void SetZero();

	void SetPose( const CSkeleton* skeleton, Bool resetTracks = true );
	void SetPose( const CAnimatedComponent* componentWithPoseLS );
	void SetPose( const SBehaviorGraphOutput& pose );
	void SetPoseFromBonesModelSpace( const CSkeleton *skeleton, const TDynArray< Matrix > & bonesMS );
	void InvertPose();

	void NormalizeRotations();

	void ExtractTrajectory( const CAnimatedComponent* animComponent );	
	void ExtractTrajectory( const CSkeleton* skeleton, Int32 bone );
	static void ExtractTrajectoryOn( const CAnimatedComponent* animComponent, AnimQsTransform* outputPose, Uint32 numBones );

	AnimQsTransform GetBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex, const Int16 *parentIndices ) const;
	AnimQsTransform GetBoneModelTransform( Int32 boneIndex, const Int16 *parentIndices ) const;
	AnimQsTransform GetBoneModelTransformWithOffset( Int32 boneIndex, const Int16 *parentIndices, const AnimQsTransform& offsetLS ) const;

	AnimQsTransform GetBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;
	AnimQsTransform GetBoneModelTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;

	AnimQsTransform GetParentBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;
	AnimQsTransform GetParentBoneModelTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;

	Matrix GetBoneWorldMatrix( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;
	Matrix GetBoneModelMatrix( const CAnimatedComponent *animComponent, Int32 boneIndex ) const;

	void GetBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<AnimQsTransform>& bonesMS, Int32 numBones = -1 ) const;
	void GetBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<Matrix>& bonesMS, Int32 numBones = -1 ) const;
	void GetBonesModelSpace( const CSkeleton *skeleton, TDynArray<AnimQsTransform>& bonesMS, Int32 numBones = -1 ) const;
	void GetBonesModelSpace( const CSkeleton *skeleton, TDynArray<Matrix>& bonesMS, Int32 numBones = -1 ) const;

	void FillPoseWithBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<Matrix>& bonesMS );
	void FillPoseWithBonesModelSpace( const CSkeleton *skeleton, TDynArray<Matrix>& bonesMS );

	//void CalcBonesWorldSpace( const CAnimatedComponent *animComponent, TDynArray<Matrix>& bonesWS ) const;

	// turn start into duration, remove ticks
	void AdvanceEvents();

	Bool AppendEvent( const CAnimationEventFired& eventFired, Float alpha=1.0f );
	Bool AppendEventAsOverlay( const CAnimationEventFired& eventFired, Float alpha=1.0f );
	void MergeEvents( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aEventAlphaMul=1.0f, Float bEventAlphaMul=1.0f );
	void MergeEventsAsOverlays( const SBehaviorGraphOutput &mainPose, const SBehaviorGraphOutput &overlayPose, Float aEventAlphaMul=1.0f, Float bEventAlphaMul=1.0f );
	void MergeEvents( const SBehaviorGraphOutput &a, Float aEventAlphaMul=1.0f );
	void MergeEventsAsOverlays( const SBehaviorGraphOutput &overlayPose, Float aEventAlphaMul=1.0f );
	void ClearEvents();

	Bool AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	Bool AppendUsedOverlayAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	Bool AppendUsedAdditiveAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight=1.0f );
	void MergeUsedAnims( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnims( const SBehaviorGraphOutput &a, Float aAnimWeightMul=1.0f );
	void MergeUsedAnimsAsOverlays( const SBehaviorGraphOutput &mainPose, const SBehaviorGraphOutput &overlayPose, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnimsAsOverlays( const SBehaviorGraphOutput &overlayPose, Float aAnimWeightMul=1.0f );
	void MergeUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aAnimWeightMul=1.0f, Float bAnimWeightMul=1.0f );
	void MergeUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, Float aAnimWeightMul=1.0f );
	void ClearUsedAnims();

	// convenient way to do both
	void MergeEventsAndUsedAnims( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aWeightMul=1.0f, Float bWeightMul=1.0f );
	void MergeEventsAndUsedAnims( const SBehaviorGraphOutput &a, Float aWeightMul=1.0f );
	void MergeEventsAndUsedAnimsAsOverlays( const SBehaviorGraphOutput &mainPose, const SBehaviorGraphOutput &overlayPose, Float aWeightMul=1.0f, Float bWeightMul=1.0f );
	void MergeEventsAndUsedAnimsAsOverlays( const SBehaviorGraphOutput &overlayPose, Float aWeightMul=1.0f );
	void MergeEventsAndUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aWeightMul=1.0f, Float bWeightMul=1.0f );
	void MergeEventsAndUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, Float aWeightMul=1.0f );
	void ClearEventsAndUsedAnims();

	RED_MOCKABLE void Reset( const CSkeleton* skeleton ); //< Resets output (also all events and other informations) and sets pose from skeleton.
	void Reset();							              //< Same as above, but all bones are set to identity.

	Uint32 GetMemSize() const;

public:
	void SetAddMulME( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float weight );
	void SetAddMulME( const SBehaviorGraphOutput &b, Float weight );

	void SetInterpolateME( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, float alpha );
	void SetEventFiredMemory( CAnimationEventFired * events );

public:
	SBehaviorGraphOutput& operator=( const SBehaviorGraphOutput& rhs );

private:
	SBehaviorGraphOutput( const SBehaviorGraphOutput& );

	void AllocateOutputPose();
	void AllocateFloatTracks();
	void AllocateEventsFired();

	void DeallocateOutputPose();
	void DeallocateFloatTracks();
	void DeallocateEventFired();
};

//////////////////////////////////////////////////////////////////////////
