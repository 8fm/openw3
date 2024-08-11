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
// AkURenderer.cpp
//
// Implementation of the Audio renderer
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMath.h"
#include "AkURenderer.h"
#include "AkTransitionManager.h"
#include "AkContinuousPBI.h"
#include "AkRTPCMgr.h"
#include "AkLEngine.h"
#include "AkRandom.h"
#include "AkRegisteredObj.h"
#include "AkRegistryMgr.h"
#include "AkSoundBase.h"
#include "AkSource.h"
#include "Ak3DListener.h"
#include "Ak3DParams.h"
#include "AkProfile.h"
#include "AkAudioLibTimer.h"
#include "AkEnvironmentsMgr.h"
#include "AkMonitor.h"
#include "AkAudioMgr.h"
#include "AkAudioLib.h"
#include "AkCritical.h"
#include "AkPlayingMgr.h"
#include "AkBankMgr.h"
#include "ActivityChunk.h"
#include "AkBus.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern CAkTransitionManager* g_pTransitionManager;
extern CAkPlayingMgr* g_pPlayingMgr;

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------
// List sizes.
#define MIN_NUM_CTX					64

#define MIN_NUM_PLAY_EVENT			16
#define MIN_NUM_RENDER_EVENT		64 // must not be zero

#define HARDCODED_MAX_NUM_SOUNDS_UNLIMITED	(0)
#define DEFAULT_MAX_NUM_SOUNDS	(256)

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
CAkURenderer::AkListCtxs			CAkURenderer::m_listCtxs;	 
CAkURenderer::AkContextNotifQueue	CAkURenderer::m_CtxNotifQueue;
AkUInt32							CAkURenderer::m_uNumVirtualizedSounds;

