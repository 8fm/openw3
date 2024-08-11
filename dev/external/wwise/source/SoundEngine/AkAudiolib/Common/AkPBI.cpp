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
// AkPBI.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPBI.h"
#include "Ak3DListener.h"
#include "AkBankMgr.h"
#include "AkSoundBase.h"
#include "AkRegisteredObj.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkPlayingMgr.h"
#include "AkParentNode.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkRTPCMgr.h"						// g_pRTPCMgr
#include "AkDefault3DParams.h"
#include "AkPathManager.h"					// g_pPathManager
#include "AkURenderer.h"
#include "AkProfile.h"
#include "AkAudioMgr.h"
#include "AkGen3DParams.h"
#include "AkBus.h"
#include "AkEnvironmentsMgr.h"
#include "AkFXMemAlloc.h"
#include "AkLEngineCmds.h"
#include "ActivityChunk.h"
#include "AkAttenuationMgr.h"
#include "AkVPLSrcCbxNode.h"
#include "AkSpeakerPan.h"

#ifdef AK_MOTION
	#include "AkFeedbackNode.h"
	#include "AkFeedbackBus.h"
	#include "AkFeedbackMgr.h"
#endif // AK_MOTION

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern CAkRegistryMgr*	g_pRegistryMgr;

// Unique incremental PBI ID Generator
AkUniqueID CAkPBI::ms_PBIIDGenerator = 0;

#define MIN_NUM_MUTED_ITEM_IN_LIST 4

CAkPBI::CAkPBI(	CAkSoundBase*				in_pSound,
			    CAkSource*					in_pSource,
				CAkRegisteredObj *			in_pGameObj,
				UserParams&					in_rUserparams,
				PlayHistory&				in_rPlayHistory,
				AkUniqueID					in_SeqID,
				const PriorityInfoCurrent&	in_rPriority,
#ifdef AK_MOTION
				bool						in_bTargetFeedback,
#endif // AK_MOTION
				AkUInt32					in_uSourceOffset,
				CAkLimiter*					in_pAMLimiter,
				CAkLimiter*					in_pBusLimiter
				)
	: m_pUsageSlot( NULL )
	, m_p3DSound( NULL )
	, m_UserParams( in_rUserparams )
	, m_SeqID( in_SeqID )
	, m_pSound( in_pSound )
	, m_pSource( in_pSource )
	, m_pGameObj( in_pGameObj )
	, m_pCbx( NULL )
	, m_Volume( AK_DEFAULT_LEVEL_DB )
	, m_LPF( AK_DEFAULT_LOPASS_VALUE )
	, m_LPFAutomationOffset( 0 )
	, m_fPlayStopFadeRatio( AK_UNMUTED_RATIO )
	, m_fPauseResumeFadeRatio( AK_UNMUTED_RATIO )
	, m_uSeekPosition( in_uSourceOffset )
	, m_LoopCount( LOOPING_ONE_SHOT )
	, m_eInitialState( PBI_InitState_Playing )
	, m_State( CtxStateStop )
	, m_bPosTypeChanged( true )
	, m_eCachedVirtualQueueBehavior( AkVirtualQueueBehavior_FromBeginning )
	, m_eCachedBelowThresholdBehavior( AkBelowThresholdBehavior_ContinueToPlay )
	, m_bVirtualBehaviorCached( false )
	, m_bAreParametersValid( false )
	, m_bGameObjPositionCached( false )
	, m_bGetAudioParamsCalled( false )
	, m_bNeedNotifyEndReached( false )
	, m_bIsNotifyEndReachedContinuous( false )
	, m_bTerminatedByStop( false )
	, m_bPlayFailed( false )
	, m_bWasStopped( false )
	, m_bWasPreStopped( false )
	, m_bWasPaused( false )
	, m_bInitPlayWasCalled( false )
	, m_bWasKicked( false )
	, m_eWasKickedForMemory( KickFrom_OverNodeLimit )
	, m_bWasPlayCountDecremented( false )
	, m_bRequiresPreBuffering( true )	// True: standard context require pre-buffering if streaming. Note: Zero-latency streams skip pre-buffering at the source level.
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	, m_bDoPriority( false )
#endif
#ifdef AK_MOTION
	, m_bFeedbackParametersValid( false )
	, m_bTargetIsFeedback( in_bTargetFeedback )
#endif // AK_MOTION
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	, m_bBypassAllFX( false )
#endif
	, m_bSeekDirty( in_uSourceOffset != 0 )
	, m_bSeekRelativeToDuration( false )
	, m_bSnapSeekToMarker( false )
	, m_bIsForcedToVirtualize( false )
	, m_bWasForcedToVirtualize( false )
	, m_bIsVirtual( false )
	, m_bNeedsFadeIn( false )
#ifndef AK_OPTIMIZED
	, m_bIsMonitoringMute( false )
#endif
#if defined(AK_VITA_HW)
	, m_bHardwareVoice( false )
#endif
	, m_PriorityInfoCurrent( in_rPriority )
	, m_ulPauseCount( 0 )
	, m_iFrameOffset( 0 )
	, m_pDataPtr( NULL )
	, m_uDataSize( 0 )
	, m_fMaxDistance( 0.0f )
#ifdef AK_MOTION
	, m_pFeedbackInfo( NULL )
#endif // AK_MOTION
	, m_pAMLimiter(in_pAMLimiter)
	, m_pBusLimiter(in_pBusLimiter)
	, m_pControlBus( in_pSound->GetControlBus() )
{
	m_PriorityInfoCurrent.SetPBIID( GetNewPBIID() );
	m_PathInfo.pPBPath = NULL;
	m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;

	m_pGameObj->AddRef();

	in_pSound->AddRef();
	in_pSound->AddPBI( this );
//	in_pSound->IncrementPlayCount( ... ); Already incremented by caller for priority handling

	m_bIsHDR = in_pSound->IsInHdrHierarchy();

	m_sMediaFormat.uChannelMask = AK_SPEAKER_FRONT_CENTER;

	m_CntrHistArray = in_rPlayHistory.HistArray;
#if defined AK_WII_FAMILY
	m_pGameObj->IncrementGameObjectPlayCount();
#endif
}

AKRESULT CAkPBI::Init( AkPathInfo* in_pPathInfo )
{
	AKASSERT( g_pRegistryMgr );
	AKASSERT( g_pPathManager );

	AKRESULT eResult = AK_Fail;

	////////////////////////////////////////////
	// Limiters
	////////////////////////////////////////////
	if( m_pAMLimiter )
	{
		m_pAMLimiter->Add( this, AKVoiceLimiter_AM );
	}
	if( m_pBusLimiter )
	{
		m_pBusLimiter->Add( this, AKVoiceLimiter_Bus );
	}
	CAkURenderer::GetGlobalLimiter().Add( this, AKVoiceLimiter_Global );
	////////////////////////////////////////////

	if( m_UserParams.PlayingID() )
	{
		AKASSERT( g_pPlayingMgr );
		eResult = g_pPlayingMgr->SetPBI( m_UserParams.PlayingID(), this, &m_uRegisteredNotif );
	}
	if( eResult == AK_Success )
	{
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
		m_bBypassAllFX = m_pSound->GetBypassAllFX( m_pGameObj );
#endif		
		// Setup 3D sound.
		m_p3DSound = NULL;
		AkPositionSourceType ePosType;
		AkPannerType ePanner;
		m_pSound->Get3DParams( m_p3DSound, m_pGameObj, ePanner, ePosType, &m_BasePosParams );
		m_ePosSourceType = ePosType;
		m_ePannerType = ePanner;

		if( m_p3DSound != NULL )
		{
			CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
			if( pAttenuation )
			{
				// Taking the value from the attenuation, if they are RTPC based, they will be overriden upon RTPC subscription, coming right after this.
				m_p3DSound->SetConeOutsideVolume( pAttenuation->m_ConeParams.fOutsideVolume );
				m_p3DSound->SetConeLPF( pAttenuation->m_ConeParams.LoPass );

				eResult = SubscribeAttenuationRTPC( pAttenuation );
				if( eResult != AK_Success )
					return eResult;

#ifndef AK_OPTIMIZED
				pAttenuation->AddPBI( this );
#endif
			}
			else
			{
				//we were expecting an attenuation but we couldn't get one
				if( m_p3DSound->GetParams()->m_uAttenuationID != AK_INVALID_UNIQUE_ID )
					return AK_Fail; //return an error (see WG-6760)
			}
		
			Init3DPath(in_pPathInfo);
			
		}
		else
		{
			m_ePannerType = Ak2D;	//Play in 2D if there was an allocation failure.
		}

		if( ( m_PathInfo.pPBPath != NULL ) )
		{
			AKRESULT tempResult = g_pPathManager->AddPathUser( m_PathInfo.pPBPath, this );
			if( tempResult == AK_Fail )
			{
				m_PathInfo.pPBPath = NULL;
			}
			else
			{
				m_PathInfo.pPBPath->SetSoundUniqueID( m_pSound->ID() );
				m_PathInfo.pPBPath->SetPlayingID( m_UserParams.PlayingID() );
			}
		}

		m_pSource->LockDataPtr( (void*&)m_pDataPtr, m_uDataSize, m_pUsageSlot );
	}

	return eResult;
}

CAkPBI::~CAkPBI()
{


#if defined( _DEBUG )
	DebugCheckLimiters();
#endif
}

void CAkPBI::Term( bool /*in_bFailedToInit*/ )
{
	AKASSERT(m_pSound);
	AKASSERT(g_pTransitionManager);
	AKASSERT(g_pPathManager);

	DecrementPlayCount();

#if defined AK_WII_FAMILY
	m_pGameObj->DecrementGameObjectPlayCount();
#endif

	if(m_PathInfo.pPBPath != NULL)
	{
		// if continous then the path got rid of the played flags
		if( m_PathInfo.pPBPath->IsContinuous() )
		{
			AkPathState* pPathState = m_pSound->GetPathState();
			pPathState->pbPlayed = NULL;
			pPathState->ulCurrentListIndex = 0;
		}
		g_pPathManager->RemovePathUser(m_PathInfo.pPBPath,this);
		m_PathInfo.pPBPath = NULL;
		m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
	}

	if( m_PBTrans.pvPSTrans )
	{
		Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
	}
	if( m_PBTrans.pvPRTrans )
	{
		Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
		g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
	}

	if( m_bNeedNotifyEndReached )
	{
		Monitor(AkMonitorData::NotificationReason_EndReached);
		m_bNeedNotifyEndReached = false;
	}

	if(m_UserParams.PlayingID())
	{
		g_pPlayingMgr->Remove(m_UserParams.PlayingID(),this);
	}

	if ( m_pGameObj )
	{
		m_pGameObj->Release();
	}

	if( m_p3DSound )
	{
		CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
		if( pAttenuation )
		{
			UnsubscribeAttenuationRTPC( pAttenuation );
#ifndef AK_OPTIMIZED
			pAttenuation->RemovePBI( this );
#endif
		}
		AkDelete( g_DefaultPoolId, m_p3DSound );
		m_p3DSound = NULL;
	}

	m_mapMutedNodes.Term();

	m_pSound->RemovePBI( this );

	// Must be done BEFORE releasing m_pSound, as the source ID is required to free the data.
	if ( m_pDataPtr )
	{
		m_pSource->UnLockDataPtr();
		m_pDataPtr = NULL;
	}
	if( m_pUsageSlot )
	{
		m_pUsageSlot->Release( false );
		m_pUsageSlot = NULL;
	}
	
	bool bSourceIsExternal = m_pSource->IsExternallySupplied();

	m_pSound->Release();

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL)
	{
		m_pFeedbackInfo->Destroy();
		m_pFeedbackInfo = NULL;
	}
