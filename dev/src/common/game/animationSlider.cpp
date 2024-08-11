
#include "build.h"
#include "animationSlider.h"
#include "../../common/engine/extAnimScriptEvent.h"
#include "../engine/renderFrame.h"

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_NAME( SlideToTarget );
RED_DEFINE_NAME( RotateEvent );
RED_DEFINE_NAME( SlideToTargetScaleEnd );

SAnimSliderTarget::SAnimSliderTarget()
	: m_isTypeStatic( true )
	, m_vec( Vector::ZERO_3D_POINT )
	, m_useRot( true )
	, m_useTrans( true )
{}

void SAnimSliderTarget::Set( const Vector& vec, Float yaw )
{
	m_isTypeStatic = true;
	m_useTrans = true;
	m_useRot = true;

	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Z ) );

	ASSERT( vec.X == vec.X );
	ASSERT( vec.Y == vec.Y );
	ASSERT( vec.Y == vec.Y );

	ASSERT( !Red::Math::NumericalUtils::IsNan( yaw ) );
	ASSERT( yaw == yaw );

	m_vec = vec;
	m_vec.W = DEG2RAD( yaw );

	ASSERT( m_vec.IsOk() );
}

void SAnimSliderTarget::Set( const Vector& vec )
{
	m_isTypeStatic = true;
	m_useTrans = true;
	m_useRot = false;

	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Z ) );

	ASSERT( vec.X == vec.X );
	ASSERT( vec.Y == vec.Y );
	ASSERT( vec.Y == vec.Y );

	m_vec = vec;
	m_vec.W = 0.f;

	ASSERT( m_vec.IsOk() );
}

void SAnimSliderTarget::Set( Float yaw )
{
	m_isTypeStatic = true;
	m_useTrans = false;
	m_useRot = true;

	ASSERT( !Red::Math::NumericalUtils::IsNan( yaw ) );
	ASSERT( yaw == yaw );

	m_vec = Vector::ZEROS;
	m_vec.W = DEG2RAD( yaw );

	ASSERT( m_vec.IsOk() );
}

void SAnimSliderTarget::Set( const CNode* node, Bool trans, Bool rot )
{
	m_isTypeStatic = false;
	m_useTrans = trans;
	m_useRot = rot;

	ASSERT( m_useRot || m_useTrans );

	m_node = node;
}

Bool SAnimSliderTarget::Get( const Matrix& defaultVal, Matrix& mat ) const
{
	if ( m_isTypeStatic )
	{
		if ( m_useRot )
		{
			mat.SetRotZ33( m_vec.W );
		}
		else
		{
			mat = defaultVal;
		}

		if ( m_useTrans )
		{
			mat.SetTranslation( m_vec.X, m_vec.Y, m_vec.Z );
		}
		else
		{
			mat.SetTranslation( defaultVal.GetTranslation() );
		}
		
		return true;
	}
	else
	{
		const CNode* node = m_node.Get();
		if ( node )
		{
			if ( m_useRot )
			{
				node->GetLocalToWorld( mat );

				if ( !m_useTrans )
				{
					mat.SetTranslation( defaultVal.GetTranslation() );
				}
			}
			else
			{
				mat = defaultVal;

				mat.SetTranslation( node->GetWorldPosition() );
			}

			return true;
		}
	}

	mat = Matrix::IDENTITY;
	return false;
}

Bool SAnimSliderTarget::IsRotationSet() const
{
	return m_useRot;
}

Bool SAnimSliderTarget::IsTranslationSet() const
{
	return m_useTrans;
}

//////////////////////////////////////////////////////////////////////////

AnimSlider::AnimSlider()
	: m_animation( NULL )
	, m_timer( 0.f )
	, m_curr( Matrix::IDENTITY )
	, m_dest( Matrix::IDENTITY )
	, m_translationScale( Vector::ONES )
	, m_translationOffset( Vector::ZEROS )
	, m_rotationScale( 1.f )
	, m_rotationOffset( 0.f )
	, m_firstUpdate( false )
	, m_hasRotation( false )
	, m_translationStart( 0.f )
	, m_translationEnd( 1.f )
	, m_timeStart( 0.f )
	, m_targetChanged( false )
	, m_target( Matrix::IDENTITY )
	, m_scaleEndTime( 1.f )
	, m_rotationOffsetNotZero( false )
	, m_targetUseRot( false )
	, m_targetUseTrans( false )
	, m_cancel( false )
	, m_rotationCancel( false )
	, m_lastRotationState( RDS_None )
	, m_currChanged( false )
	, m_calcInWS( true )
	, m_firstTargetFlag( false )
{
	m_hasTranslation[ 0 ] = false;
	m_hasTranslation[ 1 ] = false;
	m_hasTranslation[ 2 ] = false;

	m_translationOffsetNotZero[ 0 ] = false;
	m_translationOffsetNotZero[ 1 ] = false;
	m_translationOffsetNotZero[ 2 ] = false;

	m_copyTranslation[ 0 ] = -1;
	m_copyTranslation[ 1 ] = -1;
	m_copyTranslation[ 2 ] = -1;

	m_translationCancel[ 0 ] = false;
	m_translationCancel[ 1 ] = false;
	m_translationCancel[ 2 ] = false;

	m_translationMinScale = 0.000f; // allow to be kept in one place
	m_translationMaxScale = 2.f;
	m_translationMaxAcceptableScale = 1.1f; // limit movement to only scale up a little bit
	m_rotationMinScale = 0.001f;
	m_rotationMaxScale = 3.f;
}

Bool AnimSlider::Init( const CSkeletalAnimationSetEntry* animation, const SAnimSliderSettings& settings )
{
	m_settings = settings;

	if ( animation && animation->GetAnimation() )
	{
		m_animation = animation;

		FindTimesForEvent( m_animation, m_settings.m_translationEventName, m_translationStart, m_translationEnd );
		FindTimesAndSpeedForRotationEvent( m_animation, m_settings.m_rotationEventName, m_rotations );

		if ( !FindTimeForEvent( m_animation, m_settings.m_scaleEndPointEventNameA, m_scaleEndTime ) )
		{
			if ( !FindTimeForEvent( m_animation, m_settings.m_scaleEndPointEventNameB, m_scaleEndTime ) )
			{
				m_scaleEndTime = GetDuration();
			}
		}

		return true;
	}

	return false;
}

