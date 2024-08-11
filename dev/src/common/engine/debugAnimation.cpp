/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animMath.h"
#include "debugAnimation.h"
#include "skeleton.h"

#ifndef NO_DEFAULT_ANIM


IMPLEMENT_ENGINE_CLASS( CDebugAnimation );

CDebugAnimation::CDebugAnimation()
	: m_skeleton( NULL )
	, m_boneLeft( -1 )
	, m_boneRight( -1 )
{
	SetName( CNAME( DebugAnimation ) );
	m_duration = 1.0f;
	m_framesPerSecond = 30.0f;
}

void CDebugAnimation::Initialize( const CSkeleton* skeleton )
{
	ASSERT( skeleton );

	m_skeleton = skeleton;

	CacheBones();
}

void CDebugAnimation::OnSerialize( IFile &file )
{
	// Skip skeletal animation serialization
	ISerializable::OnSerialize( file );
}

Bool CDebugAnimation::HasExtractedMotion() const
{
	return false;
}
//TODO: Shoudn't these functions be removed ??? - Chris H
AnimQsTransform CDebugAnimation::GetMovementAtTime( Float time ) const
{
	ASSERT( !TXT("Dont call me!") );
	return AnimQsTransform::IDENTITY;
}

//TODO: Shoudn't these functions be removed ??? - Chris H
AnimQsTransform CDebugAnimation::GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const
{
	ASSERT( !TXT("Dont call me!") );
	return AnimQsTransform::IDENTITY;
}

Bool CDebugAnimation::IsCompressed() const
{
	return true;
}

Bool CDebugAnimation::GenerateBoundingBox( const CAnimatedComponent* component )
{
	ASSERT( !TXT("Dont call me!") );
	return false;
}

void CDebugAnimation::Preload() const
{
}

void CDebugAnimation::SyncLoad() const
{
}

EAnimationBufferDataAvailable CDebugAnimation::GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const
{
#ifndef RED_FINAL_BUILD
	outBonesLoaded = bonesRequested;
	outBonesAlwaysLoaded = bonesRequested;
	return ABDA_All;
#else
	outBonesLoaded = 0;
	outBonesAlwaysLoaded = 0;
	return ABDA_None;
#endif
}

Uint32 CDebugAnimation::GetTracksNum() const
{
	return m_skeleton ? m_skeleton->GetTracksNum() : 0;
}

Uint32 CDebugAnimation::GetBonesNum() const
{
	return m_skeleton ? m_skeleton->GetBonesNum() : 0;
}

Bool CDebugAnimation::Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const
{
	SetTPose( boneNumIn, bonesOut );

#ifndef RED_FINAL_BUILD
	if ( CanAddProceduralAnim() )
	{
		ASSERT( time <= 1.f && time >= 0.f );

		return SampleProceduralAnim( time, boneNumIn, bonesOut );
	}
#endif

	return true;
}

Bool CDebugAnimation::Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut ) const
{
	SetTPose( bonesOut.Size(), bonesOut.TypedData() );

#ifndef RED_FINAL_BUILD
	if ( CanAddProceduralAnim() )
	{
		ASSERT( time <= 1.f && time >= 0.f );

		return SampleProceduralAnim( time, bonesOut.Size(), bonesOut.TypedData() );
	}
#endif

	return true;
}

Bool CDebugAnimation::SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const
{
	return Sample( 0.0f, boneNumIn, tracksNumIn, bonesOut, tracksOut );
}

void CDebugAnimation::CacheBones()
{
	ASSERT( m_skeleton );
	m_boneLeft = m_skeleton->FindBoneByName( TXT("l_bicep") );
	m_boneRight = m_skeleton->FindBoneByName( TXT("r_bicep") );
}

void CDebugAnimation::SetTPose( Uint32 bonesNum, AnimQsTransform* bones ) const
{
	if ( m_skeleton && m_skeleton->GetBonesNum() == (Int32)bonesNum )
	{
		VERIFY( m_skeleton->CopyReferencePoseLSTo( bonesNum, bones ) );
	}
}

Bool CDebugAnimation::CanAddProceduralAnim() const
{
	return m_skeleton && m_boneLeft != -1 && m_boneRight != -1;
}

Bool CDebugAnimation::SampleProceduralAnim( Float time, Uint32 boneNumIn, AnimQsTransform* bonesOut ) const
{
	// Only rotation
	if ( m_boneLeft < (Int32)boneNumIn && m_boneRight < (Int32)boneNumIn )
	{
		RotateBone( m_boneLeft, boneNumIn, bonesOut, time );
		RotateBone( m_boneRight, boneNumIn, bonesOut, -time );

		return true;
	}

	return false;
}

void CDebugAnimation::RotateBone( Int32 boneIndex, Uint32 bonesNum, AnimQsTransform* bones, Float animProgress ) const
{
	const Float progress = Red::Math::MSin( 2.f * M_PI * animProgress );
	const AnimQuaternion rotationQuat( AnimVector4( 0, 1, 0, 1 ), DEG2RAD( 25.f * progress ) );
	const AnimQsTransform rotation( AnimVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, AnimVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	bones[ boneIndex ].SetMul( bones[ boneIndex ], rotation );
}

#endif
