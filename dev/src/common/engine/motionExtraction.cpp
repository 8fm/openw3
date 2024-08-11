/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "havokAnimationUtils.h"
#include "motionExtraction.h"

IMPLEMENT_ENGINE_CLASS( IMotionExtraction );
IMPLEMENT_ENGINE_CLASS( CHavokMotionExtraction );
IMPLEMENT_ENGINE_CLASS( CLineMotionExtraction );
IMPLEMENT_ENGINE_CLASS( CUncompressedMotionExtraction );
IMPLEMENT_ENGINE_CLASS( CLineMotionExtraction2 );


//////////////////////////////////////////////////////////////////////////
void IMotionExtraction::GetMovementAtTime( Float time, AnimQsTransform& motion ) const
{
	motion = AnimQsTransform::IDENTITY;
}

void IMotionExtraction::GetMovementBetweenTime( Float timeStart, Float timeEnd, Int32 loops, AnimQsTransform& motion ) const
{
	// Grab motion at the start
	AnimQsTransform curMotion;
	GetMovementAtTime( timeStart, curMotion );

	// Motion at the end
	AnimQsTransform futMotion; 
	GetMovementAtTime( timeEnd, futMotion );

	// Handle complete loops
	// This assumes that the number of loops will usually be small
	// i.e. it will take longer to convert the quaternion to axis angle
	if ( loops != 0 )
	{
		AnimQsTransform fullCycle; 	
		GetMovementAtTime( GetDuration(), fullCycle );

		const Uint32 fabsloops = (loops < 0) ? -loops : loops;
		for (Uint32 l=0; l < fabsloops; l++)
		{
			if ( loops < 0 )
			{
				// Underflow
				AnimQsTransform temp;
#ifdef USE_HAVOK_ANIMATION
				temp.setMulInverseMul( fullCycle, futMotion );
#else
				temp.SetMulInverseMul( fullCycle, futMotion );
#endif
				futMotion = temp;
			}
			else
			{
				// Overflow
				AnimQsTransform temp;
#ifdef USE_HAVOK_ANIMATION
				temp.setMul( fullCycle, futMotion );
#else
				temp.SetMul( fullCycle, futMotion );
#endif
				futMotion = temp;
			}
		}
	}

	//Compute output
#ifdef USE_HAVOK_ANIMATION
	motion.setMulInverseMul( curMotion, futMotion );
#else
	motion.SetMulInverseMul( curMotion, futMotion );
#endif
}

Uint32 IMotionExtraction::GetDataSize() const
{
	return 0;
}

Float IMotionExtraction::GetDuration() const
{
	return 1.f;
}

Bool IMotionExtraction::IsCompressed() const
{
	return false;
}

void IMotionExtraction::OnSerialize( IFile &file  )
{
	ISerializable::OnSerialize( file );
}

//////////////////////////////////////////////////////////////////////////

CUncompressedMotionExtraction::CUncompressedMotionExtraction()
	: m_duration( 1.f )
{

}

const TDynArray< Vector >& CUncompressedMotionExtraction::GetUncompressedFrames() const
{
	return m_frames;
}

CUncompressedMotionExtraction* CUncompressedMotionExtraction::CreateCopy()
{
	CUncompressedMotionExtraction* ret = new CUncompressedMotionExtraction;
	ret->m_duration = m_duration;
	ret->m_frames = m_frames;
	return ret;
}

