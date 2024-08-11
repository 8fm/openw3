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
// Ak3DListener.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMath.h"
#include "Ak3DListener.h"
#include "AudiolibDefs.h"
#include "AkDefault3DParams.h"
#include "AkSpeakerPan.h"
#include "AkPBI.h"
#include "AkEnvironmentsMgr.h"
#include "AkGen3DParams.h"
#include "AkPlayingMgr.h"
#include "AkLEngine.h"
#include "AkPath.h"
#include "AkOutputMgr.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
#endif // AK_MOTION

#include "AkMath.h"

extern AkReal32 g_fVolumeThreshold;
extern AkReal32 g_fVolumeThresholdDB;

AkListenerData CAkListener::m_listeners[ AK_NUM_LISTENERS ];
AkUInt32 CAkListener::m_uAudioMask = 0xFFFFFFFF;	//All are active in audio by default
AkUInt32 CAkListener::m_uFeedbackMask = 0;			//Listeners don't receive feedback by default.

AkReal32 CAkListener::m_matIdentity[3][3] = 
{
	{ 1, 0, 0 },
	{ 0, 1, 0 },
	{ 0, 0, 1 }
};

//====================================================================================================
//====================================================================================================
void CAkListener::Init()
{
	// initialise them
	for(AkUInt32 i = 0 ; i < AK_NUM_LISTENERS ; ++i)
	{
		AkListenerData* pListener = m_listeners + i;
		pListener->customSpeakerGain.Set( 1.f );
		pListener->bSpatialized = true;
		pListener->fScalingFactor = 1.0f;
		pListener->uDeviceID = AK_MAIN_OUTPUT_DEVICE;
		pListener->bPositionDirty = true;

		SetListenerPosition(i,g_DefaultListenerPosition);
	}

	m_uFeedbackMask = 0;			
	m_uAudioMask = 0xFFFFFFFF;		//All are active in audio by default
}

// Set output device for one or many listeners.
void CAkListener::RouteListenersToDevice(
	AkUInt32 in_uListenerMask,				// Bitmask representing the listeners (LSB = Listener 0, set to 1 means active)
	AkOutputDeviceID in_uDeviceID					// Device ID
	)
{
	// Set device of all listeners specified in mask to in_uDeviceID.
	AkUInt32 uListener = 0;
	while ( in_uListenerMask )
	{
		if ( in_uListenerMask & 0x01 )
			m_listeners[uListener].uDeviceID = in_uDeviceID;
		++uListener;
		in_uListenerMask >>= 1;
	}
}

void CAkListener::OnBeginFrame()
{
	// Set up mask of listeners whose position has changed.
	AkUInt32 uMaskListenersChanged = 0;
	for(AkUInt32 i = 0 ; i < AK_NUM_LISTENERS ; ++i)
	{
		if ( m_listeners[i].bPositionDirty )
			uMaskListenersChanged |= ( 1 << i );
	}

	if ( uMaskListenersChanged )
	{
		// Notify game objects.
		g_pRegistryMgr->NotifyListenerPosChanged( uMaskListenersChanged );
	}

	// Clear flag.
	for(AkUInt32 i = 0 ; i < AK_NUM_LISTENERS ; ++i)
	{
		m_listeners[i].bPositionDirty = false;
	}
}

void CAkListener::ResetListenerData()
{
	// Force Recomputation of the cached matrix
	for(AkUInt32 i = 0 ; i < AK_NUM_LISTENERS ; ++i)
	{
		AkListenerData* pListener = m_listeners + i;
		SetListenerPosition(i,pListener->position);
	}
}

AKRESULT CAkListener::SetListenerPosition( AkUInt32 in_uListener, const AkListenerPosition & in_Position )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	AkListenerData & listener = m_listeners[in_uListener];

	listener.position = in_Position;

	AkVector OrientationSide = AkMath::CrossProduct(in_Position.OrientationTop,in_Position.OrientationFront);

	AkReal32* pFloat = &( listener.Matrix[0][0] );

	// Build up rotation matrix out of our 3 basic row vectors, stored in row major order.
	*pFloat++ = OrientationSide.X;
	*pFloat++ = OrientationSide.Y;
	*pFloat++ = OrientationSide.Z;

	*pFloat++ = listener.position.OrientationTop.X;
	*pFloat++ = listener.position.OrientationTop.Y;
	*pFloat++ = listener.position.OrientationTop.Z;

	*pFloat++ = listener.position.OrientationFront.X;
	*pFloat++ = listener.position.OrientationFront.Y;
	*pFloat++ = listener.position.OrientationFront.Z;

	listener.bPositionDirty = true;

	return AK_Success;
}

void CAkListener::SetScalingFactor( 
		AkUInt32 in_uListener,
		AkReal32 in_fScalingFactor
		)
{
	AKASSERT( in_uListener < AK_NUM_LISTENERS && in_fScalingFactor > 0.0f );

	m_listeners[ in_uListener ].fScalingFactor = in_fScalingFactor;
	m_listeners[ in_uListener ].bPositionDirty = true;
}

AKRESULT CAkListener::SetListenerSpatialization( AkUInt32 in_uListener, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	m_listeners[ in_uListener ].bSpatialized = in_bSpatialized;

	if ( in_pVolumeOffsets )
	{
		m_listeners[ in_uListener ].customSpeakerGain = *in_pVolumeOffsets;
		m_listeners[ in_uListener ].customSpeakerGain.dBToLin();
	}
	else
	{
		m_listeners[ in_uListener ].customSpeakerGain.Set( 1.f );
	}

	return AK_Success;
}