#endif // AK_MOTION

	if ( bSourceIsExternal )
	{
		//External sources are cloned when used so we can change their properties with real values.
		//Release the cloned source.
		
		AkDelete(g_DefaultPoolId, m_pSource);
	}
}

void CAkPBI::_InitPlay()
{
	// Generate the next loop count.
    AKASSERT( m_pSound != NULL );
	m_LoopCount = m_pSound->Loop();

	//Ensure this function is not called twice
	AKASSERT( !m_bInitPlayWasCalled );
	if(!m_bInitPlayWasCalled)
	{
		m_bInitPlayWasCalled = true; // Must be done BEFORE calling AkMonitorData

		if( m_PathInfo.pPBPath != NULL )
		{
			g_pPathManager->Start( m_PathInfo.pPBPath,m_pSound->GetPathState());
		}
	}
}

AKRESULT CAkPBI::Init3DPath(AkPathInfo* in_pPathInfo)
{
	Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
	if( l_p3DParams == NULL )
		return AK_Success;

	if ((AkPositionSourceType)m_ePosSourceType == AkUserDef)
	{
		//Init the path only if it is set in 3D User Def or if there is an RTPC on the 3d/2d and the 3D pos is UserDef.
		bool bHasRTPC = m_pSound->HasRTPC(RTPC_PositioningType) && m_pSound->OverridesPositioning();
		CAkParameterNodeBase* pParent = m_pSound->Parent();
		while(!bHasRTPC && pParent != NULL)
		{
			bHasRTPC = pParent->HasRTPC(RTPC_PositioningType) && pParent->OverridesPositioning();
			pParent = pParent->Parent();
		}

		if( m_ePannerType == Ak2D && !bHasRTPC )
			return AK_Success;
	}

	AKRESULT eResult = AK_Success;

	// get ID
	AkUniqueID PathOwnerID = m_p3DSound->GetPathOwner();

	// got one ?
	if(in_pPathInfo->pPBPath != NULL)
	{
		// same owner ?
		if(in_pPathInfo->PathOwnerID == PathOwnerID)
		{
			// use this path
			m_PathInfo.pPBPath = in_pPathInfo->pPBPath; 
			// keep the id
			m_PathInfo.PathOwnerID = in_pPathInfo->PathOwnerID;
		}
	}

	// already got one ?				
	//If we are in continuous mode, the StepNewSound option must not be there.
	AKASSERT((l_p3DParams->m_ePathMode & (AkPathContinuous | AkPathStepNewSound)) != (AkPathContinuous | AkPathStepNewSound));
	if(m_PathInfo.pPBPath == NULL || l_p3DParams->m_ePathMode & AkPathStepNewSound)
	{
		// no, get a path from the manager
		m_PathInfo.pPBPath = g_pPathManager->AddPathToList();

		// if we've got one then proceed
		if( m_PathInfo.pPBPath != NULL)
		{
			// set m_pPath according to what's in the sound
			eResult = m_p3DSound->SetPathPlayList( m_PathInfo.pPBPath,m_pSound->GetPathState());

			if (eResult != AK_Success)
			{
				g_pPathManager->RemovePathFromList( m_PathInfo.pPBPath );

				m_PathInfo.pPBPath = NULL;
				PathOwnerID = AK_INVALID_UNIQUE_ID;
			}
			// keep the id
			m_PathInfo.PathOwnerID = PathOwnerID;
		}
	}

	if (!m_p3DSound->GetParams()->m_bFollowOrientation && m_PathInfo.pPBPath != NULL)
		m_PathInfo.pPBPath->InitRotationMatricesForNoFollowMode(m_pGameObj->GetListenerMask());

	return eResult;
}

AKRESULT CAkPBI::_Play( TransParams & in_transParams, bool in_bPaused, bool in_bForceIgnoreSync )
{
	AKRESULT eResult;

    // Start transition if applicable.
    if( in_transParams.TransitionTime != 0 )
	{
		m_fPlayStopFadeRatio = AK_MUTED_RATIO;
		CreateTransition( true, 
					 	TransTarget_Play, 
						in_transParams, 
						false );
	}

	if( in_bPaused == true || m_eInitialState == PBI_InitState_Paused )
	{
		m_bWasPaused = true;
		eResult = CAkLEngineCmds::EnqueueAction( LEStatePlayPause, this );
		
		if( m_PBTrans.pvPSTrans )
		{
			g_pTransitionManager->Pause( m_PBTrans.pvPSTrans );
		}

		PausePath(true);		
	}
	else
	{
		eResult = CAkLEngineCmds::EnqueueAction( LEStatePlay, this );
	}

	if ( eResult == AK_Success )
	{
		if( m_eInitialState == PBI_InitState_Stopped )
		{
			_Stop();
		}

		if( in_bForceIgnoreSync )
		{
			// especially useful for IM, avoid making IM playback pending one on each others.
			CAkLEngineCmds::IncrementSyncCount();
		}
#ifndef AK_OPTIMIZED
		RefreshMonitoringMute();
#endif
	}

	return eResult;
}

void CAkPBI::_Stop( AkPBIStopMode in_eStopMode /*= AkPBIStopMode_Normal*/, bool in_bIsFromTransition /*= false*/, bool /*in_bHasNotStarted = false*/)
{
	if(!m_bWasStopped)
	{
		m_bWasStopped = true;

		// In the case of transition, it is the combiner node that checks the WasStopped flag
		// once the last transition buffer has been processed.
		if(!in_bIsFromTransition)
		{
			// Necessary in the case of play-stop, or stop-and-continue.
			CAkLEngineCmds::EnqueueActionStop( this );
		}

		if( in_eStopMode == AkPBIStopMode_Normal || in_eStopMode == AkPBIStopMode_StopAndContinueSequel )
		{
			if( m_PBTrans.pvPSTrans )
			{
				Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
				g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
				m_PBTrans.pvPSTrans = NULL;
			}
			if( m_PBTrans.pvPRTrans )
			{
				Monitor(AkMonitorData::NotificationReason_Fade_Aborted);
				g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
				m_PBTrans.pvPRTrans = NULL;
			}

			if(m_PathInfo.pPBPath != NULL)
			{
				// if continous then the path got rid of the played flags
				if(m_PathInfo.pPBPath->IsContinuous())
				{
					AkPathState* pPathState = m_pSound->GetPathState();
					pPathState->pbPlayed = NULL;
					pPathState->ulCurrentListIndex = 0;
				}
				g_pPathManager->RemovePathUser(m_PathInfo.pPBPath,this);
				m_PathInfo.pPBPath = NULL;
				m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
			}

			if( m_bWasPaused )
			{
				Monitor(AkMonitorData::NotificationReason_Pause_Aborted);
			}

			if( m_bIsNotifyEndReachedContinuous || in_eStopMode == AkPBIStopMode_StopAndContinueSequel )
			{
				Monitor(AkMonitorData::NotificationReason_StoppedAndContinue, m_bNeedNotifyEndReached );
			}
			else
			{
				AkMonitorData::NotificationReason reason;

				if( !m_bWasKicked )
					reason = AkMonitorData::NotificationReason_Stopped;
				else
				{
					switch( m_eWasKickedForMemory )
					{
					case KickFrom_Stopped:
						reason = AkMonitorData::NotificationReason_Stopped;
						break;
					case KickFrom_OverNodeLimit:
						reason = AkMonitorData::NotificationReason_StoppedLimit;
						break;
					case KickFrom_OverGlobalLimit:
						reason = AkMonitorData::NotificationReason_StoppedGlobalLimit;
						break;
					case KickFrom_OverMemoryLimit:
						reason = AkMonitorData::NotificationReason_StoppedMemoryThreshold;
						break;
                    default:
                        reason = AkMonitorData::NotificationReason_None;
                        break;
					}
				}

				Monitor( reason, m_bNeedNotifyEndReached );
			}

			m_bNeedNotifyEndReached = false;
			m_bTerminatedByStop = true;
		}
	}
}

void CAkPBI::_Stop( 
	const TransParams & in_transParams,	// Fade parameters
	bool in_bUseMinTransTime			// If true, ensures that a short fade is applied if TransitionTime is 0. 
										// If false, the caller needs to ensure that the lower engine will stop the PBI using another method... (stop offset anyone?) 
	)
{
	if( m_bWasPaused || ( m_PBTrans.pvPRTrans && m_PBTrans.bIsPRTransFading ) )
	{
		// If we are paused or going to be, we stop right away.
		_Stop();
	}
	else
	{
		m_bWasPreStopped = true;
		if( in_transParams.TransitionTime != 0 )
		{
			CreateTransition(true, TransTarget_Stop, in_transParams, true );			 
		}
		else if( m_State == CtxStateStop )//That mean we are not yet started, just let it be stopped without the glitch caussed by the minimal transition time
		{
			_Stop( AkPBIStopMode_Normal, false, true );	// flag "has not started playing".
		}
		else
		{
			if( m_PBTrans.pvPSTrans )
			{
				// Force actual transition to use new duration of 0 ms
				g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPSTrans ), 
														TransTarget_Stop, 
														AK_MUTED_RATIO,
														0,//no transition time
														AkCurveInterpolation_Linear,
														AkValueMeaning_Default );
			}
			else if ( in_bUseMinTransTime )
			{
				StopWithMinTransTime();
			}
			else
			{
				// Otherwise, the caller needs to ensure that the lower engine will stop the PBI using another method... (stop offset anyone?) 
				// Currently, the only other way of stopping is to specify a stop offset.
				AKASSERT( GetStopOffset() != AK_NO_IN_BUFFER_STOP_REQUESTED );
			}
		}
	}
}

// Prepare PBI for stopping with minimum transition time. Note: also used in PBI subclasses.
void CAkPBI::StopWithMinTransTime()
{
	//The feedback engine has its own fade out which fades the *speed* of the signal, not the signal itself
	if (!IsForFeedbackPipeline())
	{
		//Set volume to 0 and flag to generate ramp in LEngine.
		m_fPlayStopFadeRatio = AK_MUTED_RATIO;
		// Set fFadeRatio to min directly instead of calling ComputeEffectiveVolumes
		m_EffectiveParams.fFadeRatio = AK_MUTED_RATIO;
	}
	// Set kicked flag to make sure the PBI is no longer counted as an active voice
	if ( ! m_bWasKicked )
	{
		m_bWasKicked = true;
		m_eWasKickedForMemory = KickFrom_Stopped;
	}
	_Stop( AkPBIStopMode_Normal, true );
}

#ifndef AK_OPTIMIZED
void CAkPBI::_StopAndContinue(
		AkUniqueID			/*in_ItemToPlay*/,
		AkUInt16			/*in_PlaylistPosition*/,
		CAkContinueListItem* /*in_pContinueListItem*/
		)
{
	_Stop();
}
#endif

void CAkPBI::_Pause(bool in_bIsFromTransition /*= false*/)
{
	if(!m_bWasStopped && !m_bWasPaused)
	{
		m_bWasPaused = true;
		//The feedback engine has its own fade out which fades the *speed* of the signal, not the signal itself
		if (!IsForFeedbackPipeline())
		{
			m_fPauseResumeFadeRatio = AK_MUTED_RATIO;
			// Set fFadeRatio to min directly instead of calling ComputeEffectiveVolumes
			m_EffectiveParams.fFadeRatio = AK_MUTED_RATIO;
		}
		AKASSERT(g_pTransitionManager);

		// In the case of transition, it is the combiner node that checks the WasStopped flag
		// once the last transition buffer has been processed.
		if(!in_bIsFromTransition)
		{
			CAkLEngineCmds::EnqueueAction( LEStatePause, this );
		}
		if( m_PBTrans.pvPSTrans )
		{
			g_pTransitionManager->Pause( m_PBTrans.pvPSTrans );
		}

		PausePath(true);
	}
}

