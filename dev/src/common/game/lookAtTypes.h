/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "lookAtController.h"

//////////////////////////////////////////////////////////////////////////
// Dialog
//////////////////////////////////////////////////////////////////////////

struct IDialogLookAtData
{
	IDialogLookAtData()
		: m_speed( 0.f )
		, m_level( LL_Body )
		, m_instant( false )
		, m_autoLimitDeact( false )
		, m_range( 180.f )
		, m_speedOverride( 0.f )
		, m_eyesLookAtConvergenceWeight( 0.f )
		, m_eyesLookAtIsAdditive( true )
		, m_eyesLookAtDampScale( 1.f )
		, m_timeFromStart( 0.f )
	{}

	Float				m_speed;
	ELookAtLevel		m_level;
	Bool				m_instant;
	Bool				m_autoLimitDeact;
	Float				m_range;
	Float				m_speedOverride;
	Float				m_eyesLookAtConvergenceWeight;
	Bool				m_eyesLookAtIsAdditive;
	Float				m_eyesLookAtDampScale;
	Float				m_timeFromStart;
};

struct SLookAtDialogBoneInfo : public SLookAtBoneInfo, public IDialogLookAtData
{
	SLookAtDialogBoneInfo() 
		: SLookAtBoneInfo( LPP_Dialog ) 
	{
		m_delay = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDelayDialog;
	}

	virtual ELookAtLevel		GetLevel() const { return m_level; }
	virtual Float				GetRange() const { return m_range; }
	virtual Float				GetSpeed() const { return m_speed; }
	virtual Bool				IsInstant() const { return m_instant; }
	virtual Bool				IsAutoLimitDeact() const { return m_autoLimitDeact; }
	virtual Float				GetSpeedOverride() const { return m_speedOverride; }
	virtual Float				GetEyesLookAtConvergenceWeight() const { return m_eyesLookAtConvergenceWeight; }
	virtual Bool				IsEyesLookAtAdditive() const { return m_eyesLookAtIsAdditive; }
	virtual Float				GetEyesLookAtDampScale() const { return m_eyesLookAtDampScale; }
	virtual Float				GetTimeFromStart() const { return m_timeFromStart; }
};

struct SLookAtQuestBoneInfo : public SLookAtDialogBoneInfo
{
	Float m_duration;
	
	SLookAtQuestBoneInfo() : m_duration ( 1 ){}

	Float GetDuration() const override { return m_duration; }
};

struct SLookAtDialogDynamicInfo : public SLookAtDynamicInfo, public IDialogLookAtData
{
	SLookAtDialogDynamicInfo() 
		: SLookAtDynamicInfo( LPP_Dialog ) 
	{
		m_delay = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDelayDialog;
	}

	virtual ELookAtLevel		GetLevel() const { return m_level; }
	virtual Float				GetRange() const { return m_range; }
	virtual Float				GetSpeed() const { return m_speed; }
	virtual Bool				IsInstant() const { return m_instant; }
	virtual Bool				IsAutoLimitDeact() const { return m_autoLimitDeact; }
	virtual Float				GetSpeedOverride() const { return m_speedOverride; }
	virtual Float				GetEyesLookAtConvergenceWeight() const { return m_eyesLookAtConvergenceWeight; }
	virtual Bool				IsEyesLookAtAdditive() const { return m_eyesLookAtIsAdditive; }
	virtual Float				GetEyesLookAtDampScale() const { return m_eyesLookAtDampScale; }
	virtual Float				GetTimeFromStart() const { return m_timeFromStart; }
};

struct SLookAtDialogStaticInfo : public SLookAtStaticInfo, public IDialogLookAtData
{
	SLookAtDialogStaticInfo() 
		: SLookAtStaticInfo( LPP_Dialog ) 
	{
		m_delay = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDelayDialog;
	}

	virtual ELookAtLevel		GetLevel() const { return m_level; }
	virtual Float				GetRange() const { return m_range; }
	virtual Float				GetSpeed() const { return m_speed; }
	virtual Bool				IsInstant() const { return m_instant; }
	virtual Bool				IsAutoLimitDeact() const { return m_autoLimitDeact; }
	virtual Float				GetSpeedOverride() const { return m_speedOverride; }
	virtual Float				GetEyesLookAtConvergenceWeight() const { return m_eyesLookAtConvergenceWeight; }
	virtual Bool				IsEyesLookAtAdditive() const { return m_eyesLookAtIsAdditive; }
	virtual Float				GetEyesLookAtDampScale() const { return m_eyesLookAtDampScale; }
	virtual Float				GetTimeFromStart() const { return m_timeFromStart; }
};

