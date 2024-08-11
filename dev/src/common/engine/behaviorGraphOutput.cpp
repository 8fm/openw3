/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphUtils.inl"
#include "../engine/skeleton.h"
#include "../engine/animatedComponent.h"
#include "../core/profiler.h"
#include "behaviorGraph.h"
#include "baseEngine.h"

#ifdef USE_DEBUG_ANIM_POSES

/*
#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
#undef LOG_CORE

# ifdef RED_PLATFORM_ORBIS
#define LOG_CORE(...) { fwprintf( stdout, __VA_ARGS__ ); fwprintf( stdout, L"\n" ); }
# else
#define LOG_CORE(...) { Char tempArray[1024]; swprintf( tempArray, 1024, __VA_ARGS__ ); OutputDebugString(tempArray); OutputDebugString(TXT("\n")); }
# endif
#endif
*/

#pragma optimize("",off)

void DAP_CallMyPlaceForBreakpoint( const Char* msg, const Char* msgFile, const Uint32 lineNum )
{
	LOG_CORE( TXT("[%ld] %ls, file: %ls, line: %d"), GEngine->GetCurrentEngineTick(), msg, msgFile, lineNum );
	RED_FATAL_ASSERT( 0, "QNaN detected!" );
	Int32 i = 0;
	i++;
}
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//#define USE_HANDOPTIMIZED_TRANSFORM_CALC
//////////////////////////////////////////////////////////////////////////

const Uint32 c_eventFiredMaxCount = 32;

SBehaviorGraphOutput::SBehaviorGraphOutput( Uint32 usedAnimSlotsToReserve )
	: m_outputPose( nullptr )
	, m_floatTracks( nullptr )
	, m_eventsFired( nullptr )
	, m_numBones( 0 )
	, m_numFloatTracks( 0 )
	, m_numEventsFired( 0 )
	, m_usedAnims( usedAnimSlotsToReserve )
	, m_refCount( 0 )
	, m_ownPoseMemory( false )
	, m_ownFloatTrackMemory( false )
	, m_ownEventFiredMemory( false )
	, m_tPose( false )
{}

SBehaviorGraphOutput::~SBehaviorGraphOutput()
{
	Deinit();
}

void SBehaviorGraphOutput::Init( Uint32 numBones, Uint32 numFloatTracks, Bool createEvents )
{
	SBehaviorGraphOutputParameter param = 
	{
		numBones,
		numFloatTracks,
		nullptr,
		nullptr,
		nullptr,
		createEvents
	};

	Init( param );
}

void SBehaviorGraphOutput::Init( const SBehaviorGraphOutputParameter & param )
{
	m_numBones = param.boneCount;
	m_numFloatTracks = param.floatTracksCount;

	if( param.poseMemory )
	{
		RED_FATAL_ASSERT( !(reinterpret_cast< MemInt >( param.poseMemory ) & static_cast< MemInt >( __alignof( AnimQsTransform ) - 1 ) ), "Pose memory not aligned to %d byte.", __alignof( AnimQsTransform ) );

		DeallocateOutputPose();
		m_outputPose = param.poseMemory;
	}
	else
	{
		AllocateOutputPose();
	}

	if( param.floatTrackMemory )
	{
		DeallocateFloatTracks();
		m_floatTracks = param.floatTrackMemory;
	}
	else
	{
		AllocateFloatTracks();
	}

	if( param.eventFiredMemory )
	{
		DeallocateEventFired();
		m_eventsFired = param.eventFiredMemory;
	}
	else if( param.createEvents )
	{
		AllocateEventsFired();
	}

	m_numEventsFired = 0;
}

void SBehaviorGraphOutput::Deinit()
{
	DeallocateOutputPose();
	DeallocateFloatTracks();
	DeallocateEventFired();
}

SBehaviorGraphOutput& SBehaviorGraphOutput::operator=( const SBehaviorGraphOutput& rhs )
{
	PC_SCOPE( BehaviorSetPoseOperator );

	if( this != &rhs )
	{
		m_tPose = rhs.m_tPose;

		const Uint32 numBones = Min( m_numBones, rhs.m_numBones );
		if ( numBones > 0 )
		{
			RED_FATAL_ASSERT( m_outputPose, "Buffer can't be null." );
			Red::System::MemoryCopy( m_outputPose, rhs.m_outputPose, sizeof( AnimQsTransform ) * numBones );
		}

		const Uint32 numTracks = Min( m_numFloatTracks, rhs.m_numFloatTracks );
		for( Uint32 i=0; i<numTracks; ++i )
		{
			m_floatTracks[i] = rhs.m_floatTracks[i];
		}

		COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
		m_customFloatTracks[ 0 ] = rhs.m_customFloatTracks[ 0 ];
		m_customFloatTracks[ 1 ] = rhs.m_customFloatTracks[ 1 ];
		m_customFloatTracks[ 2 ] = rhs.m_customFloatTracks[ 2 ];
		m_customFloatTracks[ 3 ] = rhs.m_customFloatTracks[ 3 ];
		m_customFloatTracks[ 4 ] = rhs.m_customFloatTracks[ 4 ];

		m_deltaReferenceFrameLocal = rhs.m_deltaReferenceFrameLocal;

		m_numEventsFired = rhs.m_numEventsFired;
		if ( m_numEventsFired > 0 )
		{
			RED_FATAL_ASSERT( m_eventsFired, "Buffer can't be null." );
			Red::System::MemoryCopy( m_eventsFired, rhs.m_eventsFired, sizeof( CAnimationEventFired ) * m_numEventsFired );
		}

		m_usedAnims = rhs.m_usedAnims;
	}

	return *this;
}

// DO NOT USE. Only use from CPoseHandle. Don't use it else it will corrupt CPoseProvider state
void SBehaviorGraphOutput::AddRef() 
{
	m_refCount.Increment();
}

// DO NOT USE. Only use from CPoseHandle. Don't use it else it will corrupt CPoseProvider state
Int32 SBehaviorGraphOutput::Release() 
{
	return m_refCount.Decrement();
}

void SBehaviorGraphOutput::NormalizeRotations()
{
	PC_SCOPE( NormalizeRotations );

	RedQsTransform* blockStart = m_outputPose;
	for (Uint32 i=0; i<m_numBones; ++i, ++blockStart)
	{
		blockStart->Rotation.Normalize();
	}

}

void SBehaviorGraphOutput::SetPose( const CSkeleton* skeleton, Bool resetTracks )
{
	PC_SCOPE( BehaviorSetPose );

	ASSERT( skeleton->IsValid() );

	if ( skeleton->IsValid() )
	{
		const Uint32 numBones = skeleton->GetBonesNum();
		const AnimQsTransform* referencePose = skeleton->GetReferencePoseLS();
		Red::System::MemoryCopy( m_outputPose, referencePose, sizeof( AnimQsTransform ) * numBones );
	}

	if ( resetTracks )
	{
		if ( m_floatTracks > 0 )
		{
			for( Uint32 i=0; i<m_numFloatTracks; ++i )
			{
				m_floatTracks[i] = 0.f;
			}
		}

		COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
		m_customFloatTracks[ 0 ] = 0.f;
		m_customFloatTracks[ 1 ] = 0.f;
		m_customFloatTracks[ 2 ] = 0.f;
		m_customFloatTracks[ 3 ] = 0.f;
		m_customFloatTracks[ 4 ] = 0.f;
	}

	m_tPose = true;
}

void SBehaviorGraphOutput::SetPose( const CAnimatedComponent* componentWithPoseLS )
{
	ASSERT( componentWithPoseLS );

	if ( componentWithPoseLS->GetBehaviorGraphSampleContext() )
	{
		const SBehaviorGraphOutput& pose = componentWithPoseLS->GetBehaviorGraphSampleContext()->GetMainPose();

		// Pose
		const Uint32 numBones = Min( m_numBones, pose.m_numBones );
		if ( numBones > 0 && pose.m_outputPose != m_outputPose )
		{
			Red::System::MemoryCopy( m_outputPose, pose.m_outputPose, sizeof( AnimQsTransform ) * numBones );
		}

		// Tracks
		const Uint32 trackNum = Min( m_numFloatTracks, pose.m_numFloatTracks );
		for( Uint32 i=0; i<trackNum; ++i )
		{
			m_floatTracks[i] = pose.m_floatTracks[i];
		}
	}

	// Reset motion extraction

	m_deltaReferenceFrameLocal.SetIdentity();
	m_tPose = componentWithPoseLS->IsCurrentPoseTPose();

#ifdef TPOSE_DETECTOR
	if ( componentWithPoseLS->GetEntity()->QueryActorInterface() )
	{
		ASSERT( !m_tPose );
	}
#endif
}

