
#include "build.h"
#include "animationTrajectory.h"

//////////////////////////////////////////////////////////////////////////

Float AnimationTrajectoryData::GetSyncPointTime() const
{
	return m_syncFrame * ( 1.f / 30.f );
}

Float AnimationTrajectoryData::GetDuration() const
{
	return m_pointsLS.Size() * ( 1.f / 30.f );
}

Bool AnimationTrajectoryData::FindKeys( Float time, Int32& frameA, Int32& frameB, Float& progress ) const
{
	const Uint32 size = m_pointsLS.Size();
	CalcKeys( frameA, frameB, progress, time, size );
	return frameA != -1 && frameB != -1;
}

void AnimationTrajectoryData::CalcKeys( Int32& frameA, Int32& frameB, Float& progress, Float time, Uint32 keysNum ) const
{
	if ( keysNum == 0 )
	{
		frameA = -1;
		frameB = -1;
		return;
	}

	const Float duration = GetDuration();

	if ( time < duration )
	{
		const Float p = time / duration;
		const Float frameTime = duration / ( keysNum - 1 );

		frameA = (Int32)MFloor( p * ( keysNum - 1 ) );
		frameB = frameA + 1;

		const Float frameT = time - frameA * frameTime;
		ASSERT( frameT >= 0.f && frameT <= frameTime );

		progress = frameT / frameTime;
		ASSERT( progress >= 0.f && progress <= 1.f );
	}
	else
	{
		frameA = (Int32)keysNum - 1;
		frameB = -1;
		progress = 0.f;
	}
}

void AnimationTrajectoryData::CalcKeyPoints( Vector& pointM, Vector& pointMLS, Float time, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const
{
	const Uint32 size = MS.Size();
	if ( size == 0 )
	{
		pointM = pointMLS = Vector::ZERO_3D_POINT;
		return;
	}

	const Float duration = GetDuration();

	if ( time < duration )
	{
		Int32 frameA, frameB;
		Float progress;

		CalcKeys( frameA, frameB, progress, time, size );

		ASSERT( frameA != -1 );
		ASSERT( frameB != -1 );
		ASSERT( progress >= 0.f && progress <= 1.f );

		pointMLS = Vector::Interpolate( LS[ frameA ], LS[ frameB ], progress );
		pointM = Vector::Interpolate( MS[ frameA ], MS[ frameB ], progress ) - pointMLS;
	}
	else
	{
		const Uint32 frame = size - 1;
		pointMLS = LS[ frame ];
		pointM = MS[ frame ] - pointMLS;
	}
}

Vector AnimationTrajectoryData::GetPointLS( Float time ) const
{
	Vector pointM;
	Vector pointMLS;

	CalcKeyPoints( pointM, pointMLS, time, m_pointsLS, m_pointsMS );

	return pointMLS;
}

Vector AnimationTrajectoryData::GetPointWS( Float time, const Matrix& localToWorld ) const
{
	return CalcPoint( time, localToWorld, m_pointsLSO, m_pointsMSO );
}

Vector AnimationTrajectoryData::CalcPoint( Float time, const Matrix& localToWorld, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const
{
	Vector pointM;
	Vector pointMLS;

	CalcKeyPoints( pointM, pointMLS, time, LS, MS );

	return localToWorld.TransformPoint( pointMLS );
}

void AnimationTrajectoryData::GetPointsWS( Float time, const Matrix& localToWorld, TDynArray< Vector >& points ) const
{
	return CalcPoints( time, localToWorld, points, m_pointsLSO, m_pointsMSO );
}

void AnimationTrajectoryData::CalcPoints( Float time, const Matrix& localToWorld, TDynArray< Vector >& points, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS ) const
{
	const Uint32 size = MS.Size();
	points.Resize( size );

	if ( size == 0 )
	{
		return;
	}

	Vector pointM;
	Vector pointMLS;

	CalcKeyPoints( pointM, pointMLS, time, LS, MS );

	for ( Uint32 i=0; i<size; ++i )
	{
		points[ i ] = localToWorld.TransformPoint( MS[ i ] - pointM );
	}
}

//////////////////////////////////////////////////////////////////////////

AnimationTrajectoryModifier_Null::AnimationTrajectoryModifier_Null( const AnimationTrajectoryData& input )
	: m_input( input )
{

}

void AnimationTrajectoryModifier_Null::DoModifications( const Setup& setup, AnimationTrajectoryData& output ) const
{
	output = m_input;
}

//////////////////////////////////////////////////////////////////////////

AnimationTrajectoryModifier_OnePoint::AnimationTrajectoryModifier_OnePoint( const AnimationTrajectoryData& input )
	: m_input( input )
{

}

void AnimationTrajectoryModifier_OnePoint::DoModifications( const AnimationTrajectoryModifier_OnePoint::Setup& setup, AnimationTrajectoryData& output ) const
{
	Vector syncPointWS = setup.m_localToWorld.TransformPoint( setup.m_currSyncPointMS );
	Vector offsetWS = setup.m_pointWS - syncPointWS;

	output = m_input;

	// TEMP we don't need FullInverted
	Vector offsetMS = setup.m_localToWorld.FullInverted().TransformVector( offsetWS );

	output.m_pointsLS[ m_input.m_syncFrame ] += offsetMS;
	output.m_pointsMS[ m_input.m_syncFrame ] += offsetMS;

	output.m_pointsLSO[ m_input.m_syncFrame ] += offsetMS;
	output.m_pointsMSO[ m_input.m_syncFrame ] += offsetMS;
}

//////////////////////////////////////////////////////////////////////////

AnimationTrajectoryModifier_Blend2::AnimationTrajectoryModifier_Blend2( const AnimationTrajectoryData& inputA, const AnimationTrajectoryData& inputB )
	: m_inputA( inputA )
	, m_inputB( inputB )
{

}

void AnimationTrajectoryModifier_Blend2::DoModifications( const AnimationTrajectoryModifier_Blend2::Setup& setup, AnimationTrajectoryData& output ) const
{
	Vector syncPointWS = setup.m_localToWorld.TransformPoint( setup.m_currSyncPointMS );
	Vector offsetWS = setup.m_pointWS - syncPointWS;

	output = m_inputA;

	// TEMP we don't need FullInverted
	Vector offsetMS = setup.m_localToWorld.FullInverted().TransformVector( offsetWS );

	output.m_pointsLS[ m_inputA.m_syncFrame ] += offsetMS;
	output.m_pointsMS[ m_inputA.m_syncFrame ] += offsetMS;

	output.m_pointsLSO[ m_inputA.m_syncFrame ] += offsetMS;
	output.m_pointsMSO[ m_inputA.m_syncFrame ] += offsetMS;
}

//////////////////////////////////////////////////////////////////////////