CAkURenderer::AkListLightLimiters	CAkURenderer::m_BusLimiters;
CAkURenderer::AkListLightLimiters	CAkURenderer::m_AMLimiters;
CAkLimiter							CAkURenderer::m_GlobalLimiter( DEFAULT_MAX_NUM_SOUNDS, true, true );

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Init()
{
	m_uNumVirtualizedSounds = 0;

	AKRESULT l_eResult = m_CtxNotifQueue.Init( MIN_NUM_RENDER_EVENT, AK_NO_MAX_LIST_SIZE );
	if( l_eResult != AK_Success ) return l_eResult;

	// Initialize the Lower Audio Engine. 
	l_eResult = CAkLEngine::Init();

    return l_eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate the object.
//-----------------------------------------------------------------------------
void CAkURenderer::Term()
{
	CAkLEngine::Term();

	DestroyAllPBIs();

	m_listCtxs.Term();

	m_CtxNotifQueue.Term();

	AKASSERT( m_uNumVirtualizedSounds == 0 );

	m_GlobalLimiter.Term(); // Must be terminated before m_Limiters.
	m_AMLimiters.Term();
	m_BusLimiters.Term();
} // Term

PriorityInfoCurrent CAkURenderer::_CalcInitialPriority( CAkSoundBase * in_pSound, CAkRegisteredObj * in_pGameObj, AkReal32& out_fMaxRadius )
{
	out_fMaxRadius = 0.f;
	PriorityInfoCurrent priorityInfoCurrent( in_pSound->GetPriority( in_pGameObj ) );
	if ( in_pSound->GetMaxRadius( out_fMaxRadius ) )
	{
		out_fMaxRadius *= in_pGameObj->GetScalingFactor();
		if( priorityInfoCurrent.DistanceOffset() )
		{
			AkReal32 fMinDistance = GetMinDistance( in_pGameObj->GetPosition() );

			AkReal32 priorityTemp;
			if ( fMinDistance < out_fMaxRadius )
				priorityTemp = priorityInfoCurrent.BasePriority() + ( fMinDistance/out_fMaxRadius * priorityInfoCurrent.DistanceOffset() );
			else
				priorityTemp = priorityInfoCurrent.BasePriority() + priorityInfoCurrent.DistanceOffset();

			priorityInfoCurrent.SetCurrent( AkMin( AK_MAX_PRIORITY, AkMax( AK_MIN_PRIORITY, priorityTemp ) ) );
		}
	}

	return priorityInfoCurrent;
}

AkReal32 CAkURenderer::GetMinDistance( const AkSoundPositionRef& in_rPosRef )
{
	AkReal32 fMinDistance = AK_UPPER_MAX_DISTANCE;

	unsigned int uMask = in_rPosRef.GetListenerMask();
	for ( unsigned int uListener = 0; uMask; ++uListener, uMask >>= 1 )
	{
		if ( !( uMask & 1 ) )
			continue; // listener not active for this sound

		const AkListenerData & Listener = CAkListener::GetListenerData( uListener );
		
		if( in_rPosRef.GetNumPosition() > 0 )
		{
			for( AkUInt32 i = 0; i < in_rPosRef.GetNumPosition(); ++i )
			{
				AkReal32 fDistance = AkMath::Distance( Listener.position.Position, in_rPosRef.GetPositions()[i].Position );
				fDistance = fDistance / CAkListener::GetScalingFactor( uListener );// Apply listener scaling factor.
				fMinDistance = AkMath::Min( fMinDistance, fDistance );
			}
		}
		else
		{
			// Using default position.
			AkReal32 fDistance = AkMath::Distance( Listener.position.Position, in_rPosRef.GetDefaultPosition()->Position );
			fDistance = fDistance / CAkListener::GetScalingFactor( uListener );// Apply listener scaling factor.
			fMinDistance = AkMath::Min( fMinDistance, fDistance );
		}
	}

	return fMinDistance;
}

static AKRESULT ResolveExternalSource(CAkSource* &io_pSource, AkPBIParams& in_rPBIParams)
{
	AkExternalSourceArray* pSourceArray = in_rPBIParams.userParams.CustomParam().pExternalSrcs;
	if (pSourceArray == NULL)
		return AK_Fail;

	//Clone the source for this instance of the template.
	io_pSource = io_pSource->Clone();
	if (io_pSource == NULL)
		return AK_InsufficientMemory;

	const AkExternalSourceInfo *pInfo = pSourceArray->Sources();
	for(AkUInt32 i = 0; i < pSourceArray->Count(); i++)
	{
		if (io_pSource->GetExternalSrcCookie() == pInfo->iExternalSrcCookie)
		{
			AkUInt32 pluginID = ( AkPluginTypeCodec + ( (pInfo->idCodec) << ( 4 + 12 ) ) );
			if (pInfo->szFile != NULL)
			{
				// External source.
				io_pSource->SetSource(
					io_pSource->GetSourceID(), 
					pluginID, 
					pInfo->szFile, 
					AK_INVALID_FILE_ID,	// No caching for external sources
					io_pSource->IsFromRSX(), 
					true );
				return AK_Success;
			}
			else if (pInfo->uiMemorySize > 0 && pInfo->pInMemory != NULL)
			{
				AkMediaInformation mediaInfo;
				mediaInfo.SetDefault( SrcTypeMemory, io_pSource->GetSourceID() );
				mediaInfo.bHasSource = true;
				mediaInfo.uInMemoryMediaSize = pInfo->uiMemorySize;
				mediaInfo.bExternallySupplied = true;
				io_pSource->SetSource(pluginID, pInfo->pInMemory, mediaInfo);
				return AK_Success;
			}
			else if (pInfo->idFile != 0)
			{
				AkMediaInformation mediaInfo;
				mediaInfo.SetDefault( SrcTypeFile, io_pSource->GetSourceID() );
				mediaInfo.uFileID = pInfo->idFile;
				mediaInfo.bExternallySupplied = true;
				io_pSource->SetSource(pluginID, mediaInfo);
				return AK_Success;
			}
		}

		pInfo++;
	}

	//No replacement for this source.
	AkDelete(g_DefaultPoolId, io_pSource);

	return AK_Fail;
}

bool CAkURenderer::GetVirtualBehaviorAction( AkBelowThresholdBehavior in_belowThresholdBehavior )
{
	bool bAllowedToPlay = true;

	switch( in_belowThresholdBehavior )
	{
	case AkBelowThresholdBehavior_KillVoice:
		bAllowedToPlay = false;
		break;

	case AkBelowThresholdBehavior_ContinueToPlay:
		// Let it continue to play, this sound is exempted.
		break;

	case AkBelowThresholdBehavior_SetAsVirtualVoice:
		break;

	default:
		AKASSERT( !"Unhandled below threshold type" );
		break;
	}

	return bAllowedToPlay;
}

#if defined(_MSC_VER)
#define POINTER64_FORMAT AKTEXT("0x%016llX(memory block)")
#else
#define POINTER64_FORMAT AKTEXT("0x%016zX(memory block)")
#endif

AKRESULT CAkURenderer::Play( CAkSoundBase*  in_pSound,
							 CAkSource*		in_pSource,
                             AkPBIParams&   in_rPBIParams )
{
	AKRESULT eResult = AK_Fail;
	AkMonitorData::NotificationReason eReason;

    // Check parameters.
    AKASSERT( in_pSound != NULL );

	AkReal32 fMaxRadius;
    PriorityInfoCurrent priority = _CalcInitialPriority( in_pSound, in_rPBIParams.pGameObj, fMaxRadius );

	AKRESULT eValidateLimitsResult = ValidateLimits( priority.GetCurrent(), eReason );
	bool bAllowedToPlay = eValidateLimitsResult != AK_Fail;
	if( bAllowedToPlay )
	{
		CounterParameters counterParams;
		counterParams.fPriority = priority.GetCurrent();
		counterParams.pGameObj = in_rPBIParams.pGameObj;
		AKRESULT eIncrementPlayCountResult = in_pSound->IncrementPlayCount( counterParams );

		bAllowedToPlay = eIncrementPlayCountResult != AK_Fail;
		if( eIncrementPlayCountResult == AK_MustBeVirtualized || eValidateLimitsResult == AK_MustBeVirtualized )
		{
			//Update bAllowedToPlay based on the behavior of the sound
			AkVirtualQueueBehavior _unused; 
			bAllowedToPlay = CAkURenderer::GetVirtualBehaviorAction( in_pSound->GetVirtualBehavior( _unused ) );
		}

		if ( bAllowedToPlay )
		{
			bool bExternal = in_pSource->IsExternal();
			if ( AK_EXPECT_FALSE(bExternal) )
			{
				eResult = ResolveExternalSource(in_pSource, in_rPBIParams);
#ifdef AK_OPTIMIZED
				if (eResult != AK_Success)
				{
					CounterParameters counterParams;
					counterParams.pGameObj = in_rPBIParams.pGameObj;
					in_pSound->DecrementPlayCount( counterParams );
					return eResult;
				}
#else
				if (eResult != AK_Success)
				{		
					MONITOR_ERROREX(AK::Monitor::ErrorCode_ExternalSourceNotResolved, in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_pSound->ID(), false);
					CounterParameters counterParams;
					counterParams.pGameObj = in_rPBIParams.pGameObj;
					in_pSound->DecrementPlayCount( counterParams );
					return eResult;
				}
				else
				{
					if (in_pSource->GetSrcTypeInfo()->mediaInfo.Type == SrcTypeMemory)
					{
						AkOSChar szMsg[] = AKTEXT("0x00000000000000000(memory block)");	//The string is there only to size the array properly.
						if (sizeof(void*) == 8)
							AK_OSPRINTF(szMsg, AKPLATFORM::OsStrLen(szMsg)+1, POINTER64_FORMAT, (size_t)in_pSource->GetSrcTypeInfo()->GetMedia());
						else
							AK_OSPRINTF(szMsg, AKPLATFORM::OsStrLen(szMsg)+1, AKTEXT("0x%08lX(memory block)"), (size_t)in_pSource->GetSrcTypeInfo()->GetMedia());

						MONITOR_TEMPLATESOURCE( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_pSource->GetSourceID(), szMsg);
					}
					// or else we have a file source, specified by string or by file ID.
					else if (in_pSource->GetSrcTypeInfo()->GetFilename() != NULL)
					{
						MONITOR_TEMPLATESOURCE( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_pSource->GetSourceID(), in_pSource->GetSrcTypeInfo()->GetFilename());
					}
					else if (in_pSource->GetSrcTypeInfo()->GetFileID() != AK_INVALID_FILE_ID)
					{
						AkOSChar szMsg[] = AKTEXT("1234567890(FileID)");
						AK_OSPRINTF(szMsg, AKPLATFORM::OsStrLen(szMsg)+1, AKTEXT("%u(FileID)"), in_pSource->GetSrcTypeInfo()->GetFileID() );
						MONITOR_TEMPLATESOURCE( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_pSource->GetSourceID(), szMsg);
					}
				}
#endif
			}

			CAkPBI * l_pContext = in_rPBIParams.pInstigator->CreatePBI( 
				in_pSound, 
				in_pSource, 
				in_rPBIParams, 
				priority,
				counterParams.pAMLimiter,
				counterParams.pBusLimiter
				);

			if( l_pContext != NULL )
			{
				bool bInitSucceed = false;

				if ( in_rPBIParams.eType == AkPBIParams::PBI )
				{
					AkPathInfo pathInfo = { NULL, AK_INVALID_UNIQUE_ID };

					bInitSucceed = l_pContext->Init( &pathInfo ) == AK_Success;
				}
				else
				{
					bInitSucceed = l_pContext->Init( in_rPBIParams.pContinuousParams->pPathInfo ) == AK_Success;
				}

				if( bInitSucceed )
				{
					l_pContext->SetMaxDistance( fMaxRadius );
					l_pContext->SetFrameOffset( in_rPBIParams.uFrameOffset );
					if ( Play( l_pContext, *in_rPBIParams.pTransitionParameters, in_rPBIParams.ePlaybackState ) == AK_Success )
						return AK_Success;
				}

    			l_pContext->Term( true ); // does call DecrementPlayCount()
				AkDelete( RENDERER_DEFAULT_POOL_ID, l_pContext );
				l_pContext = NULL;

			}
			else
			{
				CounterParameters counterParams;
				counterParams.pGameObj = in_rPBIParams.pGameObj;
				in_pSound->DecrementPlayCount( counterParams );
			}

			// Failure

			in_pSound->MonitorNotif( in_rPBIParams.bIsFirst ? AkMonitorData::NotificationReason_PlayFailed : AkMonitorData::NotificationReason_ContinueAborted,
				in_rPBIParams.pGameObj->ID(),
				in_rPBIParams.userParams,
				in_rPBIParams.playHistory );

			MONITOR_ERROREX( AK::Monitor::ErrorCode_PlayFailed, in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_pSound->ID(), false );
		}
		else
		{
			eResult = AK_PartialSuccess;// That mean is not an error, the sound did not play simply because the max num instance made it abort.
			eReason = AkMonitorData::NotificationReason_PlayFailedLimit;
			
			CounterParameters counterParams;
			counterParams.pGameObj = in_rPBIParams.pGameObj;
			in_pSound->DecrementPlayCount( counterParams );
		}
	}
	else
	{
		eResult = AK_PartialSuccess;// That mean is not an error, the sound did not play simply because the max num instance made it abort.
	}

	if( eResult == AK_PartialSuccess )
	{
		if( !in_rPBIParams.bIsFirst )
		{
			switch( eReason )
			{
			case AkMonitorData::NotificationReason_PlayFailedMemoryThreshold:
				eReason = AkMonitorData::NotificationReason_ContinueAbortedMemoryThreshold;
				break;
			case AkMonitorData::NotificationReason_PlayFailedLimit:
				eReason = AkMonitorData::NotificationReason_ContinueAbortedLimit;
				break;
			case AkMonitorData::NotificationReason_PlayFailedGlobalLimit:
				eReason = AkMonitorData::NotificationReason_ContinueAbortedGlobalLimit;
				break;

			}
		}
		
		in_pSound->MonitorNotif( 
				eReason,
				in_rPBIParams.pGameObj->ID(),
				in_rPBIParams.userParams,
				in_rPBIParams.playHistory );
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: Play
// Desc: Play a specified sound.
//
// Parameters:
//	CAkPBI * in_pContext				: Pointer to context to play.
//	AkPlaybackState in_ePlaybackState	: Play may be paused.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Play( CAkPBI *		 in_pContext, 
                             TransParams&    in_rTparameters,
							 AkPlaybackState in_ePlaybackState
						    )
{
	in_pContext->_InitPlay();
	
	bool l_bPaused = false;
	// Check if the play command is actually a play-pause.
	if( in_ePlaybackState == PB_Paused )
	{
		l_bPaused = true;
	}

	AKRESULT eResult = in_pContext->_Play( in_rTparameters, l_bPaused );

	if ( eResult == AK_Success )
	{
		EnqueueContext( in_pContext ); // Add PBI context to list.
	}

	return eResult;
} // Play

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Stop a specified sound.
//
// Parameters:  
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase* in_pSound				: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was scheduled to be stopped.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Stop( CAkSoundBase*		in_pSound,
						     CAkRegisteredObj*	in_pGameObj,
							 TransParams&		in_rTparameters,
							 AkPlayingID		in_PlayingID /* = AK_INVALID_PLAYING_ID */)
{
	return	ProcessCommand( ActionParamType_Stop,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							false );
} // Stop

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Pause a specified sound.
//
// Parameters:  
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase* in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was paused.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Pause( CAkSoundBase *	 in_pSound,
						      CAkRegisteredObj *	 in_pGameObj,
							  TransParams&    in_rTparameters,
							  AkPlayingID	in_PlayingID )
{
	return ProcessCommand( ActionParamType_Pause,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							false );
} // Pause

//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Resume a specified sound.
//
// Parameters:
//	AkUInt8		in_ucCommand			: Stop, Pause, Resume.
//	CAkSoundBase*  in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return:
//	Ak_Success:          Sound was resumed.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failure.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Resume( CAkSoundBase *	in_pSound,
							   CAkRegisteredObj *	in_pGameObj,
							   TransParams& in_rTparameters,
							   bool		in_bIsMasterResume,
							   AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	return ProcessCommand( ActionParamType_Resume,
							in_pSound,
							in_pGameObj,
							in_PlayingID,
                            in_rTparameters,
							in_bIsMasterResume);
} // Resume

//-----------------------------------------------------------------------------
// Name: ProcessCommand
// Desc: Process commands for playing sounds (stop, pause, resume).
//
// Parameters:  
//	UEState		in_eCommand   			: Stop, Pause, Resume.
//	CAkSoundBase * in_pSound			: Pointer to sound object to play.
//	bool		in_bIsObjectSpecific	: True = Game object active, false = no object.
//	AkTimeMs	in_lTransitionTime		: Length of transition in mSec.
//	AkCurveInterpolation	in_eFadeCurve			: Type of curve.
//
// Return: 
//	Ak_Success:          Sound was scheduled to be stopped.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             in_pSound Sound was not found.
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::ProcessCommand( ActionParamType in_eCommand,
									   CAkSoundBase *	in_pSound,
									   CAkRegisteredObj * in_pGameObj,
									   AkPlayingID in_PlayingID,
									   TransParams& in_rTparameters,
									   bool		in_bIsMasterResume )
{
	CAkPBI*	l_pPBI	= NULL;

	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
    {
		l_pPBI = *iter;
		AKASSERT( l_pPBI != NULL );

		if( l_pPBI->GetSound() == in_pSound )
		{
			if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) &&
				( in_PlayingID == AK_INVALID_PLAYING_ID || l_pPBI->GetPlayingID() == in_PlayingID ) )
			{
				switch( in_eCommand )
				{
				case ActionParamType_Stop :
					l_pPBI->_Stop( in_rTparameters, true );
					break;

				case ActionParamType_Pause :
					l_pPBI->_Pause( in_rTparameters );
					break;

				case ActionParamType_Resume :
					l_pPBI->_Resume( in_rTparameters, in_bIsMasterResume );
					break;

				default:
					AKASSERT(!"ERROR: Command not defined.");
					break;
				}
			}
		}
	} // End if.

	return AK_Success;
} // ProcessCommand


//-----------------------------------------------------------------------------
// Name: EnqueueContext
// Desc: Enqueues a PBI that was created elsewhere (the Upper Renderer creates
//       all standalone PBIs, but this is used for linked contexts (Music Renderer)
//
// Parameters:
//	PBI*
//
// Return:
//	AKRESULT
//-----------------------------------------------------------------------------
void CAkURenderer::EnqueueContext( CAkPBI * in_pContext )
{
    AKASSERT( in_pContext );
    m_listCtxs.AddLast( in_pContext );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAkURenderer::DecrementVirtualCount( bool in_bAllowKick )
{
	AKASSERT( m_uNumVirtualizedSounds != 0 );
	--m_uNumVirtualizedSounds;

	if( in_bAllowKick )
	{
		// Maybe we must kick someone
		AkUInt32 uCurrent = m_listCtxs.Length() - m_uNumVirtualizedSounds;
		CAkLimiter* pLimiter = &CAkURenderer::GetGlobalLimiter();
		AkUInt16 uMaxInstances = pLimiter->GetMaxInstances();

		if( uCurrent > uMaxInstances )
		{
			CAkParameterNodeBase* pKickedUnused = NULL;
			CAkURenderer::Kick(
				pLimiter,
				uMaxInstances,
				AK_MAX_PRIORITY + 1,
				NULL, 
				true, 
				true, //allow virtualization
				pKickedUnused,
				KickFrom_OverGlobalLimit
				);
		}
	}
}

inline AKRESULT KickEnd(
			AkReal32 in_fPriority,
			AkReal32 in_fWeakestPriority,
			CAkPBI* in_pWeakestPBI,
			bool in_bKickNewest,
			bool in_bUseVirtualBehavior,
			KickFrom in_eReason,
			CAkParameterNodeBase*& out_pKicked,
			AkBelowThresholdBehavior in_belowThresholdBehavior )
{
	if( in_fPriority < in_fWeakestPriority || (in_bKickNewest && in_fPriority == in_fWeakestPriority) )
	{
		in_pWeakestPBI = NULL;
	}
	else if ( in_pWeakestPBI )
	{
		out_pKicked = in_pWeakestPBI->GetSound();
		if( in_bUseVirtualBehavior )
		{
			switch( in_belowThresholdBehavior )
			{
			case AkBelowThresholdBehavior_KillVoice:
				in_pWeakestPBI->Kick( in_eReason );
				break;

			case AkBelowThresholdBehavior_SetAsVirtualVoice:
				break;

			case AkBelowThresholdBehavior_ContinueToPlay:
			default:
				AKASSERT( !"Unhandled below threshold type" );
				break;
			}
		}
		else
		{
			in_pWeakestPBI->Kick( in_eReason );
		}
	}

	if( in_pWeakestPBI == NULL )
	{
		if( in_bUseVirtualBehavior )
		{
			return AK_MustBeVirtualized;
		}
		else
		{
			return AK_Fail;
		}
	}
	else
		return AK_Success;
}// Kick

//-----------------------------------------------------------------------------
// Name: Kick
// Desc: Asks to kick out the Oldest sound responding to the given IDs.
//
// Parameters:
//	CAkRegisteredObj *	in_pGameObj : GameObject that must match before kicking out
//	AkUniqueID		in_NodeID  : Node to check if the sound comes from ( Excluding OverrideParent Exceptions )
//
// Return - AKRESULT - 
//      AK_SUCCESS				= caller survives.
//      AK_FAIL					= caller should kick himself.
//		AK_MustBeVirtualized	- caller should start as a virtual voice. (if it fails, it must kick itself)
//-----------------------------------------------------------------------------
AKRESULT CAkURenderer::Kick(
							CAkLimiter * in_pLimiter,
							AkUInt16 in_uMaxInstances,
							AkReal32 in_fPriority, 
							CAkRegisteredObj * in_pGameObj, 
							bool in_bKickNewest, 
							bool in_bUseVirtualBehavior, 
							CAkParameterNodeBase*& out_pKicked,
							KickFrom in_eReason
							)
{
	if( in_pLimiter == NULL )
		return AK_Success;

	CAkPBI * pWeakest = NULL;
	AkUInt16 uPlayingCount = 0;
	AkReal32 weakestPriority = (AK_MAX_PRIORITY + 1);//Max priority(100) + 1 to make it the weakest

	AkBelowThresholdBehavior virtualBehaviorWeakest = AkBelowThresholdBehavior_SetAsVirtualVoice;

	// Cycle through PBI list; PBIs are orderer by priority, then by oldest/newest
	CAkLimiter::AkSortedPBIPriorityList::Iterator itPBI = in_pLimiter->GetPBIList().Begin();
	CAkLimiter::AkSortedPBIPriorityList::Iterator itEnd = in_pLimiter->GetPBIList().End();
	for( ; itPBI != itEnd; ++itPBI )
	{
		CAkPBI* pPBI = *itPBI;

		if( in_pGameObj == NULL || pPBI->GetGameObjectPtr() == in_pGameObj )
		{
			if( ! pPBI->WasKicked() && ! pPBI->IsVirtualOrForcedVirtual() )
			{
				// Update the PBI playing count.
				++uPlayingCount;

				AkReal32 pbiPriority = pPBI->GetPriorityFloat();
				if( pbiPriority <= in_fPriority )
				{
					// Remember last PBI, if it can be kicked.
					if( in_bUseVirtualBehavior )
					{
						AkVirtualQueueBehavior _unused; 
						AkBelowThresholdBehavior virtualBehavior = pPBI->GetVirtualBehavior( _unused );
						if( AkBelowThresholdBehavior_ContinueToPlay == virtualBehavior )
						{
							// Not a candidate to be kicked, continue.
							continue;
						}
						virtualBehaviorWeakest = virtualBehavior;
					}
					pWeakest = pPBI;
					weakestPriority = pbiPriority;
				}
			}
		}
	}

	// If there aren't enough playing PBIs then there's no reason to kick anyone!
	if( (uPlayingCount + 1) <= in_uMaxInstances )
		return AK_Success;

	return KickEnd( in_fPriority, weakestPriority, pWeakest, in_bKickNewest, in_bUseVirtualBehavior, in_eReason, out_pKicked, virtualBehaviorWeakest );
}// Kick

AKRESULT CAkURenderer::Kick(
	AkReal32 in_fPriority, 
	CAkRegisteredObj * in_pGameObj, 
	bool in_bKickNewest, 
	bool in_bUseVirtualBehavior, 
	CAkParameterNodeBase*& out_pKicked,
	KickFrom in_eReason
	)
{
	CAkPBI * pWeakest = NULL;

	AkReal32 priorityWeakest = (AK_MAX_PRIORITY + 1);//Max priority(100) + 1 to make it the weakest

	AkBelowThresholdBehavior virtualBehaviorWeakest = AkBelowThresholdBehavior_SetAsVirtualVoice;

	// Cycle through PBI list; PBIs are in order of creation!
	AkListCtxs::Iterator itPBI = m_listCtxs.Begin();
	AkListCtxs::Iterator itEnd = m_listCtxs.End();
	for( ; itPBI != itEnd; ++itPBI )
	{
		CAkPBI* pPBI = *itPBI;

		if( in_pGameObj == NULL || pPBI->GetGameObjectPtr() == in_pGameObj )
		{
			if( ! pPBI->WasKicked() && ! pPBI->IsVirtualOrForcedVirtual() )
			{
				AkReal32 priority = pPBI->GetPriorityFloat();
				if( priority < priorityWeakest
					|| ( in_bKickNewest && priority == priorityWeakest )
					)
				{
					if( in_bUseVirtualBehavior )
					{
						AkVirtualQueueBehavior _unused; 
						AkBelowThresholdBehavior virtualBehavior = pPBI->GetVirtualBehavior( _unused );
						if( AkBelowThresholdBehavior_ContinueToPlay == virtualBehavior )
						{
							// Not a candidate to be kicked, continue.
							continue;
						}
						virtualBehaviorWeakest = virtualBehavior;
					}

					// Found a new weakest, remember it.
					pWeakest = pPBI;
					priorityWeakest = priority;
				}
			}
		}
	}

	return KickEnd( in_fPriority, priorityWeakest, pWeakest, in_bKickNewest, in_bUseVirtualBehavior, in_eReason, out_pKicked, virtualBehaviorWeakest );
}// Kick

// Helper
static bool CheckOverRatio( AkMemPoolId in_poolID, AkReal32 in_fLimitRatio )
{
	AK::MemoryMgr::PoolMemInfo Info;

	if( in_fLimitRatio < 1.0f )
	{
		AK::MemoryMgr::GetPoolMemoryUsed( in_poolID, Info );
		if( Info.uReserved != 0 )
		{
			return ((AkReal32)(Info.uUsed) / (AkReal32)(Info.uReserved)) > in_fLimitRatio;
		}
	}
	return false;//not over Limit.
}

// Return - bool - true = caller survives | false = caller should kick himself
bool CAkURenderer::ValidateMemoryLimit( AkReal32 in_fPriority )
{
	extern AkInitSettings g_settings;
	extern AkPlatformInitSettings g_PDSettings;

	bool bIsMemoryOverLimit = CheckOverRatio( g_DefaultPoolId, g_settings.fDefaultPoolRatioThreshold );
	if( !bIsMemoryOverLimit )
	{
		bIsMemoryOverLimit = CheckOverRatio( g_LEngineDefaultPoolId, g_PDSettings.fLEngineDefaultPoolRatioThreshold );
	}

	if( bIsMemoryOverLimit )
	{
		CAkParameterNodeBase* pKickedUnused = NULL;
		AKRESULT eKickResult = Kick(
			in_fPriority,
			NULL, 
			true, 
			false, // In case of memory threshold reached, do not virtualize, force Kill lowest priority.
			pKickedUnused,
			KickFrom_OverMemoryLimit
			);

		return eKickResult == AK_Success;
	}
	return true;
}

// Return - bool - true = caller survives | false = caller should kick himself
AKRESULT CAkURenderer::ValidateMaximumNumberVoiceLimit( AkReal32 in_fPriority )
{
	AkUInt32 uCurrent = m_listCtxs.Length() - m_uNumVirtualizedSounds;
	// Ok que faire avec cela...
	// Le nombre de virtuels est difficile a dire, on doit compter ceux qui sont "to be virtualized" dans le tas...

	AkUInt16 uMaxInstances = CAkURenderer::GetGlobalLimiter().GetMaxInstances();
	bool bIsOverLimit = uCurrent + 1 > uMaxInstances; //(+1 since we are about to add one)

	if( bIsOverLimit )
	{
		CAkParameterNodeBase* pKickedUnused = NULL;
		return Kick(
			&CAkURenderer::GetGlobalLimiter(),
			uMaxInstances,
			in_fPriority,
			NULL, 
			true, 
			true, // In this case, we allow virtualization.
			pKickedUnused,
			KickFrom_OverGlobalLimit
			);
	}
	else
	{
		return AK_Success;
	}
}

AKRESULT CAkURenderer::ValidateLimits( AkReal32 in_fPriority, AkMonitorData::NotificationReason& out_eReasonOfFailure )
{
	bool bValid = ValidateMemoryLimit( in_fPriority );
	if( !bValid )
	{
		out_eReasonOfFailure = AkMonitorData::NotificationReason_PlayFailedMemoryThreshold;
		return AK_Fail;// instructs the sound cannot play du to a limitations.
	}

	out_eReasonOfFailure = AkMonitorData::NotificationReason_PlayFailedGlobalLimit; //Setting it anyway, will not be used otherwise.
	return ValidateMaximumNumberVoiceLimit( in_fPriority );
}

void CAkURenderer::EnqueueContextNotif( CAkPBI* in_pPBI, AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason, AkReal32 in_fEstimatedTime /*= 0*/ )
{
	ContextNotif* pCtxNotif = m_CtxNotifQueue.AddLast();

	if( !pCtxNotif )
	{
		// Not enough memory to send the notification. 
		// But we cannot simply drop it since it may cause game inconsistency and leaks.
		// So we will do right now the job we are supposed to be doing after the perform of the lower engine.

		PerformContextNotif();

		// once context notifs have been performed, the m_CtxNotifQueue queue will be empty, so the nest addlast will succeed.
		pCtxNotif = m_CtxNotifQueue.AddLast();

		AKASSERT( pCtxNotif && MIN_NUM_RENDER_EVENT );// Assert, because if it fails that means the minimal size of m_CtxNotifQueue of 64
		// MIN_NUM_RENDER_EVENT just cannot be 0.
	}

	pCtxNotif->pPBI = in_pPBI;
	pCtxNotif->state = in_eState;
	pCtxNotif->DestroyReason = in_eDestroyReason;
	pCtxNotif->fEstimatedLength = in_fEstimatedTime;
}

void CAkURenderer::PerformContextNotif()
{
	while( !m_CtxNotifQueue.IsEmpty() )
	{
		ContextNotif& pCtxNotif = m_CtxNotifQueue.First();
		pCtxNotif.pPBI->ProcessContextNotif( pCtxNotif.state, pCtxNotif.DestroyReason, pCtxNotif.fEstimatedLength );
		if( pCtxNotif.state == CtxStateToDestroy )
		{
			m_listCtxs.Remove( pCtxNotif.pPBI );
			DestroyPBI( pCtxNotif.pPBI );
		}
		m_CtxNotifQueue.RemoveFirst();
	}
}

#ifndef AK_OPTIMIZED
void CAkURenderer::RefreshMonitoringMuteSolo()
{
	AkListCtxs::Iterator it = m_listCtxs.Begin();
	while ( it != m_listCtxs.End() )
	{
		(*it)->RefreshMonitoringMute();
		++it;
	}

	CAkIndexItem<CAkParameterNodeBase*>&l_rIdx = g_pIndex->m_idxBusses;
	{//Bracket for autolock
		AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

		CAkIndexItem<CAkParameterNodeBase*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			static_cast<CAkBus*>( *iter )->RefreshMonitoringMute();
			++iter;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Name: DestroyPBI
// Desc: Destroy a specified PBI.
//
// Parameters:
//	CAkPBI * in_pPBI  : Pointer to a PBI to destroy.
//-----------------------------------------------------------------------------
void CAkURenderer::DestroyPBI( CAkPBI * in_pPBI )
{
	CAkLEngineCmds::DequeuePBI( in_pPBI );
	in_pPBI->Term( false );
	AkDelete( RENDERER_DEFAULT_POOL_ID, in_pPBI );
} // DestroyPBI

void CAkURenderer::DestroyAllPBIs()
{
	while ( CAkPBI * pPBI = m_listCtxs.First() )
	{
		m_listCtxs.RemoveFirst();
		pPBI->_Stop( AkPBIStopMode_Normal, true ); // necessary to avoid infinitely regenerating continuous PBIs
        DestroyPBI( pPBI );
	}
} // DestroyAllPBIs

void CAkURenderer::StopAllPBIs( const CAkUsageSlot* in_pUsageSlot )
{
	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
    {
		CAkPBI* l_pPBI = *iter;
		if( l_pPBI->IsUsingThisSlot( in_pUsageSlot ) )
		{
			l_pPBI->_Stop( TransParams(), true ); // Immediate stop (with min fade-out)

			g_pAudioMgr->StopPendingAction( l_pPBI->GetSound(), NULL, AK_INVALID_PLAYING_ID );	
		}
	}

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS) //Wii does not support this feature.
	CAkLEngine::StopMixBussesUsingThisSlot( in_pUsageSlot );
#else
	if (AK_PERF_OFFLINE_RENDERING)
	{
		for(AkUInt32 i = 0; i < CALCULATE_VOLUME_AT_EACH_NUM_BUFFER * 2; i++)
			CAkLEngine::Perform();
	}
#endif
}

void CAkURenderer::ResetAllEffectsUsingThisMedia( const AkUInt8* in_pOldDataPtr )
{
	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
	{
		CAkPBI* l_pPBI = *iter;
		if( l_pPBI->IsUsingThisSlot( in_pOldDataPtr ) )
		{
			for( int index = 0; index < AK_NUM_EFFECTS_PER_OBJ; ++index )
			{
				l_pPBI->UpdateFx( index );
			}
		}
	}

	CAkLEngine::ResetAllEffectsUsingThisMedia( in_pOldDataPtr );
}

AkReal32 CAkURenderer::GetMaxRadius( AkGameObjectID in_GameObjId )
{
	AkReal32 MaxDistance = -1.0f;

	CAkRegisteredObj* pGameObj = g_pRegistryMgr->GetObjAndAddref( in_GameObjId );
	if( pGameObj )
	{
		for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
		{
			CAkPBI* l_pPBI = *iter;
			if( l_pPBI->GetGameObjectPtr() == pGameObj )
			{
				MaxDistance = AkMax( MaxDistance, l_pPBI->GetMaxDistance() );
			}
		}
		pGameObj->Release();
	}
	return MaxDistance;
}

AKRESULT CAkURenderer::GetMaxRadius( AK::SoundEngine::Query::AkRadiusList & io_RadiusList )
{
	// for performance reasons, we are using an hashList internally.
	typedef AkHashList< AkGameObjectID,AkReal32, AK_SMALL_HASH_SIZE > AkGameObjToMaxDst;
	AkGameObjToMaxDst m_localTempHash;

	m_localTempHash.Init( g_DefaultPoolId );

	AKRESULT eResult = AK_Success;

	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
	{
		CAkPBI* l_pPBI = *iter;
		AkGameObjectID gameObjID = l_pPBI->GetGameObjectPtr()->ID();
		
		// Find they key in the array
		AkGameObjToMaxDst::IteratorEx iterMax = m_localTempHash.FindEx( gameObjID );

		if( iterMax != m_localTempHash.End() )
		{
			(*iterMax).item = AkMax( (*iterMax).item, l_pPBI->GetMaxDistance() );
		}
		else
		{
			AkReal32* pMaxDst = m_localTempHash.Set( gameObjID );
			if( !pMaxDst )
			{
				eResult = AK_InsufficientMemory;
				break;
			}
			*pMaxDst = l_pPBI->GetMaxDistance();
		}
	}

	if( eResult == AK_Success )
	{
		// Note : Calling term to "reset" the list
		// So that if the list must be sorted, we have to make sure the list was empty with non sorted items.
		io_RadiusList.Term(); // Just in case the user provided a non empty list.
		// Allocate minimal memory for the array.
		AkUInt32 uArraySize = m_localTempHash.Length();
		eResult = io_RadiusList.Reserve( uArraySize );
		if( eResult == AK_Success )
		{
			// Copy the list over in a format the user will be able to read.
			for( AkGameObjToMaxDst::Iterator iterHash = m_localTempHash.Begin(); iterHash != m_localTempHash.End(); ++iterHash )
			{
				io_RadiusList.AddLast( AK::SoundEngine::Query::GameObjDst( (*iterHash).key, (*iterHash).item ) );
			}
		}
	}

	m_localTempHash.Term();

	return eResult;
}

#ifdef AK_MOTION
void CAkURenderer::InvalidateAllMotionPBIs()
{
	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
	{
		if((*iter)->GetPannerType() == Ak3D && (*iter)->GetPositionSourceType() == AkGameDef)
			(*iter)->InvalidateFeedbackParameters();
	}
}
#endif

void CAkURenderer::AddBusLimiter( CAkLimiter* in_pLimiter )
{
	m_BusLimiters.AddFirst( in_pLimiter );
}

void CAkURenderer::RemoveBusLimiter( CAkLimiter* in_pLimiter )
{
	m_BusLimiters.Remove( in_pLimiter );
}

void CAkURenderer::AddAMLimiter( CAkLimiter* in_pLimiter )
{
	m_AMLimiters.AddFirst( in_pLimiter );
}

void CAkURenderer::RemoveAMLimiter( CAkLimiter* in_pLimiter )
{
	m_AMLimiters.Remove( in_pLimiter );
}

void CAkURenderer::ProcessLimiters()
{
	// Clear all forced virtual flags.
	for( AkListCtxs::Iterator iter = m_listCtxs.Begin(); iter != m_listCtxs.End(); ++iter )
	{
		CAkPBI * l_pPBI = *iter;
		l_pPBI->ForceDevirtualize();
	}

	// Do Golobal limiting first.
	m_GlobalLimiter.UpdateFlags();

	// Recompute them all.
	for( AkListLightLimiters::Iterator iter = m_BusLimiters.Begin(); iter != m_BusLimiters.End(); ++iter )
	{
		CAkLimiter* pLimiter = *iter;
		AKASSERT(pLimiter);	 //A NULL limiter should not have been added to the list!
		if (pLimiter)
			pLimiter->UpdateFlags();
	}
	for( AkListLightLimiters::Iterator iter = m_AMLimiters.Begin(); iter != m_AMLimiters.End(); ++iter )
	{
		CAkLimiter* pLimiter = *iter;
		AKASSERT(pLimiter);	//A NULL limiter should not have been added to the list!
		if (pLimiter)
			pLimiter->UpdateFlags();
	}
}

#if defined (_DEBUG)
bool CAkURenderer::CheckLimitersForCtx( CAkPBI* in_pCtx )
{
	if( m_GlobalLimiter.LookForCtx( in_pCtx ) )
		return true;

	for( AkListLightLimiters::Iterator iter = m_BusLimiters.Begin(); iter != m_BusLimiters.End(); ++iter )
	{
		CAkLimiter* pLimiter = *iter;
		if( pLimiter->LookForCtx( in_pCtx ) )
			return true;
	}

	for( AkListLightLimiters::Iterator iter = m_AMLimiters.Begin(); iter != m_AMLimiters.End(); ++iter )
	{
		CAkLimiter* pLimiter = *iter;
		if( pLimiter->LookForCtx( in_pCtx ) )
			return true;
	}

	return false;
}
#endif