void SBehaviorGraphOutput::SetPose( const SBehaviorGraphOutput& pose )
{
	// Pose
	const Uint32 numBones = Min( m_numBones, pose.m_numBones );
	if ( numBones > 0 && pose.m_outputPose != m_outputPose )
	{
		Red::System::MemoryCopy( m_outputPose, pose.m_outputPose, sizeof( AnimQsTransform ) * numBones );
	}

	// Tracks
	const Uint32 trackNum = Min( m_numFloatTracks, pose.m_numFloatTracks );
	for( Uint32 i=0; i<trackNum; ++i )
	{
		m_floatTracks[i] = pose.m_floatTracks[i];
	}

	// Reset motion extraction
	m_deltaReferenceFrameLocal.SetIdentity();

	m_tPose = false; // TODO add check?
}

void SBehaviorGraphOutput::SetPoseFromBonesModelSpace( const CSkeleton *skeleton, const TDynArray< Matrix > & bonesMS )
{
	// get bones from model space to local space here but only if not key framed (in ragdoll)
	for ( Uint32 idx = 0; idx < bonesMS.Size(); ++ idx )
	{
		Int32 parentIdx = skeleton->GetParentBoneIndex( (Int32)idx );
		if ( parentIdx != -1 )
		{
			Matrix boneLS = (bonesMS[ idx ] * bonesMS[ parentIdx ].Inverted());
			RedMatrix4x4 boneRedLS = reinterpret_cast< const RedMatrix4x4& >( boneLS );
			m_outputPose[ idx ] = RedQsTransform( boneRedLS.GetTranslation(), boneRedLS.ToQuaternion(), boneRedLS.GetScale() );
		}
	}
}

void SBehaviorGraphOutput::SetIdentity()
{
	for( Uint32 i=0; i<m_numBones; ++i )
	{
		m_outputPose[i].SetIdentity();
	}

	if ( m_numFloatTracks > 0 )
	{
		for( Uint32 i=0; i<m_numFloatTracks; ++i )
		{
			m_floatTracks[i] = 0.f;
		}
	}

	COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
	m_customFloatTracks[ 0 ] = 0.f;
	m_customFloatTracks[ 1 ] = 0.f;
	m_customFloatTracks[ 2 ] = 0.f;
	m_customFloatTracks[ 3 ] = 0.f;
	m_customFloatTracks[ 4 ] = 0.f;

	m_deltaReferenceFrameLocal.SetIdentity();
}

void SBehaviorGraphOutput::SetZero()
{
	ASSERT( 0 );

	for( Uint32 i=0; i<m_numBones; ++i )
	{
		m_outputPose[i].SetZero();
	}

	for( Uint32 i=0; i<m_numFloatTracks; ++i )
	{
		m_floatTracks[i] = 0.0f;
	}

	COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
	m_customFloatTracks[ 0 ] = 0.f;
	m_customFloatTracks[ 1 ] = 0.f;
	m_customFloatTracks[ 2 ] = 0.f;
	m_customFloatTracks[ 3 ] = 0.f;
	m_customFloatTracks[ 4 ] = 0.f;

	m_deltaReferenceFrameLocal.SetZero();
}

void SBehaviorGraphOutput::Reset( const CSkeleton* skeleton )
{
	// Set reference pose
	if ( skeleton )
	{
		SetPose( skeleton );
	}
	else
	{
		// Clear custom and float tracks
		COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
		m_customFloatTracks[ 0 ] = 0.f;
		m_customFloatTracks[ 1 ] = 0.f;
		m_customFloatTracks[ 2 ] = 0.f;
		m_customFloatTracks[ 3 ] = 0.f;
		m_customFloatTracks[ 4 ] = 0.f;

		if ( m_numFloatTracks > 0 )
		{
			for( Uint32 i=0; i<m_numFloatTracks; ++i )
			{
				m_floatTracks[i] = 0.f;
			}
		}
	}

	// Clear events and used anims
	ClearEventsAndUsedAnims();

	// Clear delta reference
	m_deltaReferenceFrameLocal.SetIdentity();

}

//////////////////////////////////////////////////////////////
void SBehaviorGraphOutput::Reset()
{
	SetIdentity();
	Reset(nullptr);
}

void SBehaviorGraphOutput::SetInterpolateWithoutME( const SBehaviorGraphOutput &a,  const SBehaviorGraphOutput &b, float alpha )
{
	//ASSERT( m_numBones == a.m_numBones && a.m_numBones == b.m_numBones );

	if ( a.m_tPose && b.m_tPose )
	{
		m_tPose = true;
		//return;
	}
	else
	{
		m_tPose = false;
	}

	BehaviorUtils::BlendingUtils::BlendPosesNormal(*this, a, b, alpha);

	//ASSERT( m_numFloatTracks == a.m_numFloatTracks && a.m_numFloatTracks == b.m_numFloatTracks );

	const Uint32 floatTrackNum = Min( m_numFloatTracks, Min( a.m_numFloatTracks , b.m_numFloatTracks ) ); // [Notes/TODO]: probably the same think already happened in BehaviorUtils::BlendingUtils::BlendPosesNormal ?! (Check/Fix)
	for( Uint32 i=0; i<floatTrackNum; ++i )
	{
		m_floatTracks[i] = Lerp( alpha, a.m_floatTracks[i], b.m_floatTracks[i] );
	}

	COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
	m_customFloatTracks[0] = Lerp( alpha, a.m_customFloatTracks[0], b.m_customFloatTracks[0] );
	m_customFloatTracks[1] = Lerp( alpha, a.m_customFloatTracks[1], b.m_customFloatTracks[1] );
	m_customFloatTracks[2] = Lerp( alpha, a.m_customFloatTracks[2], b.m_customFloatTracks[2] );
	m_customFloatTracks[3] = Lerp( alpha, a.m_customFloatTracks[3], b.m_customFloatTracks[3] );
	m_customFloatTracks[4] = Lerp( alpha, a.m_customFloatTracks[4], b.m_customFloatTracks[4] );
}

void SBehaviorGraphOutput::SetInterpolate( const SBehaviorGraphOutput &a, 
										  const SBehaviorGraphOutput &b,
										  float alpha )
{
	SetInterpolateWithoutME( a, b, alpha );
	m_deltaReferenceFrameLocal.Slerp( a.m_deltaReferenceFrameLocal, b.m_deltaReferenceFrameLocal, alpha );
}