void AnimSlider::SetCurrentPosition( const Matrix& l2w )
{
	if ( !m_firstUpdate )
	{
		ASSERT( !Vector::Near2( l2w.GetTranslationRef(), Vector::ZERO_3D_POINT ) );

		m_firstUpdate = true;
		m_firstPosition = l2w;
	}

	m_curr = l2w;

	m_currChanged = true;
}

void AnimSlider::SetTarget( const Matrix& dest )
{
	Bool dataChanged = false;

	dataChanged |= MAbs( dest.GetYaw() - m_dest.GetYaw() ) > 0.001f;
	dataChanged |= !Vector::Equal3( dest.GetTranslationRef(), m_dest.GetTranslationRef() );

	if ( dataChanged )
	{
		const Float dist = m_curr.GetTranslationRef().DistanceTo( dest.GetTranslationRef() );
		if ( dist < m_settings.m_maxSlideTranslation )
		{
			m_target = dest;

			ASSERT( m_target.IsOk() );

			m_targetChanged = true;

			m_targetUseRot = true;
			m_targetUseTrans = true;
		}
		else
		{
			ASSERT( dist < m_settings.m_maxSlideTranslation );
		}
	}
}

void AnimSlider::SetTargetPosition( const Vector& destPosition )
{
	if ( !Vector::Equal3( destPosition, m_dest.GetTranslationRef() ) )
	{
		const Float dist = m_curr.GetTranslationRef().DistanceTo( destPosition );
		if ( dist < m_settings.m_maxSlideTranslation )
		{
			m_target.SetTranslation( destPosition );

			ASSERT( m_target.IsOk() );

			m_targetChanged = true;

			m_targetUseTrans = true;
		}
		else
		{
			ASSERT( dist < m_settings.m_maxSlideTranslation );
		}
	}
}

void AnimSlider::SetTargetRotation( Float destRotation )
{
	if ( MAbs( EulerAngles::AngleDistance( destRotation, m_dest.GetYaw() ) ) > 0.001f )
	{
		m_target.SetRotZ33( DEG2RAD( destRotation ) );

		ASSERT( m_target.IsOk() );

		m_targetChanged = true;

		m_targetUseRot = true;
	}
}

Bool AnimSlider::UpdateWS( const Matrix& curr, Float dt, Vector& motionTransDeltaWS, Float& motionRotDeltaWS )
{
	m_curr = curr;

	InternalUpdate();

	const Float duration = GetDuration();

	const Float currTime = Clamp( m_timer + dt, 0.f, duration );

	GetMovementDeltaWS( currTime, motionTransDeltaWS, motionRotDeltaWS );

    m_timer = currTime;

	return m_timer < duration;
}

Bool AnimSlider::UpdateAtWS( const Matrix& curr, Float startTime, Float endTime, Vector& motionTransDeltaWS, Float& motionRotDeltaWS, const Vector * destPoint )
{
	m_curr = curr;

    const Float duration = GetDuration();

    m_timer = Clamp( startTime, 0.f, duration );
    const Float currTime = Clamp( endTime, 0.f, duration );

	InternalUpdate();

    GetMovementDeltaWS( currTime, motionTransDeltaWS, motionRotDeltaWS, destPoint );

    m_timer = currTime;

	return m_timer < duration;
}

Bool AnimSlider::UpdateMS( const Matrix& curr, Float dt, Vector& motionTransDeltaMS, Float& motionRotDeltaMS )
{
	m_curr = curr;

	InternalUpdate();

	const Float duration = GetDuration();

	const Float currTime = Clamp( m_timer + dt, 0.f, duration );

	GetMovementDeltaMS( currTime, motionTransDeltaMS, motionRotDeltaMS );

	m_timer = currTime;

	return m_timer < duration;
}

Bool AnimSlider::UpdateAtMS( const Matrix& curr, Float startTime, Float endTime, Vector& motionTransDeltaMS, Float& motionRotDeltaMS )
{
	m_curr = curr;

	const Float duration = GetDuration();

	m_timer = Clamp( startTime, 0.f, duration );
	const Float currTime = Clamp( endTime, 0.f, duration );

	InternalUpdate();

	GetMovementDeltaMS( currTime, motionTransDeltaMS, motionRotDeltaMS );

	m_timer = currTime;

	return m_timer < duration;
}

void AnimSlider::InternalUpdate()
{
	if ( !m_firstUpdate )
	{
		ASSERT( !Vector::Near2( m_curr.GetTranslationRef(), Vector::ZERO_3D_POINT ) );

		m_firstUpdate = true;
		m_firstPosition = m_curr;
	}

	Bool recalc = false;

	if ( m_targetChanged )
	{
		recalc = true;

		m_dest = m_target;

		ASSERT( m_dest.IsOk() );

		if ( m_targetUseTrans )
		{
			ASSERT( m_dest.GetTranslationRef().DistanceTo( m_curr.GetTranslationRef() ) < m_settings.m_maxSlideTranslation );
		}

		m_targetChanged = false;
	}

	if ( m_currChanged )
	{
		recalc = true;

		m_currChanged = false;
	}

	if ( recalc )
	{
		CalculateScaleAndOffset();

		Float temp = m_rotationOffset;
		CalculateDebugPoints();
		m_rotationOffset = temp;
	}

	if ( m_targetUseTrans )
	{
		ASSERT( !Vector::Near2( m_dest.GetTranslationRef(), Vector::ZERO_3D_POINT ) );
		ASSERT( !Vector::Near2( m_curr.GetTranslationRef(), Vector::ZERO_3D_POINT ) );
	}
}