void CUncompressedMotionExtraction::GetMovementAtTime( Float time, AnimQsTransform& motionOut ) const
{
	const Uint32 numFrames = m_frames.Size();

	AnimVector4 motionOut_v;

	const AnimFloat timeSlice =  m_duration / ( numFrames - 1 );

	if ( time >= m_duration )
	{
		motionOut_v = reinterpret_cast<const AnimVector4&>( m_frames[ numFrames - 1 ] );
	}
	else
	{
		if ( time < 0.0f )
		{
			motionOut_v = reinterpret_cast<const AnimVector4&>( m_frames[ 0 ] );
		}
		else
		{
			const Int32 frame	= (Int32)( time / timeSlice );
			const AnimFloat delta = ( time / timeSlice ) - frame;

			const AnimVector4 motionA = reinterpret_cast<const AnimVector4&>( m_frames[ frame ] );
			const AnimVector4 motionB = reinterpret_cast<const AnimVector4&>( m_frames[ frame + 1 ] );

#ifdef USE_HAVOK_ANIMATION
			motionOut_v.setInterpolate4( motionA, motionB, delta );
#else
			motionOut_v = RedVector4::Lerp( motionA, motionB, delta );
#endif
		}
	}

#ifdef USE_HAVOK_ANIMATION
	// Construct bone transform
	static const hkVector4 up( 0.f, 0.f, 1.f );
	motionOut.m_translation = motionOut_v; // ignores w component
	motionOut.m_rotation.setAxisAngle( up, motionOut_v(3) );
	motionOut.m_scale.setAll( 1.0f );
#else
	// Construct bone transform
	motionOut.Translation = motionOut_v; // ignores w component
	motionOut.Rotation.SetAxisAngle( RedVector4::EZ, motionOut_v.W );
	motionOut.Scale.SetOnes();
#endif
}

Uint32 CUncompressedMotionExtraction::GetDataSize() const
{
	return static_cast< Uint32 >( sizeof( CUncompressedMotionExtraction ) + m_frames.DataSize() );
}

Float CUncompressedMotionExtraction::GetDuration() const
{
	return m_duration;
}

Bool CUncompressedMotionExtraction::Initialize( const CSkeletalAnimation::FactoryInfo& data )
{
#ifdef USE_HAVOK_ANIMATION
	if ( !data.m_motionExtraction )
	{
		return false;
	}

	const hkaDefaultAnimatedReferenceFrame* dme = static_cast< const hkaDefaultAnimatedReferenceFrame* >( data.m_motionExtraction );

	m_duration = dme->m_duration;

	m_frames.Resize( dme->m_numReferenceFrameSamples );
	for ( Uint32 i=0; i<m_frames.Size(); ++i )
	{
		m_frames[ i ] = TO_CONST_VECTOR_REF( dme->m_referenceFrameSamples[ i ] );
	}

	return true;
#else
	if ( data.m_motionFrames.Empty() )
	{
		return false;
	}

	m_duration = data.m_duration;
	m_frames.Resize(data.m_motionFrames.Size());
	for (Uint32 i=0; i<m_frames.Size(); ++i)
	{
		m_frames[i] = data.m_motionFrames[i];
	}

	return true;
#endif
}

Bool CUncompressedMotionExtraction::Initialize( const CSkeletalAnimation* animation )
{
#ifdef USE_HAVOK_ANIMATION
	if ( !animation->IsLoaded() || animation->GetAnimBuffer()->m_animNum == 0 )
	{
		return false;
	}
	const hkaAnimation* hkAnim = animation->GetAnimBuffer()->GetHavokAnimation( 0 );
	ASSERT( hkAnim->getType() == hkaAnimation::HK_SPLINE_COMPRESSED_ANIMATION );

	const hkaAnimatedReferenceFrame* me = ImportAnimationUtils::ExtractMotionFromAnimation( hkAnim );
	if ( me )
	{
		const hkaDefaultAnimatedReferenceFrame* dme = static_cast< const hkaDefaultAnimatedReferenceFrame* >( me );

		m_duration = dme->m_duration;

		m_frames.Resize( dme->m_numReferenceFrameSamples );
		for ( Uint32 i=0; i<m_frames.Size(); ++i )
		{
			m_frames[ i ] = TO_CONST_VECTOR_REF( dme->m_referenceFrameSamples[ i ] );
		}

		return true;
	}
#endif

	// this will never work in new implementation
	return false;
}

//////////////////////////////////////////////////////////////////////////

CHavokMotionExtraction::CHavokMotionExtraction()
{
}

void CHavokMotionExtraction::GetMovementAtTime( Float time, AnimQsTransform& motion ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaAnimatedReferenceFrame* extractedMotion = m_data.GetHavokObject();
	if ( extractedMotion )
	{
		extractedMotion->getReferenceFrame( time, motion );
	}
	else
	{
		ASSERT( extractedMotion );
	}
#else

#endif
}

