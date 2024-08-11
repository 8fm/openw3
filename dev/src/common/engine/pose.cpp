/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphOutput.h"
#include "pose.h"
#include "animationEvent.h"
#include "extAnimEvent.h"
#include "skeleton.h"
#include "poseProvider.h"

#define ANIM_ASSERT( x ) RED_ASSERT( x )
#define ANIM_FATAL_ASSERT( x, y ) RED_FATAL_ASSERT( x, y )

//#pragma optimize("",off)

Uint32 CPose::GetMemSize( const Uint32 numBones )
{
	return ( sizeof( Uint8 ) + 2 * sizeof( AnimQsTransform ) ) * numBones;
}

CPose::CPose( const CSkeleton* skeleton )
	: m_skeleton( skeleton )
	, m_modelSync( false )
	, m_localSync( false )
{
	ANIM_FATAL_ASSERT( m_skeleton, "Cannot initialize CPose without CSkeleton." );
	ANIM_FATAL_ASSERT( skeleton->GetPoseProvider(), "Cannot initialize CPose without CSkeleton Pose Provider." );

	const Int32 numBones = m_skeleton->GetBonesNum();

	// Alloc poses
	m_poseLS = m_skeleton->GetPoseProvider()->AcquirePose();
	m_poseMS = m_skeleton->GetPoseProvider()->AcquirePose();

	// Reset bone flags
	m_boneFlags.Resize( numBones );
	Red::System::MemorySet( m_boneFlags.TypedData(), 0, m_boneFlags.DataSize() );

	ANIM_ASSERT( m_boneFlags.Size() == m_poseLS->m_numBones && m_poseLS->m_numBones == m_poseMS->m_numBones );
}

Bool CPose::IsOk() const
{
	return m_skeleton && m_poseLS && m_poseMS;
}

Bool CPose::IsSyncedLS() const
{
	return m_localSync;
}

Bool CPose::IsSyncedMS() const
{
	return m_modelSync;
}

void CPose::ResetPoseLS( Bool forceTPose )
{
	if ( forceTPose )
	{
		SetTPoseLS();
		return;
	}

	ResetTracksEventsAndAnims();
}

void CPose::SetTPoseLS()
{
	RED_ASSERT( m_skeleton );
	RED_ASSERT( m_poseLS );

	const Int32 numBones = m_skeleton->GetBonesNum();
	const AnimQsTransform* refPoseLS = m_skeleton->GetReferencePoseLS();
	SetPoseLS( refPoseLS, numBones );
}

void CPose::SetPoseLS( const AnimQsTransform* poseLS, Uint32 _numBones )
{
	ANIM_FATAL_ASSERT( m_poseLS, "" );
	ANIM_FATAL_ASSERT( poseLS, "" );

	// Bones
	const Uint32 numBones = Min( m_poseLS->m_numBones, _numBones );
	for ( Uint32 i=0; i<numBones; ++i )
	{
		m_poseLS->m_outputPose[i] = poseLS[i];
	}

	// Flags
	const Uint32 numFlags = m_boneFlags.Size();
	for ( Uint32 i = 0; i < numFlags; ++i )
	{
		m_boneFlags[ i ] = PBF_ModelDirty;
	}

	m_localSync = true;
	m_modelSync = false;

	ResetTracksEventsAndAnims();

	ANIM_ASSERT( ValidatePoseData() );
}

void CPose::SetPoseMS( const AnimQsTransform* poseMS, Uint32 _numBones )
{
	ANIM_FATAL_ASSERT( m_poseMS, "" );
	ANIM_FATAL_ASSERT( poseMS, "" );

	// Bones
	const Uint32 numBones = Min( m_poseMS->m_numBones, _numBones );
	for ( Uint32 i=0; i<numBones; ++i )
	{
		m_poseMS->m_outputPose[i] = poseMS[i];
	}

	// Flags
	const Uint32 numFlags = m_boneFlags.Size();
	for ( Uint32 i = 0; i < numFlags; ++i )
	{
		m_boneFlags[ i ] = PBF_LocalDirty;
	}

	m_localSync = false;
	m_modelSync = true;

	ResetTracksEventsAndAnims();

	ANIM_ASSERT( ValidatePoseData() );
}