void AnimSlider::ApplyScale( Float time, Vector& translation, Float& rotation )
{
	// Scale and offset

	const Bool isScalingTime = time <= m_scaleEndTime;

	{
		// Translation
		if ( m_targetUseTrans )
		{
			if ( isScalingTime )
			{
				for ( Uint32 i=0; i<3; ++i )
				{
					if ( m_translationCancel[ i ] )
					{
						if ( !m_translationOffsetNotZero[ i ] )
						{
							translation.A[ i ] = m_translationOffset.A[ i ];

							ASSERT( m_translationScale.A[ i ] == 1.f );
						}
						else
						{
							// TODO
							//...
						}

						continue;
					}

					if ( m_copyTranslation[ i ] != -1 )
					{
						ASSERT( m_copyTranslation[ i ] >= 0 );
						ASSERT( m_copyTranslation[ i ] <= 2 );

						translation.A[ i ] = translation.A[ m_copyTranslation[ i ] ];
					}
					else if ( !m_hasTranslation[ i ] && m_translationOffsetNotZero[ i ] )
					{
						ASSERT( m_translationScale.A[ i ] == 1.f );

						if ( time >= m_translationEnd )
						{
							//if ( m_settings.m_translateAfterEventTime )
							{
								// Set final value
								translation.A[ i ] = m_translationOffset.A[ i ];
							}
						}
						else if ( time > m_translationStart )
						{
							// Interpolate
							Float p = ( time - m_translationStart ) / ( m_translationEnd - m_translationStart );
							ASSERT( p >= 0.f && p <= 1.f );

							//p = BehaviorUtils::BezierInterpolation( p );

							translation.A[ i ] = p * m_translationOffset.A[ i ];
						}
						else
						{
							// Nothing
						}
					}
					else if ( m_hasTranslation[ i ] && !m_translationOffsetNotZero[ i ] )
					{
						translation.A[ i ] = m_translationOffset.A[ i ];

						ASSERT( m_translationScale.A[ i ] == 1.f );
					}
				}

				translation.X *= m_translationScale.X;
				translation.Y *= m_translationScale.Y;
				translation.Z *= m_translationScale.Z;
			}
		}
		else
		{

		}

		// Rotation
		if ( m_targetUseRot )
		{
			rotation *= m_rotationScale;

			if ( m_rotationCancel || !m_hasRotation )
			{
				const Float timeDelta = time - m_timer;
				ASSERT( timeDelta >= 0.f );

				if ( m_settings.m_rotationPolicy == SAnimSliderSettings::RP_Delta )
				{
					m_lastRotationState = RDS_Motion;

					if ( !m_hasRotation && m_rotationOffsetNotZero )
					{
						ASSERT( m_rotationScale == 1.f );

						m_lastRotationState = RDS_Motion;
						for ( TDynArray< RotationSetup >::const_iterator iRot = m_rotations.Begin(); iRot != m_rotations.End(); ++ iRot )
						{
							if ( time >= iRot->m_start && time <= iRot->m_end )
							{
								// Interpolate
								Float p = Clamp( ( time - iRot->m_start ) / Max( 0.001f, iRot->m_end - iRot->m_start ), 0.0f, 1.0f );

								//p = BehaviorUtils::BezierInterpolation( p );

								// when speed is zero - don't limit it
								if ( iRot->m_speed != 0.0f )
								{
									const Float rotationAngle = timeDelta * iRot->m_speed;
									rotation = Clamp( p * m_rotationOffset, -rotationAngle, rotationAngle );
								}
								else
								{
									rotation = p * m_rotationOffset;
								}

								m_lastRotationState = RDS_Delta;
							}
						}
					}
					else if ( m_hasRotation && !m_rotationOffsetNotZero )
					{
						ASSERT( m_rotationScale == 1.f );

						rotation = m_rotationOffset;
					}
				}
				else if ( m_settings.m_rotationPolicy == SAnimSliderSettings::RP_Speed )
				{
					m_lastRotationState = RDS_Motion;

					const Float ANGLE_THRESHOLD = 0.001f;

					if ( m_rotationOffsetNotZero && ( MAbs( rotation ) < ANGLE_THRESHOLD || m_rotationCancel ) )
					{
						Float rotationSpeed = -1.0f;
						TDynArray< RotationSetup >::const_iterator inRot;
						for ( TDynArray< RotationSetup >::const_iterator iRot = m_rotations.Begin(); iRot != m_rotations.End(); ++ iRot )
						{
							if ( time >= iRot->m_start && time <= iRot->m_end )
							{
								rotationSpeed = iRot->m_speed;
								inRot = iRot;
							}
						}

						if ( rotationSpeed == 0.0f && inRot != m_rotations.End() )
						{
							// default to interpolation/delta when given speed is 0
							Float p = Clamp( ( time - inRot->m_start ) / Max( 0.001f, inRot->m_end - inRot->m_start ), 0.0f, 1.0f );

							rotation = p * m_rotationOffset;

							m_lastRotationState = RDS_Delta;
						}
						else if ( rotationSpeed > 0.0f )
						{
							ASSERT( m_rotationScale == 1.f );

							const Float rotationAngle = timeDelta * rotationSpeed;

							const Bool sign = m_rotationOffset >= 0.f;
							const Float finaleAngle = sign ? Min( rotationAngle, m_rotationOffset ) : Max( -rotationAngle, m_rotationOffset );

							m_rotationOffset -= finaleAngle;
							if ( sign )
							{
								ASSERT( m_rotationOffset >= 0.f );
							}
							else
							{
								ASSERT( m_rotationOffset <= 0.f );
							}

							rotation = finaleAngle;

							m_lastRotationState = RDS_Speed;
						}
					}
				}
				else
				{
					ASSERT( 0 );
				}
			}
		}
	}
}

void AnimSlider::CalcMovementBetween( Float start, Float end, Vector& motionTrans, Float& motionRot, Float& motionZ )
{
	ASSERT( start <= end );

	// TODO this sampling should be replaced when GetMovementBetweenTime will be reimplemented

	Float rotationEnd = m_rotations.Size()? m_rotations[ m_rotations.Size()-1 ].m_end : 1.0f;
	for ( TDynArray< RotationSetup >::const_iterator iRot = m_rotations.Begin(); iRot != m_rotations.End(); ++ iRot )
	{
		// find closest one after this one?
		if ( end <= iRot->m_end )
		{
			rotationEnd = Min( rotationEnd, iRot->m_end );
		}
	}

	// sample movement at specific points in time (try to divide rotation into pieces)
	Float midRefEndTime = ( start < rotationEnd && rotationEnd < end ) ? rotationEnd : end;
	Float midTime0 = start * 0.67f + 0.33f * midRefEndTime;
	Float midTime1 = start * 0.33f + 0.67f * midRefEndTime;

	// sample movement at specific points in time
	Matrix motionMS0;
	m_animation->GetAnimation()->GetMovementAtTime( start, motionMS0 );
	Matrix motionMS1;
	m_animation->GetAnimation()->GetMovementAtTime( midTime0, motionMS1 );
	Matrix motionMS2;
	m_animation->GetAnimation()->GetMovementAtTime( midTime1, motionMS2 );
	Matrix motionMS3;
	m_animation->GetAnimation()->GetMovementAtTime( end, motionMS3 );

	// collect translation
	Matrix motionMS03;
	motionMS03 = motionMS3 * motionMS0.Inverted();
	Vector translationMS = motionMS03.V[ 3 ];

	// get rotation based on difference between angles
	Float rotationMS = EulerAngles::NormalizeAngle180( motionMS1.GetYaw() - motionMS0.GetYaw() )
					 + EulerAngles::NormalizeAngle180( motionMS2.GetYaw() - motionMS1.GetYaw() )
					 + EulerAngles::NormalizeAngle180( motionMS3.GetYaw() - motionMS2.GetYaw() )
					 ;

	// send results back
	motionTrans = translationMS;
	motionRot = rotationMS;
	motionZ = translationMS.Z;
}