void SBehaviorGraphOutput::SetAddMul( const SBehaviorGraphOutput &a,
									 const SBehaviorGraphOutput &b,
									 Float weight )
{
	if ( a.m_tPose && b.m_tPose )
	{
		m_tPose = true;
		//return;
	}
	else
	{
		m_tPose = false;
	}

	const Float simdWeight = weight; 

	// does this really should happen? additive blending for reference frame movement?
	const RedVector4 scale = m_deltaReferenceFrameLocal.Scale;

	m_deltaReferenceFrameLocal = a.m_deltaReferenceFrameLocal;

	const Uint32 numBones = Min( Min( a.m_numBones, b.m_numBones ), m_numBones );
	const Uint32 numTracks = Min( Min( a.m_numFloatTracks, b.m_numFloatTracks ), m_numFloatTracks );

	RedQsTransform temp;
	if ( weight > 0.f )
	{
		for( Uint32 i=0; i<numBones; ++i )
		{
			temp.SetMul( b.m_outputPose[i], a.m_outputPose[i] );
			m_outputPose[i].Lerp( a.m_outputPose[i], temp, simdWeight );
		}

		m_deltaReferenceFrameLocal.BlendAddMul( b.m_deltaReferenceFrameLocal, simdWeight );
	}
	else
	{
		RedQsTransform tempInv;

		for( Uint32 i=0; i<numBones; ++i )
		{
			tempInv.SetInverse( b.m_outputPose[i] );
			temp.SetMul( tempInv, a.m_outputPose[i] );
			m_outputPose[i].Lerp( a.m_outputPose[i], temp, -simdWeight );
		}

		tempInv.SetInverse( b.m_deltaReferenceFrameLocal );
		m_deltaReferenceFrameLocal.BlendAddMul( tempInv, -simdWeight );
	}

	for( Uint32 i=0; i<numTracks; ++i )
	{
		m_floatTracks[i] = a.m_floatTracks[i] + weight * b.m_floatTracks[i];
	}

	COMPILE_ASSERT( NUM_CUSTOM_FLOAT_TRACKS == 5 );
	m_customFloatTracks[0] = a.m_customFloatTracks[0] + weight * b.m_customFloatTracks[0];
	m_customFloatTracks[1] = a.m_customFloatTracks[1] + weight * b.m_customFloatTracks[1];
	m_customFloatTracks[2] = a.m_customFloatTracks[2] + weight * b.m_customFloatTracks[2];
	m_customFloatTracks[3] = a.m_customFloatTracks[3] + weight * b.m_customFloatTracks[3];
	m_customFloatTracks[4] = a.m_customFloatTracks[4] + weight * b.m_customFloatTracks[4];

	m_deltaReferenceFrameLocal.Scale = scale;
	m_deltaReferenceFrameLocal.Rotation.Normalize();

}

void SBehaviorGraphOutput::SetAddMul( const SBehaviorGraphOutput &b,
									 Float weight )
{
	 SetAddMul( *this, b, weight );
}

void SBehaviorGraphOutput::SetMulInv( const SBehaviorGraphOutput &a )
{
	ASSERT( m_numBones == a.m_numBones );

	if ( a.m_tPose && m_tPose )
	{
		m_tPose = true;
		//return;
	}
	else
	{
		m_tPose = false;
	}

	for( Uint32 i=0; i<m_numBones; ++i )
	{
		m_outputPose[i].SetMulMulInverse( m_outputPose[i], a.m_outputPose[i] );
	}

	m_deltaReferenceFrameLocal.SetMulMulInverse( m_deltaReferenceFrameLocal, a.m_deltaReferenceFrameLocal );
}

void SBehaviorGraphOutput::InvertPose()
{
	if ( m_tPose )
	{
		//return;
	}
	else
	{
		m_tPose = false;
	}

	for( Uint32 i=0; i<m_numBones; ++i )
	{
		const AnimQsTransform temp = m_outputPose[i];
		m_outputPose[i].SetInverse( temp );
	}

	const AnimQsTransform temp = m_deltaReferenceFrameLocal;

	m_deltaReferenceFrameLocal.SetInverse( temp );

}

void SBehaviorGraphOutput::ExtractTrajectory( const CSkeleton* skeleton, Int32 boneIndex )
{
	if ( boneIndex > (Int32)m_numBones || m_numBones == 0 )
	{
		return;
	}

	AnimQsTransform& boneToExtract = m_outputPose[ boneIndex ];
	AnimQsTransform boneToExtractInv;

	boneToExtract.Rotation.Normalize();
	boneToExtractInv.SetInverse( boneToExtract );

	for ( Uint32 boneIdx=0; boneIdx<m_numBones; boneIdx++ )
	{
		AnimQsTransform& bone = m_outputPose[ boneIdx ];

		const Int32 parentIdx = skeleton->GetParentBoneIndex( boneIdx );
		ASSERT( parentIdx < (Int32)boneIdx );

		if ( parentIdx == 0 )
		{
			bone.SetMul( boneToExtractInv, bone );
		}
	}

	m_outputPose[ 0 ].SetIdentity();

}

void SBehaviorGraphOutput::ExtractTrajectory( const CAnimatedComponent* animComponent )
{
	ExtractTrajectoryOn( animComponent, m_outputPose, m_numBones );
}

void SBehaviorGraphOutput::ExtractTrajectoryOn( const CAnimatedComponent* animComponent, AnimQsTransform* outputPose, Uint32 numBones )
{
	// TODO there should be no need extract trajectory in near future and when future becomes present or past, remove this
	const Int32 trajectoryBone = animComponent->GetTrajectoryBone();

	if ( trajectoryBone == -1 || trajectoryBone > (Int32)numBones || numBones == 0 )
	{
		return;
	}

	AnimQsTransform& traj = outputPose[ trajectoryBone ];
	AnimQsTransform trajInv;

	// Traj bone's transform have to be normalize because of quat rotation. We don't want to have error scale shit.
	traj.Rotation.Normalize();
	trajInv.SetInverse( traj );

	CSkeleton* skeleton = animComponent->GetSkeleton();
	for (Uint32 boneIdx=0; boneIdx<numBones; boneIdx++)
	{
		AnimQsTransform& bone = outputPose[boneIdx];

		const Int32 parentIdx = skeleton->GetParentBoneIndex(boneIdx);
		ASSERT(parentIdx < (Int32)boneIdx);

		if ( parentIdx == 0 )
		{
			bone.SetMul( trajInv, bone );
		}
	}

	outputPose[ 0 ].SetIdentity();

}

AnimQsTransform SBehaviorGraphOutput::GetBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex, const Int16 *parentIndices ) const
{
	AnimQsTransform retVal;

	if ( boneIndex < 0 || boneIndex >= (Int32)m_numBones )
	{
		retVal.SetIdentity();
		return retVal;
	}

	// compute bone transform in model space
	retVal = m_outputPose[ boneIndex ];
	Int32 currBone = parentIndices[ boneIndex ];
	while( currBone != -1 )
	{			
		retVal.SetMul( m_outputPose[ currBone ], retVal );
		currBone = parentIndices[ currBone ];
	}

	// apply movement
	retVal.SetMul( m_deltaReferenceFrameLocal, retVal );

	// apply character position				
	RedVector4 worldPos;
	RedQuaternion worldOrient;
	worldPos = reinterpret_cast< const RedVector4& >( animComponent->GetWorldPositionRef() );
	Vector temp = animComponent->GetWorldRotation().ToQuat();
	worldOrient.Quat =  reinterpret_cast< RedVector4& >( temp );
	RedQsTransform componentWorldTransform( worldPos, worldOrient, RedVector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	retVal.SetMul( componentWorldTransform, retVal );

	return retVal;
}

AnimQsTransform SBehaviorGraphOutput::GetBoneModelTransform( Int32 boneIndex, const Int16 *parentIndices ) const
{
	if ( boneIndex < 0 || boneIndex >= (Int32)m_numBones )
	{
		RedQsTransform retVal;
		retVal.SetIdentity();
		return retVal;
	}

	// Bone in local space
	RedQsTransform bone = m_outputPose[ boneIndex ];

	Int32 currBone = parentIndices[ boneIndex ];
	while( currBone != -1 )
	{			
		bone.SetMul( m_outputPose[ currBone ], bone );
		currBone = parentIndices[ currBone ];
	}

	return bone;
}

AnimQsTransform SBehaviorGraphOutput::GetBoneModelTransformWithOffset( Int32 boneIndex, const Int16 *parentIndices, const AnimQsTransform& offsetLS ) const
{
	if ( boneIndex < 0 || boneIndex >= (Int32)m_numBones )
	{
		RedQsTransform retVal;
		retVal.SetIdentity();
		return retVal;
	}

	// Bone in local space
	RedQsTransform bone;
	bone.SetMul( m_outputPose[ boneIndex ], offsetLS );

	Int32 currBone = parentIndices[ boneIndex ];
	while( currBone != -1 )
	{			
		bone.SetMul( m_outputPose[ currBone ], bone );
		currBone = parentIndices[ currBone ];
	}

	return bone;
}

AnimQsTransform SBehaviorGraphOutput::GetBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	const CSkeleton* skeleton = animComponent->GetSkeleton();
	if ( !skeleton )
	{
		return AnimQsTransform( AnimQsTransform::IDENTITY );
	}

	return GetBoneWorldTransform( animComponent, boneIndex, skeleton->GetParentIndices() );
}

