
#include "build.h"
#include "animationTrajectoryVisualizer.h"
#include "animationTrajectory.h"
#include "renderFrame.h"

void AnimationTrajectoryVisualizer::DrawTrajectoryLS( CRenderFrame *frame, const AnimationTrajectoryData& data )
{
	const Uint32 size = data.m_pointsLS.Size();
	if ( size == 0 )
	{
		return;
	}

	Vector prev = data.m_pointsLS[ 0 ];

	for ( Uint32 i=1; i<size; ++i )
	{
		const Vector& point = data.m_pointsLS[ i ];

		Color color( 255, 0, 0 );

		frame->AddDebugLine( prev, point, color, false );

		prev = point;
	}

	frame->AddDebugSphere( data.m_pointsLS[ data.m_syncFrame ], 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryVisualizer::DrawTrajectoryMS( CRenderFrame *frame, const AnimationTrajectoryData& data )
{
	const Uint32 size = data.m_pointsLS.Size();
	if ( size == 0 )
	{
		return;
	}

	Vector prev = data.m_pointsMS[ 0 ];

	for ( Uint32 i=1; i<size; ++i )
	{
		const Vector& point = data.m_pointsMS[ i ];

		Color color( 255, 0, 0 );

		frame->AddDebugLine( prev, point, color, false );

		prev = point;
	}

	frame->AddDebugSphere( data.m_pointsMS[ data.m_syncFrame ], 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryVisualizer::DrawTrajectoryLSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld )
{
	const Uint32 size = data.m_pointsLS.Size();
	if ( size == 0 )
	{
		return;
	}

	Vector prev = localToWorld.TransformPoint( data.m_pointsLS[ 0 ] );

	for ( Uint32 i=1; i<size; ++i )
	{
		Vector point = localToWorld.TransformPoint( data.m_pointsLS[ i ] );

		Color color( 255, 0, 0 );

		frame->AddDebugLine( prev, point, color, false );

		prev = point;
	}

	frame->AddDebugSphere( localToWorld.TransformPoint( data.m_pointsLS[ data.m_syncFrame ] ), 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryVisualizer::DrawTrajectoryMSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld )
{
	const Uint32 size = data.m_pointsLS.Size();
	if ( size == 0 )
	{
		return;
	}

	Vector prev = localToWorld.TransformPoint( data.m_pointsMS[ 0 ] );

	for ( Uint32 i=1; i<size; ++i )
	{
		Vector point = localToWorld.TransformPoint( data.m_pointsMS[ i ] );

		Color color( 255, 0, 0 );

		frame->AddDebugLine( prev, point, color, false );

		prev = point;
	}

	frame->AddDebugSphere( localToWorld.TransformPoint( data.m_pointsMS[ data.m_syncFrame ] ), 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryVisualizer::InternalDrawTrajectoryWSWithPtr( CRenderFrame *frame, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS, Uint32 syncFrame, const Matrix& localToWorld, Float time, Float duration, const Color& color )
{
	const Uint32 size = LS.Size();
	if ( size == 0 )
	{
		return;
	}

	Vector pointMLS;
	Vector pointM;

	if ( time < duration )
	{
		const Float p = time / duration;
		const Float frameTime = duration / ( size - 1 );

		const Uint32 frameA = (Uint32)MFloor( p * ( size - 1 ) );
		const Uint32 frameB = frameA + 1;

		const Float frameT = time - frameA * frameTime;
		ASSERT( frameT >= 0.f && frameT <= frameTime );

		const Float frameP = frameT / frameTime;
		ASSERT( frameP >= 0.f && frameP <= 1.f );

		pointMLS = Vector::Interpolate( LS[ frameA ], LS[ frameB ], frameP );
		pointM = Vector::Interpolate( MS[ frameA ], MS[ frameB ], frameP ) - pointMLS;
	}
	else
	{
		const Uint32 frame = size - 1;
		pointMLS = LS[ frame ];
		pointM = MS[ frame ] - pointMLS;
	}

	Vector prev = localToWorld.TransformPoint( MS[ 0 ] - pointM );

	for ( Uint32 i=1; i<size; ++i )
	{
		Vector point = localToWorld.TransformPoint( MS[ i ] - pointM );

		frame->AddDebugLine( prev, point, color, false );

		prev = point;
	}

	frame->AddDebugSphere( localToWorld.TransformPoint( MS[ syncFrame ] - pointM ), 0.05f, Matrix::IDENTITY, color );

	frame->AddDebugSphere( localToWorld.TransformPoint( pointMLS ), 0.05f, Matrix::IDENTITY, color );
}

void AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtr( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration )
{
	InternalDrawTrajectoryWSWithPtr( frame, data.m_pointsLS, data.m_pointsMS, data.m_syncFrame, localToWorld, time, duration, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtrO( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration )
{
	InternalDrawTrajectoryWSWithPtr( frame, data.m_pointsLS, data.m_pointsMS, data.m_syncFrame, localToWorld, time, duration, Color( 224, 224, 224 ) );
	InternalDrawTrajectoryWSWithPtr( frame, data.m_pointsLSO, data.m_pointsMSO, data.m_syncFrame, localToWorld, time, duration, Color( 255, 0, 0 ) );
}
