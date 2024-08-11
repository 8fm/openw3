/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include "AkPrivateTypes.h"
#include "AkRTPC.h"

enum ParamType
{
	PT_All				= 0xffffffff,
    PT_Volume			= 0x0001,
    PT_Pitch			= 0x0002,
    PT_LPF				= 0x0004,
	PT_BusVolume		= 0x0008,
	PT_HDR				= 0x0010
};

enum AkLoopValue
{
	AkLoopVal_Infinite		= 0,
	AkLoopVal_NotLooping	= 1
};

enum AkMidiTriggerType
{
	AkMidiTriggerType_NoteOnAndOff,
	AkMidiTriggerType_NoteOn,
	AkMidiTriggerType_NoteOff
};

// Continous range for possible properties on ParameterNodeBase and descendants.
enum AkPropID
{
	// These four values must stay together in this order at offset 0 and equivalent to RTPC_Volume et AL.
    AkPropID_Volume					= 0,// Group 1 do not separate
    AkPropID_LFE					= 1,// Group 1 do not separate
    AkPropID_Pitch					= 2,// Group 1 do not separate
	AkPropID_LPF					= 3,// Group 1 do not separate
	AkPropID_BusVolume				= 4,// Group 1 do not separate

	AkPropID_Priority				= 5,
	AkPropID_PriorityDistanceOffset	= 6,

	AkPropID_Loop					= 7,

	AkPropID_FeedbackVolume			= 8,// Group 2 do not separate
	AkPropID_FeedbackLPF			= 9,// Group 2 do not separate

	AkPropID_MuteRatio				= 10,

	AkPropID_PAN_LR					= 11,
	AkPropID_PAN_FR					= 12,
	AkPropID_CenterPCT				= 13,

	AkPropID_DelayTime				= 14,
	AkPropID_TransitionTime			= 15,
	AkPropID_Probability			= 16,

	AkPropID_DialogueMode			= 17,

    AkPropID_UserAuxSendVolume0		= 18,// Group 3 do not separate
    AkPropID_UserAuxSendVolume1		= 19,// Group 3 do not separate
    AkPropID_UserAuxSendVolume2		= 20,// Group 3 do not separate
    AkPropID_UserAuxSendVolume3		= 21,// Group 3 do not separate

    AkPropID_GameAuxSendVolume		= 22,// Group 3 do not separate

    AkPropID_OutputBusVolume		= 23,// Group 3 do not separate
    AkPropID_OutputBusLPF			= 24,// Group 3 do not separate

	AkPropID_InitialDelay			= 25,

	AkPropID_HDRBusThreshold		= 26,
	AkPropID_HDRBusRatio			= 27,
	AkPropID_HDRBusReleaseTime		= 28,
	AkPropID_HDRBusGameParam		= 29,
	AkPropID_HDRBusGameParamMin		= 30,
	AkPropID_HDRBusGameParamMax		= 31,

	AkPropID_HDRActiveRange			= 32,
	AkPropID_MakeUpGain				= 33,

	AkPropID_LoopStart				= 34, // 
	AkPropID_LoopEnd				= 35, //
	AkPropID_TrimInTime				= 36, //
	AkPropID_TrimOutTime			= 37, //
	AkPropID_FadeInTime				= 38, //
	AkPropID_FadeOutTime			= 39, //
	AkPropID_FadeInCurve			= 40, //
	AkPropID_FadeOutCurve			= 41, //
	AkPropID_LoopCrossfadeDuration	= 42, //
	AkPropID_CrossfadeUpCurve		= 43, //
	AkPropID_CrossfadeDownCurve		= 44, //

	AkPropID_NUM // Number of properties defined above
};

#define AkPropID_StatePropNum 5

struct AkPropValue
{
	union
	{
		AkReal32 fValue;
		AkInt32  iValue;
	};
	AkPropValue() : fValue( 0.0f ) {} 
	AkPropValue( AkReal32 in_fValue ) : fValue( in_fValue ) {} 
	AkPropValue( AkInt32 in_iValue ) : iValue( in_iValue ) {} 
};

#if defined(AK_WIN) && defined(_DEBUG)
	#define AKPROP_TYPECHECK
#endif

#ifdef AK_DEFINE_AKPROPDEFAULT