AnimQsTransform SBehaviorGraphOutput::GetBoneModelTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	const CSkeleton* skeleton = animComponent->GetSkeleton();
	if ( !skeleton )
	{
		return AnimQsTransform( AnimQsTransform::IDENTITY );
	}

	return GetBoneModelTransform( boneIndex, skeleton->GetParentIndices() );
}

AnimQsTransform SBehaviorGraphOutput::GetParentBoneWorldTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	const CSkeleton* skeleton = animComponent->GetSkeleton();
	if ( !skeleton )
	{
		return AnimQsTransform( AnimQsTransform::IDENTITY );
	}

	return GetBoneWorldTransform( animComponent, skeleton->GetParentBoneIndex(boneIndex), skeleton->GetParentIndices() );
}

AnimQsTransform SBehaviorGraphOutput::GetParentBoneModelTransform( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	CSkeleton* skeleton = animComponent->GetSkeleton();
	if ( !skeleton )
	{
		return AnimQsTransform( AnimQsTransform::IDENTITY );
	}

	return GetBoneModelTransform( skeleton->GetParentBoneIndex(boneIndex), skeleton->GetParentIndices() );
}

Matrix SBehaviorGraphOutput::GetBoneWorldMatrix( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	AnimQsTransform transform = GetBoneWorldTransform( animComponent, boneIndex );
	Matrix mat;

	RedMatrix4x4 conversionMatrix = transform.ConvertToMatrixNormalized();
	mat = reinterpret_cast< const Matrix& >( conversionMatrix );

	return mat;
}

Matrix SBehaviorGraphOutput::GetBoneModelMatrix( const CAnimatedComponent *animComponent, Int32 boneIndex ) const
{
	AnimQsTransform transform = GetBoneModelTransform( animComponent, boneIndex );
	Matrix mat;

	RedMatrix4x4 conversionMatrix = transform.ConvertToMatrixNormalized();
	mat = reinterpret_cast< const Matrix& >( conversionMatrix );

	return mat;
}

void SBehaviorGraphOutput::GetBonesModelSpace( const CSkeleton *skeleton, TDynArray<AnimQsTransform>& bonesMS, Int32 _numBones ) const
{
    if ( ! skeleton || bonesMS.Size() != m_numBones )
    {
        // Invalid input data
        return;
    }

	const Uint32 bonesNum = _numBones != -1 ? Min( m_numBones, (Uint32)skeleton->GetBonesNum(), (Uint32)_numBones ) : Min( m_numBones, (Uint32)skeleton->GetBonesNum() );
	for ( Uint32 boneIdx=0; boneIdx<bonesNum; boneIdx++ )
	{
		AnimQsTransform& bone = bonesMS[boneIdx];

		// Bone in local space
		bone = m_outputPose[boneIdx];

		// Get parent index
		Int32 parentIdx = (Int32)skeleton->GetParentBoneIndex(boneIdx);
		ASSERT(parentIdx < (Int32)boneIdx);

		// If bone is not root
		if (parentIdx != -1)
		{
			// Get parent bone in model space
			const AnimQsTransform& parentMS = bonesMS[parentIdx];

			// Calc bone in model space
			bone.SetMul( parentMS, bone);
		}
	}
}

void SBehaviorGraphOutput::GetBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<AnimQsTransform>& bonesMS, Int32 _numBones ) const
{
	GetBonesModelSpace( animComponent->GetSkeleton(), bonesMS, _numBones );
}

void SBehaviorGraphOutput::GetBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<Matrix>& bonesMS, Int32 _numBones ) const
{
	GetBonesModelSpace( animComponent->GetSkeleton(), bonesMS, _numBones );
}