void CAkPBI::_Pause( TransParams & in_transParams )
{
	++m_ulPauseCount;

	AKASSERT( m_ulPauseCount != 0 );//Just in case we were at -1 unsigned

	if( in_transParams.TransitionTime != 0 )
	{
		CreateTransition(false, TransTarget_Pause, in_transParams, true );
	}
	else if( m_State == CtxStateStop )
	{	// either we were stopped or not yet started, do not use minimal transition time
		_Pause();
	}
	else
	{	
		if( m_PBTrans.pvPRTrans )
		{
			// Force actual transition to use new duration of 0 ms
			g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPRTrans ), 
													TransTarget_Pause, 
													AK_MUTED_RATIO,
													0,//no transition time
													AkCurveInterpolation_Linear,
													AkValueMeaning_Default );
		}
		else
		{
			_Pause( true );
		}
	}
}

void CAkPBI::_Resume()
{
	if(!m_bWasStopped)
	{
		if(m_bWasPaused == true)
		{
			PausePath(false);

			m_bWasPaused = false;
			AKASSERT(g_pTransitionManager);
			CAkLEngineCmds::EnqueueAction( LEStateResume, this );

			if( m_PBTrans.pvPSTrans )
			{
				g_pTransitionManager->Resume( m_PBTrans.pvPSTrans );
			}
		}
	}
}

void CAkPBI::_Resume( TransParams & in_transParams, bool in_bIsMasterResume )
{
	if( in_bIsMasterResume || m_ulPauseCount <= 1)
	{
		m_ulPauseCount = 0;

		_Resume();

		if( in_transParams.TransitionTime != 0 )
		{
			// Use given transition time
			CreateTransition( false, TransTarget_Resume, in_transParams, false );
		}
		else if( m_PBTrans.pvPRTrans )
		{
			// Force actual transition to use new duration of 0 ms
			g_pTransitionManager->ChangeParameter(	static_cast<CAkTransition*>( m_PBTrans.pvPRTrans ), 
													TransTarget_Resume, 
													AK_UNMUTED_RATIO,
													0,//no transition time
													AkCurveInterpolation_Linear,
													AkValueMeaning_Default );
		}
		else
		{
			// no transition created, using minimal transition time
			m_fPauseResumeFadeRatio = AK_UNMUTED_RATIO;
			CalculateMutedEffectiveVolume();
		}
	}
	else
	{
		--m_ulPauseCount;
	}
}

void CAkPBI::PlayToEnd( CAkParameterNodeBase * )
{
	m_LoopCount = 1;

	CAkLEngineCmds::EnqueueAction( LEStateStopLooping, this );
}

void CAkPBI::SeekTimeAbsolute( AkTimeMs in_iPosition, bool in_bSnapToMarker )
{
	// Negative seek positions are not supported on sounds.
	AKASSERT( in_iPosition >= 0 );

	// Set source offset and notify lower engine.
	SetNewSeekPosition( AkTimeConv::MillisecondsToSamples( in_iPosition ), in_bSnapToMarker );
	CAkLEngineCmds::EnqueueAction( LEStateSeek, this );
	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID(), m_pGameObj->ID(), m_UserParams.CustomParam(), AkMonitorData::NotificationReason_Seek, m_CntrHistArray, m_pSound->ID(), false, in_iPosition );
}

void CAkPBI::SeekPercent( AkReal32 in_fPercent, bool in_bSnapToMarker )
{
	// Out of bound percentage not accepted.
	AKASSERT( in_fPercent >= 0 && in_fPercent <= 1 );

	// Set source offset and notify lower engine.
	SetNewSeekPercent( in_fPercent, in_bSnapToMarker );
	CAkLEngineCmds::EnqueueAction( LEStateSeek, this );
	AkReal32 fPercent = in_fPercent*100;
	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID(), m_pGameObj->ID(), m_UserParams.CustomParam(), AkMonitorData::NotificationReason_SeekPercent, m_CntrHistArray, m_pSound->ID(), false, *(AkTimeMs*)&fPercent );
}

void CAkPBI::ParamNotification( NotifParams& in_rParams )
{
	switch( in_rParams.eType )
	{
	case RTPC_Volume:
	case RTPC_BusVolume:// BusVolumes reach here when on Wii and 3DS. Consider them just as volume as there is no difference at this point.
		if( !IsForFeedbackPipeline() || !in_rParams.bIsFromBus ) //Ignore Audio bus Volume notif on Motion Fx PBI
		{
			m_Volume += in_rParams.fValue;
			CalculateMutedEffectiveVolume();
#ifdef AK_MOTION
			if (in_rParams.bIsFromBus && m_pFeedbackInfo != NULL ) //Keep track of AudioBusVolume so it will be canceled in 'audio to motion' motion splitter
			{
				m_pFeedbackInfo->m_AudioBusVolume += in_rParams.fValue;
			}
#endif // AK_MOTION
		}
		break;
#ifdef AK_MOTION
	case RTPC_FeedbackPitch:
		if( !IsForFeedbackPipeline())
			break;
		//otherwwise do RTPC_Pitch so no break intentional.
#endif // AK_MOTION
	case RTPC_Pitch:
		m_EffectiveParams.Pitch += in_rParams.fValue;
		break;
	case RTPC_LPF:
		m_LPF += in_rParams.fValue;
		CalculateEffectiveLPF();
		break;
	case RTPC_Priority:
		{
			// Known issue:
			// There is one situation where this will not work properly.
			// It is if the base RTPC + distance offset were totalling an invalid number and this was them modified using RTPCs.
			// If this occur, there may be a slight offset in the distance attenuation factor calculation.
			// We actually ignore this error, since this error will be corrected on next iteration of the 3D calculation if it applies.
			// Fixing this would cause a lot of processing for a simple RTPC update.
			// The fix could be:
			// re-calculate the distance offset priority here ( and access all the associated parameters from here, which may take some time )
			// or keep the last non-clamped calculated offset as a member of the PBI class.( Which looks a bit heavy due to the fact that this is not supposed to happen if the user did draw consistent graphs. )
			//
			AkReal32 fNewCurrent = in_rParams.fValue + m_PriorityInfoCurrent.BasePriority() - m_PriorityInfoCurrent.GetCurrent();
			fNewCurrent = AkMin( AkMax( AK_MIN_PRIORITY, fNewCurrent ), AK_MAX_PRIORITY);
			UpdatePriority( fNewCurrent );
			m_PriorityInfoCurrent.ResetBase( in_rParams.fValue );
		}
		break;

#ifdef AK_MOTION
	case RTPC_FeedbackVolume:
		if(m_pFeedbackInfo != NULL)
		{
			m_pFeedbackInfo->m_NewVolume += in_rParams.fValue;
		}
		break;
	case RTPC_FeedbackLowpass:
		if(m_pFeedbackInfo != NULL)
		{
			m_pFeedbackInfo->m_LPF += in_rParams.fValue;
		}
		break;
#endif // AK_MOTION

	case RTPC_UserAuxSendVolume0:
		m_EffectiveParams.aUserAuxSendVolume[0] += in_rParams.fValue;
		break;
	case RTPC_UserAuxSendVolume1:
		m_EffectiveParams.aUserAuxSendVolume[1] += in_rParams.fValue;
		break;
	case RTPC_UserAuxSendVolume2:
		m_EffectiveParams.aUserAuxSendVolume[2] += in_rParams.fValue;
		break;
	case RTPC_UserAuxSendVolume3:
		m_EffectiveParams.aUserAuxSendVolume[3] += in_rParams.fValue;
		break;

	case RTPC_GameAuxSendVolume:
		m_EffectiveParams.fGameAuxSendVolume += in_rParams.fValue;
		break;
	case RTPC_OutputBusVolume:
		m_EffectiveParams.fOutputBusVolume += in_rParams.fValue;
		break;
	case RTPC_OutputBusLPF:
		m_EffectiveParams.fOutputBusLPF += in_rParams.fValue;
		break;

	case RTPC_HDRActiveRange:
		m_EffectiveParams.hdr.fActiveRange += in_rParams.fValue;
		break;

	case RTPC_MakeUpGain:
		m_EffectiveParams.normalization.fMakeUpGain += in_rParams.fValue;
		break;
	}
}

void CAkPBI::MuteNotification( AkReal32 in_fMuteRatio, AkMutedMapItem& in_rMutedItem, bool in_bPrioritizeGameObjectSpecificItems)
{
	if ( in_bPrioritizeGameObjectSpecificItems )
	{
		// Mute notifications never apply to persistent mute items.
        AKASSERT( !in_rMutedItem.m_bIsPersistent );
        
		// Search the "opposite" entry for this identifier (i.e. if we're setting
		// a global entry, let's search for a non-global entry, and vice-versa)
		AkMutedMapItem searchItem;
        searchItem.m_Identifier = in_rMutedItem.m_Identifier;
		searchItem.m_bIsGlobal = ! in_rMutedItem.m_bIsGlobal;
		searchItem.m_bIsPersistent = false; 

		if ( m_mapMutedNodes.Exists( searchItem ) )
		{
			if ( in_rMutedItem.m_bIsGlobal )
			{
				// We already have a non-global entry for this
				// identifier. Since we were asked to prioritize
				// game object-specific entries, we will simply
				// ignore the new info.
				return;
			}
			else
			{
				// We have a global entry for this identifier. Since
				// we were asked to prioritize game object-specific
				// entries, we must remove it and replace it with the
				// new, game object-specific entry (below).
				m_mapMutedNodes.Unset( searchItem );
			}
		}
	}

	// There's no point in keeping an unmuted entry, except if we're asked
	// to prioritize game object-specific items, in which case we must
	// keep it to make sure it doesn't get replaced by a global entry later.
	if( in_fMuteRatio == AK_UNMUTED_RATIO && ( ! in_bPrioritizeGameObjectSpecificItems || in_rMutedItem.m_bIsGlobal ) )
		m_mapMutedNodes.Unset( in_rMutedItem );
	else
		m_mapMutedNodes.Set( in_rMutedItem, in_fMuteRatio );

	CalculateMutedEffectiveVolume();
}

// direct access to Mute Map. Applies only to persistent items.
AKRESULT CAkPBI::SetMuteMapEntry( 
    AkMutedMapItem & in_key,
    AkReal32 in_fFadeRatio
    )
{
    AKASSERT( in_key.m_bIsPersistent );

    AKRESULT eResult;
    if ( in_fFadeRatio != AK_UNMUTED_RATIO )
        eResult = ( m_mapMutedNodes.Set( in_key, in_fFadeRatio ) ) ? AK_Success : AK_Fail;
    else
    {
        m_mapMutedNodes.Unset( in_key );
        eResult = AK_Success;
    }
    CalculateMutedEffectiveVolume();
    return eResult;
}

void CAkPBI::RemoveAllVolatileMuteItems()
{
    AkMutedMap::Iterator iter = m_mapMutedNodes.Begin();
    while ( iter != m_mapMutedNodes.End() )
	{
        if ( !((*iter).key.m_bIsPersistent) )
            iter = m_mapMutedNodes.EraseSwap( iter );
        else
            ++iter;
	}   
}

AkReal32 CAkPBI::GetVoiceVolumedB()
{
	AkReal32 fVoiceVolumedB = 0.0f;

	CAkBus* pBus = GetControlBus(); //control bus can be null when in feedback pipeline.
	if( pBus )
	{
		fVoiceVolumedB = pBus->GetVoiceVolume();
	}

	return fVoiceVolumedB;
}

AkReal32 CAkPBI::GetOutputBusVolumeValuedB()
{
	AkReal32 fOutputBusVolume =  m_EffectiveParams.fOutputBusVolume;

	CAkBus* pBus = GetControlBus(); //control bus can be null when in feedback pipeline.
	if( pBus )
	{
		fOutputBusVolume += pBus->GetControlBusVolume();
	}

	return fOutputBusVolume;
}

