
#include "animationTrajectory.h"

/*AnimationTrajectory::AnimationTrajectory( TModifier* modifier, Float duration )
	: m_modifier( modifier )
	, m_duration( duration )
	, m_initialized( false )
{

}

AnimationTrajectory::~AnimationTrajectory()
{
	delete m_modifier;
}

void AnimationTrajectory::SetSyncPoint( const Vector& posintWS, const Matrix& localToWorld, Float time )
{
	Vector pointM;
	Vector pointMLS;

	CalcKeyPoints( pointM, pointMLS, time, m_data.m_pointsLSO, m_data.m_pointsMSO );

	TModifier::Setup setup;
	setup.m_pointWS = posintWS;
	setup.m_duration = m_duration;
	setup.m_localToWorld = localToWorld;
	setup.m_dataSyncPointMS = m_data.m_pointsMSO[ m_data.m_syncFrame ];
	setup.m_currSyncPointMS = m_data.m_pointsMSO[ m_data.m_syncFrame ] - pointM;

	m_modifier.DoModifications( setup, m_data );

	m_initialized = true;
}

void AnimationTrajectory::GenerateFragments( CRenderFrame* frame, Float time, const Matrix& localToWorld, const Color& color, Bool unmodified )
{
	TDynArray< Vector > pointsCS;
	TDynArray< Vector > pointsOS;

	Color darker( 0, 255, 0 );
	//Color darker = color;
	//darker.Mul3( 0.5f );

	if ( unmodified )
	{
		CalcPoints( time, localToWorld, pointsCS, m_dataA.m_pointsLS, m_dataA.m_pointsMS );
		CalcPoints( time, localToWorld, pointsOS, m_dataA.m_pointsLSO, m_dataA.m_pointsMSO );

		for ( Uint32 i=1; i<pointsCS.Size(); ++i )
		{
			frame->AddDebugLine( pointsCS[i-1], pointsCS[i], darker, false );
			frame->AddDebugLine( pointsOS[i-1], pointsOS[i], darker, false );
		}
	}

	CalcPoints( time, localToWorld, pointsCS, m_dataB.m_pointsLS, m_dataB.m_pointsMS );
	CalcPoints( time, localToWorld, pointsOS, m_dataB.m_pointsLSO, m_dataB.m_pointsMSO );

	for ( Uint32 i=1; i<pointsCS.Size(); ++i )
	{
		frame->AddDebugLine( pointsCS[i-1], pointsCS[i], color, false );
		frame->AddDebugLine( pointsOS[i-1], pointsOS[i], color, false );
	}


	Vector pointM;
	Vector pointMLS;

	const Float radiusSize = 0.08;

	if ( unmodified )
	{
		CalcKeyPoints( pointM, pointMLS, time, m_dataA.m_pointsLSO, m_dataA.m_pointsMSO );

		frame->AddDebugSphere( localToWorld.TransformPoint( m_dataA.m_pointsMSO[ m_dataA.m_syncFrame ] - pointM ), radiusSize, Matrix::IDENTITY, darker );
		frame->AddDebugSphere( localToWorld.TransformPoint( pointMLS ), radiusSize, Matrix::IDENTITY, darker );


		CalcKeyPoints( pointM, pointMLS, time, m_dataA.m_pointsLS, m_dataA.m_pointsMS );

		frame->AddDebugSphere( localToWorld.TransformPoint( m_dataA.m_pointsMS[ m_dataA.m_syncFrame ] - pointM ), radiusSize, Matrix::IDENTITY, darker );
		frame->AddDebugSphere( localToWorld.TransformPoint( pointMLS ), radiusSize, Matrix::IDENTITY, darker );
	}

	if ( m_initialized )
	{
		CalcKeyPoints( pointM, pointMLS, time, m_dataB.m_pointsLSO, m_dataB.m_pointsMSO );

		frame->AddDebugSphere( localToWorld.TransformPoint( m_dataB.m_pointsMSO[ m_dataB.m_syncFrame ] - pointM ), radiusSize, Matrix::IDENTITY, color );
		frame->AddDebugSphere( localToWorld.TransformPoint( pointMLS ), radiusSize, Matrix::IDENTITY, color );


		CalcKeyPoints( pointM, pointMLS, time, m_dataB.m_pointsLS, m_dataB.m_pointsMS );

		frame->AddDebugSphere( localToWorld.TransformPoint( m_dataB.m_pointsMS[ m_dataB.m_syncFrame ] - pointM ), radiusSize, Matrix::IDENTITY, color );
		frame->AddDebugSphere( localToWorld.TransformPoint( pointMLS ), radiusSize, Matrix::IDENTITY, color );

		{
			Int32 frameA, frameB;
			Float progress;

			CalcKeys( frameA, frameB, progress, time, m_dataB.m_pointsLS.Size() );

			if ( frameA != -1 && frameB != -1 )
			{
				const Float r = 1.f;

				const Vector start = Vector::Interpolate( m_dataB.m_pointsLS[ frameA ], m_dataB.m_pointsLS[ frameB ], progress );
				Vector q = Vector::Interpolate( m_dataB.m_rotLS[ frameA ], m_dataB.m_rotLS[ frameB ], progress );

				hkQuaternion quat;
				quat.m_vec = TO_CONST_HK_VECTOR_REF( q );
				quat.normalize();

				hkVector4 v;
				v.setRotatedDir( quat, hkVector4( 0.f, 0.f, r ) );

				const Vector& o = TO_CONST_VECTOR_REF( v );

				const Vector s = localToWorld.TransformPoint( start );
				const Vector e = localToWorld.TransformPoint( start + o );

				frame->AddDebugLine( s, e, color, false );
			}
		}
	}
}*/

//////////////////////////////////////////////////////////////////////////