void SBehaviorGraphOutput::GetBonesModelSpace( const CSkeleton *skeleton, TDynArray<Matrix>& bonesMS, Int32 _numBones ) const
{
	if ( !skeleton || !skeleton->IsValid() || bonesMS.Size() != m_numBones ) 
	{
		// Invalid input data
		return;
	}

	// To many bones
	AnimQsTransform bonesMSTransforms[ 512 ];
	if ( bonesMS.Size() >= ARRAY_COUNT( bonesMSTransforms ) )
	{
		WARN_ENGINE( TXT("Unable to get bone transforms") );
		return;
	}


	// Calculate MS transforms
	const Uint32 bonesNum = _numBones != -1 ? Min( m_numBones, (Uint32)skeleton->GetBonesNum(), (Uint32)_numBones ) : Min( m_numBones, (Uint32)skeleton->GetBonesNum() );
	for ( Uint32 boneIdx = 0; boneIdx < bonesNum; ++boneIdx )
	{
		// Bone in local space
		AnimQsTransform& boneTransform = bonesMSTransforms[ boneIdx ];

		// Get parent index
		const Int32 parentIdx = skeleton->GetParentBoneIndex( boneIdx );

		// If bone is not root
		if ( parentIdx == -1 )
		{
			boneTransform = m_outputPose[ boneIdx ];
			const RedMatrix4x4 mat = boneTransform.ConvertToMatrix();
			bonesMS[boneIdx] = reinterpret_cast<const Matrix&>(mat);

			continue;
		}

#ifdef USE_HANDOPTIMIZED_TRANSFORM_CALC
		const AnimQsTransform& parentMS = bonesMSTransforms[parentIdx];

		RED_ASSERT( parentMS.Rotation.Quat.IsOk(), TXT("Parent animation rotation data appears to be courrpt!") );
		RED_ASSERT( parentMS.Translation.IsOk(), TXT("Parent animation translation data appears to be courrpt!") );
		RED_ASSERT( parentMS.Scale.IsOk(), TXT("Parent animation scale data appears to be courrpt!") );
		RED_ASSERT( m_outputPose[boneIdx].Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be courrpt!") );
		RED_ASSERT( m_outputPose[boneIdx].Translation.IsOk(), TXT("Animation translation data appears to be courrpt!") );
		RED_ASSERT( m_outputPose[boneIdx].Scale.IsOk(), TXT("Animation scale data appears to be courrpt!") );
		// A
		const __m128 parentTranslation = _mm_load_ps( &parentMS.Translation.V[0] );
		const __m128 parentQuaternion = _mm_load_ps( &parentMS.Rotation.Quat.V[0] );

		// B
		const __m128 outPoseTranslation = _mm_load_ps( &m_outputPose[boneIdx].Translation.V[0] );
		const __m128 outPoseQuaternion = _mm_load_ps( &m_outputPose[boneIdx].Rotation.Quat.V[0] );
		
		// MASK: 0,0,0,1
		const __m128 mask = _mm_shuffle_ps( _mm_set_ss( 1.0f ), _mm_set_ss( 1.0f ), _MM_SHUFFLE( 0, 1, 2, 3 ) );
		// MASK: 1,1,1,0
		const __m128 invMask = _mm_sub_ps( _mm_set_ps1( 1.0f ), mask );

		// Scale = Scale * Scale
		_mm_store_ps( &boneTransform.Scale.V[0], _mm_mul_ps( _mm_load_ps( &parentMS.Scale.V[0] ), _mm_load_ps( &m_outputPose[boneIdx].Scale.V[0] ) ) );

		// RotateDirection( parentQuaternion, outPoseTranslation )
		const __m128 qReal = _mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE( 3, 3, 3, 3 ) );
		const __m128 q2Minus1 = _mm_sub_ps( _mm_mul_ps( qReal, qReal ), _mm_set_ps1( 0.5f ) );
		__m128 ret = _mm_mul_ps( outPoseTranslation, q2Minus1 );
		
		// ++DOT_PRODUCT
		__m128 v0 = _mm_mul_ps( _mm_mul_ps( parentQuaternion, invMask ), _mm_mul_ps( outPoseTranslation, invMask ) );
		__m128 v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 2, 3, 0, 1 ) );
		v0 = _mm_add_ps( v0, v1 );
		v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		__m128 imagDotDir = _mm_add_ps( v0, v1 );
		// --DOT_PRODUCT
		
		ret = _mm_add_ps( ret, _mm_mul_ps( parentQuaternion, imagDotDir ) );

		// ++CROSS_PRODUCT
		__m128 imagCrossDir = _mm_sub_ps( 
			_mm_mul_ps( _mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE(3, 0, 2, 1) ), 
			_mm_shuffle_ps( outPoseTranslation, outPoseTranslation, _MM_SHUFFLE( 3, 1, 0, 2 ) ) ), 
			_mm_mul_ps( _mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE(3, 1, 0, 2) ), 
			_mm_shuffle_ps( outPoseTranslation, outPoseTranslation, _MM_SHUFFLE(3, 0, 2, 1) ) ) );
		// --CROSS_PRODUCT
		imagCrossDir = _mm_add_ps( _mm_mul_ps( imagCrossDir, invMask ), mask );

		ret = _mm_add_ps( ret, _mm_mul_ps( imagCrossDir, qReal ) );
		ret = _mm_add_ps( ret, ret );

		_mm_store_ps( &boneTransform.Translation.V[0], _mm_add_ps( parentTranslation, ret ) );
		
		// ++QUATERNION_MUL
		const __m128 wSplatA = _mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE( 3, 3, 3, 3 ) );
		const __m128 wSplatB = _mm_shuffle_ps( outPoseQuaternion, outPoseQuaternion, _MM_SHUFFLE( 3, 3, 3, 3 ) );

		// ++CrossProduct
		ret = _mm_sub_ps( 
			_mm_mul_ps( _mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE(3, 0, 2, 1)), 
			_mm_shuffle_ps( outPoseQuaternion, outPoseQuaternion, _MM_SHUFFLE( 3, 1, 0, 2 ) ) ), 
			_mm_mul_ps(_mm_shuffle_ps( parentQuaternion, parentQuaternion, _MM_SHUFFLE(3, 1, 0, 2)), 
			_mm_shuffle_ps( outPoseQuaternion, outPoseQuaternion, _MM_SHUFFLE(3, 0, 2, 1) ) ) 
			);
		// --CrossProduct
		ret = _mm_mul_ps( ret, invMask );
		
		ret = _mm_add_ps( ret, _mm_mul_ps( outPoseQuaternion, wSplatA ) );
		ret = _mm_add_ps( ret, _mm_mul_ps( parentQuaternion, wSplatB ) );

		//// ++Dot_Product
		v0 = _mm_mul_ps( _mm_mul_ps( parentQuaternion, invMask ), _mm_mul_ps( outPoseQuaternion, invMask ) );
		v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 2, 3, 0, 1 ) );
		v0 = _mm_add_ps( v0, v1 );
		v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		imagDotDir = _mm_add_ps( v0, v1 );
		//// --Dot_Product
		__m128 abMul = _mm_mul_ps( wSplatA, wSplatB );
		abMul = _mm_sub_ps( abMul, imagDotDir );
		const __m128 wVal = _mm_mul_ps( abMul, mask );
		__m128 boneTransformVal = _mm_add_ps( _mm_mul_ps( ret, invMask ), wVal );
		// --QUATERNION_MUL


		// ++Normalize
		v0 = _mm_mul_ps( boneTransformVal, boneTransformVal );
		v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 2, 3, 0, 1 ) );
		v0 = _mm_add_ps( v0, v1 );
		v1 = _mm_shuffle_ps( v0, v0, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		imagDotDir = _mm_add_ps( v0, v1 );
		
		float length = _mm_cvtss_f32( _mm_sqrt_ss( imagDotDir ) );
		if( length != 0.0f )
		{
			__m128 unitLength = _mm_rsqrt_ps(imagDotDir);
			boneTransformVal = _mm_mul_ps(boneTransformVal, unitLength);
		}
		// --Normalize
		_mm_store_ps( &boneTransform.Rotation.Quat.V[0], boneTransformVal );

		const RedMatrix4x4 mat = boneTransform.ConvertToMatrix();
		// ++ConvertToMatrix
		const __m128 boneTranslation = _mm_load_ps( &boneTransform.Translation.V[0] );
		const __m128 boneRotation = _mm_load_ps( &boneTransform.Rotation.Quat.V[0] );
		const __m128 boneRot3 = _mm_mul_ps( boneRotation, invMask );
		const __m128 boneScale = _mm_mul_ps( _mm_load_ps( &boneTransform.Scale.V[0] ), invMask );

		const __m128 xyz2 = _mm_add_ps( boneRot3, boneRot3 );
		
		//////////////////////////////////////////////////////////////////////////
		//  ( 1.0f - ( yy + zz ) ) * Scale.V[0], ( xy + wz ), ( xz - wy ), 0.0f ),
		//	( xy - wz ), ( 1.0f - ( xx + zz ) ) * Scale.V[1], ( yz + wx ), 0.0f ),
		//	( xz + wy ), ( yz - wx ), ( 1.0f - ( xx + yy ) ) * Scale.V[2], 0.0f ),
		//	Translation.V[0], Translation.V[1], Translation.V[2], 1.0f 
		//////////////////////////////////////////////////////////////////////////

		// Y * y2, X * x2, X * x2, 0.0
		__m128 matShufA = _mm_mul_ps( _mm_shuffle_ps( boneRot3, boneRot3, _MM_SHUFFLE( 3, 0, 0, 1 ) ), _mm_shuffle_ps( xyz2, xyz2, _MM_SHUFFLE( 3, 0, 0, 1 ) ) );
		// Z * z2, Z * z2, Y * Y2
		__m128 matShufB = _mm_mul_ps( _mm_shuffle_ps( boneRot3, boneRot3, _MM_SHUFFLE( 3, 1, 2, 2 ) ), _mm_shuffle_ps( xyz2, xyz2, _MM_SHUFFLE( 3, 1, 2, 2 ) ) );
		const __m128 matScaleVal = _mm_mul_ps( _mm_sub_ps( _mm_set_ps1( 1.0f ), _mm_add_ps( matShufA, matShufB ) ), boneScale );

		// X * y2, Y * z2, X * z2
		matShufA = _mm_mul_ps( _mm_shuffle_ps( boneRot3, boneRot3, _MM_SHUFFLE( 3, 0, 1,  0 ) ), _mm_shuffle_ps( xyz2, xyz2, _MM_SHUFFLE( 3, 2, 2, 1 ) ) );
		// W * z2, W * x2, W * y2
		matShufB = _mm_mul_ps( _mm_mul_ps( _mm_shuffle_ps( boneRotation, boneRotation, _MM_SHUFFLE( 3,3,3,3 ) ), invMask ), _mm_shuffle_ps( xyz2, xyz2, _MM_SHUFFLE(3, 1, 0, 2) ) );
		const __m128 negVals = _mm_sub_ps( matShufA, matShufB );
		
		// X * z2, X * y2, Y * z2
		matShufA = _mm_mul_ps( _mm_shuffle_ps( boneRot3, boneRot3, _MM_SHUFFLE( 3, 1, 0, 0 ) ), _mm_shuffle_ps( xyz2, xyz2, _MM_SHUFFLE( 3, 2, 1, 2 ) ) );
		// W * y2, W * z2, W * x2
		matShufB = _mm_shuffle_ps( matShufB, matShufB, _MM_SHUFFLE(3, 1, 0, 2 ) );
		const __m128 addVals = _mm_add_ps( matShufA, matShufB );

		// SCALE, ADDVAL, NEGVAL
		__m128 rowA = _mm_setzero_ps();
		rowA = _mm_add_ps( rowA, _mm_mul_ps( matScaleVal, _mm_shuffle_ps( mask, mask,	_MM_SHUFFLE( 0, 0, 0, 3 ) ) ) );
		rowA = _mm_add_ps( rowA, _mm_mul_ps( addVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 0, 3, 0 ) ) ) );
		rowA = _mm_add_ps( rowA, _mm_mul_ps( negVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 3, 0, 0 ) ) ) );

		// NEGVAL, SCALE, ADDVAL
		__m128 rowB = _mm_setzero_ps();
		rowB = _mm_add_ps( rowB, _mm_mul_ps( negVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 0, 0, 3 ) ) ) );
		rowB = _mm_add_ps( rowB, _mm_mul_ps( matScaleVal, _mm_shuffle_ps( mask, mask,	_MM_SHUFFLE( 0, 0, 3, 0 ) ) ) );
		rowB = _mm_add_ps( rowB, _mm_mul_ps( addVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 3, 0, 0 ) ) ) );
		

		// ADDVAL, NEGVAL, SCALE
		__m128 rowC = _mm_setzero_ps();
		rowC = _mm_add_ps( rowC, _mm_mul_ps( addVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 0, 0, 3 ) ) ) );
		rowC = _mm_add_ps( rowC, _mm_mul_ps( negVals, _mm_shuffle_ps( mask, mask,		_MM_SHUFFLE( 0, 0, 3, 0 ) ) ) );
		rowC = _mm_add_ps( rowC, _mm_mul_ps( matScaleVal, _mm_shuffle_ps( mask, mask,	_MM_SHUFFLE( 0, 3, 0, 0 ) ) ) );

		__m128 rowD = _mm_add_ps( _mm_mul_ps( boneTranslation, invMask ), mask );
		Matrix& currentMatrix = bonesMS[boneIdx];
		_mm_store_ps( &currentMatrix.V[0].A[0], rowA );
		_mm_store_ps( &currentMatrix.V[1].A[0], rowB );
		_mm_store_ps( &currentMatrix.V[2].A[0], rowC );
		_mm_store_ps( &currentMatrix.V[3].A[0], rowD );
		// --ConvertToMatrix

		RED_ASSERT( currentMatrix.IsOk(), TXT("Animation data appears to be courrpt, resulting in a bad matrix being output! Expect weird behaviour!") );