AkReal32 CAkPBI::Scale3DUserDefRTPCValue( AkReal32 in_fValue )
{
	AKASSERT( m_p3DSound );

	CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
	if( pAttenuation )
	{
		CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
		if( pVolumeDryCurve )
			return in_fValue * pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From / 100.0f;
	}

	return in_fValue;
}

void CAkPBI::PositioningChangeNotification(
		AkReal32			in_RTPCValue,
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		)
{	
	switch ( in_ParameterID )
	{
		case POSID_Position_PAN_X_2D:
			m_BasePosParams.m_fPAN_X_2D = in_RTPCValue;
			return;
		case POSID_Position_PAN_Y_2D:
			m_BasePosParams.m_fPAN_Y_2D = in_RTPCValue;
			return;
		case POSID_Position_PAN_X_3D:
			if ( m_p3DSound )
				m_p3DSound->GetParams()->m_Position.X = Scale3DUserDefRTPCValue( in_RTPCValue );
			return;
		case POSID_Position_PAN_Y_3D:
			if ( m_p3DSound )
				m_p3DSound->GetParams()->m_Position.Z = Scale3DUserDefRTPCValue( in_RTPCValue );
			return;
		case POSID_Positioning_Divergence_Center_PCT:
			m_BasePosParams.m_fCenterPCT = in_RTPCValue;
			return;
		case POSID_2DPannerEnabled:
			m_BasePosParams.bIsPannerEnabled = ( in_RTPCValue > 0 )? true : false;
			return;
		case POSID_PositioningType:
			m_bPosTypeChanged |= m_ePannerType != (AkPannerType)(AkInt)in_RTPCValue;
			m_ePannerType = (AkPannerType)(AkInt)in_RTPCValue;
			if ( m_ePannerType == Ak3D && !m_p3DSound )
			{
				AkPannerType ePanner;
				AkPositionSourceType ePosType;
				m_pSound->Get3DParams( m_p3DSound, m_pGameObj, ePanner, ePosType, &m_BasePosParams );
				if ( !m_p3DSound )
					m_ePannerType = Ak2D;	//Play in 2D if 3D params allocation failed.
			}

			return;
	}

	if( m_p3DSound )
	{
		switch (in_ParameterID)
		{
		case POSID_IsPositionDynamic:
			m_p3DSound->SetIsPositionDynamic( in_RTPCValue?true:false );
			break;
		case POSID_IsLooping:
			m_p3DSound->SetIsLooping( in_RTPCValue?true:false );
			break;
		case POSID_Transition:
			m_p3DSound->SetTransition( (AkTimeMs)in_RTPCValue );
			break;
		case POSID_PathMode:
			m_p3DSound->SetPathMode( (AkPathMode)(AkInt)in_RTPCValue );
			break;

		default:
			AKASSERT( !"Invalid or unknown Positioning parameter passed to the PBI" );
			break;
		}
	}
}

void CAkPBI::CalculateMutedEffectiveVolume()
{
	// PlayStop and PauseResume fade ratios as well as ratios of the MutedMap are [0,1].
	AkReal32 l_fRatio = 1.0f;
	for( AkMutedMap::Iterator iter = m_mapMutedNodes.Begin(); iter != m_mapMutedNodes.End(); ++iter )
	{
		l_fRatio *= (*iter).item;
	}
	l_fRatio *= m_fPlayStopFadeRatio;
	l_fRatio *= m_fPauseResumeFadeRatio;
	l_fRatio = AkMath::Max( l_fRatio, AK_MUTED_RATIO );	// Clamp to 0 because transitions may result in ratios slightly below 0.
	//AKASSERT( l_fRatio <= 1.f );	// Small overshoots may be accepted. Could also drop a term in taylor approximations of interpolation curves...
	
	m_EffectiveParams.fFadeRatio = l_fRatio;

	m_EffectiveParams.Volume = m_Volume + m_Ranges.VolumeOffset;
}

void CAkPBI::CalculateEffectiveLPF()
{
	m_EffectiveParams.LPF = m_LPF + m_LPFAutomationOffset;
}

void CAkPBI::RecalcNotification()
{
	m_bAreParametersValid = false;
#ifdef AK_MOTION
	m_bFeedbackParametersValid = false;	
#endif // AK_MOTION
}

void CAkPBI::NotifyBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ )
{
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	if ( m_pCbx 
		&& ( in_uTargetMask & ~( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) ) )
	{
		m_pCbx->SetFxBypass( in_bitsFXBypass, in_uTargetMask );
	}

	if ( in_uTargetMask & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )
		m_bBypassAllFX = ( in_bitsFXBypass & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )!=0;
#endif
}

#ifndef AK_OPTIMIZED
void CAkPBI::InvalidatePaths()
{
	if( m_PathInfo.pPBPath != NULL )
	{
		// if continuous then the path got rid of the played flags
		if(m_PathInfo.pPBPath->IsContinuous())
		{
			AkPathState* pPathState = m_pSound->GetPathState();
			pPathState->pbPlayed = NULL;
			pPathState->ulCurrentListIndex = 0;
		}
		g_pPathManager->RemovePathUser( m_PathInfo.pPBPath, this );
		g_pPathManager->RemovePathFromList( m_PathInfo.pPBPath );
		m_PathInfo.pPBPath = NULL;
		m_PathInfo.PathOwnerID = AK_INVALID_UNIQUE_ID;
	}
	if( m_p3DSound )
	{
		m_p3DSound->InvalidatePaths();
	}
}
#endif

void CAkPBI::TransUpdateValue(AkIntPtr in_eTarget,AkReal32 in_fValue,bool in_bIsTerminated)
{
	TransitionTargets eTarget = (TransitionTargets) in_eTarget;
	switch(eTarget)
	{
	case TransTarget_Stop:
	case TransTarget_Play:
		if(in_bIsTerminated)
		{
			m_PBTrans.pvPSTrans = NULL;
			Monitor(AkMonitorData::NotificationReason_Fade_Completed);

			if( eTarget == TransTarget_Stop )
			{
				_Stop( AkPBIStopMode_Normal, true );
			}
		}
		m_fPlayStopFadeRatio = in_fValue;
		break;
	case TransTarget_Pause:
	case TransTarget_Resume:
		if(in_bIsTerminated)
		{
			m_PBTrans.pvPRTrans = NULL;
			Monitor(AkMonitorData::NotificationReason_Fade_Completed);

			if( eTarget == TransTarget_Pause )
			{
				_Pause(true);
			}
		}
		m_fPauseResumeFadeRatio = in_fValue;
		break;
	default:
		AKASSERT(!"Unsupported data type");
		break;
	}

	CalculateMutedEffectiveVolume();
}

AKRESULT CAkPBI::SetParam(
			AkPluginParamID in_paramID,
			const void *	in_pParam,
			AkUInt32		in_uParamSize
			)
{
	if( m_p3DSound )
	{
		AKASSERT( in_uParamSize == sizeof( AkReal32 ) );
		AkReal32 l_newVal = *( (AkReal32*)in_pParam );

		switch ( in_paramID )
		{
		case POSID_Positioning_Cone_Attenuation:
			m_p3DSound->SetConeOutsideVolume( l_newVal );
			break;
		case POSID_Positioning_Cone_LPF:
			m_p3DSound->SetConeLPF( l_newVal );
			break;

		default:
			AKASSERT( !"Invalid or unknown Positioning parameter passed to the PBI" );
			break;
		}
	}
	return AK_Success;
}

void CAkPBI::CreateTransition(bool in_bIsPlayStopTransition, AkIntPtr in_transitionTarget, TransParams in_transParams, bool in_bIsFadingTransition )
{
	AKASSERT(g_pTransitionManager);
	CAkTransition* prTransition = in_bIsPlayStopTransition?m_PBTrans.pvPSTrans:m_PBTrans.pvPRTrans;
	if(!prTransition)
	{
		TransitionParameters Params(
			this,
			in_transitionTarget,
			in_bIsPlayStopTransition?m_fPlayStopFadeRatio:m_fPauseResumeFadeRatio,
			in_bIsFadingTransition?AK_MUTED_RATIO:AK_UNMUTED_RATIO,
			in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
			false,
			true );
		prTransition = g_pTransitionManager->AddTransitionToList(Params);

		if(in_bIsPlayStopTransition)
		{
			m_PBTrans.pvPSTrans = prTransition;
			m_PBTrans.bIsPSTransFading = in_bIsFadingTransition;
			m_bNeedsFadeIn = ( (TransitionTargets)in_transitionTarget == TransTarget_Play );
		}
		else
		{
			m_PBTrans.pvPRTrans = prTransition;
			m_PBTrans.bIsPRTransFading = in_bIsFadingTransition;
		}

		MonitorFade( AkMonitorData::NotificationReason_Fade_Started, in_transParams.TransitionTime );

		if( !prTransition )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTarget, Params.fTargetValue, true );
		}
	}
	else
	{
		g_pTransitionManager->ChangeParameter(
			static_cast<CAkTransition*>(prTransition),
			in_transitionTarget,
			in_bIsFadingTransition?AK_MUTED_RATIO:AK_UNMUTED_RATIO,
			in_transParams.TransitionTime,
			in_transParams.eFadeCurve,
			AkValueMeaning_Default);
	}
}

void CAkPBI::DecrementPlayCount()
{
	if( m_bIsVirtual )
		Devirtualize( false );

	if( m_bWasPlayCountDecremented == false )
	{
		m_bWasPlayCountDecremented = true;


		////////////////////////////////////////////
		// Limiters
		////////////////////////////////////////////
#if defined( _DEBUG )
		CAkLimiter* pAMLimiter = m_pAMLimiter;
		CAkLimiter* pBusLimiter = m_pBusLimiter;
#endif
		if( m_pAMLimiter )
		{
			m_pAMLimiter->Remove( this, AKVoiceLimiter_AM );
			m_pAMLimiter = NULL;
		}
		if( m_pBusLimiter )
		{
			m_pBusLimiter->Remove( this, AKVoiceLimiter_Bus );
			m_pBusLimiter = NULL;
		}
		CAkURenderer::GetGlobalLimiter().Remove( this, AKVoiceLimiter_Global );

#if defined( _DEBUG )
		DebugCheckLimiters();
#endif
		////////////////////////////////////////////

		CounterParameters counterParams;
		counterParams.pGameObj = m_pGameObj;
		m_pSound->DecrementPlayCount( counterParams );

	}
#if defined( _DEBUG )
	else
	{
		DebugCheckLimiters();
	}
#endif
}

#if defined( _DEBUG )
void CAkPBI::DebugCheckLimiters()
{
	// WG-24797: Added debug code for obscure bug.  If any of these asserts have triggered, please contact AK with as much information as possible (e.g. callstack).
	AKASSERT( m_pAMLimiter == NULL );
	AKASSERT( m_pBusLimiter == NULL );
	AKASSERT( ! CAkURenderer::CheckLimitersForCtx( this ) );
	// WG-24797: Added debug code for obscure bug.  If any of these asserts have triggered, please contact AK with as much information as possible (e.g. callstack).
}
#endif

// Returns true if the Context may jump to virtual voices, false otherwise.
AkBelowThresholdBehavior CAkPBI::GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior )
{
	if( !m_bVirtualBehaviorCached )
	{
		m_bVirtualBehaviorCached = true;
		AkBelowThresholdBehavior eBelowThresholdBehavior = m_pSound->GetVirtualBehavior( out_Behavior );

		//Set the cache values;
		m_eCachedVirtualQueueBehavior = out_Behavior;
		m_eCachedBelowThresholdBehavior = eBelowThresholdBehavior;

		return eBelowThresholdBehavior;
	}
	else
	{
		out_Behavior = (AkVirtualQueueBehavior)m_eCachedVirtualQueueBehavior;
		return (AkBelowThresholdBehavior)m_eCachedBelowThresholdBehavior;
	}
}

