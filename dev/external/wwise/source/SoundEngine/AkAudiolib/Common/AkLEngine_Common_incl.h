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
// AkLEngine_Common_incl.h
//
// Lower Engine common include code.
//
//////////////////////////////////////////////////////////////////////

public:

	// Initialize/terminate.
	static void ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings );
    static AKRESULT Init();
	static void	Stop();
    static void Term();
	static void GetDefaultPlatformInitSettings( 
		AkPlatformInitSettings & out_pPlatformSettings // Platform specific settings. Can be changed depending on hardware.
		);

	static void GetDefaultOutputSettings( AkSinkType in_eSinkType, AkOutputSettings & out_settings );	//To be redefined per platform
	static void GetDefaultOutputSettingsCommon( AkOutputSettings & out_settings );
	

	// Memory Pools
	static AKRESULT CreateLEnginePools();
    static void DestroyLEnginePools( void );

	 // Perform processing.
	static AkUInt32 GetNumBufferNeededAndSubmit();
	static void Perform();
	static void StartVoice();

	// Cross-platform interface for command queue.
	static AKRESULT	AddSound( AkLECmd & io_cmd );
	static AKPBI_CBXCLASS * ResolveCommandVPL( AkLECmd & io_cmd );
	static AKRESULT VPLTryConnectSource( CAkPBI * in_pContext, AKPBI_CBXCLASS * in_pCbx );
	static void	VPLDestroySource( AKPBI_CBXCLASS * in_pCbx, bool in_bNotify = false);
	static void ReevaluateBussesForDevice(AkOutputDeviceID in_uDevice, AkUInt32 in_uListenerMask);
	static void ReevaluateBussesForGameObject(CAkRegisteredObj* in_pObj, AkUInt32 in_uOldMask, AkUInt32 in_uNewMask);

	static void SetPanningRule( AkUInt32 in_iOutputID, AkSinkType in_eDeviceType, AkPanningRule in_panningRule );
	static void ReplaceMainSinkWithDummy();

typedef void( *AkForAllPluginParamFunc )(
	AK::IAkPluginParam * in_pParam,
	CAkRegisteredObj * in_pGameObj,
	void * in_pCookie 
	);

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	static void ForAllPluginParam( class CAkFxBase * in_pFx, AkForAllPluginParamFunc in_funcForAll, void * in_pCookie );
#else
	static void ForAllPluginParam( class CAkFxBase *, AkForAllPluginParamFunc, void * ) {}
#endif

	//Feedback related function
#ifdef AK_MOTION
	static void EnableFeedbackPipeline();
	static bool IsFeedbackEnabled();
#else
	static inline void EnableFeedbackPipeline(){}
	static inline bool IsFeedbackEnabled() { return false; }
#endif

// Bus management.
static void ResetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_Volume );
static void SetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_VolumeOffset );
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	static void ResetMasterBusVolume( bool in_bMain, AkVolumeValue in_Volume );
	static void SetMasterBusVolume( bool in_bMain, AkVolumeValue in_VolumeOffset );
	static void EnableVolumeCallback( AkUniqueID in_MixBusID, bool in_bEnable  );
	static void BypassBusFx( AkUniqueID in_MixBusID, AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );
	static void BypassMasterBusFx( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );
	static void PositioningChangeNotification( AkUniqueID in_MixBusID, AkReal32	in_RTPCValue, AkRTPC_ParameterID in_ParameterID );
	static void UpdateMasterBusFX( AkUInt32 in_uFXIndex );
	static void UpdateMixBusFX(  AkUniqueID in_MixBusID, AkUInt32 in_uFXIndex );
#else
	static inline void ResetMasterBusVolume( bool, AkVolumeValue ){}
	static inline void SetMasterBusVolume( bool, AkVolumeValue ){}
	static inline void EnableVolumeCallback( AkUniqueID, bool ){} 
	static inline void BypassBusFx( AkUniqueID, AkUInt32, AkUInt32 ){}
	static inline void BypassMasterBusFx( AkUInt32, AkUInt32 ) {}
	static inline void PositioningChangeNotification( AkUniqueID, AkReal32, AkRTPC_ParameterID ) {}
	static inline void UpdateMasterBusFX( AkUInt32 ) {}
	static inline void UpdateMixBusFX(  AkUniqueID, AkUInt32 ) {}
#endif

#ifndef AK_OPTIMIZED
    // Lower engine profiling.
	static void GetNumPipelines(AkUInt16 &out_rAudio, AkUInt16& out_rFeedback, AkUInt16& out_rDevMap);
	static void GetPipelineData(AkUInt16 in_uMaxPipelineData, AkMonitorData::PipelineData * out_pPipelineData, AkUInt16 in_uMaxPipelineDevMap, AkMonitorData::PipelineDevMap * out_pPipelineDevMap, bool in_bGetFeedback );
	static void PostSendsData();

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
		static void PostMeterWatches();
		static AkUInt32 GetNumBusses();
		static void GetBusMeterData( AkUInt32& in_NumBusses, AkMonitorData::BusMeterData* in_pArrayBusVolumes );
#else
		static void PostMeterWatches(){}
		static inline AkUInt32 GetNumBusses(){return 0;}
		static inline void GetBusMeterData( AkUInt32& in_NumBusses, AkMonitorData::BusMeterData* /*in_pArrayBusVolumes*/ ){ in_NumBusses = 0; }
#endif

#endif

	static bool GetSinkTypeText( AkSinkType in_sinkType, AkUInt32 in_uBufSize, char* out_pszBuf );

private: 

	static AkUniqueID			m_VPLPipelineID;			// Profiling vpl src id.
	static AkEvent				m_EventStop;				// Event to stop the thread.
	static AkUInt32				m_uLastStarvationTime;		// Last time we sent a starvation notification

#ifdef AK_MOTION
	static CAkFeedbackDeviceMgr* m_pDeviceMgr;				// Force feedback device manager
#endif

