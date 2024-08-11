
#pragma once

class CSkeletalAnimationAttackTrajectoryParam;

namespace AnimationPlayerTimers
{
	class TimeController_HitSlowMotion
	{
		enum ESMState
		{
			S_Pre,
			S_DampIn,
			S_Sync,
			S_DampOut,
			S_Post,
		};

		ESMState	m_smState;
		Float		m_smTimer;
		Float		m_smDuration;
		Float		m_smDampInOut;
		Float		m_smTimeFactor;

		Float		m_brTimer;
		Float		m_brDuration;

		Float		m_syncTime;
		Float		m_timeFactor;

	public:
		TimeController_HitSlowMotion();

		Float Update( Float dt );

		void Set( Float syncTime, Float timeFactor, Float breakDuration );
		void Reset();

		Bool IsBeforeSyncTime() const;

		Float GetTime() const;
	};

	//////////////////////////////////////////////////////////////////////////

	class TimeController_AfterHitSlowMotion
	{
		enum ESMState
		{
			S_IdlePre,
			S_Hit,
			S_PostHit,
			S_DampIn,
			S_SlowMotion,
			S_DampOut,
			S_IdlePost,
		};

		ESMState	m_smState;
		Float		m_smTimer;
		Float		m_smHitDuration;
		Float		m_smPostHitDuration;
		Float		m_smSlowMotionDuration;
		Float		m_smDampInDuration;
		Float		m_smDampOutDuration;
		Float		m_smTimeFactor;

		Float		m_brTimer;
		Float		m_brDuration;

		Float		m_syncTime;
		Float		m_timeFactor;

		Float		m_hitTimer;
		Bool		m_wasHitShown;

	public:
		TimeController_AfterHitSlowMotion();

		Float Update( Float dt );

		void Set( Float syncTime, Float timeFactor, Float breakDuration );
		void Reset();

		Bool IsBeforeSyncTime() const;

		Float GetTime() const;
	};

	//////////////////////////////////////////////////////////////////////////

	class TimeController_AfterHitSlowMotionEvt
	{
		enum ESMState
		{
			S_IdlePre,
			S_Hit,
			S_PostHit,
			S_DampIn,
			S_SlowMotion,
			S_DampOut,
			S_IdlePost,
		};

		ESMState	m_smState;
		Float		m_smTimer;
		Float		m_smHitDuration;
		Float		m_smPostHitDuration;
		Float		m_smSlowMotionDuration;
		Float		m_smDampInDuration;
		Float		m_smDampOutDuration;
		Float		m_smTimeFactor;

		Float		m_brTimer;
		Float		m_brDuration;

		Float		m_syncTime;
		Float		m_timeFactor;
		Float		m_duration;

		Float		m_hitTimer;
		Bool		m_wasHitShown;

	public:
		TimeController_AfterHitSlowMotionEvt();

		Float Update( Float dt );

		void Set( Float syncTime, Float timeFactor, Float breakDuration, Float duration, const CSkeletalAnimationAttackTrajectoryParam* param );
		void Reset();

		Bool IsBeforeSyncTime() const;

		Float GetTime() const;
	};
};