void AnimSlider::CalcMovementBetweenMS( Float start, Float end, Vector& motionTransMS, Float& motionRotMS, Float& motionMSZ )
{
	CalcMovementBetween( start, end, motionTransMS, motionRotMS, motionMSZ );

	ApplyScale( end, motionTransMS, motionRotMS );
}

void AnimSlider::GetMovementWS( Float time, Vector& motionTransWS, Float& motionRotWS, Float& motionMSZ )
{
	ASSERT( time >= m_timeStart );

	// MS
	Vector motionTransMS;
	Float motionRotMS;

	CalcMovementBetween( m_timeStart, time, motionTransMS, motionRotMS, motionMSZ );

	// WS
	if ( m_targetUseTrans )
	{
		if ( m_calcInWS )
		{
			Vector motionTransRS = m_ref.TransformVector( motionTransMS );

			ApplyScale( time, motionTransRS, motionRotMS );

			motionTransWS = m_ref.GetTranslationRef() + motionTransRS;
		}
		else
		{
			ApplyScale( time, motionTransMS, motionRotMS );

			Vector motionTransRS = m_ref.TransformVector( motionTransMS );

			motionTransWS = m_ref.GetTranslationRef() + motionTransRS;
		}
	}
	else
	{
		Vector t;
		Float r,z;
		Vector zero( Vector::ZERO_3D_POINT );

		Float temp = m_rotationOffset;
		CalcMovementBetweenMS( m_timer, time, t, r, z );

		m_rotationOffset = temp;
		ApplyScale( time, zero, motionRotMS );

		motionTransWS = m_curr.TransformPoint( t );
	}
	
	
	motionRotWS = m_ref.GetYaw() + motionRotMS;
}

void AnimSlider::GetMovementDeltaWS( Float time, Vector& motionTransDeltaWS, Float& motionRotDeltaWS, const Vector* pointDest )
{
	//if ( time < GetDuration() )
	{
		Vector thisFramePosWS;
		Float thisFrameYaw;
		Float motionMSZ;

		if ( m_targetUseTrans )
		{
			ASSERT( m_dest.GetTranslationRef().DistanceTo( m_curr.GetTranslationRef() ) < m_settings.m_maxSlideTranslation );
		}

		GetMovementWS( time, thisFramePosWS, thisFrameYaw, motionMSZ );

		if ( pointDest )
		{
			const Vector refPosition = m_curr.GetTranslation(); // use current position instead of first position as we and target could move dramatically and we're more interesed in getting into right position than along imaginary line that can take as nowhere
			Vector dir = *pointDest - refPosition;
			dir.Normalize3();

			Vector motionTransMS;
			{
				Float motionRotMS;
				Float motionMSZ2;
				CalcMovementBetween( m_timeStart, time, motionTransMS, motionRotMS, motionMSZ2 );
			}

			thisFramePosWS = m_ref.GetTranslationRef() + dir * motionTransMS.Mag3();

			motionTransDeltaWS = thisFramePosWS - m_curr.GetTranslationRef();
			motionRotDeltaWS = EulerAngles::AngleDistance( m_curr.GetYaw(), thisFrameYaw );
		}
		else
		{
			motionTransDeltaWS = thisFramePosWS - m_curr.GetTranslationRef();
			motionRotDeltaWS = EulerAngles::AngleDistance( m_curr.GetYaw(), thisFrameYaw );
		}

		ASSERT( motionTransDeltaWS.Mag3() < m_settings.m_maxSlideTranslation );
		ASSERT( !Vector::Near2( thisFramePosWS, Vector::ZERO_3D_POINT ) );

		ASSERT( motionTransDeltaWS.Mag3() < m_settings.m_maxSlideTranslationPerFrame );
		if ( motionTransDeltaWS.Mag3() >= m_settings.m_maxSlideTranslationPerFrame )
		{
			ASSERT( motionTransDeltaWS.Mag3() < m_settings.m_maxSlideTranslationPerFrame );
		}

		// TODO
		motionTransDeltaWS.Z = motionMSZ;
	}
	//else
	//{
	//	motionTransDeltaWS = m_dest.GetTranslationRef() - m_curr.GetTranslationRef();
	//	motionRotDeltaWS = EulerAngles::AngleDistance( m_curr.GetYaw(), m_dest.GetYaw() );
	//
	//	// TODO
	//	motionTransDeltaWS.Z = 0.f;
	//}

	AnimSlider* t = const_cast< AnimSlider* >( this );
	t->m_debugLastDeltaTransWS = motionTransDeltaWS;
	t->m_debugLastDeltaRotWS = motionRotDeltaWS;
}

void AnimSlider::GetMovementDeltaMS( Float time, Vector& motionTransDeltaMS, Float& motionRotDeltaMS )
{
	if ( time < GetDuration() )
	{
		Vector motionTransMS;
		Float motionYawMS;
		Float motionMSZ;

		ASSERT( time >= m_timeStart );
		ASSERT( time >= m_timer );

		CalcMovementBetweenMS( m_timer, time, motionTransMS, motionYawMS, motionMSZ );

		motionTransDeltaMS = motionTransMS;
		motionRotDeltaMS = motionYawMS;
		motionTransDeltaMS.Z = motionMSZ;
	}
	else
	{
		motionTransDeltaMS = Vector::ZERO_3D_POINT;
		motionRotDeltaMS = 0.f;
	}
}