struct SLookAtQuestStaticInfo : public SLookAtDialogStaticInfo
{
	Float m_duration;

	SLookAtQuestStaticInfo() : m_duration ( 1 ){}

	Float GetDuration() const override { return m_duration; }
};

//////////////////////////////////////////////////////////////////////////
// Reactions
//////////////////////////////////////////////////////////////////////////

enum EReactionLookAtType
{
	RLT_None,
	RLT_Glance,
	RLT_Look,
	RLT_Gaze,
	RLT_Stare,
};

BEGIN_ENUM_RTTI( EReactionLookAtType );
	ENUM_OPTION( RLT_None );
	ENUM_OPTION( RLT_Glance );
	ENUM_OPTION( RLT_Look );
	ENUM_OPTION( RLT_Gaze );
	ENUM_OPTION( RLT_Stare );
END_ENUM_RTTI();

struct IReactionLookAtData
{
	IReactionLookAtData()
		: m_type( RLT_None )
		, m_reactionPriority( 0 )
	{}

	EReactionLookAtType		m_type;
	Uint32					m_reactionPriority;

	Float GetDurationFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationGlance;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationLook;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationStare;
		default:
			return 1.f;
		}
	}

	Float GetRangeFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeGlance;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeLook;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeStare;
		default:
			return 120.f;
		}
	}

	Bool GetAutoLimitDeactFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitGlance;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitLook;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitStare;
		default:
			return true;
		}
	}

	Float GetSpeedFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedGlance;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedLook;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedStare;
		default:
			return 1.f;
		}
	}
};

struct SLookAtReactionBoneInfo : public SLookAtBoneInfo, public IReactionLookAtData
{
	SLookAtReactionBoneInfo() 
		: SLookAtBoneInfo( LPP_Reaction ) {}

	virtual Float	GetDuration() const { return GetDurationFromType(); }
	virtual Float	GetRange() const { return GetRangeFromType(); }
	virtual Bool	IsAutoLimitDeact() const { return GetAutoLimitDeactFromType(); }
	virtual Float	GetSpeed() const { return GetSpeedFromType(); }
	virtual Bool	IsValid() const { return SLookAtBoneInfo::IsValid() && m_type != RLT_None; }
};

struct SLookAtReactionDynamicInfo : public SLookAtDynamicInfo, public IReactionLookAtData
{
	SLookAtReactionDynamicInfo() 
		: SLookAtDynamicInfo( LPP_Reaction ) {}

	virtual Float	GetDuration() const { return GetDurationFromType(); }
	virtual Float	GetRange() const { return GetRangeFromType(); }
	virtual Bool	IsAutoLimitDeact() const { return GetAutoLimitDeactFromType(); }
	virtual Float	GetSpeed() const { return GetSpeedFromType(); }
	virtual Bool	IsValid() const { return SLookAtDynamicInfo::IsValid() && m_type != RLT_None; }
};

struct SLookAtReactionStaticInfo : public SLookAtStaticInfo, public IReactionLookAtData
{
	SLookAtReactionStaticInfo() 
		: SLookAtStaticInfo( LPP_Reaction ) {}

	virtual Float	GetDuration() const { return GetDurationFromType(); }
	virtual Float	GetRange() const { return GetRangeFromType(); }
	virtual Bool	IsAutoLimitDeact() const { return GetAutoLimitDeactFromType(); }
	virtual Float	GetSpeed() const { return GetSpeedFromType(); }
	virtual Bool	IsValid() const { return SLookAtStaticInfo::IsValid() && m_type != RLT_None; }
};

//////////////////////////////////////////////////////////////////////////
// Script
//////////////////////////////////////////////////////////////////////////

struct IScriptLookAtData
{
	IScriptLookAtData()
		: m_duration( 1.f )
	{}

	Float					m_duration;
};

struct SLookAtScriptBoneInfo : public SLookAtBoneInfo, public IScriptLookAtData
{
	SLookAtScriptBoneInfo() 
		: SLookAtBoneInfo( LPP_Script ) {}

	virtual Float	GetDuration() const { return m_duration > 0.01f ? m_duration : 1.f; }
};