void CAkPBI::MonitorFade( AkMonitorData::NotificationReason in_Reason, AkTimeMs in_TransitionTime )
{
	AkUniqueID id = 0;
	if(m_pSound)
	{
		id = m_pSound->ID();
	}
	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID(), m_pGameObj->ID(), m_UserParams.CustomParam(), in_Reason, m_CntrHistArray, id, false, in_TransitionTime );
}

#ifndef AK_OPTIMIZED
void CAkPBI::Monitor(AkMonitorData::NotificationReason in_Reason, bool in_bUpdateCount )
{
	AkUniqueID id = 0;
	if( m_pSound )
	{
		id = m_pSound->ID();
	}
	if( !m_bInitPlayWasCalled )
	{
		AKASSERT( !( in_Reason == AkMonitorData::NotificationReason_StoppedAndContinue ) );//Should not happen
		AKASSERT( !( in_Reason == AkMonitorData::NotificationReason_EndReachedAndContinue ) );//Should not happen

		//If the PBI was not initialized, it must be considered as a PlayFailed since it never started...
		if( in_Reason == AkMonitorData::NotificationReason_Stopped 
			|| in_Reason == AkMonitorData::NotificationReason_EndReached
			)
		{
			in_Reason = AkMonitorData::NotificationReason_ContinueAborted;
		}
		else if( in_Reason == AkMonitorData::NotificationReason_StoppedAndContinue
			|| in_Reason == AkMonitorData::NotificationReason_EndReachedAndContinue
			)
		{
			return;
		}
	}

	if ( in_bUpdateCount )
		in_Reason = (AkMonitorData::NotificationReason) ( in_Reason | AkMonitorData::NotificationReason_PlayCountBit );

	MONITOR_OBJECTNOTIF(m_UserParams.PlayingID(), m_pGameObj->ID(), m_UserParams.CustomParam(), in_Reason, m_CntrHistArray, id, false, 0 );
}
#endif
//====================================================================================================
// get the current play stop transition
//====================================================================================================
CAkTransition* CAkPBI::GetPlayStopTransition()
{
	return m_PBTrans.pvPSTrans;
}

//====================================================================================================
// get the current pause resume transition
//====================================================================================================
CAkTransition* CAkPBI::GetPauseResumeTransition()
{
	return m_PBTrans.pvPRTrans;
}

void CAkPBI::SetPauseStateForContinuous(bool)
{
	//empty function for CAkPBI, overriden by continuous PBI
}

void CAkPBI::SetEstimatedLength( AkReal32 )
{
	//nothing to do, implemented only for continuousPBI
}

void CAkPBI::PrepareSampleAccurateTransition()
{
	//nothing to do, implemented only for continuousPBI
}

void CAkPBI::Destroy( AkCtxDestroyReason in_eReason )
{
	// IMPORTANT: Clear Cbx now because other commands may refer to this PBI, 
	// during the same audio frame, between now and when the PBI is actually destroyed.
	m_pCbx = NULL;

	CAkURenderer::EnqueueContextNotif( this, CtxStateToDestroy, in_eReason );
}

void CAkPBI::Play( AkReal32 in_fDuration )
{
	m_State = CtxStatePlay;
	CAkURenderer::EnqueueContextNotif( this, CtxStatePlay, CtxDestroyReasonFinished, in_fDuration );
}

void CAkPBI::Stop()
{
	m_State = CtxStateStop;

	//Commented out since unused
	//CAkURenderer::EnqueueContextNotif( this, CtxStateStop );
}

void CAkPBI::Pause()
{
	m_State = CtxStatePause;
	CAkURenderer::EnqueueContextNotif( this, CtxStatePause );
}

void CAkPBI::Resume()
{
	m_State = CtxStatePlay;
	CAkURenderer::EnqueueContextNotif( this, CtxStateResume );
}

void CAkPBI::NotifAddedAsSA()
{
	m_State = CtxStatePlay;
}

void CAkPBI::ProcessContextNotif( AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason, AkReal32 in_fEstimatedLength )
{
	switch( in_eState )
	{
	case CtxStatePlay:
		Monitor(AkMonitorData::NotificationReason_Play);
		m_bNeedNotifyEndReached = true;
		PrepareSampleAccurateTransition();
		SetEstimatedLength( in_fEstimatedLength );
		break;
	case CtxStateToDestroy:
		if( in_eDestroyReason == CtxDestroyReasonPlayFailed )
		{
			m_bNeedNotifyEndReached = false;
			m_bPlayFailed = true;
			Monitor(AkMonitorData::NotificationReason_PlayFailed);
		}
		break;
	case CtxStatePause:
		Monitor(AkMonitorData::NotificationReason_Paused);
		break;
	case CtxStateResume:
		Monitor(AkMonitorData::NotificationReason_Resumed);
		break;
	default:
		AKASSERT( !"Unexpected Context notification" );
		break;
	}
}

AkUniqueID CAkPBI::GetSoundID() const
{
	return m_pSound->ID();
}

CAkBus* CAkPBI::GetOutputBusPtr()
{
	CAkBus* pBusOutput = NULL;
#ifdef AK_MOTION
	if(m_bTargetIsFeedback)
	{
		pBusOutput = m_pSound->GetFeedbackParentBusOrDefault();
	}
	else
#endif // AK_MOTION
	{
		pBusOutput = m_pSound->GetMixingBus();

		///////////////////////////////////////////////////////
		// This is the groundBreaking change
		// if null, passing the MasterBusPointer here will have an impact at many levels, dont check-in before everything else is settled.
		///////////////////////////////////////////////////////
		if( !pBusOutput )
		{
			//pBusOutput = GetMasterBus();
		}
		///////////////////////////////////////////////////////
	}
	return pBusOutput;
}

AkUniqueID CAkPBI::GetBusOutputProfilingID()
{
	CAkBus* pBusOutputProfiling = GetOutputBusPtr();
	return pBusOutputProfiling ? pBusOutputProfiling->ID() : AK_INVALID_UNIQUE_ID;
}

void CAkPBI::RefreshParameters()
{
	m_EffectiveParams.ClearEx();

	// Make sure we start with an empty map before calling
	// GetAudioParameters(), to avoid keeping obsolete entries.
    // Note: Only volatile map items are removed. Persistent items are never obsolete.
	RemoveAllVolatileMuteItems();

	m_pSound->UpdateBaseParams( m_pGameObj, &m_BasePosParams, m_p3DSound );

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	if ( m_pCbx )
		m_pCbx->RefreshBypassFx();

	m_bBypassAllFX = m_pSound->GetBypassAllFX( m_pGameObj );
#endif

#ifdef AK_MOTION
	if (m_bTargetIsFeedback)
	{
		CAkFeedbackNode* pFeedbackNode = static_cast<CAkFeedbackNode*>(m_pSound);
		m_EffectiveParams.Volume = pFeedbackNode->GetSourceVolumeOffset(m_pSource);
		GetFeedbackParameters();
	}
#endif // AK_MOTION

	///////////////////////////////////
	// Must reset previous Audio parameters so newly added Aux busses can be added.
	GetPrevPosParams().Invalidate();
	///////////////////////////////////

	AkUInt32 eDesiredParams = PT_All;
	if ( !IsHDR() )
		eDesiredParams &= ~( PT_HDR );

	m_pSound->GetAudioParameters( 
		m_EffectiveParams,
		eDesiredParams,
		m_mapMutedNodes,
		m_pGameObj,
		!m_bGetAudioParamsCalled,
		m_Ranges );
	m_bGetAudioParamsCalled = true;
	m_EffectiveParams.Pitch	+= m_Ranges.PitchOffset;
	
	m_LPF = m_EffectiveParams.LPF + m_Ranges.LPFOffset;
	CalculateEffectiveLPF();

	m_Volume				= m_EffectiveParams.Volume;
	CalculateMutedEffectiveVolume();

#ifdef AK_MOTION
	if( IsForFeedbackPipeline() && m_pFeedbackInfo )
	{
		m_EffectiveParams.Pitch += m_pFeedbackInfo->m_MotionBusPitch;
	}
#endif // AK_MOTION

	PriorityInfo priorityInfo = m_pSound->GetPriority( m_pGameObj );

	if( priorityInfo.priority != m_PriorityInfoCurrent.BasePriority() 
		|| priorityInfo.distanceOffset != m_PriorityInfoCurrent.DistanceOffset() )
	{
		m_PriorityInfoCurrent.Reset( priorityInfo );
		UpdatePriority( priorityInfo.priority );
	}

	m_bAreParametersValid = true;
}

void CAkPBI::UpdatePriority( AkReal32 in_NewPriority )
{
	// First update all the lists, and only then change the priority.
	// The priority is the key to find the object and must not be changed before things have been reordered.
	AkReal32 fLastPriority = GetPriorityFloat();
	if( fLastPriority != in_NewPriority )
	{
		////////////////////////////////////////////
		// Limiters
		////////////////////////////////////////////
		if( m_pAMLimiter )
		{
			m_pAMLimiter->Update( in_NewPriority, this );
		}
		if( m_pBusLimiter )
		{
			m_pBusLimiter->Update( in_NewPriority, this );
		}
		CAkURenderer::GetGlobalLimiter().Update( in_NewPriority, this );
		////////////////////////////////////////////

		m_PriorityInfoCurrent.SetCurrent( in_NewPriority );
	}
}

void CAkPBI::ComputePriorityOffset( 
	AkReal32 in_fMinDistance,
	Gen3DParams * in_p3DParams
	)
{
	AKASSERT( in_p3DParams );

	AkReal32 distanceOffsetTemp = m_PriorityInfoCurrent.DistanceOffset();
	AkReal32 priorityTemp = m_PriorityInfoCurrent.BasePriority();

	if ( distanceOffsetTemp != 0.0f )
	{
		// Priority changes based on distance to listener

		//Get curve max radius
		CAkAttenuation* pAttenuation = in_p3DParams->GetAttenuation();
		if( pAttenuation )
		{
			CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
			if( pVolumeDryCurve )
			{
				AkReal32 fMaxDistance = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From;
				if ( in_fMinDistance < fMaxDistance && fMaxDistance > 0 ) 
					priorityTemp += in_fMinDistance/fMaxDistance * distanceOffsetTemp;
				else
					priorityTemp += distanceOffsetTemp;
			}
		}

		priorityTemp = AkMath::Min( AK_MAX_PRIORITY, AkMax( AK_MIN_PRIORITY, priorityTemp ) );
	}
	UpdatePriority( priorityTemp );
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)

	m_bDoPriority = m_PriorityInfoCurrent.GetCurrent() != priorityTemp;
#endif
}

bool CAkPBI::IsMultiPositionTypeMultiSources()
{
	if ( (AkPannerType)m_ePannerType == Ak3D && (AkPositionSourceType)m_ePosSourceType == AkGameDef )
	{
		AKASSERT( m_p3DSound );
		Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
		AKASSERT( l_p3DParams != NULL );
		const AkSoundPositionRef & posRef = ( l_p3DParams->m_bIsDynamic ) ? GetGameObjectPtr()->GetPosition() : m_cachedGameObjectPosition;
		return ( posRef.GetMultiPositionType() == AK::SoundEngine::MultiPositionType_MultiSources );
	}
	return false;
}

