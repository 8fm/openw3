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

//////////////////////////////////////////////////////////////////////
//
// AkPrivateTypes.h
//
// Audiokinetic Data Type Definition (internal)
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKPRIVATETYPES_H
#define _AKPRIVATETYPES_H

// Moved to SDK.
#include <AK/SoundEngine/Common/AkTypes.h>

// Below: Internal only definitions.
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
#include "AudiolibLimitations.h"

//----------------------------------------------------------------------------------------------------
// Structs.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Macros.
//----------------------------------------------------------------------------------------------------

#define DECLARE_BASECLASS( baseClass )	\
	private:							\
		typedef baseClass __base		\

//----------------------------------------------------------------------------------------------------
// Enums
//----------------------------------------------------------------------------------------------------

enum AkVirtualQueueBehavior
{
//DONT ADD ANYTHING HERE, STORED ON 3 BITS!
	// See documentation for the definition of the behaviors.
	AkVirtualQueueBehavior_FromBeginning 	= 0,
	AkVirtualQueueBehavior_FromElapsedTime  = 1,
	AkVirtualQueueBehavior_Resume 			= 2
//DONT ADD ANYTHING HERE, STORED ON 3 BITS!
#define VIRTUAL_QUEUE_BEHAVIOR_NUM_STORAGE_BIT 3
};

enum AkBelowThresholdBehavior
{
//DONT ADD ANYTHING HERE, STORED ON 3 BITS!
	// See documentation for the definition of the behaviors.
	AkBelowThresholdBehavior_ContinueToPlay 	= 0,
	AkBelowThresholdBehavior_KillVoice			= 1,
	AkBelowThresholdBehavior_SetAsVirtualVoice 	= 2
//DONT ADD ANYTHING HERE, STORED ON 3 BITS!
#define BELOW_THRESHOLD_BEHAVIOR_NUM_STORAGE_BIT 4
};

///Transition mode selection
enum AkTransitionMode
{
//DONT ADD ANYTHING HERE, STORED ON 4 BITS!
	Transition_Disabled				= 0,	// Sounds are followed without any delay
	Transition_CrossFadeAmp			= 1,	// Sound 2 starts before sound 1 finished (constant amplitude)
	Transition_CrossFadePower		= 2,	// Sound 2 starts before sound 1 finished (constant power)
	Transition_Delay				= 3,	// There is a delay before starting sound 2 once sound 1 terminated
	Transition_SampleAccurate		= 4,	// Next sound is prepared in advance and uses the same pipeline than the previous one
	Transition_TriggerRate			= 5		// Sound 2 starts after a fixed delay
//DONT ADD ANYTHING HERE, STORED ON 4 BITS!
#define TRANSITION_MODE_NUM_STORAGE_BIT 4
};

//----------------------------------------------------------------------------------------------------
// Private types, common to Wwise and the sound engine
//----------------------------------------------------------------------------------------------------

// Type for game objects used in the authoring tool, and in communication.
// IMPORTANT: Never use AkGameObjectID in communication and in the tool, otherwise there would
// be incompatibilities between 32 and 64-bit versions of the sound engine and authoring tool.
typedef AkUInt64	AkWwiseGameObjectID;
static const AkWwiseGameObjectID	WWISE_INVALID_GAME_OBJECT	= (AkWwiseGameObjectID)(-1);	// Invalid game object (may also mean all game objects)

/// Internal structure.
class AkExternalSourceArray;

/// Optional parameter.
struct AkCustomParamType
{
	AkInt64					customParam;	///< Reserved, must be 0
	AkUInt32				ui32Reserved;	///< Reserved, must be 0
	AkExternalSourceArray*  pExternalSrcs;	///< Reserved
};

// Loudness frequency weighting types.
enum AkLoudnessFrequencyWeighting
{
	FrequencyWeighting_None	= 0,
	FrequencyWeighting_K,
	FrequencyWeighting_A,
	AK_NUM_FREQUENCY_WEIGHTINGS
};

typedef AkUInt64		AkOutputDeviceID;			///< Audio Output device ID

//----------------------------------------------------------------------------------------------------
// Common defines
//----------------------------------------------------------------------------------------------------

#define AK_UNMUTED_RATIO	(1.f)
#define AK_MUTED_RATIO	  	(0.f)

#define AK_DEFAULT_LEVEL_DB		(0.0f)
#define AK_MAXIMUM_VOLUME_DBFS	(0.0f)
#define AK_MINIMUM_VOLUME_DBFS	(-96.3f)

#define AK_DEFAULT_HDR_BUS_THRESHOLD			(96.f)
#define AK_DEFAULT_HDR_BUS_RATIO				(16.f)	// 16:1 = infinity
#define AK_DEFAULT_HDR_BUS_RELEASE_TIME			(1.f)	// seconds
#define AK_DEFAULT_HDR_ACTIVE_RANGE				(12.f)	// dB
#define AK_DEFAULT_HDR_BUS_GAME_PARAM_MIN		(0.f)
#define AK_DEFAULT_HDR_BUS_GAME_PARAM_MAX		(200.f)

#define AK_LOUDNESS_BIAS				(14.125375f)	// 23 dB

#define AK_EVENTWITHCOOKIE_RESERVED_BIT 0x00000001
#define AK_EVENTFROMWWISE_RESERVED_BIT	0x40000000

//ID Bit that can be used only by audionodes created by the sound engine.
#define AK_SOUNDENGINE_RESERVED_BIT (0x80000000)

#define DOUBLE_WEIGHT_TO_INT_CONVERSION_FACTOR	1000
#define DEFAULT_RANDOM_WEIGHT					(50*DOUBLE_WEIGHT_TO_INT_CONVERSION_FACTOR)
#define DOUBLE_WEIGHT_TO_INT( _in_double_val_ ) ( (AkUInt32)(_in_double_val_*DOUBLE_WEIGHT_TO_INT_CONVERSION_FACTOR) )

#define DEFAULT_PROBABILITY		100

#define NO_PLAYING_ID			0

#define AK_DEFAULT_PITCH						(0)
#define AK_LOWER_MIN_DISTANCE					(0.001f)
#define AK_UPPER_MAX_DISTANCE					(10000000000.0f)

#define AK_DEFAULT_DISTANCE_FACTOR				(1.0f)

// cone

#define AK_OMNI_INSIDE_CONE_ANGLE				(TWOPI)
#define AK_OMNI_OUTSIDE_CONE_ANGLE				(TWOPI)
#define AK_DEFAULT_OUTSIDE_VOLUME				(-10.0f)

//(<255) Stored on AkUInt8 
#define	AK_MIN_LOPASS_VALUE						(0)
#define	AK_MAX_LOPASS_VALUE						(100) 
#define	AK_DEFAULT_LOPASS_VALUE					(0) 

#define AK_MIN_PAN_RL_VALUE						(-100)
#define AK_MAX_PAN_RL_VALUE						(100)
#define AK_DEFAULT_PAN_RL_VALUE					(0)

#define AK_MIN_PAN_FR_VALUE						(-100)
#define AK_MAX_PAN_FR_VALUE						(100)
#define AK_DEFAULT_PAN_FR_VALUE					(0)

#define	AK_MIN_LOPASS_VALUE						(0)
#define	AK_MAX_LOPASS_VALUE						(100) 
#define	AK_DEFAULT_LOPASS_VALUE					(0)

#define AK_SMALL_HASH_SIZE						(31)

#define AK_LARGE_HASH_SIZE						(193)

// Sample accurate stopping.
#define AK_NO_IN_BUFFER_STOP_REQUESTED			(AK_UINT_MAX)

// Bus ducking.
#define AK_DEFAULT_MAX_BUS_DUCKING				(-96.3f)	// Arbitrary. Should be -INF.

#endif // _AKPRIVATETYPES_H