AKRESULT CAkListener::GetListenerSpatialization( AkUInt32 in_uListener, bool& out_rbSpatialized, AkSpeakerVolumes& out_rVolumeOffsets )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	out_rbSpatialized  = m_listeners[ in_uListener ].bSpatialized;
	AkSIMDSpeakerVolumes customSpeakerGain = m_listeners[ in_uListener ].customSpeakerGain;
	customSpeakerGain.FastLinTodB();
	out_rVolumeOffsets = customSpeakerGain.volumes;

	return AK_Success;
}

//====================================================================================================
// Determines if the specified listener should listen to Audio and/or Feedback data.
//====================================================================================================
void CAkListener::SetListenerPipeline(
		AkUInt32		in_uListener,	//Listener ID.
		bool			in_bAudio,		//Is this listener used for audio data?
		bool			in_bFeedback)	//Is this listener used for feedback data?
{
	AkUInt32 uMask = 1 << in_uListener;
	m_uAudioMask = m_uAudioMask & ~uMask;
	if (in_bAudio)
		m_uAudioMask = m_uAudioMask | uMask;

	m_uFeedbackMask = m_uFeedbackMask & ~uMask;
	if (in_bFeedback)
		m_uFeedbackMask = m_uFeedbackMask | uMask;
}

// Update emitter-listener rays, compute hierarchy and distance based volume attenuation.
void CAkListener::Get3DVolumes( 
	AkPositionSourceType	in_ePosType,
	bool				in_bIsAuxRoutable,
	CAkPBI* AK_RESTRICT in_pContext,
    AkVolumeDataArray & io_arVolumeData		// In: emitter-listener pairs, out: filled with volume data.
	)
{
	AKASSERT( !io_arVolumeData.IsEmpty() );

	AkReal32 fDryLevel = ( in_bIsAuxRoutable ) ? in_pContext->GetDryLevelValue() : 1.f;
	AkReal32 fMainOutputBusGain = in_pContext->GetOutputBusVolumeValue();

	AkReal32 fListenerVolumeDry;
	AkReal32 fListenerVolumeGameDefAux;
	AkReal32 fListenerVolumeUserDefAux;

	Gen3DParams *		pGen3DParams = in_pContext->Get3DSound()->GetParams();
	AkReal32			fConeOutsideVolume = pGen3DParams->m_fConeOutsideVolume;
	CAkAttenuation *	pAttenuation = pGen3DParams->GetAttenuation();
	
	// 3D Game Defined
	if( in_ePosType == AkGameDef )
	{
		AkVolumeDataArray::Iterator it = io_arVolumeData.Begin();
		do
		{
			AkUInt32 uListener = (*it).ListenerIdx();
			GetListenerVolume( pAttenuation, (*it), fConeOutsideVolume, (*it).fConeInterp, fListenerVolumeDry, fListenerVolumeGameDefAux, fListenerVolumeUserDefAux );

			// Occlusion
			AkReal32 fOccValue = in_pContext->GetOcclusionValue( uListener );

			AkReal32 fOccVolume = g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveVol )
				? g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveVol, fOccValue * 100 )
				: 1.0f;

			// Obstruction
			AkReal32 fObsValue = in_pContext->GetObstructionValue( uListener );
			AkReal32 fObsAndOccVolume = g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveVol )
				? fOccVolume * g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveVol, fObsValue * 100 )
				: fOccVolume;

			// Note: fDryLevel equals 1 if voice has no routing to an aux bus.
			(*it).fDryMixGain = fObsAndOccVolume * fListenerVolumeDry * fDryLevel * fMainOutputBusGain;
			(*it).fGameDefAuxMixGain = fOccVolume * fListenerVolumeGameDefAux;
			(*it).fUserDefAuxMixGain = fOccVolume * fListenerVolumeUserDefAux;

			++it;
		}
		while ( it != io_arVolumeData.End() );
	}
	else // 3D User Defined
	{
		AKASSERT( in_ePosType == AkUserDef );

		AkVolumeDataArray::Iterator it = io_arVolumeData.Begin();
		do
		{
			GetListenerVolume( pAttenuation, (*it), fConeOutsideVolume, (*it).fConeInterp, fListenerVolumeDry, fListenerVolumeGameDefAux, fListenerVolumeUserDefAux );

			// Note: fDryLevel equals 1 if voice has no routing to an aux bus.
			(*it).fDryMixGain = fListenerVolumeDry * fDryLevel * fMainOutputBusGain;
			(*it).fGameDefAuxMixGain = fListenerVolumeGameDefAux;
			(*it).fUserDefAuxMixGain = fListenerVolumeUserDefAux;

			++it;
		}
		while ( it != io_arVolumeData.End() );
	}
}

#define GET_VOLUME_MATRIX_ARRAY_ITEM( _volume_matrix_, _volume_index_ ) &_volume_matrix_[_volume_index_ * AK_VOICE_MAX_NUM_CHANNELS ]
#define GET_VOLUME_MATRIX_ITEM( _volume_matrix_, _volume_index_, _channel_index_ ) _volume_matrix_[_volume_index_ * AK_VOICE_MAX_NUM_CHANNELS + _channel_index_ ]