AKRESULT CAkPBI::GetGameObjectPosition(
	AkUInt32 in_uIndex,
	AkSoundPosition & out_position
	) const
{
	if ( in_uIndex < GetNumGameObjectPositions() )
	{
		out_position = GetGameObjectPtr()->GetPosition().GetPositions()[in_uIndex];
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT CAkPBI::GetListenerData(
	AkUInt32 in_uListenerMask,
	AkListener & out_listener
	) const
{
	AkUInt32 uMask = GetGameObjectPtr()->GetPosition().GetListenerMask();
	in_uListenerMask = in_uListenerMask & uMask;
	if ( in_uListenerMask )
	{
		AkUInt8 uListener = 0;
		while ( !( in_uListenerMask & 1 ) )
		{
			in_uListenerMask >>= 1;
			++uListener;
		}

		out_listener = CAkListener::GetListenerData( uListener );
		return AK_Success;
	}
	return AK_Fail;
}

// Helper: Computes ray for given emitter position and listener data.
static AkReal32 ComputeRay( 
	const AkListenerData & in_listenerData,
	const AkSoundPosition & in_emitterPosition,
	AkReal32 in_fGameObjectScalingFactor,
	bool in_bComputePositionAngles,
	bool in_bComputeEmitterDirectionAngle,
	AkRayVolumeData & out_ray
	)
{
	AkVector vPosFromListener;	// Position in the referential of the listener.
	const AkListenerPosition & listenerPosition = in_listenerData.position;
	const AkVector & listenerPositionVector = listenerPosition.Position;
	const AkVector & positionVector = in_emitterPosition.Position;
	vPosFromListener.X = positionVector.X - listenerPositionVector.X;
	vPosFromListener.Y = positionVector.Y - listenerPositionVector.Y;
	vPosFromListener.Z = positionVector.Z - listenerPositionVector.Z;

	AkReal32 fDistance = AkMath::Magnitude( vPosFromListener );
	// Distance is scaled by both the listener and game object scaling factor.
	AkReal32 fScaledDistance = fDistance / ( in_fGameObjectScalingFactor * in_listenerData.fScalingFactor );
	out_ray.SetDistance( fScaledDistance );
	
	// Compute position angles only if the game object and listener are spatialized.
	if ( in_bComputePositionAngles )
	{
		// Compute angles relative to listener: rotate the vector from listener to emitter
		// according to the inverse of listener orientation. The xz-plane (phi=0) corresponds to no elevation.
		AkVector vRotatedPosition;
		const AkReal32 * pRotationMatrix = &in_listenerData.Matrix[0][0];
		AkMath::UnrotateVector( vPosFromListener, pRotationMatrix, vRotatedPosition );
		
		AkReal32 fAzimuth, fElevation;
		CAkSpeakerPan::CartesianToSpherical( vRotatedPosition.X, vRotatedPosition.Z, vRotatedPosition.Y, fDistance, fAzimuth, fElevation );
		out_ray.SetAngles( fAzimuth, fElevation );
	}
		
	// Compute angle between direction of sound and the [listener,sound] vector
	// for cone attenuation (if applicable).
	if ( in_bComputeEmitterDirectionAngle ) 
	{
		// compute angle between direction of sound and the [listener,sound] vector
		// Note: Negate projection value since vPosFromListener is the vector that goes
		// away from the listener.
		AkReal32 fAngle;
		if ( fDistance > 0.0f )
			fAngle = -AkMath::DotProduct( vPosFromListener, in_emitterPosition.Orientation ) / fDistance;
		else
			fAngle = 1.0f; // front

		// we can sometime get something slightly over 1.0
		fAngle = AkMath::Min(fAngle, 1.0f);
		fAngle = AkMath::Max(fAngle, -1.0f);

		/// TODO PERF arccos approx.
		fAngle = AkMath::ACos(fAngle);

		out_ray.SetEmitterAngle( fAngle );
	}

	return fScaledDistance;
}

// Get the ith emitter-listener pair. The returned pair is fully computed (distance and angles).
AKRESULT CAkPBI::GetEmitterListenerPair( 
	AkUInt32 in_uIndex,
	AkEmitterListenerPair & out_emitterListenerPair
	)
{
	AkRayVolumeData ray;
	bool bComputePositionAngles = true;
	bool bComputeEmitterDirection = true;

	// Look in game object cache. We need a full-featured cached object.
	if ( GetGameObjectPtr()->IsPositionCached() )
	{
		// Find desired pair.
		AkUInt32 uPair = 0;
		const AkEmitterListenerPairArray & arEmitListenPairs = GetGameObjectPtr()->GetCachedEmitListenPairs();
		AkEmitterListenerPairArray::Iterator it = arEmitListenPairs.Begin();
		while ( uPair != in_uIndex
				&& it != arEmitListenPairs.End() )
		{
			++it;
			++uPair;
		}

		if ( it != arEmitListenPairs.End() )
		{
			// Found it. Copy, and check if there are missing features.
			ray.Copy( *it );
			bComputePositionAngles = !ray.ArePositionAnglesSet();
			bComputeEmitterDirection = !ray.IsEmitterAngleSet();
		}

		if ( !bComputePositionAngles && !bComputeEmitterDirection )
		{
			// Found it, and it is feature-complete. Bail out.
			ray.CopyTo( out_emitterListenerPair );
			return AK_Success;
		}
	}

	// Not found. Compute full featured emitter-listener pair.
	{
		const AkSoundPositionRef & posRef = GetGameObjectPtr()->GetPosition();

		// Find desired pair.
		AkUInt32 uPair = 0;
		AkUInt32 uNumPosition = posRef.GetNumPosition();

		AkUInt32 uMask = posRef.GetListenerMask();

		AkUInt32 uListener = 0;
		while ( uMask )
		{
			if ( !( uMask & 1 ) )
			{
				uMask >>= 1;
				++uListener;
				continue; // listener not active for this sound
			}

			for( AkUInt32 uCurrentPosition = 0; uCurrentPosition < uNumPosition; ++uCurrentPosition )
			{
				if ( in_uIndex == uPair )
				{
					// This is the desired pair.
					AkUInt8 uActiveListenerMask = ( 1 << uListener );
					ray.SetListenerMask( uActiveListenerMask );
					
					AkReal32 fScaledDistance = ComputeRay(
						CAkListener::GetListenerData( uListener ),
						posRef.GetPositions()[uCurrentPosition],
						GetGameObjectPtr()->GetScalingFactor(),
						bComputePositionAngles,
						bComputeEmitterDirection,
						ray );

					ray.CopyTo( out_emitterListenerPair );

					GetGameObjectPtr()->CacheEmitListenPair( ray );
					
					return AK_Success;
				}
				++uPair;
			}
		}
	}

	return AK_Fail;
}

// Compute volume data rays for 3D positioning. All emitter-listener pairs (rays) are computed
// and cached on game object if applicable. 
// Returns the number of rays.
AkUInt32 CAkPBI::ComputeVolumeData3D( 
	AkPositionSourceType in_eType,		// Useless but since we usually have it on the stack...
	AkVolumeDataArray & out_arVolumeData	
	)
{
	// Empty array (do not free/resize it, 99% of the time it will have and need only 1 item).
	out_arVolumeData.RemoveAll();

	AKASSERT( m_p3DSound != NULL );
	Gen3DParams * l_p3DParams = m_p3DSound->GetParams();

	CAkAttenuation * pAttenuation = l_p3DParams->GetAttenuation();
	bool bRequiresCone = ( pAttenuation && pAttenuation->m_bIsConeEnabled );

	if ( in_eType == AkGameDef ) 
	{
		// Cache game object position the first time.
		if( !l_p3DParams->m_bIsDynamic && !m_bGameObjPositionCached )
		{
			if ( m_cachedGameObjectPosition.Copy( GetGameObjectPtr()->GetPosition() ) == AK_Success )
				m_bGameObjPositionCached = true;
		}

		AkReal32 fMinDistance = AK_UPPER_MAX_DISTANCE;

		if ( GetGameObjectPtr()->IsPositionCached()
			&& l_p3DParams->m_bIsDynamic )
		{
			// Try to use cached rays.				
			const AkEmitterListenerPairArray & arEmitListenPairs = GetGameObjectPtr()->GetCachedEmitListenPairs();
			AkEmitterListenerPairArray::Iterator it = arEmitListenPairs.Begin();
			AKASSERT( it != arEmitListenPairs.End() );	// Would not be cached if it was empty.
				
			// Important: position and cone angles are not necessarily set. If they aren't and this sound requires them,
			// recompute rays from scratch.
			/// OPTI Currently cached data will not be used if listeners are not spatialized. 
			if ( ( (*it).ArePositionAnglesSet() || !l_p3DParams->m_bIsSpatialized )
				&& ( (*it).IsEmitterAngleSet() || !bRequiresCone ) )
			{
				// Angle is set or not needed.
				do
				{
					AkRayVolumeData * pRay = out_arVolumeData.AddLast();
					if ( !pRay )
						break;	// Cannot process volume for this ray. No memory, so bail out now.

					pRay->Copy( *it );

					// Get min distance for priority computation.
					AkReal32 fDistance = ( *it ).Distance();
					if ( fDistance < fMinDistance )
						fMinDistance = fDistance;

					++it;
				}
				while ( it != arEmitListenPairs.End() );

				ComputePriorityOffset( fMinDistance, l_p3DParams );
				return out_arVolumeData.Length();
			}
		}

		// Get position from game object or from cache (if "update at each frame" unticked).
		const AkSoundPositionRef & posRef = ( l_p3DParams->m_bIsDynamic ) ? GetGameObjectPtr()->GetPosition() : m_cachedGameObjectPosition;

		// Transform all source positions of all active listeners to an array of emitter-listener pairs.

		AkUInt32 uNumPosition = posRef.GetNumPosition();
		AkUInt32 uMask = posRef.GetListenerMask();
		AkUInt32 uNumListeners = AK::GetNumChannels( uMask );
		AkUInt32 uNumRays = uNumPosition * uNumListeners;
		if ( uNumRays > out_arVolumeData.Reserved() )
		{
			if ( !out_arVolumeData.GrowArray( uNumRays - out_arVolumeData.Reserved() ) )
				return 0;
		}

		AkUInt32 uListener = 0;
		while ( uMask )
		{
			if ( !( uMask & 1 ) )
			{
				uMask >>= 1;
				++uListener;
				continue; // listener not active for this sound
			}

			AkUInt8 uActiveListenerMask = ( 1 << uListener );
			const AkListenerData & listenerData = CAkListener::GetListenerData( uListener );
			
			for( AkUInt32 uCurrentPosition = 0; uCurrentPosition < uNumPosition; ++uCurrentPosition )
			{
				AkRayVolumeData * pRay = out_arVolumeData.AddLast();
				AKASSERT( pRay );	// Should have been reserved.
				
				pRay->SetListenerMask( uActiveListenerMask );

				AkReal32 fScaledDistance = ComputeRay(
					listenerData,
					posRef.GetPositions()[uCurrentPosition],
					GetGameObjectPtr()->GetScalingFactor(),
					( l_p3DParams->m_bIsSpatialized && listenerData.bSpatialized ),
					bRequiresCone,
					*pRay );

				// For priority handling.
				if ( fScaledDistance < fMinDistance )
					fMinDistance = fScaledDistance;
			}

			++uListener;
			uMask >>= 1;
		}

		// Cache data if applicable.
		if ( l_p3DParams->m_bIsDynamic )
			GetGameObjectPtr()->CacheEmitListenPairs( out_arVolumeData );

		ComputePriorityOffset( fMinDistance, l_p3DParams );
	}
	else
	{
		if( m_PathInfo.pPBPath )
		{
			// this one might have been changed so pass it along
			bool bIsLooping = l_p3DParams->m_bIsLooping;
			m_PathInfo.pPBPath->SetIsLooping(bIsLooping);

			if(bIsLooping
				&& m_PathInfo.pPBPath->IsContinuous()
				&& m_PathInfo.pPBPath->IsIdle())
			{
				g_pPathManager->Start(m_PathInfo.pPBPath,m_pSound->GetPathState());
			}
		}

		// Create rays.
		AkUInt32 uMask = GetGameObjectPtr()->GetPosition().GetListenerMask();
		AkUInt32 uListener = 0;

		// Idx for accessing cached listener rotation matrix (no follow mode).
		AkUInt32 uCachedListenerRotationIdx = 0;

		while ( uMask )
		{
			if ( !( uMask & 1 ) )
			{
				uMask >>= 1;
				++uListener;
				continue; // listener not active for this sound
			}

			AkRayVolumeData * pRay = out_arVolumeData.AddLast();
			if ( !pRay )
				return out_arVolumeData.Length();	// Cannot process volume for this ray.

			pRay->SetListenerMask( 1 << uListener );

			const AkListenerData & listenerData = CAkListener::GetListenerData( uListener );

			AkReal32 fDistance = AkMath::Magnitude( l_p3DParams->m_Position );
			// Note: in user defined positioning, distance is only scaled by the game object scaling factor.
			AkReal32 fScaledDistance = fDistance / ( GetGameObjectPtr()->GetScalingFactor() * listenerData.fScalingFactor );
			pRay->SetDistance( fScaledDistance );

			bool bSpatialized = l_p3DParams->m_bIsSpatialized && listenerData.bSpatialized;
			if ( bSpatialized || bRequiresCone )
			{
				// Get position from user-defined path. Rotate it in "no follow" mode.
				AkVector vRotatedPosition;

				if ( l_p3DParams->m_bFollowOrientation
					|| !m_PathInfo.pPBPath )
				{
					// No rotation to do.
					vRotatedPosition = l_p3DParams->m_Position;
				}
				else
				{
					// Multiply current listener position with the inverted rotation matrix that was 
					// stored when the path was created in order to get the relative rotation.
					AkReal32 mxRotationNoFollow[3][3];
					AkMath::MatrixMul3by3(
						CAkListener::GetListenerMatrix( uListener ), 
						m_PathInfo.pPBPath->GetNoFollowRotationMatrix( uCachedListenerRotationIdx ), 
						&mxRotationNoFollow[0][0]); 

					// The matrix computed above represents the relative rotation of the listener since 
					// the "path" started. In order to simulate a non following spot effect, rotate the position 
					// vector of the source in the opposite direction by multiplying it by the transpose 
					// of the listener's relative rotation.
					AkMath::UnrotateVector( l_p3DParams->m_Position, &mxRotationNoFollow[0][0], vRotatedPosition );
					++uCachedListenerRotationIdx;
				}

				// Compute
				if ( bSpatialized )
				{
					AkReal32 fAzimuth, fElevation;
					CAkSpeakerPan::CartesianToSpherical( vRotatedPosition.X, vRotatedPosition.Z, vRotatedPosition.Y, fDistance, fAzimuth, fElevation );
					pRay->SetAngles( fAzimuth, fElevation );
				}
				
				// Compute angle between direction of sound and the [listener,sound] vector
				// for cone attenuation (if applicable).				
				if ( bRequiresCone )
				{
					// In user defined mode, the cone is computed around the listener. Thus, it is the 
					// listener orientation that we need to project on the position vector.
					// In "follow orientation" mode, the position is always the default one. 
					AkReal32 fAngle;
					if ( fDistance > 0.0f )
					{
						AkVector vecListOrientation = { AK_DEFAULT_LISTENER_FRONT_X, AK_DEFAULT_LISTENER_FRONT_Y, AK_DEFAULT_LISTENER_FRONT_Z };
						fAngle = AkMath::DotProduct( vRotatedPosition, vecListOrientation ) / fDistance;
					}
					else
						fAngle = 1.0f; // front

					// we can sometime get something slightly over 1.0
					fAngle = AkMath::Min(fAngle, 1.0f);
					fAngle = AkMath::Max(fAngle, -1.0f);

					/// TODO PERF arccos approx.
					fAngle = AkMath::ACos(fAngle);

					pRay->SetEmitterAngle( fAngle );
				}
			}
				
			++uListener;
			uMask >>= 1;
		}
	}
	
	return out_arVolumeData.Length();
}

// Call this instead of ComputeVolumeData when "virtualized".
void CAkPBI::VirtualPositionUpdate()
{
	AKASSERT( IsForcedVirtualized() );

	if (m_ePannerType == Ak2D)
	{
		// Make sure 2d volumes are recalculated when we come back.
		GetPrevPosParams().Invalidate();
	}
	else
	{
		if (m_ePosSourceType == AkGameDef)
		{
			Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
			AKASSERT( l_p3DParams != NULL );

			// Compute distance and update priority.
			if( l_p3DParams->m_bIsDynamic || !m_bGameObjPositionCached )
			{
				const AkSoundPositionRef & posEntry = m_pGameObj->GetPosition();
				AkReal32 fMinDistance = CAkURenderer::GetMinDistance( posEntry ) / GetGameObjectPtr()->GetScalingFactor();
				
				Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
				AKASSERT( l_p3DParams != NULL );
				
				ComputePriorityOffset( fMinDistance, l_p3DParams );
			}
		}
		else
		{
			if( m_PathInfo.pPBPath )
			{
				Gen3DParams * l_p3DParams = m_p3DSound->GetParams();
				AKASSERT( l_p3DParams != NULL );

				// this one might have been changed so pass it along
				bool bIsLooping = l_p3DParams->m_bIsLooping;
				m_PathInfo.pPBPath->SetIsLooping(bIsLooping);

				if(bIsLooping
					&& m_PathInfo.pPBPath->IsContinuous()
					&& m_PathInfo.pPBPath->IsIdle())
				{
					g_pPathManager->Start(m_PathInfo.pPBPath,m_pSound->GetPathState());
				}
			}
		}
	}
}	

bool CAkPBI::IsInitiallyUnderThreshold( AkVolumeDataArray & /**out_arVolumeData**/ )
{
	//must gather params since that have never been queried yet
	CalcEffectiveParams();

	// Can only test against fade. The rest is bogus if we don't perform a graph analysis.
	/// TODO Get rid of list of sources not connected, connect sources all the time but don't AddPipeline
	/// until needed. Connect, fetch gains, test for initially under threshold, and then start stream. AddPipeline in StartRun().
	/// TEMP Compute initially under threshold as it was done before.
	AkReal32 fVoiceVolume = ComputeCollapsedVoiceVolume();
	return ( !IsAuxRoutable() ) ? 
				( ( fVoiceVolume * GetOutputBusVolumeValue() ) <= g_fVolumeThreshold )
				: ( fVoiceVolume <= g_fVolumeThreshold );
}

void CAkPBI::GetAuxSendsValues( AkAuxSendValueEx* AK_RESTRICT io_paEnvVal )
{
	AKASSERT( m_pGameObj != NULL );

	const AkAuxSendValue * AK_RESTRICT pValues = m_pGameObj->GetGameDefinedAuxSends();

	AkUInt32 cValues = 0;
	if( IsGameDefinedAuxEnabled() )
	{
		for( AkUInt32 i = 0; i < AK_MAX_AUX_PER_OBJ; ++i )
		{
			AkAuxBusID auxBusID = pValues[i].auxBusID;

			if( auxBusID == AK_INVALID_AUX_ID )
				break;

			AkReal32 fLinGameAuxSendVolume = AkMath::dBToLin( m_EffectiveParams.fGameAuxSendVolume );
			AkReal32 fControlValue = fLinGameAuxSendVolume * pValues[i].fControlValue;
			if( fControlValue > g_fVolumeThreshold )
			{
				io_paEnvVal[cValues].auxBusID = auxBusID;
				io_paEnvVal[cValues].fControlValue = fControlValue;
				io_paEnvVal[cValues].eAuxType = AkAuxType_GameDef;

				++cValues;
			}
		}
	}

	for( AkUInt32 i = 0; i < AK_NUM_AUX_SEND_PER_OBJ && cValues < AK_MAX_AUX_SUPPORTED ; ++i )
	{
		AkAuxBusID auxID =  m_EffectiveParams.aAuxSend[i];
		if( auxID == AK_INVALID_AUX_ID )
			continue;

		if( m_EffectiveParams.aUserAuxSendVolume[i] > g_fVolumeThresholdDB )
		{
			io_paEnvVal[cValues].auxBusID =  m_EffectiveParams.aAuxSend[i];
			io_paEnvVal[cValues].fControlValue = AkMath::dBToLin( m_EffectiveParams.aUserAuxSendVolume[i] );
			io_paEnvVal[cValues].eAuxType = AkAuxType_UserDef;

			++cValues;
		}
	}

	if ( cValues < AK_MAX_AUX_SUPPORTED )
		io_paEnvVal[cValues].auxBusID = AK_INVALID_AUX_ID;

	if (IsForcedVirtualized())
	{
		//If the sound is force virtualized, set the control values to 0 so the aux volume fade out instead of click.
		for( AkUInt32 i = 0; i < cValues; ++i )
			io_paEnvVal[i].fControlValue = 0.f;
	}
}

AKRESULT CAkPBI::SubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation )
{
	AKRESULT eResult = AK_Success;

	const CAkAttenuation::RTPCSubsArray & list = in_pAttenuation->GetRTPCSubscriptionList();
	for( CAkAttenuation::RTPCSubsArray::Iterator iter = list.Begin(); iter != list.End(); ++iter )
	{
		CAkAttenuation::RTPCSubs& l_rRTPC = *iter;

		eResult = g_pRTPCMgr->SubscribeRTPC( this,
											l_rRTPC.RTPCID,
											l_rRTPC.ParamID,
											l_rRTPC.RTPCCurveID,
											l_rRTPC.ConversionTable.m_eScaling,
											l_rRTPC.ConversionTable.m_pArrayGraphPoints,
											l_rRTPC.ConversionTable.m_ulArraySize,
											m_pGameObj,
											CAkRTPCMgr::SubscriberType_PBI
											);
		if( eResult != AK_Success )
			break;
	}
	return eResult;
}

void CAkPBI::UnsubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation )
{
	const CAkAttenuation::RTPCSubsArray & list = in_pAttenuation->GetRTPCSubscriptionList();
	for( CAkAttenuation::RTPCSubsArray::Iterator iter = list.Begin(); iter != list.End(); ++iter )
	{
		CAkAttenuation::RTPCSubs& l_rRTPC = *iter;

		g_pRTPCMgr->UnSubscribeRTPC( this, l_rRTPC.ParamID );
	}
}

void CAkPBI::PausePath(bool in_bPause)
{
	if(m_PathInfo.pPBPath != NULL)
	{
		if(in_bPause)
		{
			g_pPathManager->Pause(m_PathInfo.pPBPath);
		}
		else
		{
			g_pPathManager->Resume(m_PathInfo.pPBPath);
		}
	}
}

void CAkPBI::Kick( KickFrom in_eIsForMemoryThreshold )
{
	if ( ! m_bWasKicked )
	{
		m_eWasKickedForMemory = in_eIsForMemoryThreshold;
		m_bWasKicked = true;
	}
    TransParams transParams;
    transParams.TransitionTime = 0;
    transParams.eFadeCurve = AkCurveInterpolation_Linear;
	_Stop( transParams, true );
}

AkUInt32 CAkPBI::GetStopOffset() const
{
	//Not zero since 0 is an acceptable stop
	return AK_NO_IN_BUFFER_STOP_REQUESTED; 
}

AkUInt32 CAkPBI::GetAndClearStopOffset()
{ 
	//Not zero since 0 is an acceptable stop
	return AK_NO_IN_BUFFER_STOP_REQUESTED; 
}

AkCtxVirtualHandlingResult CAkPBI::NotifyVirtualOff( AkVirtualQueueBehavior )
{
	// Instances of the actor-mixer hierarchy don't handle VirtualOff behaviors in any special way.
	return VirtualHandling_NotHandled;
}