#else
		ASSERT( parentIdx < (Int32)boneIdx );
		// Get parent bone in model space
		const AnimQsTransform& parentMS = bonesMSTransforms[parentIdx];
		// Calc bone in model space
		boneTransform.SetMul( parentMS, m_outputPose[boneIdx] );
		boneTransform.Rotation.Normalize();
		ASSERT( boneTransform.Rotation.IsOk() );	
		const RedMatrix4x4 mat = boneTransform.ConvertToMatrix();
		bonesMS[boneIdx] = reinterpret_cast<const Matrix&>(mat);
#endif
	}
}

void SBehaviorGraphOutput::FillPoseWithBonesModelSpace( const CAnimatedComponent *animComponent, TDynArray<Matrix>& bonesMS )
{
	FillPoseWithBonesModelSpace( animComponent->GetSkeleton(), bonesMS );
}

void SBehaviorGraphOutput::FillPoseWithBonesModelSpace( const CSkeleton *skeleton, TDynArray<Matrix>& bonesMS )
{
	if ( !skeleton || !skeleton->IsValid() || bonesMS.Size() != m_numBones ) 
	{
		// Invalid input data
		return;
	}

	// To many bones
	AnimQsTransform bonesMSTransforms[ 512 ];
	if ( bonesMS.Size() >= ARRAY_COUNT( bonesMSTransforms ) )
	{
		WARN_ENGINE( TXT("Unable to get bone transforms") );
		return;
	}

	const Uint32 bonesNum = Min( m_numBones, (Uint32)skeleton->GetBonesNum() );

	// Get matrices as transforms
	{
		AnimQsTransform* destMSTransform = bonesMSTransforms;
		Matrix const * srcBoneMS = bonesMS.TypedData();
		for (Uint32 boneIdx=0; boneIdx < bonesNum; ++ boneIdx, ++ destMSTransform, ++ srcBoneMS)
		{
			*destMSTransform = MatrixToAnimQsTransform( *srcBoneMS );
		}
	}

	// Calculate LS transforms
	AnimQsTransform const * iBoneMSTransform = bonesMSTransforms;
	AnimQsTransform * iOutputBoneLS = m_outputPose;
	for (Uint32 boneIdx=0; boneIdx<bonesNum; ++boneIdx, ++iBoneMSTransform, ++ iOutputBoneLS)
	{
		AnimQsTransform const & boneMSTransform = *iBoneMSTransform;

		// Get parent index
		Int32 parentIdx = skeleton->GetParentBoneIndex( boneIdx );

		// If bone is not root
		if ( parentIdx != -1 )
		{
			ASSERT( parentIdx < (Int32)boneIdx );

			// Get parent bone in model space
			const AnimQsTransform& parentMS = bonesMSTransforms[parentIdx];

			// Calc bone in local space
			iOutputBoneLS->SetMulInverseMul( parentMS, boneMSTransform );
		}
		else
		{
			*iOutputBoneLS = boneMSTransform;
		}
	}
}

namespace
{
	void LazySetMul( RedQsTransform& out, RedVector4& rest, const RedQsTransform& a, const RedQsTransform& b )
	{
		// Save extra trans
		rest.RotateDirection(a.Rotation, b.Translation);

		// Transform
		out.Translation = a.Translation;

		// Rotation
		out.Rotation.SetMul( a.Rotation, b.Rotation );

		// No scale, sorry
		//...
	}
}

void SBehaviorGraphOutput::AdvanceEvents()
{
	for (Uint32 i=0; i<m_numEventsFired; i++)
	{
		CAnimationEventFired& eventFired = m_eventsFired[i];
		if ( eventFired.m_type == AET_DurationStart || eventFired.m_type == AET_DurationStartInTheMiddle )
		{
			eventFired.m_type = AET_Duration;
		}
		if ( eventFired.m_type == AET_DurationEnd || eventFired.m_type == AET_Tick )
		{
			// remove this event - it should be no longer processed;
			-- m_numEventsFired;
			CAnimationEventFired* eventDest = &m_eventsFired[i];
			CAnimationEventFired* eventSrc = &m_eventsFired[i + 1];
			for (Uint32 j=i; j<m_numEventsFired; ++ j, ++ eventDest, ++ eventSrc)
			{
				*eventDest = *eventSrc;
			}
			-- i;
		}
	}

#ifdef RED_FATAL_ASSERT
	{
		for (Uint32 i=0; i<m_numEventsFired; i++)
		{
			const CAnimationEventFired& eventFired = m_eventsFired[i];

			RED_FATAL_ASSERT( eventFired.m_extEvent, "Anim event ext event is nullptr" );
		}
	}
#endif
}

Bool SBehaviorGraphOutput::AppendEvent( const CAnimationEventFired& eventFired, Float alpha/* =1.0f  */)
{
	if ( m_eventsFired == NULL )
	{
		return false;
	}

	RED_FATAL_ASSERT( eventFired.m_extEvent, "Anim event ext event is nullptr" );

	// Merge new event
	for (Uint32 i=0; i<m_numEventsFired; i++)
	{
		if ( m_eventsFired[i].CanMergeWith(eventFired) )
		{
			m_eventsFired[i].MergeWith(eventFired, alpha);
			return false;
		}
	}

	// Add new event
	m_eventsFired[m_numEventsFired] = eventFired;
	m_eventsFired[m_numEventsFired].m_alpha *= alpha;

	// Inc events counter
	++m_numEventsFired;

	RED_FATAL_ASSERT( m_numEventsFired < c_eventFiredMaxCount, "Behavior output event list overflow" );

	// Default number of eventFired allocated is 16. This memory is preallocated from CPosePool. 
	// A few case will need more. In such case, Allocate a new buffer only for this pose of 32 event.
	if ( m_numEventsFired == c_eventFiredDefaultCount && !m_ownEventFiredMemory )
	{
		CAnimationEventFired * oldEvent = m_eventsFired;
		m_eventsFired = nullptr;

		AllocateEventsFired();

		Red::System::MemoryCopy( m_eventsFired, oldEvent, sizeof( CAnimationEventFired ) * c_eventFiredDefaultCount );
	}
	
	return true;
}

Bool SBehaviorGraphOutput::AppendEventAsOverlay( const CAnimationEventFired& eventFired, Float alpha/* =1.0f  */)
{
	if ( m_eventsFired == NULL )
	{
		return false;
	}

	RED_FATAL_ASSERT( eventFired.m_extEvent, "Anim event ext event is nullptr" );

	// Merge new event
	for (Uint32 i=0; i<m_numEventsFired; i++)
	{
		if ( m_eventsFired[i].CanMergeWith(eventFired) )
		{
			return false;
		}
	}

	// Add new event
	m_eventsFired[m_numEventsFired] = eventFired;
	m_eventsFired[m_numEventsFired].m_alpha *= alpha;

	// Inc events counter
	++m_numEventsFired;

	RED_FATAL_ASSERT( m_numEventsFired < c_eventFiredMaxCount, "Behavior output event list overflow" );

	// Default number of eventFired allocated is 16. This memory is preallocated from CPosePool. 
	// A few case will need more. In such case, Allocate a new buffer only for this pose of 32 event.
	if ( m_numEventsFired == c_eventFiredDefaultCount && !m_ownEventFiredMemory )
	{
		CAnimationEventFired * oldEvent = m_eventsFired;
		m_eventsFired = nullptr;

		AllocateEventsFired();

		Red::System::MemoryCopy( m_eventsFired, oldEvent, sizeof( CAnimationEventFired ) * c_eventFiredDefaultCount );
	}

	return true;
}