void CHavokMotionExtraction::GetMovementBetweenTime( Float timeStart, Float timeEnd, Int32 loops, AnimQsTransform& motion ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaAnimatedReferenceFrame* extractedMotion = m_data.GetHavokObject();
	if ( extractedMotion )
	{
		extractedMotion->getDeltaReferenceFrame( timeStart, timeEnd, loops, motion );
	}
	else
	{
		ASSERT( extractedMotion );
	}
#else
	IMotionExtraction::GetMovementBetweenTime( timeStart, timeEnd, loops, motion );
#endif
}

void CHavokMotionExtraction::OnSerialize( IFile &file )
{
	ISerializable::OnSerialize( file );

#ifdef USE_HAVOK_ANIMATION
	if ( file.IsWriter() )
	{
		m_data.Serialize( file );
		m_data.Unload();
	}
	else
	{
		m_data.Serialize( file );
		m_data.Load();
	}
#else
#endif
}

Float CHavokMotionExtraction::GetDuration() const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaAnimatedReferenceFrame* extractedMotion = m_data.GetHavokObject();
	if ( extractedMotion )
	{
		return extractedMotion->getDuration();
	}
	else
	{
		return 0.f;
	}
#else
	return m_duration;
#endif
}

Bool CHavokMotionExtraction::HasData() const
{
#ifdef USE_HAVOK_ANIMATION
	return m_data.GetHavokObject() != NULL;
#else
	return !m_keys.Empty();
#endif
}

Uint32 CHavokMotionExtraction::GetDataSize() const
{
#ifdef USE_HAVOK_ANIMATION
	return sizeof( CHavokMotionExtraction ) + m_data.GetSize();
#else
	return (Uint32)(sizeof( CHavokMotionExtraction ) + m_keys.SizeOfAllElements());
#endif
}

Uint32 CHavokMotionExtraction::GetSamplesNum() const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaAnimatedReferenceFrame* extractedMotion = m_data.GetHavokObject();
	const hkaDefaultAnimatedReferenceFrame* def = static_cast< const hkaDefaultAnimatedReferenceFrame* >( extractedMotion );
	return def ? (Uint32)def->m_numReferenceFrameSamples : 0;
#else
	return m_keys.Size();
#endif
}

Bool CHavokMotionExtraction::Initialize( const CSkeletalAnimation::FactoryInfo& data )
{
#ifdef USE_HAVOK_ANIMATION
	if ( !data.m_motionExtraction )
	{
		return false;
	}

	m_data.CopyFromContainer( data.m_motionExtraction, &hkaDefaultAnimatedReferenceFrameClass, true );
	return true;
#else
	return false;
#endif
}