void CAkPBI::GetFXDataID( AkUInt32 in_uFXIndex, AkUInt32 in_uDataIndex, AkUInt32& out_rDataID )
{
	GetSound()->GetFXDataID( in_uFXIndex, in_uDataIndex, out_rDataID );
}

void CAkPBI::UpdateFx(
	AkUInt32	   	in_uFXIndex
	)
{
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	if ( m_pCbx )
	{
		m_pCbx->UpdateFx( in_uFXIndex );
	}
#endif
}

#ifndef AK_OPTIMIZED

void CAkPBI::UpdateAttenuationInfo()
{
	AKASSERT( m_p3DSound );
	
	CAkAttenuation* pAttenuation = m_p3DSound->GetParams()->GetAttenuation();
	if( pAttenuation )
	{
		// Taking the value from the attenuation, if they are RTPC based, they will be overriden upon RTPC subscription, coming right after this.
		m_p3DSound->SetConeOutsideVolume( pAttenuation->m_ConeParams.fOutsideVolume );
		m_p3DSound->SetConeLPF( pAttenuation->m_ConeParams.LoPass );

		g_pRTPCMgr->UnSubscribeRTPC( this ); // Do not call UnsubscribeAttenuationRTPC( pAttenuation ); here: list may have changed since last subscription
		SubscribeAttenuationRTPC( pAttenuation );
	}
}

#endif //AK_OPTIMIZED

#ifdef AK_MOTION
// Feedback devices support
void CAkPBI::ValidateFeedbackParameters()
{
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if(pMgr == NULL || !pMgr->IsFeedbackEnabled() )
		return;
	
	//If there is at least one, allocate the structure
	if ( m_pFeedbackInfo == NULL )
	{
		if( m_pSound->FeedbackParentBus() != NULL || IsForFeedbackPipeline() )
		{
			//The structure is of variable size.  Depends on the number of channels of the input
			//and the number of connected players.
#if defined(AK_WII) || defined(AK_3DS)
			AkChannelMask uChanMask = GetMediaFormat().GetChannelMask();
#else
			AkChannelMask uChanMask = GetCbx()? ((CAkVPLSrcCbxNode*)(GetCbx()))->GetOutputChannelMask() : GetMediaFormat().GetChannelMask();
#endif
			
			m_pFeedbackInfo = AkFeedbackParams::Create(pMgr->GetPlayerCount(), uChanMask, (AkPannerType)m_ePannerType, (AkPositionSourceType)m_ePosSourceType);
		}
	}

	if (m_pFeedbackInfo != NULL)
	{
		m_pSound->GetFeedbackParameters(*m_pFeedbackInfo, m_pSource, m_pGameObj, true);
	}

	m_bFeedbackParametersValid = true;
}

void CAkPBI::InvalidateFeedbackParameters()
{
	if (m_pFeedbackInfo != NULL)
	{
		m_pFeedbackInfo->Destroy();
		m_pFeedbackInfo = NULL;

		m_bFeedbackParametersValid = false;	
	}
}
#endif // AK_MOTION

bool CAkPBI::IsUsingThisSlot( const CAkUsageSlot* in_pSlotToCheck )
{
	bool bRet = false;

	if( m_pUsageSlot == in_pSlotToCheck )
	{
		if( !FindAlternateMedia( in_pSlotToCheck ) )
		{
			// Found something using this data and could not change it successfully, must stop it!
			bRet = true;
		}
	}
	
	if( !bRet )
	{
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)

		// We still have to check for potential effects that could be using this slot using Plugin Media.

		if( m_pCbx )
		{
			bRet = m_pCbx->IsUsingThisSlot( in_pSlotToCheck );
		}
#endif
	}

	return bRet;
}

bool CAkPBI::IsUsingThisSlot( const AkUInt8* in_pData )
{
	bool bRet = false;

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)

	// We still have to check for potential effects that could be using this slot using Plugin Media.

	if( m_pCbx )
	{
		bRet = m_pCbx->IsUsingThisSlot( in_pData );
	}
#endif

	return bRet;
}

bool CAkPBI::FindAlternateMedia( const CAkUsageSlot* in_pSlotToCheck )
{
	//Hardware only pipelines never support HotSwapping (media relocation)
#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS) && !defined(AK_VITA_HW)
	CAkVPLSrcNode* pSrcCtx = NULL;
	if( m_pCbx )
	{
		for( AkUInt32 i = 0; i < MAX_NUM_SOURCES; ++i )
		{
			CAkVPLSrcNode** ppSources =  m_pCbx->GetSources();
			// Must find which source is the good one as we may be the sample accurate pending one.
			if( ppSources[i] && ppSources[i]->GetContext() == this )
			{
				pSrcCtx = ppSources[i];
			}
		}
	}

	if( pSrcCtx )
	{
		if( pSrcCtx->SupportMediaRelocation() )
		{
			AkUInt8* pData = NULL;
			AkUInt32 uSize = 0;
			CAkUsageSlot* pSlot = NULL;
			m_pSource->LockDataPtr( (void*&)pData, uSize, pSlot );
			if( pData )
			{
				if( pSrcCtx->RelocateMedia( pData, m_pDataPtr ) == AK_Success )
				{
					if( pSrcCtx->MustRelocatePitchInputBufferOnMediaRelocation() )
					{
						m_pCbx->RelocateMedia( pData, m_pDataPtr );
					}
					if( pSrcCtx->MustRelocateAnalysisDataOnMediaRelocation() )
					{
						pSrcCtx->RelocateAnalysisData( pData, m_pDataPtr );
					}
					// Here Release old data and do swap in PBI to.
					if ( m_pDataPtr )
					{
						m_pSource->UnLockDataPtr();
					}
					if( m_pUsageSlot )
					{
						m_pUsageSlot->Release( false );
					}
					m_pDataPtr = pData;
					m_pUsageSlot = pSlot;

					return true;
				}
				else
				{
					//It is quite unexpected that a call to RelocateMedia would fail, but just in case we still need to release.
					if ( pData )
						m_pSource->UnLockDataPtr();
					if( pSlot )
						pSlot->Release( false );
				}
			}
		}
	}
#endif

	return false;
}

void CAkPBI::ForceVirtualize( KickFrom in_eReason )
{
	AkVirtualQueueBehavior _unused; 
	switch( GetVirtualBehavior( _unused ) )
	{
	case AkBelowThresholdBehavior_KillVoice:
		Kick( in_eReason );
		break;

	case AkBelowThresholdBehavior_SetAsVirtualVoice:
		ForceVirtualize();
		break;

	case AkBelowThresholdBehavior_ContinueToPlay:
		// Ignoring the order to go virtual
		break;
	}
}

void CAkPBI::Virtualize()
{
	if( !m_bIsVirtual )
	{
		m_bIsVirtual = true;
		CAkURenderer::IncrementVirtualCount();

		CounterParameters counterParams;
		counterParams.pGameObj = m_pGameObj;
		m_pSound->IncrementVirtualCount( counterParams );
	}
}

void CAkPBI::Devirtualize( bool in_bAllowKick )
{
	if( m_bIsVirtual )
	{
		m_bIsVirtual = false;
		CAkURenderer::DecrementVirtualCount( in_bAllowKick );

		CounterParameters counterParams;
		counterParams.pGameObj = m_pGameObj;
		counterParams.bAllowKick = in_bAllowKick;
		m_pSound->DecrementVirtualCount( counterParams );
	}
}

#ifdef AK_VITA_HW
void CAkPBI::SetHardware()
{
	AkSrcTypeInfo * pSrcType = GetSrcTypeInfo();
	AkSrcType eType = (AkSrcType) pSrcType->mediaInfo.Type;
	AkUInt32 uiID = pSrcType->dwID;
	
	// BALARY todo: rename m_bHardwareRenderedVoice
	m_bHardwareVoice = ( eType != SrcTypeModelled && eType != SrcTypeNone &&
		( CODECID_FROM_PLUGINID( uiID ) == AKCODECID_VAG ||
		  CODECID_FROM_PLUGINID( uiID ) == AKCODECID_ATRAC9 ||
		  CODECID_FROM_PLUGINID( uiID ) == AKCODECID_PCM ) );
}
#endif
	
#ifndef AK_OPTIMIZED
// Walk through hierarchy to determine status of monitoring mute.
void CAkPBI::RefreshMonitoringMute()	// True if at least one node is soloed in the hierarchy.
{
	if ( !CAkParameterNodeBase::IsMonitoringMuteSoloActive() )
		m_bIsMonitoringMute = false;
	else
	{
		bool bSolo = false;
		bool bMute = false;
		m_pSound->GetMonitoringMuteSoloState( 
// Solo Mute Split controls on aux busses is not totally supported yet on hardware platforms.
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS) || defined( AK_VITA_HW )
			true,
#else
			false,
#endif
			bSolo, 
			bMute );
		
		m_bIsMonitoringMute = ( bMute || ( CAkParameterNodeBase::IsMonitoringSoloActive() && !bSolo ) );
	}
}
#endif

bool CAkPBI::IsAuxRoutable()
{
	AKASSERT( m_bAreParametersValid );

	//////////////////////////////////////////////////////////////////////////
	// This function should return true if there is an Aux bus set in Wwise 
	// or if it is possible to use Game Defined Aux Sends.
	//////////////////////////////////////////////////////////////////////////
	return IsGameDefinedAuxEnabled() || HasUserDefineAux();
}

bool CAkPBI::HasUserDefineAux()
{
	for( int i = 0; i < AK_NUM_AUX_SEND_PER_OBJ; ++i )
	{
		if( m_EffectiveParams.aAuxSend[i] != AK_INVALID_UNIQUE_ID )
			return true;
	}
	return false;
}

AkReal32 CAkPBI::GetLoopStartOffsetSeconds() const
{
	return GetSound()->LoopStartOffset();
}

 //Seconds from the loop end to the end of the file
AkReal32 CAkPBI::GetLoopEndOffsetSeconds() const
{
	return GetSound()->LoopEndOffset();
}

AkUInt32 CAkPBI::GetLoopStartOffsetFrames() const
{
	AKASSERT( GetMediaFormat().uSampleRate != 0 );
	return (AkUInt32)floor( GetSound()->LoopStartOffset() * (AkReal32)GetMediaFormat().uSampleRate + 0.5f );
}

 //Seconds from the loop end to the end of the file
AkUInt32 CAkPBI::GetLoopEndOffsetFrames() const
{
	AKASSERT( GetMediaFormat().uSampleRate != 0 );
	return (AkUInt32)floor( GetSound()->LoopEndOffset() * (AkReal32)GetMediaFormat().uSampleRate + 0.5f );
}

AkReal32 CAkPBI::GetLoopCrossfadeSeconds() const
{
	return GetSound()->LoopCrossfadeDuration();
}

void CAkPBI::LoopCrossfadeCurveShape( AkCurveInterpolation& out_eCrossfadeUpType,  AkCurveInterpolation& out_eCrossfadeDownType) const
{
	GetSound()->LoopCrossfadeCurveShape(out_eCrossfadeUpType, out_eCrossfadeDownType);
}

void CAkPBI::GetTrimSeconds(AkReal32& out_fBeginTrim, AkReal32& out_fEndTrim) const
{
	GetSound()->GetTrim( out_fBeginTrim, out_fEndTrim );
}

void CAkPBI::GetSourceFade(	AkReal32& out_fBeginFadeOffsetSec, AkCurveInterpolation& out_eBeginFadeCurveType, 
					AkReal32& out_fEndFadeOffsetSec, AkCurveInterpolation& out_eEndFadeCurveType  )
{
	GetSound()->GetFade(	out_fBeginFadeOffsetSec, out_eBeginFadeCurveType, 
					out_fEndFadeOffsetSec, out_eEndFadeCurveType  );
}
