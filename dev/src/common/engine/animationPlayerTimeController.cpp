
#include "build.h"
#include "animationPlayerTimeController.h"
#include "animationGameParams.h"
#include "behaviorIncludes.h"

namespace AnimationPlayerTimers
{

TimeController_HitSlowMotion::TimeController_HitSlowMotion()
{
	Reset();
}

Bool TimeController_HitSlowMotion::IsBeforeSyncTime() const
{
	return m_smTimer < m_syncTime;
}

void TimeController_HitSlowMotion::Set( Float syncTime, Float timeFactor, Float breakDuration )
{
	m_syncTime = syncTime;
	m_timeFactor = timeFactor;
	m_brDuration = breakDuration;
}

void TimeController_HitSlowMotion::Reset()
{
	const Float oneFrame = 1.f / 30.f;

	m_smTimer = 0.f;
	m_smDuration = oneFrame / 4.f;
	m_smDampInOut = oneFrame / 4.f;
	m_smTimeFactor = 0.01f;
	m_smState = S_Pre;

	m_brTimer = 0.f;
	m_brDuration = 0.f;

	m_syncTime = -1.f;
	m_timeFactor = 1.f;
}

Float TimeController_HitSlowMotion::Update( Float dt )
{
	dt *= m_timeFactor;

	if ( m_syncTime < 0.f )
	{
		m_smTimer += dt;
		return m_smTimer;
	}

	const Float durationHalf = m_smDuration / 2.f;

	const Float smDampInEnd = Max( m_syncTime - durationHalf, 0.f );
	const Float smDampInStart = smDampInEnd - m_smDampInOut;

	const Float smDampOutStart = m_syncTime + durationHalf;
	const Float smDampOutEnd = smDampOutStart + m_smDampInOut;

	Bool running = true;

	while ( running )
	{
		running = false;

		switch ( m_smState )
		{
		case S_Pre:
			{
				ASSERT( m_smTimer >= 0.f );

				const Float time = m_smTimer + dt;

				if ( time > smDampInStart )
				{
					dt = time - smDampInStart;
					ASSERT( dt >= 0.f );

					m_smTimer = smDampInStart;

					m_smState = S_DampIn;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampIn:
			{
				ASSERT( m_smTimer >= smDampInStart );

				const Float p = ( m_smTimer - smDampInStart ) / ( smDampInEnd - smDampInStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( p, 1.f, m_smTimeFactor );

				const Float time = m_smTimer + scale * dt;

				if ( time > smDampInEnd )
				{
					dt = time - smDampInEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = smDampInEnd;

					m_smState = S_Sync;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_Sync:
			{
				ASSERT( m_smTimer >= smDampInEnd );

				const Float time = m_smTimer + m_smTimeFactor * dt;

				if ( time > smDampOutStart )
				{
					dt = time - smDampOutStart;
					ASSERT( dt >= 0.f );

					m_smTimer = smDampOutStart;

					m_smState = S_DampOut;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampOut:
			{
				ASSERT( m_smTimer >= smDampOutStart );

				const Float p = ( m_smTimer - smDampOutStart ) / ( smDampOutEnd - smDampOutStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( p, 1.f, m_smTimeFactor );

				const Float time = m_smTimer + scale * dt;

				if ( time > smDampOutEnd )
				{
					dt = time - smDampOutEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = smDampOutEnd;

					m_smState = S_Post;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_Post:
			{
				ASSERT( smDampOutEnd >= 0.f );

				const Float time = m_smTimer + dt;

				m_smTimer = time;

				break;
			}
		}
	}

	return m_smTimer;
}

Float TimeController_HitSlowMotion::GetTime() const
{
	return m_smTimer;
}

//////////////////////////////////////////////////////////////////////////

TimeController_AfterHitSlowMotion::TimeController_AfterHitSlowMotion()
{
	Reset();
}

void TimeController_AfterHitSlowMotion::Set( Float syncTime, Float timeFactor, Float breakDuration )
{
	m_syncTime = syncTime;
	m_timeFactor = timeFactor;
	m_brDuration = breakDuration;
}

void TimeController_AfterHitSlowMotion::Reset()
{
	const Float oneFrame = 1.f / 30.f;

	m_smTimer = 0.f;
	m_smHitDuration = 0.1f;
	m_smSlowMotionDuration = oneFrame * ( 1.f / 12.f );
	m_smPostHitDuration = 0.0001f;//oneFrame * ( 1.f / 2.f );
	m_smDampInDuration = 0.0001f;//oneFrame / 12.f;
	m_smDampOutDuration = oneFrame * 2.f;
	m_smTimeFactor = 0.01f;
	m_smState = S_IdlePre;

	m_brTimer = 0.f;
	m_brDuration = 0.f;

	m_syncTime = -1.f;
	m_timeFactor = 1.f;

	m_hitTimer = 0.f;
	m_wasHitShown = false;
}

Bool TimeController_AfterHitSlowMotion::IsBeforeSyncTime() const
{
	return m_smState < S_Hit;
}

Float TimeController_AfterHitSlowMotion::Update( Float dt )
{
	dt *= m_timeFactor;

	if ( m_syncTime < 0.f )
	{
		m_smTimer += dt;
		return m_smTimer;
	}

	const Float hitTime = m_syncTime;

	const Float postHitStart = hitTime;
	const Float postHitEnd = postHitStart + m_smPostHitDuration;

	const Float dampInStart = postHitEnd;
	const Float dampInEnd = dampInStart + m_smDampInDuration;

	const Float slowMotionStart = dampInEnd;
	const Float slowMotionEnd = slowMotionStart + m_smSlowMotionDuration;

	const Float dampOutStart = slowMotionEnd;
	const Float dampOutEnd = dampOutStart + m_smDampOutDuration;

	Bool running = true;

	while ( running )
	{
		running = false;

		switch ( m_smState )
		{
		case S_IdlePre:
			{
				ASSERT( m_smTimer >= 0.f );

				const Float time = m_smTimer + dt;

				if ( time > hitTime )
				{
					dt = time - hitTime;
					ASSERT( dt >= 0.f );

					m_smTimer = hitTime;

					m_smState = S_Hit;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_Hit:
			{
				ASSERT( m_smTimer >= hitTime );

				m_hitTimer += dt;

				if ( m_hitTimer > m_smHitDuration && m_wasHitShown )
				{
					m_smState = S_PostHit;

					running = true;
				}

				m_wasHitShown = true;

				break;
			}


		case S_PostHit:
			{
				ASSERT( m_smTimer >= postHitStart );

				const Float time = m_smTimer + dt;

				if ( time > postHitEnd )
				{
					dt = time - postHitEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = postHitEnd;

					m_smState = S_DampIn;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampIn:
			{
				ASSERT( m_smTimer >= dampInStart );

				const Float p = ( m_smTimer - dampInStart ) / ( dampInEnd - dampInStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( p, 1.f, m_smTimeFactor );

				const Float time = m_smTimer + scale * dt;

				if ( time > dampInEnd )
				{
					dt = time - dampInEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = dampInEnd;

					m_smState = S_SlowMotion;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_SlowMotion:
			{
				ASSERT( m_smTimer >= slowMotionStart );

				const Float time = m_smTimer + m_smTimeFactor * dt;

				if ( time > slowMotionEnd )
				{
					dt = time - slowMotionEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = slowMotionEnd;

					m_smState = S_DampOut;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampOut:
			{
				ASSERT( m_smTimer >= dampOutStart );

				const Float p = ( m_smTimer - dampOutStart ) / ( dampOutEnd - dampOutStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( p*p, m_smTimeFactor, 1.f );

				const Float time = m_smTimer + scale * dt;

				if ( time > dampOutEnd )
				{
					dt = time - dampOutEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = dampOutEnd;

					m_smState = S_IdlePost;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_IdlePost:
			{
				ASSERT( dampOutEnd >= 0.f );

				const Float time = m_smTimer + dt;

				m_smTimer = time;

				break;
			}
		}
	}

	return m_smTimer;
}

Float TimeController_AfterHitSlowMotion::GetTime() const
{
	return m_smTimer;
}

//////////////////////////////////////////////////////////////////////////

TimeController_AfterHitSlowMotionEvt::TimeController_AfterHitSlowMotionEvt()
{
	Reset();
}

void TimeController_AfterHitSlowMotionEvt::Set( Float syncTime, Float timeFactor, Float breakDuration, Float duration, const CSkeletalAnimationAttackTrajectoryParam* param )
{
	m_syncTime = syncTime;
	m_timeFactor = timeFactor;
	m_brDuration = breakDuration;
	m_duration = duration;

	if ( param )
	{
		const Float MIN_TIME = 0.00001f;

		const Float hitDuration = param->GetHitDuration();
		const Float postHitEnd = param->GetPostHitEnd();
		const Float slowMotionStart = param->GetSlowMotionTimeStart();
		const Float slowMotionEnd = param->GetSlowMotionTimeEnd();
		const Float dampOutEnd = param->GetDampOutEnd();

		m_smHitDuration = Max( hitDuration, MIN_TIME );
		m_smPostHitDuration = Max( postHitEnd - m_syncTime, MIN_TIME );
		m_smDampInDuration = Max( slowMotionStart - postHitEnd, MIN_TIME );
		m_smSlowMotionDuration = Max( slowMotionEnd - slowMotionStart, MIN_TIME );
		m_smDampOutDuration = Max( dampOutEnd - slowMotionEnd, MIN_TIME );

		m_smTimeFactor = param->GetTimeFactor();
	}
}

void TimeController_AfterHitSlowMotionEvt::Reset()
{
	const Float oneFrame = 1.f / 30.f;

	m_smTimer = 0.f;
	m_smState = S_IdlePre;

	m_brTimer = 0.f;
	m_brDuration = 0.f;

	m_syncTime = -1.f;
	m_timeFactor = 1.f;

	m_hitTimer = 0.f;
	m_wasHitShown = false;

	// Default
	m_smHitDuration = 0.1f;
	m_smSlowMotionDuration = oneFrame * ( 1.f / 12.f );
	m_smPostHitDuration = 0.0001f;//oneFrame * ( 1.f / 2.f );
	m_smDampInDuration = 0.0001f;//oneFrame / 12.f;
	m_smDampOutDuration = oneFrame * 2.f;
	m_smTimeFactor = 0.01f;
}

Bool TimeController_AfterHitSlowMotionEvt::IsBeforeSyncTime() const
{
	return m_smState < S_Hit;
}

Float TimeController_AfterHitSlowMotionEvt::Update( Float dt )
{
	dt *= m_timeFactor;

	if ( m_syncTime < 0.f )
	{
		m_smTimer += dt;
		return m_smTimer;
	}

	const Float hitTime = m_syncTime;

	const Float postHitStart = hitTime;
	const Float postHitEnd = postHitStart + m_smPostHitDuration;

	const Float dampInStart = postHitEnd;
	const Float dampInEnd = dampInStart + m_smDampInDuration;

	const Float slowMotionStart = dampInEnd;
	const Float slowMotionEnd = slowMotionStart + m_smSlowMotionDuration;

	const Float dampOutStart = slowMotionEnd;
	const Float dampOutEnd = dampOutStart + m_smDampOutDuration;

	Bool running = true;

	while ( running )
	{
		running = false;

		switch ( m_smState )
		{
		case S_IdlePre:
			{
				ASSERT( m_smTimer >= 0.f );

				const Float time = m_smTimer + dt;

				if ( time > hitTime )
				{
					dt = time - hitTime;
					ASSERT( dt >= 0.f );

					m_smTimer = hitTime;

					m_smState = S_Hit;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_Hit:
			{
				ASSERT( m_smTimer >= hitTime );

				m_hitTimer += dt;

				if ( m_hitTimer > m_smHitDuration && m_wasHitShown )
				{
					m_smState = S_PostHit;

					running = true;
				}

				m_wasHitShown = true;

				break;
			}


		case S_PostHit:
			{
				ASSERT( m_smTimer >= postHitStart );

				const Float time = m_smTimer + dt;

				if ( time > postHitEnd )
				{
					dt = time - postHitEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = postHitEnd;

					m_smState = S_DampIn;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampIn:
			{
				ASSERT( m_smTimer >= dampInStart );

				const Float p = ( m_smTimer - dampInStart ) / ( dampInEnd - dampInStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( p, 1.f, m_smTimeFactor );

				const Float time = m_smTimer + scale * dt;

				if ( time > dampInEnd )
				{
					dt = time - dampInEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = dampInEnd;

					m_smState = S_SlowMotion;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_SlowMotion:
			{
				ASSERT( m_smTimer >= slowMotionStart );

				const Float time = m_smTimer + m_smTimeFactor * dt;

				if ( time > slowMotionEnd )
				{
					dt = time - slowMotionEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = slowMotionEnd;

					m_smState = S_DampOut;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_DampOut:
			{
				ASSERT( m_smTimer >= dampOutStart );

				const Float p = ( m_smTimer - dampOutStart ) / ( dampOutEnd - dampOutStart );
				ASSERT( p >= 0.f && p <= 1.f );

				Float scale = Lerp( BehaviorUtils::BezierInterpolation( p ), m_smTimeFactor, 1.f );

				const Float time = m_smTimer + scale * dt;

				if ( time > dampOutEnd )
				{
					dt = time - dampOutEnd;
					ASSERT( dt >= 0.f );

					m_smTimer = dampOutEnd;

					m_smState = S_IdlePost;

					running = true;
				}
				else
				{
					m_smTimer = time;
				}

				break;
			}


		case S_IdlePost:
			{
				ASSERT( dampOutEnd >= 0.f );
				ASSERT( m_duration > 0.f );
				ASSERT( m_smTimer <= m_duration );

				const Float time = m_smTimer + dt;

				m_smTimer = Min( time, m_duration );

				break;
			}
		}
	}

	return m_smTimer;
}

Float TimeController_AfterHitSlowMotionEvt::GetTime() const
{
	return m_smTimer;
}

};