void CAkListener::ComputeSpeakerMatrix( 
	bool				in_bIsAuxRoutable,	/// TODO Get rid of this
	CAkPBI* AK_RESTRICT in_pContext,
	const AkVolumeDataArray & in_arVolumeData,
	AkChannelMask		in_uInputConfig,
	AkReal32 			in_fBehavioralVolume,
	CAkOutputDevices &	in_deviceVolumes
	)
{
	AKASSERT( !in_arVolumeData.IsEmpty() );
	// Discard LFE, consider only fullband channels.
	AkUInt32 uNumChannels = AK::GetNumChannels( in_uInputConfig );
	AKASSERT( uNumChannels );

#ifdef AK_LFECENTER
	bool bSourceHasLFE = AK::HasLFE( in_uInputConfig );
	AkUInt32 uChanLFE = uNumChannels - 1;	// Only relevant if source has LFE.
	AkReal32 fDivergenceCenter = in_pContext->GetDivergenceCenter();
	AkUInt32 uNumFullBandChannels = ( bSourceHasLFE ) ? uNumChannels - 1 : uNumChannels;
#else
	AkReal32 fDivergenceCenter = 0.0f;
	AkUInt32 uNumFullBandChannels = uNumChannels;
#endif

	AkUInt32 uNumRays = in_arVolumeData.Length();
	AkSIMDSpeakerVolumes * arVolumesMatrix = (AkSIMDSpeakerVolumes*)AkAlloca( uNumRays * AK_VOICE_MAX_NUM_CHANNELS * sizeof(AkSIMDSpeakerVolumes) );
	memset( arVolumesMatrix, 0, uNumRays * AK_VOICE_MAX_NUM_CHANNELS * sizeof(AkSIMDSpeakerVolumes) );
	AkReal32 * arOccCtrlValue = (AkReal32*)AkAlloca( uNumRays * sizeof(AkReal32) );
	AkReal32 * arObsCtrlValue = (AkReal32*)AkAlloca( uNumRays * sizeof(AkReal32) );
	AkReal32 * arLPFDistance = (AkReal32*)AkAlloca( uNumRays * sizeof(AkReal32) );

	Gen3DParams * 		pGen3DParams = in_pContext->Get3DSound()->GetParams();
	CAkAttenuation *	pAttenuation = pGen3DParams->GetAttenuation();
	
	// get a set of volumes relative to the location of the sound
	CAkAttenuation::AkAttenuationCurve* pSpreadCurve = NULL;
	CAkAttenuation::AkAttenuationCurve* pLPFCurve = NULL;

	if( pAttenuation )
	{
		pSpreadCurve  = pAttenuation->GetCurve( AttenuationCurveID_Spread );
		pLPFCurve	  = pAttenuation->GetCurve( AttenuationCurveID_LowPassFilter );
	}

	AkUInt32 uRay = 0;
	AkPositionSourceType ePosType = in_pContext->GetPositionSourceType();

#ifdef AK_MOTION
	AkFeedbackParams * pFeedbackParams = in_pContext->GetFeedbackParameters();
	if( pFeedbackParams )
	{
		pFeedbackParams->ZeroNewVolumes();
		pFeedbackParams->ZeroNewAttenuations();
	}
#endif // AK_MOTION

	// 3D Game Defined
	if( ePosType == AkGameDef )
	{
		AkVolumeDataArray::Iterator it = in_arVolumeData.Begin();
		do
		{
			AkReal32 fSpread = 0.0f;
			arLPFDistance[uRay] = AK_MIN_LOPASS_VALUE;

			AkUInt32 uListener = (*it).ListenerIdx();
			const AkListenerData & listenerData = GetListenerData( uListener );
			
			// Occlusion
			arOccCtrlValue[uRay] = in_pContext->GetOcclusionValue( uListener );

			// Obstruction
			arObsCtrlValue[uRay] = in_pContext->GetObstructionValue( uListener );

			AkSIMDSpeakerVolumes * volumes = GET_VOLUME_MATRIX_ARRAY_ITEM( arVolumesMatrix, uRay );
			AkDevice * pDevice = CAkOutputMgr::GetDevice( listenerData.uDeviceID );
			AkDeviceInfo * pPerDeviceVolumes = in_deviceVolumes.GetVolumesByID( listenerData.uDeviceID );
			if ( uNumFullBandChannels && pDevice && pPerDeviceVolumes )	//The device might have been disconnected.
			{		
				if ( listenerData.bSpatialized )
				{
					if( pGen3DParams->m_bIsSpatialized )
					{
						if ( pSpreadCurve )
						{
							/// WG-21599 Project distance on the plane first.
							fSpread = pSpreadCurve->Convert( (*it).ProjectedDistanceOnPlane() );
						}

						CAkSpeakerPan::GetSpeakerVolumesPlane( 
							(*it).Azimuth(),
							fDivergenceCenter,
							fSpread,
							volumes,
							uNumFullBandChannels,
							pPerDeviceVolumes->GetOutputConfig(),
							pDevice);
					}
					else //3D sound is not spatialized, use 2D panner
					{
						CAkSpeakerPan::GetSpeakerVolumes2DPan( 0.5f, 0.5f, 
							fDivergenceCenter, 
							true,
							in_uInputConfig,
							pPerDeviceVolumes->GetOutputConfig(),
							volumes );
					}
				}
				else
				{
					// volumes should correspond 1:1 to channels of output bus. Clear other channels.
					volumes[0].Zero();
					AkUInt32 uOutputConfig = pPerDeviceVolumes->GetOutputConfig();
#ifdef AK_LFECENTER
					if ( uOutputConfig & AK_SPEAKER_FRONT_LEFT )
						volumes[0].volumes.fFrontLeft = 1.f;
					if ( uOutputConfig & AK_SPEAKER_FRONT_RIGHT )
						volumes[0].volumes.fFrontRight = 1.f;
					if ( uOutputConfig & AK_SPEAKER_FRONT_CENTER )
						volumes[0].volumes.fCenter = 1.f;
#else
					if ( uOutputConfig & AK_SPEAKER_FRONT_CENTER )
					{
						// Mono.
						volumes[0].volumes.fFrontLeft = ONE_OVER_SQRT_OF_TWO;
						volumes[0].volumes.fFrontRight = ONE_OVER_SQRT_OF_TWO;
					}
					else
					{
						// Stereo.
						volumes[0].volumes.fFrontLeft = 1.f;
						volumes[0].volumes.fFrontRight = 1.f;
					}
#endif
#ifdef AK_REARCHANNELS
					if ( uOutputConfig & AK_SPEAKER_BACK_LEFT )
						volumes[0].volumes.fRearLeft = 1.f;
					if ( uOutputConfig & AK_SPEAKER_BACK_RIGHT )
						volumes[0].volumes.fRearRight = 1.f;
#endif
#ifdef AK_71AUDIO
					if ( uOutputConfig & AK_SPEAKER_SIDE_LEFT )
						volumes[0].volumes.fSideLeft = 1.f;
					if ( uOutputConfig & AK_SPEAKER_SIDE_RIGHT )
						volumes[0].volumes.fSideRight = 1.f;
#endif
					// WG-21804: Custom listener spatialization is not constant power across different input configs.
					AkReal32 fChannelCompensation = AkInvSqrtEstimate( (AkReal32)uNumFullBandChannels );
					volumes[0].Mul( fChannelCompensation );
					for(AkUInt32 iChannel=1; iChannel<uNumFullBandChannels; iChannel++)
						volumes[iChannel] = volumes[iChannel-1]; //results in mono sound
				}
			}
			
			if ( pLPFCurve )
				arLPFDistance[uRay] = pLPFCurve->Convert( (*it).Distance() );

#ifdef AK_MOTION
			CAkFeedbackDeviceMgr* pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
			if (pFeedbackParams != NULL && (m_uFeedbackMask & (1 << uListener)) != 0)
			{
				// If we are using these volumes for feedback, keep the attenuation and position volume separate.
				// We will normalize the position volumes and recombine the attenuation later (see AkFeedbackMgr::GetPlayerVolumes())
				// Each player has a listener assigned and they will have different positions therefore different volumes.							
				AkUInt8 uPlayers = pFeedbackMgr->ListenerToPlayer((AkUInt8)uListener);
				if ( uPlayers != 0 )
				{
					for (AkUInt8 iBit = 1, iPlayer = 0; iPlayer < AK_MAX_DEVICES; iPlayer++, iBit <<= 1)
					{
						if (iBit & uPlayers)
						{
							// Actual behavior for feedback with multi-positions is inconsistent.
							// The actual behavior is always to take the max.
							// while it should somehow be additive if bIsAdditiveMode == true;
							/// BEHAVIOR CHANGE: now includes main output bus gain
							AkReal32 fAttenuationVolume = (*it).fDryMixGain;
							pFeedbackParams->m_fNextAttenuation[iPlayer] = AkMath::Max( fAttenuationVolume, pFeedbackParams->m_fNextAttenuation[iPlayer] );								
						}
					}
				}

				// Not for audio. Clear volumes.
				if ( (m_uAudioMask & (1 << uListener)) == 0 || in_pContext->IsForFeedbackPipeline() )
				{
					for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
					{
						GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, iChannel ).Zero();
					}
				}
			}
#endif // AK_MOTION

			// Handle LFE separately.
#ifdef AK_LFECENTER
			if ( bSourceHasLFE )
			{
				GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, uChanLFE ).Zero();
				GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, uChanLFE ).volumes.fLfe = 1.f;
			}