struct SLookAtScriptDynamicInfo : public SLookAtDynamicInfo, public IScriptLookAtData
{
	SLookAtScriptDynamicInfo() 
		: SLookAtDynamicInfo( LPP_Script ) {}

	virtual Float	GetDuration() const { return m_duration > 0.01f ? m_duration : 1.f; }
};

struct SLookAtScriptStaticInfo : public SLookAtStaticInfo, public IScriptLookAtData
{
	SLookAtScriptStaticInfo() 
		: SLookAtStaticInfo( LPP_Script ) {}

	virtual Float	GetDuration() const { return m_duration > 0.01f ? m_duration : 1.f; }
};

//////////////////////////////////////////////////////////////////////////
// Debug
//////////////////////////////////////////////////////////////////////////

struct SLookAtDebugStaticInfo : public SLookAtStaticInfo
{
	SLookAtDebugStaticInfo() 
		: SLookAtStaticInfo( LPP_Debug ) 
	{
		m_delay = 0.f;
	}

	virtual Bool IsAutoLimitDeact() const { return false; }
};

struct SLookAtDebugDynamicInfo : public SLookAtDynamicInfo
{
	SLookAtDebugDynamicInfo() 
		: SLookAtDynamicInfo( LPP_Debug ) {}

	virtual ELookAtLevel GetLevel() const
	{
		static ELookAtLevel level = LL_Body;
		return level;
	}

	virtual Float	GetSpeed() const
	{
		static Float s = 1.f;
		return s;
	}

	virtual Bool	IsInstant() const
	{
		static Bool ret = false;
		return ret;
	}

	virtual Float	GetDuration() const
	{
		static Float d = 0.f;
		return d;
	}

	virtual Bool	IsAutoLimitDeact() const
	{
		static Bool ret = false;
		return ret;
	}

	virtual Float	GetRange() const
	{
		static Float ret = 180.f;
		return ret;
	}
};

struct SLookAtDebugBoneInfo : public SLookAtBoneInfo
{
	SLookAtDebugBoneInfo() 
		: SLookAtBoneInfo( LPP_Debug ) {}

	virtual ELookAtLevel GetLevel() const
	{
		static ELookAtLevel level = LL_Body;
		return level;
	}

	virtual Float	GetSpeed() const
	{
		static Float s = 1.f;
		return s;
	}

	virtual Bool	IsInstant() const
	{
		static Bool ret = false;
		return ret;
	}

	virtual Float	GetDuration() const
	{
		static Float d = 0.f;
		return d;
	}

	virtual Bool	IsAutoLimitDeact() const
	{
		static Bool ret = true;
		return ret;
	}

	virtual Float	GetRange() const
	{
		static Float ret = 180.f;
		return ret;
	}
};

struct SLookAtDebugReactionBoneInfo : public SLookAtDebugBoneInfo
{
	SLookAtDebugReactionBoneInfo() 
		: SLookAtDebugBoneInfo()
		, m_type( 0 )
	{}

	Int32	m_type;

	Float GetDurationFromType() const
	{
		Float randVal = 0.f;
		Float val = 1.f;

		switch ( m_type )
		{
		case RLT_Glance:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationGlance;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationRandGlance;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Look:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationLook;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationRandLook;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Gaze:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationGaze;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationRandGaze;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Stare:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationStare;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDurationRandStare;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		}

		return val + randVal;
	}

	Float GetRangeFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeGlance;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeLook;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtRangeStare;
		default:
			return 120.f;
		}
	}

	Bool GetAutoLimitDeactFromType() const
	{
		switch ( m_type )
		{
		case RLT_Glance:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitGlance;
		case RLT_Look:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitLook;
		case RLT_Gaze:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitGaze;
		case RLT_Stare:
			return GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtAutoLimitStare;
		default:
			return true;
		}
	}

	Float GetSpeedFromType() const
	{
		Float randVal = 0.f;
		Float val = 1.f;

		switch ( m_type )
		{
		case RLT_Glance:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedGlance;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedRandGlance;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Gaze:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedLook;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedRandLook;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Look:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedGaze;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedRandGaze;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		case RLT_Stare:
			{
				val = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedStare;
				Float temp = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtSpeedRandStare;
				if ( temp > 0.f ) randVal = GEngine->GetRandomNumberGenerator().Get< Float >( -temp , temp );
				break;
			}
		}

		return val + randVal;
	}
};
