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
// AkLEngine_SoftwarePipeline_incl.h
//
// Lower Engine common LEngine Software include code.
//
//////////////////////////////////////////////////////////////////////

public:
	static AKRESULT				SoftwareInit();
	static void					SoftwareTerm();
	static void					SoftwarePerform();

	// Bus management.
	static AkVPL *				CreateVPLMixBus( CAkBusCtx in_BusCtx, AkOutputDeviceID in_uDevice, AkVPL* in_pParentBus
#ifdef AK_MOTION
		, bool in_bForFeedback
#endif // AK_MOTION
		);

	static AkVPL *				GetExistingVPLMixBus( AkUniqueID in_busID, AkOutputDeviceID in_uDevice  );
	static AkVPL *				GetVPLMixBusInternal( CAkBusCtx & in_ctxBus, AkOutputDeviceID in_uDevice 
#ifdef AK_MOTION
		, bool in_bForFeedback
#endif // AK_MOTION
		);

	static void					DestroyAllVPLMixBusses();
	static AKRESULT				EnsureVPLExists(CAkVPLSrcCbxNode * in_pCbx, CAkPBI *in_pCtx);
	static void 				EnsureAuxBusExist( CAkVPLSrcCbxNodeBase * in_pCbx, AkMergedEnvironmentValue* in_pMergedEnvSlot );
	static AkVPL *				GetVPLMixBus( CAkBusCtx & in_ctxBus, AkOutputDeviceID in_uDevice 
#ifdef AK_MOTION
		, bool in_bForFeedback 
#endif
		);

	static AkVPL* GetAndConnectBus( CAkPBI * in_pCtx, CAkVPLSrcCbxNodeBase * in_pCbx, AkOutputDeviceID in_uDevice);
	static AkVPL* GetAuxBus( CAkBus* in_pAuxBus, AkMergedEnvironmentValue* in_pSend, AkOutputDeviceID in_uDevice, CAkVPLSrcCbxNodeBase * in_pCbx );

	#define LENGINE_LIST_POOL_BLOCK_SIZE	(32)
	typedef AkArray<AkVPL*, AkVPL*, ArrayPoolLEngineDefault, LENGINE_LIST_POOL_BLOCK_SIZE / sizeof( AkVPL * ) > AkArrayVPL;

	// Execution.
	static void					RunVPL( struct AkRunningVPL & io_runningVPL );
	static void					AnalyzeMixingGraph();
	static void					FinishRun( CAkVPLSrcCbxNode * in_pCbx, AkVPLState & io_state );
	static void					GetBuffer();
	static void					TransferBuffer( AkVPL* in_pVPL );
	static void					ProcessVPLList( AkArrayVPL& in_VPLList );
	static void					HandleStarvation();
	static void					ResetMixingVoiceCount();

	// VPL management.
	static void					RemoveMixBusses();
	static CAkVPLSrcCbxNode *	FindExistingVPLSrc( CAkPBI * in_pCtx );

	// Cached buffer
	static void * GetCachedAudioBuffer( AkUInt32 in_uSize );
	static void ReleaseCachedAudioBuffer( AkUInt32 in_uSize, void * in_pvBuffer );

	static void StopMixBussesUsingThisSlot( const CAkUsageSlot* in_pSlot );
	static void ResetAllEffectsUsingThisMedia( const AkUInt8* in_pData );	

private: 
	static AkArrayVPL			m_arrayVPLs;				// Array of VPLs.
	static AkListVPLSrcs		m_Sources;					// All sources.

	typedef AkStaticArray<void *, void *, NUM_CACHED_BUFFERS> BufferCache;
	static BufferCache			m_CachedAudioBuffers[NUM_CACHED_BUFFER_SIZES];