void AnimSlider::FindTimesForEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, Float& start, Float& end ) const
{
	TDynArray< CExtAnimEvent* > events;
	animation->GetAllEvents( events );

	const Uint32 size = events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CExtAnimEvent* e = events[ i ];
		if ( e->GetEventName() == eventName )
		{
			start = e->GetStartTime();
			end = e->GetEndTimeWithoutClamp();

			return;
		}
	}

	start = 0.f;
	end = animation->GetDuration();
}

RED_DEFINE_STATIC_NAME( ERotationRate );

void AnimSlider::FindTimesAndSpeedForRotationEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, TDynArray< RotationSetup > & rotations ) const
{
	rotations.ClearFast();

	TDynArray< CExtAnimEvent* > events;
	animation->GetAllEvents( events );

	const Uint32 size = events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CExtAnimEvent* e = events[ i ];
		if ( e->GetEventName() == eventName )
		{
			Float start = e->GetStartTime();
			Float end = e->GetEndTimeWithoutClamp();
			Float speed = 1080.0f;

			// Why do we need this?
			if( IsType< CEASEnumEvent >( e ) )
			{
				const CEASEnumEvent* enumEvt = static_cast< const CEASEnumEvent* >( e );
				
				const SEnumVariant& enumData = enumEvt->GetEnumData();
				if ( enumData.m_enumType == CNAME( ERotationRate ) )
				{
					speed = DecodeRotationEnum( enumData.m_enumValue );
				}
			}

			rotations.PushBack( RotationSetup( start, end, speed ) );
		}
	}

	if ( rotations.Empty() )
	{
		rotations.PushBack( RotationSetup( 0.0f, animation->GetDuration(), 1080.0f ) );
	}
}

Bool AnimSlider::FindTimeForEvent( const CSkeletalAnimationSetEntry* animation, const CName& eventName, Float& time ) const
{
	TDynArray< CExtAnimEvent* > events;
	animation->GetAllEvents( events );

	const Uint32 size = events.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CExtAnimEvent* e = events[ i ];
		if ( e->GetEventName() == eventName )
		{
			time = e->GetStartTime();

			return true;
		}
	}

	time = 0.f;

	return false;
}

Float AnimSlider::DecodeRotationEnum( Int32 enumValue ) const
{
	return (Float)enumValue;

	/*switch ( enumValue )
	{
	case 0:
		return 0.f;
	case 1:
		return 30.f;
	case 2:
		return 60.f;
	case 3:
		return 90.f;
	case 4:
		return 180.f;
	case 5:
		return 360.f;
	case 6:
		return 1080.f;
	case 7:
		return 2160.f;
	default:
		return 1.f;
	}*/
}