void SBehaviorGraphOutput::MergeEvents( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aEventAlphaMul/* =1.0f */, Float bEventAlphaMul/* =1.0f  */)
{
	if ( m_eventsFired == NULL )
	{
		return;
	}

	for (Uint32 i=0; i<a.m_numEventsFired; i++)
	{
		AppendEvent(a.m_eventsFired[i], aEventAlphaMul);
	}

	for (Uint32 i=0; i<b.m_numEventsFired; i++)
	{
		AppendEvent(b.m_eventsFired[i], bEventAlphaMul);
	}
}

void SBehaviorGraphOutput::MergeEventsAsOverlays( const SBehaviorGraphOutput &mainPose, const SBehaviorGraphOutput &overlayPose, Float aEventAlphaMul/* =1.0f */, Float bEventAlphaMul/* =1.0f  */)
{
	if ( m_eventsFired == NULL )
	{
		return;
	}

	for (Uint32 i=0; i<mainPose.m_numEventsFired; i++)
	{
		AppendEventAsOverlay(mainPose.m_eventsFired[i], aEventAlphaMul);
	}

	for (Uint32 i=0; i<overlayPose.m_numEventsFired; i++)
	{
		AppendEventAsOverlay(overlayPose.m_eventsFired[i], bEventAlphaMul);
	}
}

void SBehaviorGraphOutput::MergeEvents( const SBehaviorGraphOutput &a, Float aEventAlphaMul/* =1.0f  */)
{
	if ( m_eventsFired == NULL )
	{
		return;
	}

	for (Uint32 i=0; i<a.m_numEventsFired; i++)
	{
		AppendEvent(a.m_eventsFired[i], aEventAlphaMul);
	}
}

void SBehaviorGraphOutput::MergeEventsAsOverlays( const SBehaviorGraphOutput &overlayPose, Float aEventAlphaMul )
{
	if ( m_eventsFired == NULL )
	{
		return;
	}

	for (Uint32 i=0; i<overlayPose.m_numEventsFired; i++)
	{
		AppendEventAsOverlay(overlayPose.m_eventsFired[i], aEventAlphaMul);
	}
}

void SBehaviorGraphOutput::ClearEvents()
{
	m_numEventsFired = 0;
}

// those are portal methods, to make it consistent with event methods and to use nicely packed m_usedAnims member variable

Bool SBehaviorGraphOutput::AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight/* =1.0f  */)
{
	return m_usedAnims.AppendUsedAnim( usedAnim, weight );
}

Bool SBehaviorGraphOutput::AppendUsedOverlayAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight/* =1.0f  */)
{
	return m_usedAnims.AppendUsedOverlayAnim( usedAnim, weight );
}

Bool SBehaviorGraphOutput::AppendUsedAdditiveAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight/* =1.0f  */)
{
	return m_usedAnims.AppendUsedAdditiveAnim( usedAnim, weight );
}