Bool CHavokMotionExtraction::Initialize( const CSkeletalAnimation* animation )
{
#ifdef USE_HAVOK_ANIMATION
	if ( !animation->IsLoaded() || animation->GetAnimBuffer()->m_animNum == 0 )
	{
		return false;
	}

	const hkaAnimation* hkAnim = animation->GetAnimBuffer()->GetHavokAnimation( 0 );
	ASSERT( hkAnim->getType() == hkaAnimation::HK_SPLINE_COMPRESSED_ANIMATION );

	const hkaAnimatedReferenceFrame* me = ImportAnimationUtils::ExtractMotionFromAnimation( hkAnim );
	if ( me )
	{
		m_data.CopyFromContainer( me, &hkaDefaultAnimatedReferenceFrameClass, true );

		return true;
	}
	
	return false;
#else
	// this will never work in new implementation
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////

CLineMotionExtraction::CLineMotionExtraction()
{

}

void CLineMotionExtraction::GetMovementAtTime( Float time, AnimQsTransform& motionOut ) const
{
	const Uint32 numFrames = m_frames.Size();
	const Float duration = GetDuration();

	AnimVector4 motionOut_v;

	if ( time >= duration )
	{
		motionOut_v = reinterpret_cast<const AnimVector4&>( m_frames[ numFrames - 1 ] );
	}
	else
	{
		if ( time < 0.0f )
		{
			motionOut_v = reinterpret_cast<const AnimVector4&>( m_frames[ 0 ] );
		}
		else
		{
			Int32 frame = 0;
			Float weight = 0.f;

			FindFrameAndWeight( time, frame, weight );

			const AnimVector4 motionA = reinterpret_cast<const AnimVector4&>( m_frames[ frame ] );
			const AnimVector4 motionB = reinterpret_cast<const AnimVector4&>( m_frames[ frame + 1 ] );

#ifdef USE_HAVOK_ANIMATION
			motionOut_v.setInterpolate4( motionA, motionB, weight );
#else
			motionOut_v = RedVector4::Lerp( motionA, motionB, weight );
#endif
		}
	}

	// Construct bone transform
#ifdef USE_HAVOK_ANIMATION
	static const hkVector4 up( 0.f, 0.f, 1.f );
	motionOut.m_translation = motionOut_v; // ignores w component
	motionOut.m_rotation.setAxisAngle( up, motionOut_v(3) );
	motionOut.m_scale.setAll( 1.0f );
#else
	// Construct bone transform
	motionOut.Translation = motionOut_v; // ignores w component
	motionOut.Rotation.SetAxisAngle( RedVector4::EZ, motionOut_v.W ); // z is up?
	motionOut.Scale.SetOnes();
#endif
}

Uint32 CLineMotionExtraction::GetDataSize() const
{
	return static_cast< Uint32 >( sizeof( CLineMotionExtraction ) + m_frames.DataSize() + m_times.DataSize() );
}

Float CLineMotionExtraction::GetDuration() const
{
	return m_times.Size() > 0 ? m_times.Back() : 1.f;
}

const TDynArray< Float >& CLineMotionExtraction::GetFrameTimes() const
{
	return m_times;
}

Bool CLineMotionExtraction::Initialize( TDynArray< Vector >& frames, TDynArray< Float >& times )
{
	m_frames = frames;
	m_times = times;
	return true;
}

void CLineMotionExtraction::FindFrameAndWeight( Float time, Int32& frame, Float& weight ) const
{
	ASSERT( time < GetDuration() );
	ASSERT( time >= 0.f );

	frame = 0;

	for ( Int32 i=m_times.SizeInt()-1; i>=0; --i )
	{
		if ( m_times[ i ] < time )
		{
			frame = i;
			break;
		}
	}

	ASSERT( frame + 1 < m_frames.SizeInt() );

	weight = ( time - m_times[ frame ] ) / ( m_times[ frame + 1 ] - m_times[ frame ] );

	ASSERT( weight <= 1.f && weight >= 0.f );
}

//////////////////////////////////////////////////////////////////////////

const Float CLineMotionExtraction2::TIME_SLICE = 1.f / 30.f;

CLineMotionExtraction2::CLineMotionExtraction2()
{
}

void CLineMotionExtraction2::GetMovementAtTime( Float time, AnimQsTransform& motionOut ) const
{
	const Float duration = GetDuration();

	AnimVector4 motionOut_v;

	if ( time >= duration )
	{
		motionOut_v = DecompressFrame( GetNumFrames() - 1 );
	}
	else
	{
		if ( time <= 0.0f )
		{
			motionOut_v = DecompressFrame( 0 );
		}
		else
		{
			Int32 frame = 0;
			Float weight = 0.f;

			FindFrameAndWeight( time, frame, weight );

			AnimVector4 motionA = DecompressFrame( frame );
			AnimVector4 motionB = DecompressFrame( frame + 1 );

#ifdef USE_HAVOK_ANIMATION
			motionOut_v.setInterpolate4( motionA, motionB, weight );
#else
			motionOut_v = RedVector4::Lerp( motionA, motionB, weight );
#endif
		}
	}

	// Construct bone transform
#ifdef USE_HAVOK_ANIMATION
	static const hkVector4 up( 0.f, 0.f, 1.f );
	motionOut.m_translation = motionOut_v; // ignores w component
	motionOut.m_rotation.setAxisAngle( up, motionOut_v(3) );
	motionOut.m_scale.setAll( 1.0f );
#else
	motionOut.Translation = motionOut_v; // ignores w component
	motionOut.Rotation.SetAxisAngle( RedVector4::EZ, motionOut_v.W ); // Z is up?
	motionOut.Scale.SetOnes();
#endif
}

Uint32 CLineMotionExtraction2::GetDataSize() const
{
	return static_cast< Uint32 >( sizeof( CLineMotionExtraction2 ) + m_frames.DataSize() + m_deltaTimes.DataSize() );
}

Float CLineMotionExtraction2::GetDuration() const
{
	if ( m_duration == 0.0f )
	{
		Uint32 sum = 0;

		const Uint32 size = m_deltaTimes.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			sum += m_deltaTimes[ i ];
		}

		m_duration = sum > 0 ? (Float)sum * TIME_SLICE : 1.f;
	}

	return m_duration;
}