extern const AkPropValue g_AkPropDefault[ AkPropID_NUM ] =
{
	0.0f, // AkPropID_Volume
	0.0f, // AkPropID_LFE
	0.0f, // AkPropID_Pitch
	0.0f, // AkPropID_LPF
	0.0f, // AkPropID_BusVolume
	(AkReal32) AK_DEFAULT_PRIORITY, // AkPropID_Priority
	-10.0f, // AkPropID_PriorityDistanceOffset
	(AkInt32) AkLoopVal_NotLooping, // AkPropID_Loop
	0.0f, // AkPropID_FeedbackVolume
	0.0f, // AkPropID_FeedbackLPF
	AK_UNMUTED_RATIO, // AkPropID_MuteRatio
	(AkReal32) AK_DEFAULT_PAN_RL_VALUE, // AkPropID_PAN_LR
	(AkReal32) AK_DEFAULT_PAN_FR_VALUE, // AkPropID_PAN_FR
	0.0f, // AkPropID_CenterPCT
	(AkInt32) 0, // AkPropID_DelayTime
	(AkInt32) 0, // AkPropID_TransitionTime
	(AkReal32) DEFAULT_PROBABILITY, // AkPropID_Probability
	(AkInt32) 0, // AkPropID_DialogueMode
    0.0f, // AkPropID_UserAuxSendVolume0
    0.0f, // AkPropID_UserAuxSendVolume1
    0.0f, // AkPropID_UserAuxSendVolume2
    0.0f, // AkPropID_UserAuxSendVolume3
    0.0f, // AkPropID_GameAuxSendVolume
    0.0f, // AkPropID_OutputBusVolume
    0.0f, // AkPropID_OutputBusLPF
	0.0f, // AkPropID_InitialDelay
	AK_DEFAULT_HDR_BUS_THRESHOLD, // AkPropID_HDRBusThreshold
	AK_DEFAULT_HDR_BUS_RATIO, // AkPropID_HDRBusRatio
	AK_DEFAULT_HDR_BUS_RELEASE_TIME, // AkPropID_HDRBusReleaseTime
	(AkInt32)AK_INVALID_UNIQUE_ID, // AkPropID_HDRBusGameParam
	AK_DEFAULT_HDR_BUS_GAME_PARAM_MIN, // AkPropID_HDRBusGameParamMin
	AK_DEFAULT_HDR_BUS_GAME_PARAM_MAX, // AkPropID_HDRBusGameParamMax
	AK_DEFAULT_HDR_ACTIVE_RANGE, // AkPropID_HDRActiveRange
	0.0f, // AkPropID_MakeUpGain
	
	0.0f,  // AkPropID_LoopStart
	0.0f,  // AkPropID_LoopEnd
	0.0f,  // AkPropID_TrimInTime
	0.0f,  // AkPropID_TrimOutTime
	0.0f,  // AkPropID_FadeInTime	
	0.0f,  // AkPropID_FadeOutTime
	(AkInt32)AkCurveInterpolation_Sine,  // AkPropID_FadeInCurve
	(AkInt32)AkCurveInterpolation_Sine,  // AkPropID_FadeOutCurve
	0.0f,  // AkPropID_LoopCrossfadeDuration
	(AkInt32)AkCurveInterpolation_Sine,		//AkPropID_CrossfadeUpCurve
	(AkInt32)AkCurveInterpolation_SineRecip, //AkPropID_CrossfadeDownCurve
};

