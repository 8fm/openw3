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
// Ak3DListener.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkCommon.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkFeedbackStructs.h"
#include "Ak3DParams.h"

class AkAudioBuffer;
struct AkSoundParams;
struct AkSpeakerVolumes;
class  CAkPBI;
struct Gen3DParams;
struct ConeParams;
class CAkAttenuation;
class CAkOutputDevices;

class CAkListener
{
public:

	static void Init();

	// Set output device for one or many listeners.
	static void RouteListenersToDevice(
		AkUInt32 in_uListenerMask,				// Bitmask representing the listeners (LSB = Listener 0, set to 1 means active)
		AkOutputDeviceID in_uDeviceID					// Device ID
		);

	static AkForceInline const AkListenerData & GetListenerData( 
		AkUInt32 in_uListener
		)
	{
		return m_listeners[ in_uListener ];
	}

	static AKRESULT SetListenerPosition( 
		AkUInt32 in_uListener,
		const AkListenerPosition & in_Position
		);

	static void SetScalingFactor( 
		AkUInt32 in_uListener,
		AkReal32 in_fScalingFactor
		);

	static AkForceInline void GetPosition( 
		AkUInt32 in_uListener,
		AkListenerPosition & out_position
		)
	{
		AKASSERT( in_uListener < AK_NUM_LISTENERS );
		out_position = m_listeners[ in_uListener ].position;
	}

	static AkForceInline AkReal32 GetScalingFactor( 
		AkUInt32 in_uListener
		)
	{
		AKASSERT( in_uListener < AK_NUM_LISTENERS );
		return m_listeners[ in_uListener ].fScalingFactor;
	}

	static AKRESULT SetListenerSpatialization(
		AkUInt32 in_uListener,
		bool in_bSpatialized,
		AkSpeakerVolumes * in_pVolumeOffsets
		);

	static AKRESULT GetListenerSpatialization(
		AkUInt32 in_uListener,
		bool& out_rbSpatialized,
		AkSpeakerVolumes& out_rVolumeOffsets
		);

	static void Get3DVolumes(
		AkPositionSourceType	in_ePosType,
		bool				in_bIsAuxRoutable,
		CAkPBI* AK_RESTRICT in_pContext,
		AkVolumeDataArray & out_arVolumeData		// In: emitter-listener pairs, out: filled with volume data.
		);

	static void ComputeSpeakerMatrix(
		bool				in_bIsAuxRoutable,
		CAkPBI*	AK_RESTRICT	in_pContext,
	    const AkVolumeDataArray & in_arVolumeData,
		AkChannelMask		in_uInputConfig,
		AkReal32 			in_fBehavioralVolume,	// Collapsed volumes of actor and voice bus hierarchies, and fades,
		CAkOutputDevices &	in_deviceVolumes
		);

	static void SetListenerPipeline(
		AkUInt32		in_uListener,	//Listener ID.
		bool			in_bAudio,		//Is this listener used for audio data?
		bool			in_bFeedback	//Is this listener used for feedback data?
		);

	static AkForceInline AkUInt32 GetFeedbackMask() 
	{
		return m_uFeedbackMask;
	}

	static void DoSpeakerVolumeMatrixCallback(
		AkPlayingID in_playingID,
		AkUInt32 in_uNumChannels,
		AkUInt32 in_uInputConfig,
		AkUInt32 in_uOutputConfig,
		AkDeviceInfo & io_rDeviceVolume
	);

	static void OnBeginFrame();

	static void ResetListenerData();

	static inline AkReal32 * GetListenerMatrix(AkUInt32 in_uListener) { return &m_listeners[in_uListener].Matrix[0][0]; }
	
//----------------------------------------------------------------------------------------------------
// audiolib only variables
//----------------------------------------------------------------------------------------------------
private:

	static void GetListenerVolume( 
		CAkAttenuation * in_pAttenuation, 
		const AkEmitterListenerPair & in_ray,
		AkReal32 in_fConeOutsideVolume,
		AkReal32& out_fMinCone,
		AkReal32& out_fListenerVolumeDry,		// Linear
		AkReal32& out_fListenerVolumeGameDefAux,// Linear
		AkReal32& out_fListenerVolumeUserDefAux	// Linear
		);

	// Returns volume attenuation due to cone,
	// and out_fCone is the interpolated cone value [0,1] for LPF evaluation, later.
	static inline AkReal32 ComputeConeAttenuation(
		AkReal32	in_fInsideAngle, 
		AkReal32	in_fOutsideAngle, 
		AkReal32	in_fConeOutsideVolume,
		AkReal32	in_fAngle,
		AkReal32 &	out_fCone
		)
	{
		AkReal32 fConeValue = AkMath::Interpolate( in_fInsideAngle, 0.0f, in_fOutsideAngle, 1.0f, in_fAngle );
		out_fCone = fConeValue;
		// REVIEW: Linear interpolation of decibels...!
		return AkMath::dBToLin( fConeValue * in_fConeOutsideVolume );
	}

	// the listeners we know about
	static AkListenerData m_listeners[ AK_NUM_LISTENERS ];
	static AkUInt32	m_uAudioMask;
	static AkUInt32 m_uFeedbackMask;

	static AkReal32 m_matIdentity[3][3];
};