#endif
			// Offset by custom speaker gain.
			for(AkUInt32 iChannel=0; iChannel<uNumChannels; iChannel++)
				volumes[iChannel].Mul( listenerData.customSpeakerGain );

			++uRay;
			++it;
		}
		while ( it != in_arVolumeData.End() );
	}
	else // 3D User Defined
	{
		AkVolumeDataArray::Iterator it = in_arVolumeData.Begin();
		do
		{
			arLPFDistance[uRay] = AK_MIN_LOPASS_VALUE;

			AkUInt32 uListener = (*it).ListenerIdx();
			const AkListenerData & listenerData = GetListenerData( uListener );
			AkDeviceInfo * pPerDeviceVolumes = in_deviceVolumes.GetVolumesByID( listenerData.uDeviceID );
			AkDevice * pDevice = CAkOutputMgr::GetDevice( listenerData.uDeviceID );
			AkSIMDSpeakerVolumes * volumes = GET_VOLUME_MATRIX_ARRAY_ITEM( arVolumesMatrix, uRay );

			if (pDevice && pPerDeviceVolumes)	//If the device is not connected, don't collapse the rays.
			{
				if ( pGen3DParams->m_bIsSpatialized )
				{
					if ( pLPFCurve )
						arLPFDistance[uRay] = pLPFCurve->Convert( (*it).Distance() );

					if ( uNumFullBandChannels )	
					{
						AkReal32 fSpread = 0.0f;
						if ( pSpreadCurve )
						{
							/// WG-21599 Project distance on the plane first.
							fSpread = pSpreadCurve->Convert( (*it).ProjectedDistanceOnPlane() );
						}


						CAkSpeakerPan::GetSpeakerVolumesPlane( 
							(*it).Azimuth(),
							fDivergenceCenter,
							fSpread,
							volumes,
							uNumFullBandChannels,
							pPerDeviceVolumes->GetOutputConfig(),
 							pDevice );
					}
				}
				else
				{
					CAkSpeakerPan::GetSpeakerVolumes2DPan( 0.5f, 0.5f, //default position is center front 
						fDivergenceCenter, 
						true,
						in_uInputConfig,
						pPerDeviceVolumes->GetOutputConfig(),
						volumes );
				}

				// Handle LFE separately.
	#ifdef AK_LFECENTER
				if ( bSourceHasLFE )
				{
					GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, uChanLFE ).Zero();
					GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, uChanLFE ).volumes.fLfe = 1.f;
				}
	#endif
			}
			/// REVIEW Offset by custom speaker gain not supported with 3D User-defined. 