void SBehaviorGraphOutput::MergeUsedAnims( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aAnimWeightMul/* =1.0f */, Float bAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnims( a.m_usedAnims, b.m_usedAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorGraphOutput::MergeUsedAnims( const SBehaviorGraphOutput &a, Float aAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnims( a.m_usedAnims, aAnimWeightMul );
}

void SBehaviorGraphOutput::MergeUsedAnimsAsOverlays( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aAnimWeightMul/* =1.0f */, Float bAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnimsAsOverlays( a.m_usedAnims, b.m_usedAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorGraphOutput::MergeUsedAnimsAsOverlays( const SBehaviorGraphOutput &a, Float aAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnimsAsOverlays( a.m_usedAnims, aAnimWeightMul );
}

void SBehaviorGraphOutput::MergeUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aAnimWeightMul/* =1.0f */, Float bAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnimsAsAdditives( a.m_usedAnims, b.m_usedAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorGraphOutput::MergeUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, Float aAnimWeightMul/* =1.0f  */)
{
	m_usedAnims.MergeUsedAnimsAsAdditives( a.m_usedAnims, aAnimWeightMul );
}

void SBehaviorGraphOutput::ClearUsedAnims()
{
	m_usedAnims.ClearUsedAnims();
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnims( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aWeightMul, Float bWeightMul )
{
	MergeEvents( a, b, aWeightMul, bWeightMul );
	MergeUsedAnims( a, b, aWeightMul, bWeightMul );
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnims( const SBehaviorGraphOutput &a, Float aWeightMul )
{
	MergeEvents( a, aWeightMul );
	MergeUsedAnims( a, aWeightMul );
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnimsAsOverlays( const SBehaviorGraphOutput &mainPose, const SBehaviorGraphOutput &overlayPose, Float aWeightMul, Float bWeightMul )
{
	MergeEventsAsOverlays( mainPose, overlayPose, aWeightMul, bWeightMul );
	MergeUsedAnimsAsOverlays( mainPose, overlayPose, aWeightMul, bWeightMul );
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnimsAsOverlays( const SBehaviorGraphOutput &overlayPose, Float aWeightMul )
{
	MergeEventsAsOverlays( overlayPose, aWeightMul );
	MergeUsedAnimsAsOverlays( overlayPose, aWeightMul );
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float aWeightMul, Float bWeightMul )
{
	MergeEvents( a, b, aWeightMul, bWeightMul );
	MergeUsedAnimsAsAdditives( a, b, aWeightMul, bWeightMul );
}

void SBehaviorGraphOutput::MergeEventsAndUsedAnimsAsAdditives( const SBehaviorGraphOutput &a, Float aWeightMul )
{
	MergeEvents( a, aWeightMul );
	MergeUsedAnimsAsAdditives( a, aWeightMul );
}

void SBehaviorGraphOutput::ClearEventsAndUsedAnims()
{
	ClearEvents();
	ClearUsedAnims();
}

void SBehaviorGraphOutput::SetAddMulME( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, Float weight )
{
	m_deltaReferenceFrameLocal = a.m_deltaReferenceFrameLocal;
	m_deltaReferenceFrameLocal.BlendAddMul( b.m_deltaReferenceFrameLocal, weight );
}

void SBehaviorGraphOutput::SetAddMulME( const SBehaviorGraphOutput &b, Float weight )
{
	SetAddMulME( *this, b, weight );
}

void SBehaviorGraphOutput::SetInterpolateME( const SBehaviorGraphOutput &a, const SBehaviorGraphOutput &b, float alpha )
{
	m_deltaReferenceFrameLocal.Slerp( a.m_deltaReferenceFrameLocal, b.m_deltaReferenceFrameLocal, alpha );
}

Uint32 SBehaviorGraphOutput::GetMemSize() const
{
	Uint32 ret = 0;

	ret += sizeof( SBehaviorGraphOutput );
	ret += sizeof( AnimQsTransform ) * ( m_numBones - 1 );
	ret += sizeof( AnimFloat ) * ( m_numFloatTracks - 1 );
	ret += sizeof( CAnimationEventFired ) * ( c_eventFiredDefaultCount - 1 );
	ret += m_usedAnims.GetMemSize();

	return ret;
}

void SBehaviorGraphOutput::AllocateFloatTracks()
{
	if( m_numFloatTracks )
	{
		RED_FATAL_ASSERT( !m_floatTracks, "Float tracks already allocated." );
		m_floatTracks = reinterpret_cast< Float* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Animation, MC_PoseBuffer, sizeof( AnimFloat ) * m_numFloatTracks, __alignof( AnimFloat ) ) );
		m_ownFloatTrackMemory = true;
	}
}

void SBehaviorGraphOutput::DeallocateFloatTracks()
{
	if( m_ownFloatTrackMemory )
	{
		RED_MEMORY_FREE( MemoryPool_Animation, MC_PoseBuffer, m_floatTracks );
		m_floatTracks = nullptr;
		m_ownFloatTrackMemory = false;
	}	
}

void SBehaviorGraphOutput::AllocateOutputPose()
{
	if ( m_numBones )
	{
		RED_FATAL_ASSERT( !m_outputPose, "Bone buffer already allocated." );
		m_outputPose = static_cast< AnimQsTransform * >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Animation, MC_PoseBuffer, sizeof( AnimQsTransform ) * m_numBones, __alignof( AnimQsTransform ) ) );
		m_ownPoseMemory = true;
	}
}

void SBehaviorGraphOutput::DeallocateOutputPose()
{
	if( m_ownPoseMemory )
	{
		RED_MEMORY_FREE( MemoryPool_Animation, MC_PoseBuffer, m_outputPose );
		m_outputPose = nullptr;
		m_ownPoseMemory = false;
	}
}

void SBehaviorGraphOutput::AllocateEventsFired()
{
	RED_FATAL_ASSERT( !m_eventsFired, "Event buffer already allocated." );
	m_eventsFired = new CAnimationEventFired[c_eventFiredMaxCount];
	m_ownEventFiredMemory = true;
}

void SBehaviorGraphOutput::DeallocateEventFired()
{
	if( m_ownEventFiredMemory )
	{
		delete [] m_eventsFired;
		m_eventsFired = nullptr;
		m_ownEventFiredMemory = false;
	}
}

void SBehaviorGraphOutput::SetEventFiredMemory( CAnimationEventFired * events )
{
	DeallocateEventFired();
	m_eventsFired = events;
}

//////////////////////////////////////////////////////////////////////////

SBehaviorUsedAnimationDataSet::SBehaviorUsedAnimationDataSet( Uint32 slotsToReserve )
{
	m_usedAnims.Reserve( slotsToReserve );
}

SBehaviorUsedAnimationDataSet& SBehaviorUsedAnimationDataSet::operator=( const SBehaviorUsedAnimationDataSet& uads )
{
	m_usedAnims.CopyFast( uads.m_usedAnims );
	return *this;
}

Bool SBehaviorUsedAnimationDataSet::AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight/* =1.0f  */)
{
	// Merge new event
	const Uint32 count = m_usedAnims.Size();
	for (Uint32 i=0; i<count; i++)
	{
		if ( m_usedAnims[i].CanMergeWith(usedAnim) )
		{
			m_usedAnims[i].MergeWith(usedAnim, weight);
			return false;
		}
	}

	Float newWeight = usedAnim.m_weight * weight;
	m_usedAnims.PushBack( usedAnim );
	m_usedAnims[ m_usedAnims.Size() - 1 ].m_weight = newWeight;
	return true;
}

void SBehaviorUsedAnimationDataSet::MergeUsedAnims( const SBehaviorUsedAnimationDataSet &a, const SBehaviorUsedAnimationDataSet &b, Float aAnimWeightMul/* =1.0f */, Float bAnimWeightMul/* =1.0f  */)
{
	Uint32 count = a.GetNum();
	for (Uint32 i=0; i<count; i++)
	{
		AppendUsedAnim(a.m_usedAnims[i], aAnimWeightMul);
	}

	count = b.GetNum();
	for (Uint32 i=0; i<count; i++)
	{
		AppendUsedAnim(b.m_usedAnims[i], bAnimWeightMul);
	}
}

void SBehaviorUsedAnimationDataSet::MergeUsedAnims( const SBehaviorUsedAnimationDataSet &a, Float aAnimWeightMul/* =1.0f  */)
{
	Uint32 count = a.GetNum();
	for (Uint32 i=0; i<count; i++)
	{
		AppendUsedAnim(a.m_usedAnims[i], aAnimWeightMul);
	}
}

void SBehaviorUsedAnimationDataSet::ClearUsedAnims()
{
	m_usedAnims.Clear();
}

const SBehaviorUsedAnimationData* SBehaviorUsedAnimationDataSet::FindWithHighestWeight() const
{
	const SBehaviorUsedAnimationData* usedAnim = m_usedAnims.TypedData();
	Float highestWeight = 0.0f;
	const SBehaviorUsedAnimationData* bestAnim = nullptr;
	for ( Int32 idx = m_usedAnims.Size(); idx > 0; -- idx, ++ usedAnim )
	{
		if ( ! bestAnim || highestWeight < usedAnim->m_weight )
		{
			highestWeight = usedAnim->m_weight;
			bestAnim = usedAnim;
		}
	}
	return bestAnim;
}

//////////////////////////////////////////////////////////////////////////

Uint32 SBehaviorUsedAnimations::GetMemSize() const
{
	return m_anims.GetMemSize() + m_overlayAnims.GetMemSize() + m_additiveAnims.GetMemSize();
}

SBehaviorUsedAnimations& SBehaviorUsedAnimations::operator=( const SBehaviorUsedAnimations& uads )
{
	m_anims = uads.m_anims;
	m_overlayAnims = uads.m_overlayAnims;
	m_additiveAnims = uads.m_additiveAnims;
	return *this;
}

Bool SBehaviorUsedAnimations::AppendUsedAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight )
{
	return m_anims.AppendUsedAnim( usedAnim, weight );
}

Bool SBehaviorUsedAnimations::AppendUsedOverlayAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight )
{
	return m_overlayAnims.AppendUsedAnim( usedAnim, weight );
}

Bool SBehaviorUsedAnimations::AppendUsedAdditiveAnim( const SBehaviorUsedAnimationData& usedAnim, Float weight )
{
	return m_additiveAnims.AppendUsedAnim( usedAnim, weight );
}

void SBehaviorUsedAnimations::MergeUsedAnims( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul, Float bAnimWeightMul )
{
	m_anims.MergeUsedAnims( a.m_anims, b.m_anims, aAnimWeightMul, bAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_overlayAnims, b.m_overlayAnims, aAnimWeightMul, bAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_additiveAnims, b.m_additiveAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorUsedAnimations::MergeUsedAnims( const SBehaviorUsedAnimations &a, Float aAnimWeightMul )
{
	m_anims.MergeUsedAnims( a.m_anims, aAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_overlayAnims, aAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_additiveAnims, aAnimWeightMul );
}

void SBehaviorUsedAnimations::MergeUsedAnimsAsOverlays( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul, Float bAnimWeightMul )
{
	m_overlayAnims.MergeUsedAnims( a.m_anims, b.m_anims, aAnimWeightMul, bAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_overlayAnims, b.m_overlayAnims, aAnimWeightMul, bAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_additiveAnims, b.m_additiveAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorUsedAnimations::MergeUsedAnimsAsOverlays( const SBehaviorUsedAnimations &a, Float aAnimWeightMul )
{
	m_overlayAnims.MergeUsedAnims( a.m_anims, aAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_overlayAnims, aAnimWeightMul );
	m_overlayAnims.MergeUsedAnims( a.m_additiveAnims, aAnimWeightMul );
}

void SBehaviorUsedAnimations::MergeUsedAnimsAsAdditives( const SBehaviorUsedAnimations &a, const SBehaviorUsedAnimations &b, Float aAnimWeightMul, Float bAnimWeightMul )
{
	m_additiveAnims.MergeUsedAnims( a.m_anims, b.m_anims, aAnimWeightMul, bAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_overlayAnims, b.m_overlayAnims, aAnimWeightMul, bAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_additiveAnims, b.m_additiveAnims, aAnimWeightMul, bAnimWeightMul );
}

void SBehaviorUsedAnimations::MergeUsedAnimsAsAdditives( const SBehaviorUsedAnimations &a, Float aAnimWeightMul )
{
	m_additiveAnims.MergeUsedAnims( a.m_anims, aAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_overlayAnims, aAnimWeightMul );
	m_additiveAnims.MergeUsedAnims( a.m_additiveAnims, aAnimWeightMul );
}

void SBehaviorUsedAnimations::ClearUsedAnims()
{
	m_anims.ClearUsedAnims();
	m_overlayAnims.ClearUsedAnims();
	m_additiveAnims.ClearUsedAnims();
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif