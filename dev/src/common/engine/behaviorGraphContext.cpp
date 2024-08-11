/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"
#include "../engine/skeleton.h"
#include "../engine/entity.h"
#include "../core/profiler.h"
#include "poseProvider.h"
#include "morphedMeshComponent.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

static Int32 ONE_FRAME_ALLOCATOR_SIZE = 128;

SBehaviorUpdateContext::SBehaviorUpdateContext()
	: m_updateID( 0 )
	, m_lod( BL_Lod0 )
	, m_oneFrameAllocator()
{
	m_oneFrameAllocator.CreatePool( ONE_FRAME_ALLOCATOR_SIZE*2 );
};

//////////////////////////////////////////////////////////////
void SBehaviorUpdateContext::PrepareForUpdate()
{
	m_updateID++; 
	ClearPostActions(); 
	GetOneFrameAllocator().RemoveAllFromStack();
}

SBehaviorSampleContext::SBehaviorSampleContext()
	: m_owner( nullptr )
	, m_isConstrainted( false )
	, m_poseSkeleton( nullptr )
	, m_mimicSkeleton( nullptr )
	, m_mimicPostProcessData( nullptr )
	, m_poseAlloc( nullptr )
	, m_mimicAlloc( nullptr )
	, m_pose( nullptr )
	, m_sampledID( 0 )
	, m_oneFrameAllocator()
{
	m_oneFrameAllocator.CreatePool( ONE_FRAME_ALLOCATOR_SIZE );
}

SBehaviorSampleContext::~SBehaviorSampleContext()
{
	// Deinitialize poses
	Deinit();

	ASSERT( !m_pose );
	ASSERT( !m_rememberedPose );
	ASSERT( !m_constraintedPose );
	ASSERT( !m_lastFullUpdateAndSamplePose );
	ASSERT( !m_prevFullUpdateAndSamplePose );
	ASSERT( !m_frozenPose );
	ASSERT( !m_poseAlloc );
	ASSERT( !m_mimicAlloc );
}

Bool SBehaviorSampleContext::IsValid() const
{
	return m_owner && m_poseSkeleton && m_pose ? true : false;
}

Bool SBehaviorSampleContext::Init( const IAnimatedObjectInterface* owner, CSkeleton* poseSkeleton, CSkeleton* mimicSkeleton )
{
	ASSERT( !m_owner );

	// Force deinit
	Deinit();

	if ( poseSkeleton )
	{
		// Pose allocators
		m_poseAlloc = poseSkeleton->GetPoseProvider();
		m_mimicAlloc = mimicSkeleton ? mimicSkeleton->GetPoseProvider() : NULL;

		if ( !m_poseAlloc )
		{
			return false;
		}

		// Create pose
		m_pose = new CPose( poseSkeleton );
		m_pose->SetTPoseLS();
		
		// Pose skeleton
		m_poseSkeleton = poseSkeleton;

		// Remembered pose
		m_rememberedPose = m_poseAlloc->AcquirePose();
		ResetPose( m_rememberedPose.Get() );

		// Last & prev full Update and Sample pose
		m_lastFullUpdateAndSamplePose = m_poseAlloc->AcquirePose();
		ResetPose( m_lastFullUpdateAndSamplePose.Get() );
		m_prevFullUpdateAndSamplePose = m_poseAlloc->AcquirePose();
		ResetPose( m_prevFullUpdateAndSamplePose.Get() );
		
		// Mimics
		if ( mimicSkeleton )
		{
			m_mimicSkeleton = mimicSkeleton;

			// Create mimic areas
			if ( !m_mimicPostProcessData )
			{
				CreateMimicPostProcessData();
			}
			else
			{
				ASSERT( !m_mimicPostProcessData );
			}
		}

		m_owner = owner;
	}
	else
	{
		return false;
	}

	return true;
}

void SBehaviorSampleContext::Deinit()
{
	m_postActions.Clear();

	if ( m_pose )
	{
		delete m_pose;
		m_pose = NULL;
	}

	if ( m_poseAlloc )
	{
		m_rememberedPose.Reset();
		m_lastFullUpdateAndSamplePose.Reset();
		m_prevFullUpdateAndSamplePose.Reset();
		m_constraintedPose.Reset();
		m_frozenPose.Reset();
	}

	m_poseAlloc = NULL;
	m_mimicAlloc = NULL;

	m_poseSkeleton = NULL;
	m_mimicSkeleton = NULL;

	delete m_mimicPostProcessData;
	m_mimicPostProcessData = NULL;

	m_owner = NULL;
}

void SBehaviorSampleContext::ResetPose( SBehaviorGraphOutput* pose, Bool forceTPose ) const
{
	if ( forceTPose )
	{
		// Set reference pose ( TPose )
		pose->SetPose( m_poseSkeleton );
	}
	else
	{
		// Clear custom and float tracks
		COMPILE_ASSERT( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5 );
		pose->m_customFloatTracks[0] = 0.0f;
		pose->m_customFloatTracks[1] = 0.0f;
		pose->m_customFloatTracks[2] = 0.0f;
		pose->m_customFloatTracks[3] = 0.0f;
		pose->m_customFloatTracks[4] = 0.0f;

		for ( Uint32 i=0; i<pose->m_numFloatTracks; i++ )
		{
			pose->m_floatTracks[i] = 0.0f;
		}
	}

	// Clear events
	pose->ClearEventsAndUsedAnims();
	
#ifdef USE_HAVOK_ANIMATION
	// Clear delta reference
	pose->m_deltaReferenceFrameLocal.setIdentity();
#else
	// Clear delta reference
	pose->m_deltaReferenceFrameLocal.SetIdentity();
#endif
}

CPoseHandle SBehaviorSampleContext::GetPose()
{
	PC_SCOPE( ContextGetPose );

	CPoseHandle pose = m_poseAlloc->AcquirePose();

	// TODO TPose arg
	ResetPose( pose.Get(), true );

	return pose;
}

CPoseHandle SBehaviorSampleContext::GetMimicPose()
{
	PC_SCOPE( ContextGetPose );

	Bool forceTPose = 
#ifdef DISABLE_SAMPLING_AT_LOD3
		m_lod <= BL_Lod2;
#else
		false;
#endif

	CPoseHandle pose = m_mimicAlloc->AcquirePose();
	ResetPose( pose.Get(), forceTPose );
	if ( pose )
	{
		for ( Uint32 i=0; i<pose->m_numBones; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			pose->m_outputPose[ i ].setIdentity();
#else
			pose->m_outputPose[ i ].SetIdentity();
#endif
		}
	}
	return pose;
}

SMimicPostProcessData* SBehaviorSampleContext::GetMimicPostProcessData()
{
	return m_mimicPostProcessData;
}

void SBehaviorSampleContext::ForceTPose()
{
	SetTPose( &GetMainPose(), false );
	if ( m_constraintedPose )
	{
		SetTPose( m_constraintedPose.Get(), false );
	}
}

void SBehaviorSampleContext::SetTPose( SBehaviorGraphOutput* pose, Bool mimic ) const
{
	if ( !mimic )
	{
		pose->SetPose( m_poseSkeleton );
	}
	else
	{
		pose->SetPose( m_mimicSkeleton );
	}
}

SBehaviorGraphOutput& SBehaviorSampleContext::PrepareForSample( Bool forceTPose )
{
	PC_SCOPE( BehaviorSamplePrepare );

	ASSERT( m_poseSkeleton );
	ASSERT( m_poseAlloc );
	ASSERT( m_postActions.Empty() );

	m_sampledID++;

	// Reset main pose
	m_pose->ResetPoseLS( forceTPose );

	// Reset mimic post process data
	ResetMimicPostProcessData();

	// Reset constraint flag
	m_isConstrainted = false;

	//return *m_mainPose;
	ASSERT( m_pose->IsSyncedLS() );

	ClearPostActions();
	GetOneFrameAllocator().RemoveAllFromStack();

	return *m_pose->AccessUnsyncedPoseLS();
}

SBehaviorGraphOutput& SBehaviorSampleContext::PrepareForSampleConstraints( SBehaviorGraphOutput& output )
{
	PC_SCOPE( BehaviorForSampleConstraints );

	ASSERT( m_poseSkeleton );
	ASSERT( m_poseAlloc );
	ASSERT( m_pose );
	ASSERT( m_postActions.Empty() );

	// Constrainted pose
	if ( !m_constraintedPose && m_poseAlloc )
	{
		m_constraintedPose = m_poseAlloc->AcquirePose();
		ResetPose( m_constraintedPose.Get() );
	}

	if ( m_constraintedPose && m_constraintedPose->IsValid() )
	{
		*m_constraintedPose = output;

		// Set constraint flag
		m_isConstrainted = true;

		return *m_constraintedPose;
	}
	else
	{
		return output;
	}
}

SBehaviorGraphOutput& SBehaviorSampleContext::GetMainPose()
{
	ASSERT( m_pose->IsSyncedLS() );
	return *m_pose->AccessUnsyncedPoseLS();
}

const SBehaviorGraphOutput& SBehaviorSampleContext::GetMainPose() const
{
	ASSERT( m_pose->IsSyncedLS() );
	return *m_pose->AccessUnsyncedPoseLS();
}

SBehaviorGraphOutput& SBehaviorSampleContext::GetSampledPose()
{
	ASSERT( m_poseSkeleton );

	if ( !m_isConstrainted )
	{
		ASSERT( m_pose->IsSyncedLS() );
		return *m_pose->AccessUnsyncedPoseLS();
	}
	else
	{
		ASSERT( m_constraintedPose && m_constraintedPose->IsValid() );
		ASSERT( m_pose->IsSyncedLS() );
		return m_constraintedPose && m_constraintedPose->IsValid() ? *m_constraintedPose : *m_pose->AccessUnsyncedPoseLS();
	}
}

const SBehaviorGraphOutput& SBehaviorSampleContext::GetSampledPose() const
{
	ASSERT( m_poseSkeleton );

	if ( !m_isConstrainted )
	{
		ASSERT( m_pose->IsSyncedLS() );
		return *m_pose->AccessUnsyncedPoseLS();
	}
	else
	{
		ASSERT( m_constraintedPose && m_constraintedPose->IsValid() );
		ASSERT( m_pose->IsSyncedLS() );
		return m_constraintedPose && m_constraintedPose->IsValid() ? *m_constraintedPose : *m_pose->AccessUnsyncedPoseLS();
	}
}

void SBehaviorSampleContext::CreateAndCacheFrozenPose()
{
	ASSERT( !HasFrozenPose() );

	if ( !m_frozenPose )
	{
		m_frozenPose = m_poseAlloc->AcquirePose();
	}

	*m_frozenPose = GetMainPose();
}

SBehaviorGraphOutput* SBehaviorSampleContext::GetFrozenPose()
{
	ASSERT( HasFrozenPose() );
	return m_frozenPose.Get();
}

void SBehaviorSampleContext::ReleaseFrozenPose()
{
	m_frozenPose.Reset();
}

Bool SBehaviorSampleContext::HasFrozenPose() const
{
	return m_frozenPose;
}

void SBehaviorSampleContext::CompleteSample()
{
	// Sync pose
	ASSERT( m_pose->IsSyncedLS() );
}

void SBehaviorSampleContext::CompleteSampleConstraints()
{
}

void SBehaviorSampleContext::CreateMimicPostProcessData()
{
	ASSERT( !m_mimicPostProcessData );

	m_mimicPostProcessData = new SMimicPostProcessData;

	ResetMimicPostProcessData();
}

void SBehaviorSampleContext::ResetMimicPostProcessData()
{
	if ( m_mimicPostProcessData )
	{
		for ( Uint32 i=0; i<MIMIC_POSE_AREAS_NUM; ++i )
		{
			m_mimicPostProcessData->m_mimicAreas[ i ] = 0;
		}

		COMPILE_ASSERT( MIMIC_POSE_BONES_NUM == 0 );
		//m_mimicPostProcessData->m_mimicBones[ 0 ].SetIdentity();
		//m_mimicPostProcessData->m_mimicBones[ 1 ].SetIdentity();
	}
}

void SBehaviorSampleContext::CachePoseFromPrevSampling()
{
	*m_rememberedPose = *m_pose->AccessUnsyncedPoseLS();
}

const SBehaviorGraphOutput& SBehaviorSampleContext::GetPoseFromPrevSampling() const
{
	return *m_rememberedPose;
}

void SBehaviorSampleContext::CachePoseForLastFullUpdateAndSample()
{
	*m_prevFullUpdateAndSamplePose = *m_lastFullUpdateAndSamplePose;
	*m_lastFullUpdateAndSamplePose = *m_pose->AccessUnsyncedPoseLS();
}

void SBehaviorSampleContext::ForcePrevToBeLastFullUpdateAndSample()
{
	*m_prevFullUpdateAndSamplePose = *m_lastFullUpdateAndSamplePose;
}

SBehaviorGraphOutput& SBehaviorSampleContext::RestorePoseFromLastFullUpdateAndSample()
{
	SBehaviorGraphOutput& pose = PrepareForSample( false );
	pose = *m_lastFullUpdateAndSamplePose;
	return pose;
}

const SBehaviorGraphOutput& SBehaviorSampleContext::GetPoseFromLastFullUpdateAndSample() const
{
	return *m_lastFullUpdateAndSamplePose;
}

const SBehaviorGraphOutput& SBehaviorSampleContext::GetPoseFromPrevFullUpdateAndSample() const
{
	return *m_prevFullUpdateAndSamplePose;
}

Bool SBehaviorSampleContext::HasMimic() const
{
	return m_mimicSkeleton != NULL;
}

SBehaviorGraphOutput& SBehaviorSampleContext::ForcePoseLocalSpace( const SBehaviorGraphOutput& pose )
{
	*m_pose->AccessUnsyncedPoseLS() = pose;

	return *m_pose->AccessUnsyncedPoseLS();
}

Uint32 SBehaviorSampleContext::GetAnimEventsNum() const
{
	return m_pose->GetAnimEventsNum();
}

Matrix SBehaviorSampleContext::GetBoneMatrixLocalSpace( Uint32 boneIndex ) const
{
	if ( boneIndex < m_pose->GetBonesNum() )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkQsTransform& bone = m_pose->GetBoneLS( boneIndex );
		Matrix mat;
		HavokTransformToMatrix_Renormalize( bone, &mat );
#else
		const RedQsTransform& bone = m_pose->GetBoneLS( boneIndex );
		Matrix mat;
		RedMatrix4x4 conversionMatrix;
		conversionMatrix = bone.ConvertToMatrixNormalized();
		mat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		return mat;
	}

	return Matrix::IDENTITY;
}

AnimQsTransform SBehaviorSampleContext::GetBoneTransformLocalSpace( Uint32 boneIndex ) const
{
	AnimQsTransform transform( AnimQsTransform::IDENTITY );

	if ( boneIndex < m_pose->GetBonesNum() )
	{
		transform = m_pose->GetBoneLS( boneIndex );
	}

	return transform;
}

Bool SBehaviorSampleContext::GetBoneTransformLocalSpaceRef( Uint32 boneIndex, AnimQsTransform& transform ) const
{
	if ( boneIndex < m_pose->GetBonesNum() )
	{
		transform = m_pose->GetBoneLS( boneIndex );
		return true;
	}

	return false;
}

Uint32 SBehaviorSampleContext::GetFloatTrackNum() const
{
	return m_pose->GetFloatTrackNum();
}

Float SBehaviorSampleContext::GetFloatTrack( Uint32 trackIndex ) const
{
	if ( trackIndex < m_pose->GetFloatTrackNum() )
	{
		return m_pose->GetFloatTrack( trackIndex );
	}

	return 0.f;
}

Bool SBehaviorSampleContext::IsMainPoseTPose() const
{
	// PTom TODO - remove this
	return false;

	//ASSERT( m_mainPose );
	//return m_mainPose ? m_mainPose->m_tPose : true;
}

Bool SBehaviorSampleContext::HasAnimationEventOccurred( const CName& animEventName ) const
{
	return m_pose ? m_pose->HasAnimationEventOccurred( animEventName ) : false;
}

void SBehaviorSampleContext::SetPoseCorrection( SBehaviorGraphOutput& pose ) const
{
	Int32 numBones = (Int32)pose.m_numBones;
	for ( Uint32 i=0; i<m_boneCorrections.Size(); ++i )
	{
		Int32 boneIndex = m_boneCorrections[ i ].m_bone;

		ASSERT( boneIndex < numBones );

		if ( boneIndex != -1 && boneIndex < numBones )
		{
			pose.m_outputPose[ boneIndex ] = m_boneCorrections[ i ].m_transform;
		}
	}
}