#ifdef AK_MOTION
			CAkFeedbackDeviceMgr* pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
			if (pFeedbackParams != NULL && (m_uFeedbackMask & (1 << uListener)) != 0)
			{
				// If we are using these volumes for feedback, keep the attenuation and position volume separate.
				// We will normalize the position volumes and recombine the attenuation later (see AkFeedbackMgr::GetPlayerVolumes())
				// Each player has a listener assigned and they will have different positions therefore different volumes.							
				AkUInt8 uPlayers = pFeedbackMgr->ListenerToPlayer((AkUInt8)uListener);
				if ( uPlayers != 0 )
				{
					for (AkUInt8 iBit = 1, iPlayer = 0; iPlayer < AK_MAX_DEVICES; iPlayer++, iBit <<= 1)
					{
						if (iBit & uPlayers)
						{
							// Actual behavior for feedback with multi-positions is inconsistent.
							// The actual behavior is always to take the max.
							// while it should somehow be additive if bIsAdditiveMode == true;
							/// BEHAVIOR CHANGE: now includes main output bus gain
							AkReal32 fAttenuationVolume = (*it).fDryMixGain;
							pFeedbackParams->m_fNextAttenuation[iPlayer] = AkMath::Max( fAttenuationVolume, pFeedbackParams->m_fNextAttenuation[iPlayer] );								
						}
					}
				}

				// Not for audio. Clear volumes.
				if ( (m_uAudioMask & (1 << uListener)) == 0 || in_pContext->IsForFeedbackPipeline() )
				{
					for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
					{						
						GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uRay, iChannel ).Zero();
					}
				}
			}
#endif // AK_MOTION

			/// No obs/occ on 3D user-defined.
			arOccCtrlValue[uRay] = 0;
			arObsCtrlValue[uRay] = 0;

			++uRay;
			++it;
		}
		while ( it != in_arVolumeData.End() );
	}


	//////////////////////////////////////////////////////////////////////////////////////////////
	// What is the difference between these values? none has the same signification, even if they look the same:
	//
	// 1- pAttenuation->m_ConeParams.LoPass;				// Core unmodified value from attenuation.
	// 2- io_params.Cone.LoPass;							// LowPass Acquired, up to now, from others sections, not including cone effect.
	// 3- pGen3DParams->GetParams()->m_ConeParams.LoPass;	// LoPass including RTPC.
	//
	//////////////////////////////////////////////////////////////////////////////////////////////
	AkReal32 fMaxConeLPF = ( pAttenuation && pAttenuation->m_bIsConeEnabled ) ? pGen3DParams->m_fConeLoPass : 0;

	// Collapse rays into an array of devices. Corresponding volume and LPF info will be needed to mix the voice
	// into each device bus tree.
	{
		if ( in_arVolumeData.Length() == 1 )
		{
			const AkRayVolumeData & volumeData = in_arVolumeData[0];
			AkDeviceInfo * pVolumeMx = in_deviceVolumes.GetVolumesByID(CAkListener::GetListenerData(volumeData.ListenerIdx()).uDeviceID);
			if ( pVolumeMx )
			{
				for(AkUInt32 iChannel=0; iChannel<uNumChannels; iChannel++)
				{
					GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, 0, iChannel ).CopyTo( pVolumeMx->mxDirect[iChannel].Next );
				}

				pVolumeMx->mxAttenuations.dry.fNext	= volumeData.fDryMixGain * in_fBehavioralVolume;
				/// TODO Get rid of this if(). Should be checked just once, during pipeline processing.
				if ( in_bIsAuxRoutable )
				{
					pVolumeMx->mxAttenuations.gameDef.fNext = volumeData.fGameDefAuxMixGain * in_fBehavioralVolume;
					pVolumeMx->mxAttenuations.userDef.fNext = volumeData.fUserDefAuxMixGain * in_fBehavioralVolume;
				}
				else
				{
					pVolumeMx->mxAttenuations.gameDef.fNext = 0;
					pVolumeMx->mxAttenuations.userDef.fNext = 0;
				}

				// LPF
				
				// Get max between base and distance-dependent LPF.
				AkReal32 fLPF = AkMath::Max( in_pContext->GetEffectiveParams().LPF, arLPFDistance[0] );

				// Evaluate and get the max between the cone LPF and value above.
				if( fMaxConeLPF > 0 )
				{
					AkReal32 fConeLPF = AkMath::Interpolate( 0.0f, (AkReal32)AK_MIN_LOPASS_VALUE, 1.0f, fMaxConeLPF, volumeData.fConeInterp );
					fLPF = AkMath::Max( fLPF, fConeLPF );
				}

				// Evaluate occlusion and obstruction curves if applicable.
				if ( ePosType == AkGameDef )
				{
					// Evaluate occlusion and obstruction curves if applicable.
					if ( g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveLPF ) )
					{
						AkReal32 fOccLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveLPF, arOccCtrlValue[0] * 100 );
						fLPF = AkMath::Max( fLPF, fOccLPF );
					}

					if ( g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveLPF ) )
					{
						// We now have our collapsed obstruction LPF value.
						pVolumeMx->fObsLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveLPF, arObsCtrlValue[0] * 100 );
					}
				}

				// We now have our collapsed global LPF value.
				pVolumeMx->fLPF = fLPF;
			}
		}
		else
		{
			// Multiple rays: clear volumes of all currently allocated devices in order to compute max contribution of all rays.
			in_deviceVolumes.ClearNext( uNumChannels );

			// Allocate arrays for per-device LPF values. They are indexed by device ID, hence some slots might not be used.
			AkReal32 arMinLPFDistance[AK_NUM_LISTENERS];
			AkReal32 arMinOccCtrlValue[AK_NUM_LISTENERS];
			AkReal32 arMinObsCtrlValue[AK_NUM_LISTENERS];
			AkReal32 arMinConeInterp[AK_NUM_LISTENERS];
			for ( AkUInt32 uDevice = 0; uDevice < AK_NUM_LISTENERS; uDevice++ )
			{
				arMinLPFDistance[uDevice] = AK_MAX_LOPASS_VALUE;
				arMinOccCtrlValue[uDevice] = 1.f;
				arMinObsCtrlValue[uDevice] = 1.f;
				arMinConeInterp[uDevice] = 1.f;
			}

			AkReal32 fMaxDryFactor		= 0.f;
			AkReal32 fMaxUserSendFactor = 0.f;
			AkReal32 fMaxGameSendFactor = 0.f;
			
			if( in_pContext->IsMultiPositionTypeMultiSources() )
			{
				AkVolumeDataArray::Iterator it = in_arVolumeData.Begin();
				AkUInt32 uVolume = 0;

				do //Loop on listener
				{
					AkUInt8 uCurListener = (*it).ListenerIdx();
					AkOutputDeviceID idDevice = GetListenerData( uCurListener ).uDeviceID;
					AkDeviceInfo * pVolumeMx = NULL;
					for(AkDeviceInfoList::Iterator itVolumes = in_deviceVolumes.Begin(); itVolumes != in_deviceVolumes.End() && pVolumeMx == NULL; ++itVolumes)
					{
						if ((*itVolumes)->uDeviceID == idDevice)
							pVolumeMx = *itVolumes;
					}

					if (pVolumeMx)	//Device may have been disconnected.
					{

						AkReal32 fMinLPFDistance = AK_MAX_LOPASS_VALUE;
						AkReal32 fMinOccCrtlValue = 1.f;
						AkReal32 fMinObsCrtlValue = 1.f;
						AkReal32 fMinConeInterp = 1.f;

						AkSIMDSpeakerVolumes temp[AK_VOICE_MAX_NUM_CHANNELS];
						memset( temp, 0, sizeof( AkSIMDSpeakerVolumes ) * AK_VOICE_MAX_NUM_CHANNELS );
						do //Loop on rays, until the next listener
						{
							for ( AkUInt32 iChannel=0; iChannel<uNumChannels; iChannel++ )
							{
								AkSIMDSpeakerVolumes tempDry = GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uVolume, iChannel );

								AkReal32 fDryFactor = (*it).fDryMixGain * in_fBehavioralVolume;
								tempDry.Mul( fDryFactor );
								temp[iChannel].Add( tempDry );
							}

							fMaxDryFactor = AkMax( (*it).fDryMixGain, fMaxDryFactor );
							if ( in_bIsAuxRoutable )
							{
								fMaxUserSendFactor = AkMax( (*it).fUserDefAuxMixGain, fMaxUserSendFactor );
								fMaxGameSendFactor = AkMax( (*it).fGameDefAuxMixGain, fMaxGameSendFactor );
							}

							fMinLPFDistance = AkMath::Min( fMinLPFDistance, arLPFDistance[uVolume] );
							fMinOccCrtlValue = AkMath::Min( fMinOccCrtlValue, arOccCtrlValue[uVolume] );
							fMinObsCrtlValue = AkMath::Min( fMinObsCrtlValue, arObsCtrlValue[uVolume] );
							fMinConeInterp = AkMath::Min( fMinConeInterp, (*it).fConeInterp );

							++uVolume;
							++it;
						}
						while ( it != in_arVolumeData.End() && (*it).ListenerIdx() == uCurListener );

						//Add the (possible) contribution of this listener on the device
						for(AkUInt32 iChannel=0; iChannel < uNumChannels; iChannel++)
							pVolumeMx->mxDirect[iChannel].Next.Max( temp[iChannel] );				

						// Store listener-based global LPF and occ/obs control values
						// "Min" is mandatory because data for this device can be set from another listener.
						arMinLPFDistance[uCurListener] = AkMath::Min( arMinLPFDistance[uCurListener], fMinLPFDistance );
						arMinOccCtrlValue[uCurListener] = AkMath::Min( arMinOccCtrlValue[uCurListener], fMinOccCrtlValue );
						arMinObsCtrlValue[uCurListener] = AkMath::Min( arMinObsCtrlValue[uCurListener], fMinObsCrtlValue );
						arMinConeInterp[uCurListener] = AkMath::Min( arMinConeInterp[uCurListener], fMinConeInterp );
					}
					else
					{
						do // Loop on rays until listener change.
						{
							++uVolume;
							++it;
						}while ( it != in_arVolumeData.End() && (*it).ListenerIdx() == uCurListener );
					}
				}
				while ( it != in_arVolumeData.End() );
			}
			else
			{
				AkUInt32 uVolume = 0;
				AkVolumeDataArray::Iterator it = in_arVolumeData.Begin();
				do //Loop on listener
				{
					AkUInt8 uCurListener = (*it).ListenerIdx();
					AkOutputDeviceID idDevice = GetListenerData( uCurListener ).uDeviceID;
					AkDeviceInfo * pVolumeMx = NULL;
					for(AkDeviceInfoList::Iterator itVolumes = in_deviceVolumes.Begin(); itVolumes != in_deviceVolumes.End() && pVolumeMx == NULL; ++itVolumes)
					{
						if ((*itVolumes)->uDeviceID == idDevice)
							pVolumeMx = *itVolumes;
					}

					if (pVolumeMx)	//Device may have been disconnected.
					{
						do // Loop on rays until listener change.
						{
							for(AkUInt32 iChannel=0; iChannel<uNumChannels; iChannel++)
							{
								AkSIMDSpeakerVolumes tempDry = GET_VOLUME_MATRIX_ITEM( arVolumesMatrix, uVolume, iChannel );
								tempDry.Mul( (*it).fDryMixGain * in_fBehavioralVolume );
								pVolumeMx->mxDirect[iChannel].Next.Max( tempDry );
							}

							fMaxDryFactor = AkMax( (*it).fDryMixGain, fMaxDryFactor );
							if ( in_bIsAuxRoutable )
							{
								fMaxUserSendFactor = AkMax( (*it).fUserDefAuxMixGain, fMaxUserSendFactor );
								fMaxGameSendFactor = AkMax( (*it).fGameDefAuxMixGain, fMaxGameSendFactor );
							}

							// Get listener-based global LPF and occ/obs control values for this device. 
							// "Min" is mandatory because data for this device can be set from another listener.
							arMinLPFDistance[uCurListener] = AkMath::Min( arMinLPFDistance[uCurListener], arLPFDistance[uVolume] );
							arMinOccCtrlValue[uCurListener] = AkMath::Min( arMinOccCtrlValue[uCurListener], arOccCtrlValue[uVolume] );
							arMinObsCtrlValue[uCurListener] = AkMath::Min( arMinObsCtrlValue[uCurListener], arObsCtrlValue[uVolume] );
							arMinConeInterp[uCurListener] = AkMath::Min( arMinConeInterp[uCurListener], (*it).fConeInterp );

							++uVolume;
							++it;
						}while ( it != in_arVolumeData.End() && (*it).ListenerIdx() == uCurListener );
					}
					else
					{
						do // Loop on rays until listener change.
						{
							++uVolume;
							++it;
						}while ( it != in_arVolumeData.End() && (*it).ListenerIdx() == uCurListener );
					}
				}
				while ( it != in_arVolumeData.End() );
			}

			// Compute and store listener-based global LPF and occ/obs control values for this device.  
			for(AkDeviceInfoList::Iterator itDevice = in_deviceVolumes.Begin(); itDevice != in_deviceVolumes.End(); ++itDevice)
			{
				AkDeviceInfo & volumesDevice = **itDevice;
				AkUInt32 uDeviceListeners = CAkOutputMgr::GetDeviceListeners(volumesDevice.uDeviceID);

				volumesDevice.mxAttenuations.dry.fNext = 1.f; // now dry is baked inside the matrix.
				if ( in_bIsAuxRoutable && fMaxDryFactor > 0.f )
				{
					volumesDevice.mxAttenuations.gameDef.fNext = fMaxGameSendFactor/fMaxDryFactor;
					volumesDevice.mxAttenuations.userDef.fNext = fMaxUserSendFactor/fMaxDryFactor;
				}
				else
				{
					// known issue here, in multipositionning, if there is no Dry, there is no wet....
					volumesDevice.mxAttenuations.gameDef.fNext = 0.f;
					volumesDevice.mxAttenuations.userDef.fNext = 0.f;
				}

				AkUInt32 uListenerMask = uDeviceListeners & in_pContext->GetGameObjectPtr()->GetListenerMask();
				AkUInt32 uListener = 0;
				AkReal32 fFinalLPF = AK_MAX_LOPASS_VALUE;
				while(uListenerMask)
				{
					if (uListenerMask & 1)
					{						
						AkReal32 fLPF = arMinLPFDistance[uListener];

						// Evaluate and get the max between the cone LPF and value above.
						if ( fMaxConeLPF > 0 )
						{
							AkReal32 fConeLPF = AkMath::Interpolate( 0.0f, (AkReal32)AK_MIN_LOPASS_VALUE, 1.0f, fMaxConeLPF, arMinConeInterp[uListener] );
							fLPF = AkMath::Max( fLPF, fConeLPF );
						}

						if ( ePosType == AkGameDef )
						{
							// Evaluate occlusion and obstruction curves if applicable.
							if ( g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveLPF ) )
							{
								AkReal32 fOccLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveLPF, arMinOccCtrlValue[uListener] * 100 );
								fLPF = AkMath::Max( fLPF, fOccLPF );
							}

							if ( g_pEnvironmentMgr->IsCurveEnabled( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveLPF ) )
							{
								// We now have our collapsed obstruction LPF value.
								volumesDevice.fObsLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveLPF, arMinObsCtrlValue[uListener] * 100 );
							}
						}

						fFinalLPF = AkMath::Min(fFinalLPF, fLPF);
					}
					uListener++;
					uListenerMask >>= 1;
				}

				// Get max between base and distance-dependent LPF.
				// We now have our collapsed global LPF value.
				volumesDevice.fLPF = AkMax(fFinalLPF, in_pContext->GetEffectiveParams().LPF);
			}
		}
	}
}

void CAkListener::DoSpeakerVolumeMatrixCallback(
	AkPlayingID in_playingID,
	AkUInt32 in_uNumChannels,
	AkUInt32 in_uInputConfig,
	AkUInt32 in_uOutputConfig,
    AkDeviceInfo & io_rDeviceVolume
	)
{
	AkSpeakerVolumeMatrixCallbackInfo info;

	AkUInt32 iDryChannel = 0;

	for( ; iDryChannel < in_uNumChannels; iDryChannel++ )
		info.pVolumes[ iDryChannel ] = &( io_rDeviceVolume.mxDirect[ iDryChannel ].Next.volumes );

	for( ; iDryChannel < AK_VOICE_MAX_NUM_CHANNELS; iDryChannel++ )
		info.pVolumes[ iDryChannel ] = NULL;

	info.io_pfDryAttenuation =					&io_rDeviceVolume.mxAttenuations.dry.fNext;				
	info.io_pfGameDefAuxSendAttenuation =		&io_rDeviceVolume.mxAttenuations.gameDef.fNext;	
	info.io_pfUserDefAuxSendDryAttenuation =	&io_rDeviceVolume.mxAttenuations.userDef.fNext;

	info.uInputConfig = in_uInputConfig;
	info.uOutputConfig = in_uOutputConfig;

	g_pPlayingMgr->NotifySpeakerVolumeMatrix( in_playingID, &info );
}

void CAkListener::GetListenerVolume( 
	CAkAttenuation* in_pAttenuation,
	const AkEmitterListenerPair & in_ray,
	AkReal32 in_fConeOutsideVolume,	
	AkReal32& out_fMinCone,
	AkReal32& out_fListenerVolumeDry,	// Linear
	AkReal32& out_fListenerVolumeGameDefAux,	// Linear
	AkReal32& out_fListenerVolumeUserDefAux	// Linear
	)
{
	if( in_pAttenuation )
	{
		AkReal32 fDistance = in_ray.Distance();

		CAkAttenuation::AkAttenuationCurve* pAttenuationCurveDry = in_pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );

		AkReal32 fListenerVolumeDry;
		if( pAttenuationCurveDry )
			fListenerVolumeDry = pAttenuationCurveDry->Convert( fDistance );
		else
			fListenerVolumeDry = 1.0f;
		out_fListenerVolumeDry = fListenerVolumeDry;

		CAkAttenuation::AkAttenuationCurve* pAttenuationCurveAuxGameDef = in_pAttenuation->GetCurve( AttenuationCurveID_VolumeAuxGameDef );

		if( pAttenuationCurveAuxGameDef == pAttenuationCurveDry ) //optimize when Wet is UseCurveDry
			out_fListenerVolumeGameDefAux = fListenerVolumeDry;
		else if( pAttenuationCurveAuxGameDef )
			out_fListenerVolumeGameDefAux = pAttenuationCurveAuxGameDef->Convert( fDistance );
		else
			out_fListenerVolumeGameDefAux = 1.0f;

		CAkAttenuation::AkAttenuationCurve* pAttenuationCurveAuxUserDef = in_pAttenuation->GetCurve( AttenuationCurveID_VolumeAuxUserDef );

		if( pAttenuationCurveAuxUserDef == pAttenuationCurveDry ) //optimize when Wet is UseCurveDry
			out_fListenerVolumeUserDefAux = fListenerVolumeDry;
		else if( pAttenuationCurveAuxUserDef )
			out_fListenerVolumeUserDefAux = pAttenuationCurveAuxUserDef->Convert( fDistance );
		else
			out_fListenerVolumeUserDefAux = 1.0f;

		if ( in_pAttenuation->m_bIsConeEnabled )
			out_fListenerVolumeDry *= ComputeConeAttenuation( in_pAttenuation->m_ConeParams.fInsideAngle, in_pAttenuation->m_ConeParams.fOutsideAngle, in_fConeOutsideVolume, in_ray.EmitterAngle(), out_fMinCone );
		else
			out_fMinCone = 1.0f; // assume no cone attenuation at listener pos
	}
	else
	{
		out_fMinCone = 1.0f; // assume no cone attenuation at listener pos
		out_fListenerVolumeDry = 1.0f;
		out_fListenerVolumeGameDefAux = 1.0f;
		out_fListenerVolumeUserDefAux = 1.0f;
	}
}

