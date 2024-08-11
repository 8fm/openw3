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
// AkRTPC.h
//
// Data structures used for RTPC.
//
//////////////////////////////////////////////////////////////////////
#ifndef _RTPC_H_
#define _RTPC_H_

#include "AkPrivateTypes.h"

// Every point of a conversion graph
template< class VALUE_TYPE >
struct AkRTPCGraphPointBase
{
	AkReal32				From;		// Representing X Axis
	VALUE_TYPE				To;	    	// Representing Y Axis
	AkCurveInterpolation	Interp;		// How to interpolate between this point and the next
};

// Every point of a conversion graph
typedef AkRTPCGraphPointBase<AkReal32> AkRTPCGraphPoint;

// Points in switch conversion curves
typedef AkRTPCGraphPointBase<AkUInt32> AkRTPCGraphPointInteger;

enum AkCurveScaling // Written as AkUInt8 in bank
{
	AkCurveScaling_None							= 0, // No special scaling
	AkCurveScaling_dB							= 2, // dB scaling (mirrored around 0)
	AkCurveScaling_Log							= 3, // log scaling (typically used to map frequencies)
	AkCurveScaling_dBToLin						= 4  // curve is dB values, result wanted in linear gain
};

// IDs of the AudioLib known RTPC capable parameters
enum AkRTPC_ParameterID
{
	// Main Audionode
	RTPC_Volume										= 0,
	RTPC_LFE										= 1,
	RTPC_Pitch										= 2,

	// Main LPF
	RTPC_LPF										= 3,

	RTPC_BusVolume								= 4,

	// Random/Sequence Container
	RTPC_PlayMechanismSpecialTransitionsValue		= 5,

	RTPC_InitialDelay								= 6,

	// Advanced Settings
	RTPC_Priority									= 8,
	RTPC_MaxNumInstances							= 9,

	// Positioning - Parameters common to Radius and Position
	RTPC_PositioningType							= 10,
	RTPC_Positioning_Divergence_Center_PCT			= 11,
	RTPC_Positioning_Cone_Attenuation_ON_OFF		= 12,
	RTPC_Positioning_Cone_Attenuation				= 13,
	RTPC_Positioning_Cone_LPF						= 14,

    RTPC_UserAuxSendVolume0							= 15,
    RTPC_UserAuxSendVolume1							= 16,
    RTPC_UserAuxSendVolume2							= 17,
    RTPC_UserAuxSendVolume3							= 18,

	RTPC_GameAuxSendVolume							= 19,

	// Positioning - Position specific
	RTPC_Position_PAN_X_2D							= 20,
	RTPC_Position_PAN_Y_2D							= 21,

    RTPC_OutputBusVolume							= 22,
    RTPC_OutputBusLPF								= 23,

	// The 4 effect slots
	RTPC_BypassFX0									= 24,
	RTPC_BypassFX1									= 25,
	RTPC_BypassFX2									= 26,
	RTPC_BypassFX3									= 27,
	RTPC_BypassAllFX								= 28, // placed here to follow bit order in bitfields

	RTPC_FeedbackVolume								= 29,
	RTPC_FeedbackLowpass							= 30,
	RTPC_FeedbackPitch								= 31, // not actually a rtpcable parameter

	// HDR
	RTPC_HDRBusThreshold							= 32,
	RTPC_HDRBusReleaseTime							= 33,
	RTPC_HDRBusRatio								= 34,
	RTPC_HDRActiveRange								= 35,

	RTPC_MakeUpGain									= 36,

	// Positioning - 3D position
	RTPC_Position_PAN_X_3D							= 37,
	RTPC_Position_PAN_Y_3D							= 38,

	RTPC_MaxNumRTPC									= 64

	// Should not exceed a value of 64, if it does, then the m_RTPCBitArray need to grow
};

#endif //_RTPC_H_