void SBehaviorSampleContext::SetPoseCorrectionIdentity( SBehaviorGraphOutput& pose ) const
{
	Int32 numBones = (Int32)pose.m_numBones;
	for ( Uint32 i=0; i<m_boneCorrections.Size(); ++i )
	{
		Int32 boneIndex = m_boneCorrections[ i ].m_bone;

		ASSERT( boneIndex < numBones );

		if ( boneIndex != -1 && boneIndex < numBones )
		{
			pose.m_outputPose[ boneIndex ].SetIdentity();
		}
	}
}

void SBehaviorSampleContext::ResetBoneCorrection()
{
	m_boneCorrections.Clear();
}

void SBehaviorSampleContext::SetupBoneCorrection( const CAnimatedComponent* parent, const BoneMappingContainer& bones )
{
	if ( m_boneCorrections.Size() != bones.Size() )
	{
		m_boneCorrections.Resize( bones.Size() );
	}

	SBehaviorSampleContext* parentContext = parent->GetBehaviorGraphSampleContext();
	if ( parentContext == NULL || m_poseSkeleton == NULL )
	{
		return;
	}

	const SBehaviorGraphOutput& sampledPose = parentContext->GetSampledPose();
	
	SetupBoneCorrection( sampledPose, bones );
}

void SBehaviorSampleContext::SetupBoneCorrection( const SBehaviorGraphOutput& sampledPose, const BoneMappingContainer& bones )
{
	if ( m_boneCorrections.Size() != bones.Size() )
	{
		m_boneCorrections.Resize( bones.Size() );
	}

	Int32 parentBoneNum = (Int32)sampledPose.m_numBones;
	Int32 bonesNum = m_poseSkeleton->GetBonesNum();

	for ( Uint32 i=0; i<m_boneCorrections.Size(); ++i )
	{
		const SBoneMapping& boneMapping = bones[ i ];

		ASSERT( boneMapping.m_boneA != -1 ); // Child
		ASSERT( boneMapping.m_boneB != -1 ); // Parent

		if ( boneMapping.m_boneB < parentBoneNum && boneMapping.m_boneA < bonesNum )
		{
			m_boneCorrections[ i ].m_bone = boneMapping.m_boneA;
			m_boneCorrections[ i ].m_transform = sampledPose.m_outputPose[ boneMapping.m_boneB ];
		}
		else
		{
			m_boneCorrections[ i ].m_bone = -1;
			ASSERT(  boneMapping.m_boneA < bonesNum );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CAnimationPostActionList::CAnimationPostActionList()
{
}

CAnimationPostActionList::~CAnimationPostActionList()
{
	Clear();
}

void CAnimationPostActionList::Add( IAnimationPostAction* action )
{
	m_actions.PushBack( action );
}

void CAnimationPostActionList::Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose )
{
	const Uint32 size = m_actions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		IAnimationPostAction* action = m_actions[ i ];
		action->Process( ac, pose );
	}
	Clear();
}

void CAnimationPostActionList::Clear()
{
	m_actions.ClearFast();
}

Bool CAnimationPostActionList::Empty() const
{
	return m_actions.Empty();
}

//////////////////////////////////////////////////////////////////////////

CAnimationEffectAction::CAnimationEffectAction( const CName& effect, Float time ) 
	: m_effect( effect )
	, m_time( time )
{
}

void CAnimationEffectAction::Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose )
{
	CEntity* entity = ac->GetEntity();
	entity->PlayEffectForAnimation( m_effect, m_time );
}

//////////////////////////////////////////////////////////////////////////

CAnimationMorphAction::CAnimationMorphAction()
	: m_value( 0.f )
{
}

void CAnimationMorphAction::Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose )
{
	CEntity* entity = ac->GetEntity();
	if ( CMorphedMeshComponent* mmc = entity->FindComponent< CMorphedMeshComponent >() )
	{
		mmc->SetMorphRatio( Clamp( m_value, 0.f, 1.f ) );
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif

