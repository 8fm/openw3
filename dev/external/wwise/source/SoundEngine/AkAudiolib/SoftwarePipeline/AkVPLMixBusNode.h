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
// AkVPLMixBusNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_MIX_BUS_NODE_H_
#define _AK_VPL_MIX_BUS_NODE_H_

#include "AkLEngineDefs.h"
#include "AkLEngineStructs.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkFXContext.h"

#include "AkMixer.h"
#include "AkVPLNode.h"
#include "AkBusCtx.h"
#include "AkPBI.h"
#include "AkSpeakerPan.h"
#include "AkMeterTools.h"

#ifdef AK_VITA_HW
#include "AkVitaProcessingRack.h"
#endif

class CAkBusVolumes
{
public:
	CAkBusVolumes()
		:m_uChannelMask(0)
		,m_busID(AK_INVALID_UNIQUE_ID)
		,m_pParent( NULL ) 
#ifndef AK_OPTIMIZED
		,m_pMeterCtx( NULL )
#endif
		,m_bIsPositioningEnabled(false)
		,m_bCallbackEnabled(false)
	{
		m_BusPosParams.Init();
		m_PrevPosParams.Init();
		m_PrevPosParams.Invalidate();//make sure the panning gets updated the first time. ( m_BusPosParams must != m_PrevPosParams )

		unsigned int iChannel=0;
		do {
			m_PanningVolumes[iChannel].Previous.Zero();
			m_PanningVolumes[iChannel].Next.Zero();
		}while ( ++iChannel < AK_VOICE_MAX_NUM_CHANNELS );
	}

	void InitPan( CAkParameterNodeBase* in_pBus, AkChannelMask in_ChanMsk, AkChannelMask in_uParentMask );
	void Update2DParams( CAkParameterNodeBase* in_pBus );

	void SetNextVolume( AkReal32 in_dBVolume )
	{
		m_fNextVolumedB = in_dBVolume;
		m_fNextVolume = AkMath::dBToLin( in_dBVolume );
	}

	void SetVolumeOffset( AkReal32 in_VolumeOffsetdB )
	{
		m_fNextVolumedB += in_VolumeOffsetdB;
		m_fNextVolume = AkMath::dBToLin( m_fNextVolumedB );
	}

	inline AkReal32 GetNextVolume(){ return m_fNextVolume; }
	inline AkReal32 GetNextVolumeDB(){ return m_fNextVolumedB; }
	inline AkReal32 GetPreviousVolume(){ return m_fPreviousVolume; }
	inline AkReal32 GetPreviousVolumedB(){ return m_fPreviousVolumedB; }
	inline bool VolumeChanged(){ return m_fNextVolume != m_fPreviousVolume; }

	void PositioningChangeNotification(
		AkReal32			in_RTPCValue,
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		);

protected:
	void TagPreviousVolumes()
	{
		m_fPreviousVolume = m_fNextVolume;
		m_fPreviousVolumedB = m_fNextVolumedB;
	}

	void ResetVolumes()
	{
		m_fPreviousVolume			= 1.0f;
		m_fNextVolume				= 1.0f;
		m_fNextVolumedB				= 0.0f;
		m_fPreviousVolumedB			= 0.0f;
	}

private:
	void TagPreviousPan();

protected:
	void UpdatePanningVolumes();

public:
	void EnableVolumeCallbackCheck( bool in_bEnabled ){ m_bCallbackEnabled = in_bEnabled; }
	void UpdateFinalVolumes();

	AkAudioMix				m_PanningVolumes[AK_VOICE_MAX_NUM_CHANNELS];
	AkAudioMix				m_FinalVolumes[AK_VOICE_MAX_NUM_CHANNELS];
	AkChannelMask			m_uChannelMask;
	AkChannelMask			m_uParentMask;
	AkUniqueID				m_busID;

	AkAudioBufferBus& GetBusBufferRef(){ return m_BufferOut; }
	AkVPL * m_pParent;

protected:
	CAkMixer				m_Mixer;				// Mixer.
	AkAudioBufferBus		m_BufferOut;			// mix output buffer.	
	AkUInt32				m_ulBufferOutSize;		// mix output buffer size.
#ifndef AK_OPTIMIZED
	AkMeterCtx *			m_pMeterCtx;
	AkUInt32				m_uMixingVoiceCount;
#endif

private:

	AkReal32				m_fNextVolume;			// Next bus volume.
	AkReal32				m_fPreviousVolume;		// Previous bus volume.

	AkReal32				m_fNextVolumedB;
	// We keep the previous value in dB for profiling purposes only, 
	// otherwise we get annoying lintodb precision error on profiling.(WG-21205)
	AkReal32				m_fPreviousVolumedB;	// Previous bus volume.

	BaseGenParams			m_BusPosParams;
	BaseGenParams			m_PrevPosParams;

	bool					m_bIsPositioningEnabled :1;
protected:
	bool					m_bCallbackEnabled		:1;
};

class CAkBusFX : public CAkBusVolumes
{
public:

	CAkBusFX();
	~CAkBusFX();
	
	void FindFXByContext( 
		CAkBusFXContext * in_pBusFXContext, 
		AkPluginID & out_pluginID, 
		AkUInt32 & out_uFXIndex 
		)
	{
		for ( AkUInt32 uFX = 0; uFX<AK_NUM_EFFECTS_PER_OBJ; uFX++ )
		{
			if ( m_aFX[ uFX ].pBusFXContext == in_pBusFXContext )
			{
				out_pluginID = m_aFX[ uFX ].id;
				out_uFXIndex = uFX;
				return;
			}
		}
		AKASSERT( !"Invalid BusFXContext" );
	}

	bool IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			if( m_aFX[ uFXIndex ].pBusFXContext && m_aFX[ uFXIndex ].pBusFXContext->IsUsingThisSlot( in_pUsageSlot, m_aFX[ uFXIndex ].pEffect ) )
				return true;
		}

		return false;
	}

	bool IsUsingThisSlot( const AkUInt8* in_pData )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			if( m_aFX[ uFXIndex ].pBusFXContext && m_aFX[ uFXIndex ].pBusFXContext->IsUsingThisSlot( in_pData ) )
				return true;
		}

		return false;
	}

	AKRESULT SetInsertFx( const CAkBusCtx & in_busCtx, AkUInt32 in_uFXIndex );
	void SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );
	void DropFx( AkUInt32 in_uFXIndex );
	void DropFx();

	AkForceInline AkPluginID GetFXID( AkUInt32 in_uFXIndex ) { return m_aFX[ in_uFXIndex ].id; }
	AkForceInline AK::IAkPluginParam * GetPluginParam( AkUInt32 in_uFXIndex ) { return m_aFX[ in_uFXIndex ].pParam; }
	AkForceInline AkChannelMask GetChannelMask() { return m_BufferOut.GetChannelMask(); }

#ifndef AK_OPTIMIZED
	inline const AkMeterCtx *	GetMeterCtx() { return m_pMeterCtx; }
	inline void				IncrementMixingVoiceCount(){ ++m_uMixingVoiceCount; }
	inline void				ResetMixingVoiceCount(){ m_uMixingVoiceCount = 0; }
	inline AkUInt32			GetMixingVoiceCount(){ return m_uMixingVoiceCount; }
	void					RefreshMeterWatch( AkUniqueID in_busID );
#endif

#if defined(AK_PS3)
	void ClearLastItemMix() { m_pLastItemMix = NULL; }
#endif

#ifdef AK_VITA_HW
	AkVitaProcessingRackList* GetProcessingRack(){ return &m_ProcessingRackList; }
	SceNgsHVoice GetBusInputVoice(){ return m_ProcessingRackList.GetInputVoice(); }

	void UpdateVolume()
	{
		if ( VolumeChanged() || m_bUpdateVolumeOnNextFrame )
		{	
			m_ProcessingRackList.SetOutputVolume(GetNextVolume());
			m_bUpdateVolumeOnNextFrame = false;
		}
		TagPreviousVolumes();
	}

#endif

protected:

	struct FX
	{
		AkPluginID id;					// Effect unique type ID. 
		AK::IAkPluginParam * pParam;	// Parameters.
		AK::IAkInPlaceEffectPlugin * pEffect;	// Pointer to a bus fx filter node.
		CAkBusFXContext * pBusFXContext;// Bus FX context
		AkUInt8 bBypass : 1;			// Bypass state
		AkUInt8 bLastBypass : 1;		// Bypass state on previous buffer
	};

	FX						m_aFX[ AK_NUM_EFFECTS_PER_OBJ ];

	AkUInt8					m_bBypassAllFX : 1;
	AkUInt8					m_bLastBypassAllFX : 1;

#if defined(AK_PS3)
	AkPipelineBufferBase*	m_pLastItemMix;
#endif

#ifdef AK_VITA_HW
	AkVitaProcessingRackList m_ProcessingRackList;
	AkReal32				m_TailDuration;
	AkReal32				m_TimeToLive;
	AkUInt8					m_bUpdateVolumeOnNextFrame : 1;
#endif
};

class CAkVPLMixBusNode : public CAkBusFX
{
public:
	~CAkVPLMixBusNode();

#ifdef AK_VITA_HW
	AKRESULT			Init( AkChannelMask in_uChannelMask, AkChannelMask in_uParentMask, AkUInt16 in_uMaxFrames, CAkBusCtx in_busContext, CAkBusFX* in_parentBus );
#else
	AKRESULT			Init( AkChannelMask in_uChannelMask, AkChannelMask in_uParentMask, AkUInt16 in_uMaxFrames, CAkBusCtx in_busContext );
#endif
	void				Stop() { m_eState = NodeStateStop; }
	void				Connect();
	void				Disconnect();
	void				InitVolumes();	
	AKRESULT			SetAllInsertFx();
	void				UpdateBypassFx();

	//New execution model
	void				ConsumeBuffer( 
							AkAudioBufferBus&		io_rpBuffer,
							bool					in_bPan,
							AkAudioMix				in_PanMix[]
							);

	void				ConsumeBuffer(	  AkVPLState & io_rVPLState
										, AkAudioMix in_arMix[]
#ifdef AK_PS3
										, AkInt8 in_iVPLEnvironmentIndex
										, AkVolumeOffset* in_pAttenuation
#endif
	);

#ifndef AK_OPTIMIZED
	// Just update state to "playing" without mixing anything.
	void				ConsumeBufferMute( AkVPLState & 
#ifdef AK_PS3
		io_state 
#endif
		);	
#endif
	void				ProcessDone( AkVPLState & io_state );
	void				GetResultingBuffer( AkAudioBufferBus*& io_rpBuffer );
	void				ReleaseBuffer();
	void				ProcessAllFX();
	void				ProcessFX( AkUInt32 in_fxIndex, bool & io_bfxProcessed );
	void				PostProcessFx( AkAudioBufferBus*& io_rpBuffer );

	AkUniqueID			GetBusID() const { return m_BusContext.ID(); }
	VPLNodeState		GetState(){ return m_eState; }

	const CAkBusCtx &	GetBusContext() { return m_BusContext; }

	inline AkUInt32		ConnectionCount() const { return m_uConnectCount; }

	bool IsPanning();

	void ResetNextVolume( AkReal32 in_dBVolume );

	inline bool			EffectsCreated() {return m_bEffectCreated;}

#ifdef AK_VITA_HW
	bool IsExpired( AkReal32 in_uElapsedTime );
	void Activate(){ m_bIsActive = true; }
#endif

protected:

	// Helpers.
	void				ResetStateForNextPass();	

protected:
	CAkBusCtx				m_BusContext;
	VPLNodeState			m_eState;

	AkUInt32				m_uConnectCount;		// Number of inputs actually connected
	bool					m_bEffectCreated;
#ifdef AK_VITA_HW
	bool					m_bIsActive;
#endif

};

#endif //_AK_VPL_MIX_BUS_NODE_H_