extern const AkRTPC_ParameterID g_AkPropRTPCID[ AkPropID_NUM ] =
{
	RTPC_Volume, // AkPropID_Volume
	RTPC_LFE, // AkPropID_LFE
	RTPC_Pitch, // AkPropID_Pitch
	RTPC_LPF, // AkPropID_LPF
	RTPC_BusVolume, // AkPropID_BusVolume
	RTPC_Priority, // AkPropID_Priority
	RTPC_MaxNumRTPC, // AkPropID_PriorityDistanceOffset
	RTPC_MaxNumRTPC, // AkPropID_Loop
	RTPC_FeedbackVolume, // AkPropID_FeedbackVolume
	RTPC_FeedbackLowpass, // AkPropID_FeedbackLPF
	RTPC_MaxNumRTPC, // AkPropID_MuteRatio
	RTPC_Position_PAN_X_2D, // AkPropID_PAN_LR
	RTPC_Position_PAN_Y_2D, // AkPropID_PAN_FR
	RTPC_Positioning_Divergence_Center_PCT, // AkPropID_CenterPCT
	RTPC_MaxNumRTPC, // AkPropID_DelayTime
	RTPC_MaxNumRTPC, // AkPropID_TransitionTime
	RTPC_MaxNumRTPC, // AkPropID_Probability
	RTPC_MaxNumRTPC, // AkPropID_DialogueMode
    RTPC_UserAuxSendVolume0,	// AkPropID_UserAuxSendVolume0
    RTPC_UserAuxSendVolume1,	// AkPropID_UserAuxSendVolume1
    RTPC_UserAuxSendVolume2,	// AkPropID_UserAuxSendVolume2
    RTPC_UserAuxSendVolume3,	// AkPropID_UserAuxSendVolume3
    RTPC_GameAuxSendVolume,		// AkPropID_GameAuxSendVolume
    RTPC_OutputBusVolume,		// AkPropID_OutputBusVolume
    RTPC_OutputBusLPF,			// AkPropID_OutputBusLPF
	RTPC_InitialDelay,			// AkPropID_InitialDelay
	RTPC_HDRBusThreshold,		// AkPropID_HDRBusThreshold
	RTPC_HDRBusRatio,			// AkPropID_HDRBusRatio
	RTPC_HDRBusReleaseTime,		// AkPropID_HDRBusReleaseTime
	RTPC_MaxNumRTPC,			// AkPropID_HDRBusGameParam
	RTPC_MaxNumRTPC,			// AkPropID_HDRBusGameParamMin
	RTPC_MaxNumRTPC,			// AkPropID_HDRBusGameParamMax
	RTPC_HDRActiveRange,		// AkPropID_HDRActiveRange
	RTPC_MakeUpGain,			// AkPropID_MakeUpGain
	RTPC_MaxNumRTPC, // AkPropID_LoopStart
	RTPC_MaxNumRTPC, // AkPropID_LoopEnd
	RTPC_MaxNumRTPC,  // AkPropID_TrimInTime
	RTPC_MaxNumRTPC,  // AkPropID_TrimOutTime
	RTPC_MaxNumRTPC,  // AkPropID_FadeInTime	
	RTPC_MaxNumRTPC,  // AkPropID_FadeOutTime
	RTPC_MaxNumRTPC,  // AkPropID_FadeInCurve
	RTPC_MaxNumRTPC,  // AkPropID_FadeOutCurve
	RTPC_MaxNumRTPC,  // AkPropID_LoopCrossfadeDuration
	RTPC_MaxNumRTPC,  // AkPropID_CrossfadeUpCurve
	RTPC_MaxNumRTPC,  // AkPropID_CrossfadeDownCurve
};

extern const bool g_AkPropDecibel[ AkPropID_NUM ] =
{
	true, // AkPropID_Volume
	true, // AkPropID_LFE
	false, // AkPropID_Pitch
	false, // AkPropID_LPF
	true, // AkPropID_BusVolume
	false, // AkPropID_Priority
	false, // AkPropID_PriorityDistanceOffset
	false, // AkPropID_Loop
	true, // AkPropID_FeedbackVolume
	false, // AkPropID_FeedbackLPF
	false, // AkPropID_MuteRatio
	false, // AkPropID_PAN_LR
	false, // AkPropID_PAN_FR
	false, // AkPropID_CenterPCT
	false, // AkPropID_DelayTime
	false, // AkPropID_TransitionTime
	false, // AkPropID_Probability
	false, // AkPropID_DialogueMode
	true, // AkPropID_UserAuxSendVolume0
	true, // AkPropID_UserAuxSendVolume1
	true, // AkPropID_UserAuxSendVolume2
	true, // AkPropID_UserAuxSendVolume3
	true, // AkPropID_GameAuxSendVolume
	true, // AkPropID_OutputBusVolume
	false, // AkPropID_OutputBusLPF
	false, // AkPropID_InitialDelay
	true, // AkPropID_HDRBusThreshold
	false, // AkPropID_HDRBusRatio
	false, // AkPropID_HDRBusReleaseTime	
	false, // AkPropID_HDRBusGameParam
	false, // AkPropID_HDRBusGameParamMin
	false, // AkPropID_HDRBusGameParamMax
	true, // AkPropID_HDRActiveRange
	true, // AkPropID_MakeUpGain
	
	false,  // AkPropID_LoopStart
	false,  // AkPropID_LoopEnd
	false,  // AkPropID_TrimInTime
	false,  // AkPropID_TrimOutTime
	false,  // AkPropID_FadeInTime	
	false,  // AkPropID_FadeOutTime
	false,  // AkPropID_FadeInCurve
	false,  // AkPropID_FadeOutCurve
	false,  // AkPropID_LoopCrossfadeDuration
	false,  // AkPropID_CrossfadeUpCurve
	false,  // AkPropID_CrossfadeDownCurve
};

#ifdef AKPROP_TYPECHECK
	#include <typeinfo>
	extern const std::type_info * g_AkPropTypeInfo[ AkPropID_NUM ] = 
	{	// The lifetime of the object returned by typeid extends to the end of the program.
		&typeid(AkReal32), // AkPropID_Volume
		&typeid(AkReal32), // AkPropID_LFE
		&typeid(AkReal32), // AkPropID_Pitch
		&typeid(AkReal32), // AkPropID_LPF
		&typeid(AkReal32), // AkPropID_BusVolume
		&typeid(AkReal32), // AkPropID_Priority
		&typeid(AkReal32), // AkPropID_PriorityDistanceOffset
		&typeid(AkInt32),  // AkPropID_Loop
		&typeid(AkReal32), // AkPropID_FeedbackVolume
		&typeid(AkReal32), // AkPropID_FeedbackLPF
		&typeid(AkReal32), // AkPropID_MuteRatio
		&typeid(AkReal32), // AkPropID_PAN_LR
		&typeid(AkReal32), // AkPropID_PAN_FR
		&typeid(AkReal32), // AkPropID_CenterPCT
		&typeid(AkInt32),  // AkPropID_DelayTime
		&typeid(AkInt32),  // AkPropID_TransitionTime
		&typeid(AkReal32), // AkPropID_Probability
		&typeid(AkInt32),  // AkPropID_DialogueMode
		&typeid(AkReal32), // AkPropID_UserAuxSendVolume0
		&typeid(AkReal32), // AkPropID_UserAuxSendVolume1
		&typeid(AkReal32), // AkPropID_UserAuxSendVolume2
		&typeid(AkReal32), // AkPropID_UserAuxSendVolume3
		&typeid(AkReal32), // AkPropID_GameAuxSendVolume
		&typeid(AkReal32), // AkPropID_OutputBusVolume
		&typeid(AkReal32), // AkPropID_OutputBusLPF
		&typeid(AkReal32), // AkPropID_InitialDelay
		&typeid(AkReal32), // AkPropID_HDRBusThreshold
		&typeid(AkReal32), // AkPropID_HDRBusRatio
		&typeid(AkReal32), // AkPropID_HDRBusReleaseTime	
		&typeid(AkInt32), // AkPropID_HDRBusGameParam
		&typeid(AkReal32), // AkPropID_HDRBusGameParamMin
		&typeid(AkReal32), // AkPropID_HDRBusGameParamMax
		&typeid(AkReal32), // AkPropID_HDRActiveRange
		&typeid(AkReal32), // AkPropID_MakeUpGain
		
		&typeid(AkReal32),  // AkPropID_LoopStart
		&typeid(AkReal32),  // AkPropID_LoopEnd
		&typeid(AkReal32),  // AkPropID_TrimInTime
		&typeid(AkReal32),  // AkPropID_TrimOutTime
		&typeid(AkReal32),  // AkPropID_FadeInTime	
		&typeid(AkReal32),  // AkPropID_FadeOutTime
		&typeid(AkInt32),  // AkPropID_FadeInCurve
		&typeid(AkInt32),  // AkPropID_FadeOutCurve
		&typeid(AkReal32),  // AkPropID_LoopCrossfadeDuration
		&typeid(AkInt32),  // AkPropID_CrossfadeUpCurve
		&typeid(AkInt32),  // AkPropID_CrossfadeDownCurve
	};
#endif

#else

extern const AkPropValue g_AkPropDefault[ AkPropID_NUM ];
extern const AkRTPC_ParameterID g_AkPropRTPCID[ AkPropID_NUM ];
extern const bool g_AkPropDecibel[ AkPropID_NUM ];

#ifdef AKPROP_TYPECHECK
	#include <typeinfo>
	extern const std::type_info * g_AkPropTypeInfo[ AkPropID_NUM ];
#endif

#endif

template <class T_Type>
struct RANGED_MODIFIERS
{
	T_Type m_min;
	T_Type m_max;
};

template <class T_Type>
struct RANGED_PARAMETER
{
	T_Type m_base;
	RANGED_MODIFIERS<T_Type> m_mod;
	RANGED_PARAMETER()
		:m_base(0)
	{
		m_mod.m_min = 0;
		m_mod.m_max = 0;
	}
};

struct AkLoop
{
	AkInt16 lLoopCount;		// Number of loop before continue
	AkUInt8 bIsEnabled  :1;	// Is Looping enabled
	AkUInt8 bIsInfinite :1;	// Is looping infinite
	AkLoop()
		:lLoopCount(0)
		,bIsEnabled(false)
		,bIsInfinite(false)
	{}
};

enum AkValueMeaning
{
	AkValueMeaning_Default		= 0,		//Use default parameter instead
	AkValueMeaning_Independent	= 1,		//Use this parameter directly (also means override when on an object)
	AkValueMeaning_Offset		= 2			//Use this parameter as an offset from the default value
#define VALUE_MEANING_NUM_STORAGE_BIT 4
};

struct AkPBIModValues
{
	AkVolumeValue	VolumeOffset;
	AkPitchValue	PitchOffset;
	AkLPFType		LPFOffset;

	AkPBIModValues()
		:VolumeOffset(0)
		,PitchOffset(0)
		,LPFOffset(0)
	{}
};