void CPose::ResetTracksEventsAndAnims()
{
	// We do not use extra data for MS pose

	// Custom tracks
	static_assert( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5, "Invalid number of float tracks. Should be 5" );
	m_poseLS->m_customFloatTracks[ 0 ] = 0.f;
	m_poseLS->m_customFloatTracks[ 1 ] = 0.f;
	m_poseLS->m_customFloatTracks[ 2 ] = 0.f;
	m_poseLS->m_customFloatTracks[ 3 ] = 0.f;
	m_poseLS->m_customFloatTracks[ 4 ] = 0.f;

	// Tracks
	const Uint32 numTracks = m_poseLS->m_numFloatTracks;
	for ( Uint32 i = 0; i < numTracks; i++ )
	{
		m_poseLS->m_floatTracks[ i ] = 0.f;
	}

	// Clear events and used animations
	m_poseLS->ClearEventsAndUsedAnims();

	// Clear delta reference
	m_poseLS->m_deltaReferenceFrameLocal.SetIdentity();
}

Bool CPose::ValidatePoseData() const
{
	const Int32 numBones = m_skeleton->GetBonesNum();
	for ( Int32 i = 0; i < numBones; i++ )
	{
		if ( HasFlag( i, PBF_LocalDirty ) && HasFlag( i, PBF_ModelDirty ) )
		{
			return false;
		}
	}

	if ( m_localSync )
	{
		for ( Int32 i = 0; i < numBones; i++ )
		{
			if ( HasFlag( i, PBF_LocalDirty ) )
			{
				return false;
			}
		}
	}
	if ( m_modelSync )
	{
		for ( Int32 i = 0; i < numBones; i++ )
		{
			if ( HasFlag( i, PBF_ModelDirty ) )
			{
				return false;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

const AnimQsTransform& CPose::CalcBoneMSFromLS( const Int32 boneIdx ) const
{
	ANIM_FATAL_ASSERT( CheckFlagIsClear( PBF_InternalCalcMS ), "" );
	ANIM_FATAL_ASSERT( HasFlag( boneIdx, PBF_ModelDirty ), "" );
	ANIM_FATAL_ASSERT( !HasFlag( boneIdx, PBF_LocalDirty ), "" );

	Int32 firstBoneInChainIdx = boneIdx;
	const Int16* const parentIndices = m_skeleton->GetParentIndices();

	// 1) Searching for first predecessor with NO PBF_ModelDirty flag, marking bones on the way with PBF_InternalCalcMS flag.
	while ( true )
	{
		if ( !HasFlag( firstBoneInChainIdx, PBF_ModelDirty ) )
		{
			break;
		}

		const Int16 parentIdx = parentIndices[ firstBoneInChainIdx ];
		if ( parentIdx == -1 )
		{
			ANIM_ASSERT( !HasFlag( firstBoneInChainIdx, PBF_LocalDirty ) );
			m_poseMS->m_outputPose[ firstBoneInChainIdx ] = m_poseLS->m_outputPose[ firstBoneInChainIdx ];
			ClearFlag( firstBoneInChainIdx, PBF_ModelDirty );
			break;
		}

		SetFlag( firstBoneInChainIdx, PBF_InternalCalcMS );
		firstBoneInChainIdx = parentIdx;
	}

	ANIM_FATAL_ASSERT( !HasFlag( firstBoneInChainIdx, PBF_ModelDirty ), "" );

	// 2) Recalculating for all bones marked with PBF_InternalCalcMS flag starting from first model clear bone.
	for ( Int32 i = firstBoneInChainIdx + 1; i <= boneIdx; ++i )
	{
		if ( !HasFlag( i, PBF_InternalCalcMS ) )
		{
			continue;
		}

		const Int16 parentIdx = parentIndices[ i ];
		ANIM_FATAL_ASSERT( !HasFlag( i, PBF_LocalDirty ), "" );
		ANIM_FATAL_ASSERT( !HasFlag( parentIdx, PBF_ModelDirty ), "" );
		const AnimQsTransform& parentMS = m_poseMS->m_outputPose[ parentIdx ];

		m_poseMS->m_outputPose[ i ].SetMul( parentMS, m_poseLS->m_outputPose[ i ] );

		Uint8 f = m_boneFlags[ i ];
		ClearFlagExplicit( f, PBF_InternalCalcMS );
		ClearFlagExplicit( f, PBF_ModelDirty );
		m_boneFlags[ i ] = f;
	}

	ANIM_ASSERT( !HasFlag( boneIdx, PBF_ModelDirty ) );
	ANIM_ASSERT( ValidatePoseData() );
	ANIM_ASSERT( CheckFlagIsClear( PBF_InternalCalcMS ) );

	return m_poseMS->m_outputPose[ boneIdx ];
}

void CPose::SyncMSFromLS()
{
	if ( m_modelSync )
	{
		return;
	}

	const Int16* const parentIndices = m_skeleton->GetParentIndices();

	const Int32 numBones = m_poseLS->m_numBones;
	for ( int i=0; i<numBones; ++i )
	{
		if ( HasFlag( i, PBF_ModelDirty ) )
		{
			const AnimQsTransform& boneLS = m_poseLS->m_outputPose[ i ];
			ANIM_ASSERT( !HasFlag( i, PBF_LocalDirty ) );

			const Int16 parentIdx = parentIndices[ i ];
			if ( parentIdx != -1 )
			{
				ANIM_ASSERT( !HasFlag( parentIdx, PBF_ModelDirty ) );
				const AnimQsTransform& parentMS = m_poseMS->m_outputPose[ parentIdx ];

				m_poseMS->m_outputPose[ i ].SetMul( parentMS, boneLS );
			}
			else
			{
				m_poseMS->m_outputPose[ i ] = boneLS;
			}

			ClearFlag( i, PBF_ModelDirty );

		}
	}

	m_modelSync = true;

	ANIM_ASSERT( ValidatePoseData() );
}

const AnimQsTransform& CPose::CalcBoneLSFromMS( const Int32 boneIdx ) const
{
	ANIM_FATAL_ASSERT( !HasFlag( boneIdx, PBF_ModelDirty ), "" );

	const Int16* const parentIndices = m_skeleton->GetParentIndices();

	const AnimQsTransform& boneMS = m_poseMS->m_outputPose[ boneIdx ];

	const Int16 parentIdx = parentIndices[ boneIdx ];
	if ( parentIdx != -1 )
	{
		const AnimQsTransform& parentMS = GetBoneMS( parentIdx );

		m_poseLS->m_outputPose[ boneIdx ].SetMulInverseMul( parentMS, boneMS );

	}
	else
	{
		m_poseLS->m_outputPose[ boneIdx ] = boneMS;
	}

	ClearFlag( boneIdx, PBF_LocalDirty );
	ANIM_ASSERT( !HasFlag( boneIdx, PBF_ModelDirty ) );
	ANIM_ASSERT( ValidatePoseData() );

	return m_poseLS->m_outputPose[ boneIdx ];
}

void CPose::SyncLSFromMS()
{
	if ( m_localSync ) 
	{
		return;
	}

	const Int16* const parentIndices = m_skeleton->GetParentIndices();

	const Int32 numBones = m_poseLS->m_numBones;
	for ( Int32 i=0; i<numBones; ++i )
	{
		if ( HasFlag( i, PBF_LocalDirty ) )
		{
			ANIM_ASSERT( !HasFlag( i, PBF_ModelDirty ) );
			const AnimQsTransform& boneMS = m_poseMS->m_outputPose[ i ];

			const Int16 parentIdx = parentIndices[ i ];
			if ( parentIdx != -1 )
			{
				const AnimQsTransform& parentMS = GetBoneMS( parentIdx );
				ANIM_FATAL_ASSERT( !HasFlag( parentIdx, PBF_ModelDirty ), "" );

				m_poseLS->m_outputPose[ i ].SetMulInverseMul( parentMS, boneMS );
			}
			else
			{
				m_poseLS->m_outputPose[ i ] = boneMS;
			}

			ClearFlag( i, PBF_LocalDirty );
		}
	}

	m_localSync = true;

	ANIM_ASSERT( ValidatePoseData() );
}

void CPose::SyncAll()
{
	SyncLSFromMS();
	SyncMSFromLS();
}

void CPose::MakeFirstChildrenModelSpace( Int32 boneIdx )
{
	const Int16* const parentIndices = m_skeleton->GetParentIndices();

	const Int32 numBones = m_poseMS->m_numBones;
	for ( Int32 i = boneIdx + 1; i < numBones; ++i )
	{
		const Int16 parentIdx = parentIndices[ i ];
		if ( parentIdx == boneIdx )
		{
			GetBoneMS( i ); // sync model
			SetFlag( i, PBF_LocalDirty );
			m_localSync = false;
		}
	}
}

void CPose::MakeAllChildrenLocalSpace( const Int32 boneIdx )
{
	ANIM_ASSERT( CheckFlagIsClear( PBF_InternalLS ) );

	const Int16* const parentIndices = m_skeleton->GetParentIndices();
	const Int32 numBones = m_poseLS->m_numBones;

	SetFlag( boneIdx, PBF_InternalLS );
	for ( Int32 i = boneIdx + 1; i < numBones; ++i )
	{
		const Int16 parentIdx = parentIndices[ i ];
		if ( parentIdx != -1 && HasFlag( parentIdx, PBF_InternalLS ) )
		{
			GetBoneLS( i ); // sync local
			SetFlag( i, PBF_InternalLS );
			m_modelSync = false;
		}
	} 

	for ( Int32 i = boneIdx + 1; i < numBones; ++i )
	{
		Uint8 f = m_boneFlags[ i ];
		if ( HasFlagExplicit( f, PBF_InternalLS ) )
		{
			SetFlagExplicit( f, PBF_ModelDirty );
			ClearFlagExplicit( f, PBF_InternalLS );
			m_boneFlags[ i ] = f;
		}
	}
	ClearFlag( boneIdx, PBF_InternalLS );

	ANIM_ASSERT( CheckFlagIsClear( PBF_InternalLS ) );
	ANIM_ASSERT( ValidatePoseData() );
}

//////////////////////////////////////////////////////////////////////////

Bool CPose::HasFlag( Int32 boneIdx, EPoseBoneFlag flag ) const
{
	return 0 != ( m_boneFlags[ boneIdx ] & flag );
}

void CPose::SetFlag( Int32 boneIdx, EPoseBoneFlag flag ) const
{
	m_boneFlags[ boneIdx ] |= flag;
}

void CPose::ClearFlag( Int32 boneIdx, EPoseBoneFlag flag ) const
{
	m_boneFlags[ boneIdx ] &= ~flag;
}

Bool CPose::CheckFlagIsClear( EPoseBoneFlag flag ) const
{
	for ( Uint32 i = 0; i < m_boneFlags.Size(); i++ )
	{
		if ( HasFlag( i, flag ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPose::HasFlagExplicit( Uint8 f, EPoseBoneFlag flag ) const
{
	return 0 != ( f & flag );
}

void CPose::SetFlagExplicit( Uint8& f, EPoseBoneFlag flag ) const
{
	f |= flag;
}

void CPose::ClearFlagExplicit( Uint8& f, EPoseBoneFlag flag ) const
{
	f &= ~flag;
}

//////////////////////////////////////////////////////////////////////////

const AnimQsTransform& CPose::GetBoneLS( const Int32 boneIdx ) const
{
	RED_FATAL_ASSERT( boneIdx >= 0 && boneIdx < (Int32)m_poseLS->m_numBones, "Incorrect bone index" );

	if ( !HasFlag( boneIdx, PBF_LocalDirty ) )
	{
		return m_poseLS->m_outputPose[ boneIdx ];
	}
	else
	{
		return CalcBoneLSFromMS( boneIdx );
	}
}

const AnimQsTransform& CPose::GetBoneMS( const Int32 boneIdx ) const
{
	RED_FATAL_ASSERT( boneIdx >= 0 && boneIdx < (Int32)m_poseMS->m_numBones, "Incorrect bone index" );

	if ( !HasFlag( boneIdx, PBF_ModelDirty ) )
	{
		return m_poseMS->m_outputPose[ boneIdx ];
	}
	else
	{
		return CalcBoneMSFromLS( boneIdx );
	}
}

AnimQsTransform& CPose::AccessBoneLS( const Int32 boneIdx )
{
	MakeAllChildrenLocalSpace( boneIdx );

	GetBoneLS( boneIdx ); // Sync local

	SetFlag( boneIdx, PBF_ModelDirty );
	m_modelSync = false;

	ANIM_ASSERT( ValidatePoseData() );

	return m_poseLS->m_outputPose[ boneIdx ];
}

AnimQsTransform& CPose::AccessBoneMS( const Int32 boneIdx, const Bool propagate )
{
	if ( propagate )
	{
		MakeAllChildrenLocalSpace( boneIdx );
	}
	else
	{
		MakeFirstChildrenModelSpace( boneIdx );
	}

	GetBoneMS( boneIdx ); // Sync modal

	SetFlag( boneIdx, PBF_LocalDirty );
	m_localSync = false;

	ANIM_ASSERT( ValidatePoseData() );

	return m_poseMS->m_outputPose[ boneIdx ];
}

SBehaviorGraphOutput* CPose::AccessSyncedPoseLS()
{
	SyncLSFromMS();

	return AccessUnsyncedPoseLS();
}

SBehaviorGraphOutput* CPose::AccessSyncedPoseMS()
{
	SyncMSFromLS();

	return AccessUnsyncedPoseMS();
}

SBehaviorGraphOutput* CPose::AccessUnsyncedPoseLS()
{
	// Bones
	for ( Uint32 i = 0; i < m_poseLS->m_numBones; ++i )
	{
		ClearFlag( i, PBF_LocalDirty );
		SetFlag( i, PBF_ModelDirty );
	}

	// Set flags
	m_modelSync = false;
	m_localSync = true;

	ANIM_ASSERT( ValidatePoseData() );

	return m_poseLS.Get();
}

SBehaviorGraphOutput* CPose::AccessUnsyncedPoseMS()
{
	// Bones
	for ( Uint32 i = 0; i < m_poseMS->m_numBones; ++i )
	{
		ClearFlag( i, PBF_ModelDirty );
		SetFlag( i, PBF_LocalDirty );
	}

	// Set flags
	m_modelSync = true;
	m_localSync = false; 

	ANIM_ASSERT( ValidatePoseData() );

	return m_poseMS.Get();
}

void CPose::SetBoneLS( Int32 boneIdx, const AnimQsTransform& boneLS, Bool propagate )
{
	ANIM_FATAL_ASSERT( boneIdx >= 0 && boneIdx < (Int32)m_poseLS->m_numBones, "Incorrect bone index" );

	if ( propagate )
	{
		MakeAllChildrenLocalSpace( boneIdx );
	}
	else
	{
		MakeFirstChildrenModelSpace( boneIdx );
	}

	m_poseLS->m_outputPose[ boneIdx ] = boneLS;

	ClearFlag( boneIdx, PBF_LocalDirty );
	SetFlag( boneIdx, PBF_ModelDirty );

	m_modelSync = false;

	ASSERT( ValidatePoseData() );
}

void CPose::SetBoneMS( Int32 boneIdx, const AnimQsTransform& boneMS, Bool propagate )
{
	RED_FATAL_ASSERT( boneIdx >= 0 && boneIdx < (Int32)m_poseMS->m_numBones, "Incorrect bone index" );

	if ( propagate )
	{
		MakeAllChildrenLocalSpace( boneIdx );
	}
	else
	{
		MakeFirstChildrenModelSpace( boneIdx );
	}

	m_poseMS->m_outputPose[ boneIdx ] = boneMS;

	SetFlag( boneIdx, PBF_LocalDirty );
	ClearFlag( boneIdx, PBF_ModelDirty );

	m_localSync = false;

	ASSERT( ValidatePoseData() );
}

//////////////////////////////////////////////////////////////////////////

Uint32 CPose::GetBonesNum() const
{
	return m_boneFlags.Size();
}

Uint32 CPose::GetAnimEventsNum() const
{
	return m_poseLS->m_numEventsFired;
}

Uint32 CPose::GetFloatTrackNum() const
{
	return m_poseLS->m_numFloatTracks;
}

Float CPose::GetFloatTrack( Uint32 track ) const
{
	RED_FATAL_ASSERT( track >= 0 && track < m_poseLS->m_numFloatTracks, "Incorrect track index" );
	return m_poseLS->m_floatTracks[ track ];
}

Bool CPose::HasAnimationEventOccurred( const CName& animEventName ) const
{
	ASSERT( m_poseLS );
	const Uint32 size = m_poseLS->m_numEventsFired;
	for ( Uint32 i = 0; i < size; ++i )
	{
		if ( m_poseLS->m_eventsFired[ i ].m_extEvent->GetEventName() == animEventName )
		{
			return true;
		}
	}
	return false;
}

//#pragma optimize("",on)