void AnimSlider::CalculateScaleAndOffset()
{
	const Float duration = GetDuration();

	const Float p = m_timer / duration;
	ASSERT( p >= 0.f && p <= 1.f );

	m_timeStart = m_timer;
	m_ref = m_curr;

	// get motion
	Float motionYaw = 0.0f;
	Float motionZ = 0.0f;

	// MS
	Vector motionTransMS;
	CalcMovementBetween( m_timeStart, m_scaleEndTime, motionTransMS, motionYaw, motionZ );

	// WS
	Vector motionTransRS = m_ref.TransformVector( motionTransMS );

	// Has rotation and translation
	const Float ANGLE_THRESHOLD = 0.01f;
	const Float TRANS_THRESHOLD = 0.0001f;

	m_hasRotation = m_settings.m_useRotationScaling && MAbs( motionYaw ) > ANGLE_THRESHOLD;
	m_rotationCancel = false;
	
	static Bool DEBUG_WS_MS = false;
	m_calcInWS = DEBUG_WS_MS;

	Vector motionTrans;
	if ( m_calcInWS )
	{
		motionTrans = motionTransRS;
	}
	else
	{
		motionTrans = motionTransMS;
	}

	m_hasTranslation[ 0 ] = MAbs( motionTrans.A[ 0 ] ) > TRANS_THRESHOLD;
	m_hasTranslation[ 1 ] = MAbs( motionTrans.A[ 1 ] ) > TRANS_THRESHOLD;
	m_hasTranslation[ 2 ] = MAbs( motionTrans.A[ 2 ] ) > TRANS_THRESHOLD;
	
	m_translationCancel[ 0 ] = false;
	m_translationCancel[ 1 ] = false;
	m_translationCancel[ 2 ] = false;

	// Rotation scale and offset
	
	const Float destYaw = m_dest.GetYaw();
	const Float startYaw = m_ref.GetYaw();

    Float diffYaw = EulerAngles::AngleDistance( startYaw, destYaw );

    // If we do have a rotation motion
    if ( m_hasRotation )
    {
		// diffYaw should consider motionYaw as point of reference (as we want to scale, right?)
		diffYaw = EulerAngles::NormalizeAngle180( diffYaw - motionYaw ) + motionYaw;
    }

	m_rotationOffsetNotZero = MAbs( diffYaw ) > ANGLE_THRESHOLD;
	m_rotationOffset = diffYaw;

	m_rotationScale = m_hasRotation && m_rotationOffsetNotZero ? diffYaw / motionYaw : 1.f;

	if ( m_rotationScale > m_rotationMaxScale || m_rotationScale < m_rotationMinScale )
	{
		m_hasRotation = false;
		m_rotationScale = 1.f;

		m_rotationCancel = true;
	}

	// Translation scale and offset
	Vector diffTrans;
	if ( m_calcInWS )
	{
		diffTrans = m_dest.GetTranslationRef() - m_ref.GetTranslationRef();
	}
	else
	{
		Matrix retInv = m_ref.FullInverted();
		diffTrans = retInv.TransformPoint( m_dest.GetTranslation() );
	}

	m_translationOffsetNotZero[ 0 ] = MAbs( diffTrans.A[ 0 ] ) > TRANS_THRESHOLD;
	m_translationOffsetNotZero[ 1 ] = MAbs( diffTrans.A[ 1 ] ) > TRANS_THRESHOLD;
	m_translationOffsetNotZero[ 2 ] = MAbs( diffTrans.A[ 2 ] ) > TRANS_THRESHOLD;

	m_translationOffset = diffTrans;

	m_copyTranslation[ 0 ] = -1;
	m_copyTranslation[ 1 ] = -1;
	m_copyTranslation[ 2 ] = -1;

	const Bool canUseTranslationCopy = true;
	if ( canUseTranslationCopy )
	{
		if ( !m_hasTranslation[ 0 ] && m_translationOffsetNotZero[ 0 ] )
		{
			if ( m_hasTranslation[ 1 ] )
			{
				m_copyTranslation[ 0 ] = 1;
				motionTrans.A[ 0 ] = motionTrans.A[ 1 ];
				m_hasTranslation[ 0 ] = true;
			}
			else if ( m_hasTranslation[ 2 ] )
			{
				m_copyTranslation[ 0 ] = 2;
				motionTrans.A[ 0 ] = motionTrans.A[ 2 ];
				m_hasTranslation[ 0 ] = true;
			}
		}

		if ( !m_hasTranslation[ 1 ] && m_translationOffsetNotZero[ 1 ] )
		{
			if ( m_hasTranslation[ 0 ] )
			{
				m_copyTranslation[ 1 ] = 0;
				motionTrans.A[ 1 ] = motionTrans.A[ 0 ];
				m_hasTranslation[ 1 ] = true;
			}
			else if ( m_hasTranslation[ 2 ] )
			{
				m_copyTranslation[ 1 ] = 2;
				motionTrans.A[ 1 ] = motionTrans.A[ 2 ];
				m_hasTranslation[ 1 ] = true;
			}
		}

		if ( !m_hasTranslation[ 2 ] && m_translationOffsetNotZero[ 2 ] )
		{
			if ( m_hasTranslation[ 1 ] )
			{
				m_copyTranslation[ 2 ] = 1;
				motionTrans.A[ 2 ] = motionTrans.A[ 1 ];
				m_hasTranslation[ 2 ] = true;
			}
			else if ( m_hasTranslation[ 0 ] )
			{
				m_copyTranslation[ 2 ] = 0;
				motionTrans.A[ 2 ] = motionTrans.A[ 0 ];
				m_hasTranslation[ 2 ] = true;
			}
		}
	}

	m_translationScale.A[ 0 ] = m_hasTranslation[ 0 ] && m_translationOffsetNotZero[ 0 ] ? diffTrans.A[ 0 ] / motionTrans.A[ 0 ] : 1.f;
	m_translationScale.A[ 1 ] = m_hasTranslation[ 1 ] && m_translationOffsetNotZero[ 1 ] ? diffTrans.A[ 1 ] / motionTrans.A[ 1 ] : 1.f;
	m_translationScale.A[ 2 ] = m_hasTranslation[ 2 ] && m_translationOffsetNotZero[ 2 ] ? diffTrans.A[ 2 ] / motionTrans.A[ 2 ] : 1.f;

	for ( Uint32 i=0; i<3; ++i )
	{
		const Float scale = MAbs( m_translationScale.A[ i ] );

		if ( scale > m_translationMaxScale || scale < m_translationMinScale )
		{
			m_hasTranslation[ i ] = false;
			m_translationScale.A[ i ] = 1.f;
			m_translationCancel[ i ] = true;
		}
		else
		{
			// limit to acceptable values
			Float & translationScale = m_translationScale.A[ i ];
			translationScale = Clamp( translationScale, -m_translationMaxAcceptableScale, m_translationMaxAcceptableScale );
		}
	}

	/*{
		Float temp = m_rotationOffset;

		Vector finalFramePosWS;
		Float finalFrameYaw;
		Float z;

		GetMovementWS( duration, finalFramePosWS, finalFrameYaw, z );

		const Bool translationCancel = m_translationCancel[ 0 ] || m_translationCancel[ 1 ] || m_translationCancel[ 2 ];

		if ( m_targetUseTrans && !translationCancel )
		{
			const Float diff = finalFramePosWS.DistanceTo2D( m_dest.GetTranslation() );
			if ( MAbs ( diff ) > 0.001f )
			{
				ASSERT( 0 );
			}
		}
		
		if ( m_targetUseRot && !m_rotationCancel && m_settings.m_rotationPolicy == SAnimSliderSettings::RP_Delta )
		{
			const Float diff = EulerAngles::AngleDistance( finalFrameYaw, m_dest.GetYaw() );
			if ( MAbs( diff ) > 0.01f )
			{
				ASSERT( 0 );
			}
		}

		m_rotationOffset = temp;
	}*/
}

void AnimSlider::CalculateDebugPoints()
{
	TDynArray< Matrix > points;
	points.Reserve( m_debugPoints.Size() );

	const Float timeStep = 1.f / 30.f;
	const Float duration = GetDuration();

	Float time = 0.f;
	Uint32 counter = 0;

	Vector pointTrans;
	Float pointRot;

	Float motionMSZ = 0.f;

	for ( ; time<duration; time += timeStep )
	{
		if ( time >= m_timeStart )
		{
			GetMovementWS( time, pointTrans, pointRot, motionMSZ );

			Matrix mat;
			mat.SetRotZ33( DEG2RAD( pointRot ) );
			mat.SetTranslation( pointTrans );

			points.PushBack( mat );
		}
		else if ( m_debugPoints.Empty() )
		{
			points.PushBack( m_ref );
		}
		else
		{
			points.PushBack( m_debugPoints[ counter ] );
		}

		counter++;
	}

	if ( time < duration )
	{
		time = duration;

		if ( time >= m_timeStart )
		{
			GetMovementWS( time, pointTrans, pointRot, motionMSZ );

			Matrix mat;
			mat.SetRotZ33( DEG2RAD( pointRot ) );
			mat.SetTranslation( pointTrans );

			points.PushBack( mat );
		}
		else
		{
			points.PushBack( m_debugPoints[ counter ] );
		}
	}

	m_debugPoints = points;
}

Float AnimSlider::GetDuration() const
{
	ASSERT( m_animation );
	return m_animation ? m_animation->GetDuration() : 1.f;
}

