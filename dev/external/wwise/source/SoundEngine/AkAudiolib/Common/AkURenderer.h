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
// AkURenderer.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPPER_RENDERER_H_
#define _UPPER_RENDERER_H_

#include "ActivityChunk.h"
#include "AkRTPC.h"
#include "PrivateStructures.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkLock.h>

using namespace AK;

class CAkSoundBase;
class CAkSource;
class CAkLimiter;
struct AkRTPCFXSubscription;
struct AkRTPCEnvSubscription;

//-----------------------------------------------------------------------------
// CAkURenderer class.
//-----------------------------------------------------------------------------
class CAkURenderer
{
public:
    // Initialise the renderer
    //
    // Return - AKRESULT - AK_Success: everything succeed
    //                     AK_Fail: Was not able to initialize the renderer
    static AKRESULT Init();

    // Uninit the renderer
    static void Term();

	// Call a play on the definition directly
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Play(  CAkSoundBase*    in_pSound,          // Node instanciating the play
						   CAkSource*		in_pSource,			// Source
		                   AkPBIParams&     in_rPBIParams );

	static AKRESULT Play(	CAkPBI *		in_pContext, 
                    TransParams&    in_rTparameters,
					AkPlaybackState	in_ePlaybackState
					);

	// Stop All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Stop(	CAkSoundBase*	 in_pSound,
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	// Pause All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Pause(	CAkSoundBase*	 in_pSound, 
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	// Resume All the specified node associated with the specified object
    //
    // Return - AKRESULT - Ak_Success if succeeded
	static AKRESULT Resume(	CAkSoundBase*	  in_pSound,
							CAkRegisteredObj * in_pGameObj,
                            TransParams&    in_rTparameters,
							bool		  in_bIsMasterResume,
							AkPlayingID		in_pPlayingID = AK_INVALID_PLAYING_ID );

	static void StopAllPBIs( const CAkUsageSlot* in_pUsageSlot );
	static void ResetAllEffectsUsingThisMedia( const AkUInt8* in_pOldDataPtr);

#ifdef AK_MOTION
	static void InvalidateAllMotionPBIs();
#endif

    static void EnqueueContext( CAkPBI * in_pContext );

	static AkForceInline void IncrementVirtualCount() { ++m_uNumVirtualizedSounds; }
	static void DecrementVirtualCount( bool in_bAllowKick );

	// Asks to kick out the Oldest sound responding to the given IDs.
	// Theorically called to apply the Max num of instances per Node.
    //
    // Return - AKRESULT - 
	//      AK_SUCCESS				= caller survives.
	//      AK_FAIL					= caller should kick himself.
	//		AK_MustBeVirtualized	- caller should start as a virtual voice. (if it fails, it must kick itself)
	static AKRESULT Kick( CAkLimiter * in_pLimiter, AkUInt16 in_uMaxInstances, AkReal32 in_fPriority, CAkRegisteredObj * in_pGameObj, bool in_bKickNewest, bool in_bUseVirtualBehavior, CAkParameterNodeBase*& out_pKicked, KickFrom in_eReason );
	static AKRESULT Kick( AkReal32 in_fPriority, CAkRegisteredObj * in_pGameObj, bool in_bKickNewest, bool in_bUseVirtualBehavior, CAkParameterNodeBase*& out_pKicked, KickFrom in_eReason );

	// Return - bool - true = caller survives | false = caller should kick himself
	static bool ValidateMemoryLimit( AkReal32 in_fPriority );

	// Return - AKRESULT - 
	//      AK_SUCCESS				= caller survives.
	//      AK_FAIL					= caller should kick himself.
	//		AK_MustBeVirtualized	- caller should start as a virtual voice. (if it fails, it must kick itself)
	static AKRESULT ValidateMaximumNumberVoiceLimit( AkReal32 in_fPriority );

	static AKRESULT ValidateLimits( AkReal32 in_fPriority, AkMonitorData::NotificationReason& out_eReasonOfFailure );

	static bool GetVirtualBehaviorAction( AkBelowThresholdBehavior in_belowThresholdBehavior );

	struct ContextNotif
	{
		CAkPBI* pPBI;
		AkCtxState state;
		AkCtxDestroyReason DestroyReason;
		AkReal32 fEstimatedLength;
	};

	static void	EnqueueContextNotif( CAkPBI* in_pPBI, AkCtxState in_State, AkCtxDestroyReason in_eDestroyReason = CtxDestroyReasonFinished, AkReal32 in_fEstimatedTime = 0.0f );

	typedef CAkList2<ContextNotif, const ContextNotif&, AkAllocAndFree> AkContextNotifQueue;
	typedef AkListBare<CAkPBI,AkListBareNextItem<CAkPBI>,AkCountPolicyWithCount> AkListCtxs;

	static void PerformContextNotif();

	static PriorityInfoCurrent _CalcInitialPriority( CAkSoundBase * in_pSound, CAkRegisteredObj * in_pGameObj, AkReal32& out_fMaxRadius );
	static AkReal32 GetMinDistance( const AkSoundPositionRef& in_rPosRef );

	static AkReal32 GetMaxRadius( AkGameObjectID in_GameObjId );

	static AKRESULT GetMaxRadius( AK::SoundEngine::Query::AkRadiusList & io_RadiusList );

#ifndef AK_OPTIMIZED
	static void RefreshMonitoringMuteSolo();
#endif

	static void AddBusLimiter( CAkLimiter* in_pLimiter );
	static void RemoveBusLimiter( CAkLimiter* in_pLimiter );
	static void AddAMLimiter( CAkLimiter* in_pLimiter );
	static void RemoveAMLimiter( CAkLimiter* in_pLimiter );

	static void ProcessLimiters();
#if defined (_DEBUG)
	static bool CheckLimitersForCtx( CAkPBI* in_pCtx );
#endif

	static AkForceInline CAkLimiter& GetGlobalLimiter(){ return m_GlobalLimiter; }

private:

	static void	DestroyPBI( CAkPBI * in_pPBI );
	static void	DestroyAllPBIs();

	// Source management.
	static AKRESULT	ProcessCommand( ActionParamType in_eCommand, 
								CAkSoundBase*	 in_pSound, 
								CAkRegisteredObj * in_pGameObj, 
								AkPlayingID		in_PlayingID,
								TransParams&    in_rTparameters,
								bool       in_bIsMasterResume);

private:	 
	static AkListCtxs				m_listCtxs;					// List of PBIs/Contexts.	 
	static AkContextNotifQueue		m_CtxNotifQueue;
	static AkUInt32					m_uNumVirtualizedSounds;

	typedef AkListBareLight<CAkLimiter> AkListLightLimiters;
	static AkListLightLimiters m_BusLimiters;
	static AkListLightLimiters m_AMLimiters;
	static CAkLimiter m_GlobalLimiter;
};

#endif //#define _UPPER_RENDERER_H_