void CLineMotionExtraction2::GetFrameTimes( TDynArray< Float >& times ) const
{
	const Float timeSlice = GetTimeSlice();
	Uint32 sum = 0;

	times.PushBack( 0.f );

	const Uint32 size = m_deltaTimes.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		sum += m_deltaTimes[ i ];

		times.PushBack( (Float)sum * timeSlice );
	}
}

Uint32 CLineMotionExtraction2::GetFrameSize() const
{
	Uint32 temp = 0;

	if ( ( m_flags & LMEF_X ) != 0 )
	{
		++temp;
	}
	if ( ( m_flags & LMEF_Y ) != 0 )
	{
		++temp;
	}
	if ( ( m_flags & LMEF_Z ) != 0 )
	{
		++temp;
	}
	if ( ( m_flags & LMEF_R ) != 0 )
	{
		++temp;
	}

	return temp;
}

Uint32 CLineMotionExtraction2::GetNumFrames() const
{
	ASSERT( m_frames.Size() % GetFrameSize() == 0 );
	ASSERT( m_frames.Size() / GetFrameSize() == m_deltaTimes.Size() + 1 );
	return m_deltaTimes.Size() + 1;
}

AnimVector4 CLineMotionExtraction2::DecompressFrame( Int32 frame ) const
{
	ASSERT( (Int32)GetNumFrames() > frame );

	const Uint32 frameSize = GetFrameSize();
	Uint32 temp = 0;

	Float x = 0.0f;
	Float y = 0.0f;
	Float z = 0.0f;
	Float w = 0.0f;

	if ( ( m_flags & LMEF_X ) != 0 )
	{
		x = m_frames[ frame * frameSize ];
		++temp;
	}
	if ( ( m_flags & LMEF_Y ) != 0 )
	{
		y = m_frames[ frame * frameSize + temp ];
		++temp;
	}
	if ( ( m_flags & LMEF_Z ) != 0 )
	{
		z = m_frames[ frame * frameSize + temp ];
		++temp;
	}
	if ( ( m_flags & LMEF_R ) != 0 )
	{
		w = m_frames[ frame * frameSize + temp ];
		++temp;
	}

	return AnimVector4(x,y,z,w);
}

void CLineMotionExtraction2::FindFrameAndWeight( Float time, Int32& frame, Float& weight ) const
{
	ASSERT( time >= 0.f );

	frame = 0;
	Uint32 sum = 0;

	Float accPrevTime = 0.f;
	Float accTime = 0.f;

	const Float timeSlice = GetTimeSlice();

	const Uint32 size = m_deltaTimes.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		sum += m_deltaTimes[ i ];

		accTime = (Float)sum * timeSlice;

		if ( accTime >= time )
		{
			frame = i;
			break;
		}

		accPrevTime = accTime;
	}

	ASSERT( (Int32)GetFrameSize() * ( frame + 1 ) < m_frames.SizeInt() );

	weight = ( time - accPrevTime ) / ( accTime - accPrevTime );

	ASSERT( weight <= 1.f && weight >= 0.f );
}