void AnimSlider::GenerateDebugFragments( CRenderFrame* frame )
{
	//if ( m_firstUpdate )
	{
		const Bool translationCancel = m_translationCancel[ 0 ] && m_translationCancel[ 1 ] && m_translationCancel[ 2 ];

		Color color( 0, 0, 255 );

		frame->AddDebugLine( m_firstPosition.GetTranslationRef(), m_dest.GetTranslationRef(), color );

		Matrix prev;

		if ( !translationCancel )
		{
			const Uint32 size = m_debugPoints.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				const Matrix& point = m_debugPoints[ i ];

				if ( i > 0 )
				{
					frame->AddDebugLine( prev.GetTranslationRef(), point.GetTranslationRef(), color );
				}

				frame->AddDebugSphere( point.GetTranslationRef(), 0.05f, Matrix::IDENTITY, Color( 255, 255, 0 ) );

				//frame->AddDebugAxis( point.GetTranslationRef(), point, 0.05f );

				prev = point;
			}
		}

		frame->AddDebugSphere( m_ref.GetTranslationRef(), 0.3f, Matrix::IDENTITY, Color( 255, 255, 0 ) );
		frame->AddDebugSphere( m_firstPosition.GetTranslationRef(), 0.28f, Matrix::IDENTITY, color );
		frame->AddDebugSphere( m_dest.GetTranslationRef(), 0.26f, Matrix::IDENTITY, color );
		frame->AddDebugSphere( m_curr.GetTranslationRef(), 0.24f, Matrix::IDENTITY, Color( 0, 255, 0 ) );

		//frame->AddDebugAxis( m_firstPosition.GetTranslationRef(), m_firstPosition, 0.1f );
		//frame->AddDebugAxis( m_dest.GetTranslationRef(), m_firstPosition, 0.1f );
		//frame->AddDebugAxis( m_curr.GetTranslationRef(), m_curr, 0.1f );

		//String text = String::Printf( TXT("%1.2f"), m_timer / GetDuration() );
		//frame->AddDebugText( m_curr.GetTranslationRef() + Vector( 0.f, 0.f, 0.1f ), text, true );

		Uint32 x = frame->GetFrameOverlayInfo().m_width - 300;
		Uint32 yStep = 20;
		Uint32 y = 100;

		String text;

		frame->AddDebugRect( x-20, y-20, 250, 320, Color( 0,0,0 ) );

		{
			text = TXT("F: ") + ToString( m_firstPosition.GetTranslationRef() );
			text += String::Printf( TXT(" - %1.2f"), m_firstPosition.GetYaw() );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			text = TXT("R: ") + ToString( m_ref.GetTranslationRef() );
			text += String::Printf( TXT(" - %1.2f"), m_ref.GetYaw() );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			text = TXT("C: ") + ToString( m_curr.GetTranslationRef() );
			text += String::Printf( TXT(" - %1.2f"), m_curr.GetYaw() );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			text = TXT("D: ") + ToString( m_dest.GetTranslationRef() );
			text += String::Printf( TXT(" - %1.2f"), m_dest.GetYaw() );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			Vector dist = m_translationOffset;
			text = TXT("Dist: ") + ToString( dist );
			text += String::Printf( TXT(" - %1.2f"), m_rotationOffset );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		y += yStep;

		{
			text = TXT("Last: ") + ToString( m_debugLastDeltaTransWS );
			text += String::Printf( TXT(" - %1.2f"), m_debugLastDeltaRotWS );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		y += yStep;

		{
			text = String::Printf( TXT("Use trans - %d"), (Int32)m_targetUseTrans );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			String policyStr = m_settings.m_rotationPolicy == SAnimSliderSettings::RP_Delta ? TXT("Delta") : TXT("Speed");
			String rotState;

			switch ( m_lastRotationState )
			{
			case RDS_None:
				rotState = TXT("None");
				break;
			case RDS_Motion:
				rotState = TXT("Motion");
				break;
			case RDS_Delta:
				rotState = TXT("Delta");
				break;
			case RDS_Speed:
				rotState = TXT("Speed");
				break;
			}

			text = String::Printf( TXT("Use rot   - %d - [%s] - {%s}"), (Int32)m_targetUseRot, rotState.AsChar(), policyStr.AsChar() );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			Float rotationSpeed = 0.0f;
			for ( TDynArray< RotationSetup >::const_iterator iRot = m_rotations.Begin(); iRot != m_rotations.End(); ++ iRot )
			{
				if ( m_timer >= iRot->m_start && m_timer <= iRot->m_end )
				{
					rotationSpeed = iRot->m_speed;
				}
			}
			text = String::Printf( TXT("Rot speed - %1.1f"), rotationSpeed );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			Color color = ( m_targetUseTrans && translationCancel ) || ( m_targetUseRot && m_rotationCancel ) ? Color( 0, 0, 255 ) : Color( 255, 255, 255 );
			
			String strT;
			if ( translationCancel && m_targetUseTrans )
			{
				strT = TXT("[CANCELED]");
			}

			String strR;
			if ( m_rotationCancel && m_targetUseRot )
			{
				strR = TXT("[CANCELED]");
			}

			text = String::Printf( TXT("Scale - %1.3f, %1.3f, %1.3f %s - %1.3f %s"), m_translationScale.X, m_translationScale.Y, m_translationScale.Z, strT.AsChar(), m_rotationScale, strR.AsChar() );
			frame->AddDebugScreenText( x, y, text, color, NULL, true );
			y += yStep;
		}

		{
			text = String::Printf( TXT("Dist - %1.3f, %1.3f, %1.3f - %1.3f"), m_translationOffset.X, m_translationOffset.Y, m_translationOffset.Z, m_rotationOffset );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			text = String::Printf( TXT("Has mo - %d, %d, %d - %d"), (Int32)m_hasTranslation[0], (Int32)m_hasTranslation[1], (Int32)m_hasTranslation[2], m_hasRotation );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}

		{
			text = String::Printf( TXT("Not zer - %d, %d, %d - %d"), (Int32)m_translationOffsetNotZero[0], (Int32)m_translationOffsetNotZero[1], (Int32)m_translationOffsetNotZero[2], m_rotationOffsetNotZero );
			frame->AddDebugScreenText( x, y, text, Color( 255, 255, 255 ), NULL, true );
			y += yStep;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

AnimationSlider2::AnimationSlider2()
	: m_startWasSet( false )
{

}

AnimationSlider2::~AnimationSlider2()
{
	
}

Bool AnimationSlider2::Init( const CSkeletalAnimationSetEntry* animation, const SAnimSliderSettings& settings )
{
	m_rotate = false;
	m_translate = false;

	if ( animation && animation->GetAnimation() )
	{
		TDynArray< CExtAnimEvent* > events;
		animation->GetAllEvents( events );

		m_timer = 0.f;
		m_duration = animation->GetDuration();
		if ( m_duration <= 0.f )
		{
			return false;
		}

		// TODO: Events for this
		m_startTime = 0.f;						//<--!!!
		m_endTime = animation->GetDuration();	//<--!!!

		const Uint32 size = events.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CExtAnimEvent* e = events[ i ];
			const CName& eName = e->GetEventName();

			if ( m_translate && eName == settings.m_translationEventName )
			{
				const Float start = e->GetStartTime();
				const Float end = e->GetEndTimeWithoutClamp();

				m_translationEventTimes.PushBack( TEventTime( start, end ) );
			}

			if ( m_rotate && eName == settings.m_rotationEventName )
			{
				Float speed = 1080.f;

				// TODO: All events for slider should be special 'slider' events
				if( IsType< CEASEnumEvent >( e ) )
				{
					const CEASEnumEvent* enumEvt = static_cast< const CEASEnumEvent* >( e );

					const SEnumVariant& enumData = enumEvt->GetEnumData();
					if ( enumData.m_enumType == CNAME( ERotationRate ) )
					{
						speed = DecodeRotationEnum( enumData.m_enumValue );
					}
				}

				const Float start = e->GetStartTime();
				const Float end = e->GetEndTimeWithoutClamp();

				m_rotationEventTime.PushBack( TEventTime( start, end ) );
				m_rotatonEventSpeeds.PushBack( speed );
			}
		}

		return true;
	}

	return false;
}

Float AnimationSlider2::DecodeRotationEnum( Int32 enumValue ) const
{
	return (Float)enumValue;
}

void AnimationSlider2::SetTarget( const SAnimSliderTarget& target )
{
	m_target = target;
}

void AnimationSlider2::SetTarget( const CNode* dest )
{
	ASSERT( 0 );
}

void AnimationSlider2::SetTarget( const Matrix& dest )
{
	ASSERT( 0 );
}

void AnimationSlider2::SetTargetPosition( const Vector& destPosition )
{
	ASSERT( 0 );
}

void AnimationSlider2::SetTargetRotation( Float destRotation )
{
	m_target.Set( destRotation );
	m_rotate = true;
}

void AnimationSlider2::GenerateDebugFragments( CRenderFrame* frame )
{

}

void AnimationSlider2::ConvertMatrixToPoint( const Matrix& in, AnimationSlider2::Point& out ) const
{
	out.m_position = in.GetTranslation();
	out.m_rotation = in.ToEulerAngles();
}

void AnimationSlider2::ConvertPointToMatrix( const Point& in, Matrix& out ) const
{
	in.m_rotation.ToMatrix( out );
	out.SetTranslation( in.m_position );
}

void AnimationSlider2::InterpolateTranslation( const Point& pointA, const Point& pointB, Point& out, Float weight ) const
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	
	out.m_position = Vector::Interpolate( pointA.m_position, pointB.m_position, weight );
}

void AnimationSlider2::InterpolateRotation( const Point& pointA, const Point& pointB, Point& out, Float weight ) const
{
	ASSERT( weight >= 0.f && weight <= 1.f );

	out.m_rotation.Yaw = Lerp( weight, pointA.m_rotation.Yaw, pointB.m_rotation.Yaw );
}

void AnimationSlider2::Start( const Matrix& startPointMat )
{
	ConvertMatrixToPoint( startPointMat, m_startPoint );
	m_startMat = startPointMat;
	m_startWasSet = true;
}

Bool AnimationSlider2::IsFinished() const
{
	return m_timer >= m_duration;
}

Bool AnimationSlider2::Update( const Matrix& currWS, Float dt, Vector& deltaTransWS, Float& deltaRotWS )
{
	m_timer += dt;

	return UpdateAt( currWS, m_timer, deltaTransWS, deltaRotWS );
}

Bool AnimationSlider2::UpdateAt( const Matrix& currWS, Float time, Vector& deltaTransWS, Float& deltaRotWS )
{
	Matrix thisTimeTransform;

	Bool ret = UpdateAt( currWS, time, thisTimeTransform );
	if ( ret )
	{
		deltaTransWS = thisTimeTransform.GetTranslationRef() - currWS.GetTranslationRef();
		deltaRotWS = EulerAngles::AngleDistance( currWS.GetYaw(), thisTimeTransform.ToEulerAngles().Yaw );
	}
	else
	{
		deltaTransWS = Vector::ZERO_3D_POINT;
		deltaRotWS = 0.f;
	}

	return ret;
}

Bool AnimationSlider2::Update( const Matrix& currWS, Float dt, Matrix& transformWSOut )
{
	m_timer += dt;

	return UpdateAt( currWS, m_timer, transformWSOut );
}

Bool AnimationSlider2::UpdateAt( const Matrix& currWS, Float time, Matrix& transformWSOut )
{
	ASSERT( m_duration > 0.f );

	time = Clamp( time, 0.f, m_duration );

	const Float progress = time / m_duration;

	if ( !m_startWasSet )
	{
		ASSERT( m_startWasSet );
		return true;
	}

	ASSERT( m_rotate || m_translate );

	// TODO cache it and don't use this crap!
	Matrix targetMat;									// <--!!!
	m_target.Get( m_startMat, targetMat );				// <--!!!
	Point targetPoint;									// <--!!!
	ConvertMatrixToPoint( targetMat, targetPoint );		// <--!!!

	Point currPoint;
	if ( !m_rotate || !m_translate )
	{
		ConvertMatrixToPoint( currWS, currPoint );
	}

	if ( m_rotate )
	{
		const Float progressRotation = progress;

		InterpolateRotation( m_startPoint, targetPoint, currPoint, progressRotation );

		if ( !m_translate )
		{

		}
	}

	if ( m_translate )
	{
		const Float progressTranslation = progress;

		InterpolateTranslation( m_startPoint, targetPoint, currPoint, progressTranslation );
	}
	
	ConvertPointToMatrix( currPoint, transformWSOut );

	return IsFinished();
}
