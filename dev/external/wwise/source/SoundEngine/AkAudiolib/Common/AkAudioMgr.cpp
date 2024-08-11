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
// CAkAudioMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAudioMgr.h"
#include "AkAudioLib.h"
#include "AkAudioLibIndex.h"
#include "AkActions.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkSound.h"
#include "AkRandom.h"

#include "Ak3DListener.h"
#include "AkLEngine.h"
#include "AkEnvironmentsMgr.h"
#include "AkURenderer.h"
#include "AkTransitionManager.h"
#include "AkPathManager.h"
#include "AkPlayingMgr.h"
#include "AkEffectsMgr.h"
#include "AkStateMgr.h"
#include "AkMonitor.h"
#include "AkCritical.h"
#include "AkParentNode.h"
#include "AkRegistryMgr.h"
#include "AkRTPCMgr.h"
#include "AkCntrHistory.h"
#include "AkEvent.h"
#include "AkBus.h"
#include "AkDynamicSequence.h"

#include "AkProfile.h"
#include "AkAudioLibTimer.h"

#include "AkBankMgr.h"
#include "AkOutputMgr.h"

#include "ICommunicationCentral.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
	#include <AK/Plugin/AkRumbleFactory.h>
#endif // AK_MOTION

#include "AkSink.h"
extern CAkSink* g_pAkSink;
extern AkPlatformInitSettings g_PDSettings;

#if defined AK_WII_FAMILY
#include "AkWiimoteMgr.h"
#endif

#ifndef AK_OPTIMIZED
	extern AK::Comm::ICommunicationCentral * g_pCommCentral;
#endif

typedef AkArray<AkGlobalCallbackFunc, AkGlobalCallbackFunc, ArrayPoolDefault> BehavioralExtensionArray;
extern BehavioralExtensionArray g_aBehavioralExtensions;

extern AkInitSettings					g_settings;

AkTimeMs	AkMonitor::m_ThreadTime = 0;

#define AK_MIN_NUM_EVENT_QUEUE_ITEMS 32
#define AK_MAX_NUM_EVENT_QUEUE_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_EVENT_ACTION_ITEMS 32
#define AK_MAX_NUM_EVENT_ACTION_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_PENDING_ITEMS 32
#define AK_MAX_NUM_PENDING_ITEMS AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_PAUSED_PENDING_ITEMS 32
#define AK_MAX_NUM_PAUSED_PENDING_ITEMS AK_NO_MAX_LIST_SIZE

#define BASE_AKQUEUEMSGSIZE offsetof( AkQueuedMsg, event )
#define DEF_MSG_OBJECT_SIZE( _NAME_, _OBJECT_ ) \
AkUInt16 AkQueuedMsg::_NAME_() { return BASE_AKQUEUEMSGSIZE + sizeof( _OBJECT_ ); }

AkUInt16 AkQueuedMsg::Sizeof_EndOfList() 
{ 
	return BASE_AKQUEUEMSGSIZE; 
}

AkUInt16 AkQueuedMsg::Sizeof_GameObjMultiPositionBase() 
{
	return BASE_AKQUEUEMSGSIZE + offsetof( AkQueuedMsg_GameObjMultiplePosition, aMultiPosition );
}

DEF_MSG_OBJECT_SIZE( Sizeof_Event,						AkQueuedMsg_Event )
DEF_MSG_OBJECT_SIZE( Sizeof_Rtpc,						AkQueuedMsg_Rtpc )
DEF_MSG_OBJECT_SIZE( Sizeof_RtpcWithTransition,			AkQueuedMsg_RtpcWithTransition )
DEF_MSG_OBJECT_SIZE( Sizeof_State,						AkQueuedMsg_State )
DEF_MSG_OBJECT_SIZE( Sizeof_Switch,						AkQueuedMsg_Switch )
DEF_MSG_OBJECT_SIZE( Sizeof_Trigger,					AkQueuedMsg_Trigger )
DEF_MSG_OBJECT_SIZE( Sizeof_RegisterGameObj,			AkQueuedMsg_RegisterGameObj )
DEF_MSG_OBJECT_SIZE( Sizeof_UnregisterGameObj,			AkQueuedMsg_UnregisterGameObj )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjPosition,			AkQueuedMsg_GameObjPosition )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjActiveListeners, 	AkQueuedMsg_GameObjActiveListeners )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjActiveControllers,	AkQueuedMsg_GameObjActiveControllers )
DEF_MSG_OBJECT_SIZE( Sizeof_ListenerPosition,			AkQueuedMsg_ListenerPosition )
DEF_MSG_OBJECT_SIZE( Sizeof_ListenerSpatialization, 	AkQueuedMsg_ListenerSpatialization )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjEnvValues,			AkQueuedMsg_GameObjEnvValues )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjDryLevel,			AkQueuedMsg_GameObjDryLevel )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjObstruction, 		AkQueuedMsg_GameObjObstruction )
DEF_MSG_OBJECT_SIZE( Sizeof_ResetSwitches,				AkQueuedMsg_ResetSwitches )
DEF_MSG_OBJECT_SIZE( Sizeof_ResetRTPC,					AkQueuedMsg_ResetRTPC )
DEF_MSG_OBJECT_SIZE( Sizeof_ResetRTPCValue,				AkQueuedMsg_ResetRTPCValue )
DEF_MSG_OBJECT_SIZE( Sizeof_ResetRTPCValueWithTransition,	AkQueuedMsg_ResetRtpcValueWithTransition )
DEF_MSG_OBJECT_SIZE( Sizeof_SetSecondaryOutputVolume,	AkQueuedMsg_SetSecondaryOutputVolume )
DEF_MSG_OBJECT_SIZE( Sizeof_OpenDynamicSequence,		AkQueuedMsg_OpenDynamicSequence )
DEF_MSG_OBJECT_SIZE( Sizeof_DynamicSequenceCmd, 		AkQueuedMsg_DynamicSequenceCmd )
DEF_MSG_OBJECT_SIZE( Sizeof_KillBank,					AkQueuedMsg_KillBank )
DEF_MSG_OBJECT_SIZE( Sizeof_StopAll,					AkQueuedMsg_StopAll )
DEF_MSG_OBJECT_SIZE( Sizeof_ListenerPipeline,			AkQueuedMsg_ListenerPipeline )
DEF_MSG_OBJECT_SIZE( Sizeof_SetPlayerListener,			AkQueuedMsg_SetPlayerListener )
DEF_MSG_OBJECT_SIZE( Sizeof_SetPlayerVolume,			AkQueuedMsg_SetPlayerVolume )
DEF_MSG_OBJECT_SIZE( Sizeof_AddRemovePlayerDevice,		AkQueuedMsg_AddRemovePlayerDevice )
DEF_MSG_OBJECT_SIZE( Sizeof_StopPlayingID,				AkQueuedMsg_StopPlayingID )
DEF_MSG_OBJECT_SIZE( Sizeof_EventAction,				AkQueuedMsg_EventAction )
DEF_MSG_OBJECT_SIZE( Sizeof_GameObjScalingFactor,		AkQueuedMsg_GameObjScalingFactor )
DEF_MSG_OBJECT_SIZE( Sizeof_ListenerScalingFactor,		AkQueuedMsg_ListenerScalingFactor )
DEF_MSG_OBJECT_SIZE( Sizeof_Seek,						AkQueuedMsg_Seek )
DEF_MSG_OBJECT_SIZE( Sizeof_PlaySourcePlugin,		    AkQueuedMsg_SourcePluginAction )
DEF_MSG_OBJECT_SIZE( Sizeof_StartStopCapture,			AkQueuedMsg_StartStopCapture )
DEF_MSG_OBJECT_SIZE( Sizeof_SetEffect,					AkQueuedMsg_SetEffect )
DEF_MSG_OBJECT_SIZE( Sizeof_SetPanningRule,				AkQueuedMsg_SetPanningRule )
DEF_MSG_OBJECT_SIZE( Sizeof_SetSpeakerAngles,			AkQueuedMsg_SetSpeakerAngles )
DEF_MSG_OBJECT_SIZE( Sizeof_SetProcessMode,				AkQueuedMsg_SetProcessMode )
#ifdef AK_WIIU
DEF_MSG_OBJECT_SIZE( Sizeof_SendMainOutputToDevice,		AkQueuedMsg_SendMainOutputToDevice )
#endif

///////////////////////////////////////
//Helpers
inline bool CheckObjAndPlayingID(  const CAkRegisteredObj* in_pObjSearchedFor, AkPlayingID in_PlayingIDSearched, AkPendingAction* pActualPendingAction )
{
	return CheckObjAndPlayingID( in_pObjSearchedFor, pActualPendingAction->GameObj(), in_PlayingIDSearched, pActualPendingAction->UserParam.PlayingID() );
}
//////////////////////////////////////

//-----------------------------------------------------------------------------
// Name: CAkAudioMgr
// Desc: Constructor.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
CAkAudioMgr::CAkAudioMgr()
	:m_bDrainMsgQueue(false)
	,m_MsgQueueActualSize(0)
#ifndef AK_OPTIMIZED
	,m_MsgQueuePercentageFilled(0.0f)
#endif
	,m_uBufferTick(0)
	,m_ulWriterFlag(0)
	,m_ulReaderFlag(0)
	,m_timeLastBuffer(0)
	,m_uCallsWithoutTicks(0)
{
}

//-----------------------------------------------------------------------------
// Name: ~CAkAudioMgr
// Desc: Destructor.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
CAkAudioMgr::~CAkAudioMgr()
{
}

AKRESULT CAkAudioMgr::Init()
{
	m_ulWriterFlag = 0;
	m_ulReaderFlag = 0;

	// Calculate the number of blocks we will create in the command queue
	AkUInt32 totalsize = g_settings.uCommandQueueSize;
	AkUInt32 numblocks = totalsize / m_MsgQueue.GetChunkSize();
	AKRESULT eResult = m_MsgQueue.Init(numblocks);
	
	if( eResult == AK_Success )
	{
		eResult = m_mmapPending.Init( AK_MIN_NUM_PENDING_ITEMS, AK_MAX_NUM_PENDING_ITEMS );
	}
	if( eResult == AK_Success )
	{
		eResult = m_mmapPausedPending.Init( AK_MIN_NUM_PAUSED_PENDING_ITEMS, AK_MAX_NUM_PAUSED_PENDING_ITEMS );
	}
	return eResult;
}

void CAkAudioMgr::Term()
{
	Stop();

	RemoveAllPreallocAndReferences();
	RemoveAllPausedPendingAction();
	RemoveAllPendingAction();

	m_MsgQueue.Term();
	m_mmapPending.Term();
	m_mmapPausedPending.Term();
}

//-----------------------------------------------------------------------------
// Name: RenderAudio
// Desc: Render Upper Audio Engine.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success :
//  AK_Fail    :
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::RenderAudio()
{
	m_queueLock.Lock();
	bool bProcess = !m_MsgQueue.IsEmpty(); // do not unnecessarily wake up the audio thread if no events have been enqueued.
	
	if (bProcess)
	{
		AkQueuedMsg EndOfListItem( QueuedMsgType_EndOfList );
		EndOfListItem.size = AkQueuedMsg::Sizeof_EndOfList();
		LockedEnqueue(EndOfListItem, AkQueuedMsg::Sizeof_EndOfList());
		++m_ulWriterFlag;
	}

	m_queueLock.Unlock();

	//In offline mode, process a frame even if there isn't any events.
	if ( AK_PERF_OFFLINE_RENDERING )
		Perform();
	else if (bProcess)
		m_audioThread.WakeupEventsConsumer();

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Enqueue
// Desc: Enqueue actions.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success : VPL executed. 
//  AK_Fail    : Failed to execute a VPL.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::Enqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize )
{
	m_queueLock.Lock();
	AKRESULT eResult = LockedEnqueue(in_rItem, in_uSize);
	m_queueLock.Unlock();
	return eResult;
}

void CAkAudioMgr::IncrementBufferTick()
{ 
	++m_uBufferTick; 
#ifndef AK_OPTIMIZED
	AkMonitor::SetThreadTime( (AkTimeMs) ( (AkReal64) m_uBufferTick * 1000.0 * AK_NUM_VOICE_REFILL_FRAMES / AK_CORE_SAMPLERATE ) ); 
	AkMonitor::Monitor_TimeStamp();
#endif
}

AKRESULT CAkAudioMgr::LockedEnqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize )
{
	in_rItem.size = (AkUInt16) in_uSize;

	AKRESULT eResult; 
	eResult = m_MsgQueue.Write( &in_rItem, in_uSize );
	
	// Our message queue is full; drain it and wait for the audio thread
	// to tell us that we can re-enqueue this message
	while (eResult == AK_InsufficientMemory)
	{
		if (AK_EXPECT_FALSE(in_uSize > m_MsgQueue.GetChunkSize()))
		{
			MONITOR_ERROR(AK::Monitor::ErrorCode_CommandTooLarge);
			return AK_CommandTooLarge;
		}

		if ( !AK_PERF_OFFLINE_RENDERING )
			m_hEventMgrThreadDrainEvent.Reset();

		// tell audio thread to drain the message queue
		m_bDrainMsgQueue = true;

		m_queueLock.Unlock();

		MONITOR_ERROR(AK::Monitor::ErrorCode_CommandQueueFull);

		if ( GetThreadID() == AKPLATFORM::CurrentThread()
			|| AK_PERF_OFFLINE_RENDERING )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			ProcessMsgQueue();
		}
		else
		{
			// wake up the audio thread
			m_audioThread.WakeupEventsConsumer();			

			// wait for audio thread to signal this thread 
			m_hEventMgrThreadDrainEvent.Wait();			
		}

		// try writing again
		m_queueLock.Lock();
		eResult = m_MsgQueue.Write( &in_rItem, in_uSize );		
	}

	return eResult;
}

void CAkAudioMgr::Perform()
{
	g_csMain.Lock();

	AK_START_TIMER_AUDIO();

	AkUInt32 l_uNumBufferToFill = AK_PERF_OFFLINE_RENDERING ? 1 : CAkLEngine::GetNumBufferNeededAndSubmit();
	bool bHasTicked = ( l_uNumBufferToFill > 0 );

	do
	{
		// Process events from the main queue.
		ProcessMsgQueue();
	
		// Process events that are pending.
		ProcessPendingList();
	
		// Execute all new "play sound" low-level commands. If we've been awaken because of a RenderAudio(),
		// it is beneficial to perform this now in order to be ready to play at the next audio frame.
		if ( CAkLEngineCmds::ProcessPlayCmdsNeeded() )
			CAkLEngineCmds::ProcessPlayCommands();
	
		if ( l_uNumBufferToFill == 0 )
			break;
	
		for ( int i = (int) g_aBehavioralExtensions.Length() - 1; i >= 0; --i ) // from end in case an extension unregisters itself in callback.
			g_aBehavioralExtensions[ i ]( false );

		// Here, the +1 forces the one buffer look ahead.
		// We must tell the transitions where we want to be on next buffer, not where we are now
		AkUInt32 l_ActualBufferTick = GetBufferTick() + 1;

		g_pTransitionManager->ProcessTransitionsList( l_ActualBufferTick );
		g_pPathManager->ProcessPathsList( l_ActualBufferTick );

#ifndef AK_OPTIMIZED
		if ( CAkParameterNodeBase::IsRefreshMonitoringMuteSoloNeeded() )
			CAkURenderer::RefreshMonitoringMuteSolo();
#endif

		CAkListener::OnBeginFrame();
		CAkURenderer::ProcessLimiters();
		CAkLEngine::Perform();
		CAkURenderer::PerformContextNotif();

		IncrementBufferTick();
		--l_uNumBufferToFill;
	}
	while ( true );

#ifdef AK_MOTION
	//Command driven devices don't automatically grab their samples.  We must send them periodically.
	//This is the case for all rumble controllers for example.
	CAkFeedbackDeviceMgr *pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
	if (pFeedbackMgr != NULL)
	{
#if defined AK_WII_FAMILY_HW
		//On the wii, send a sample only once every 4 buffers.  This makes a sample rate of 83 samples per second.
		if ((GetBufferTick() & (AK_FEEDBACK_DIVIDER - 1)) == 0)
#endif
		pFeedbackMgr->CommandTick();
	}
#endif // AK_MOTION

	AK_STOP_TIMER_AUDIO();

	g_csMain.Unlock();
	

#ifdef AK_IOS
	HandleLossOfHardwareResponse(bHasTicked);
#else
	if ( bHasTicked )
	{
		AK_PERF_TICK_AUDIO();
	}
#endif
	
#ifndef AK_OPTIMIZED
	// Process communications
	if ( bHasTicked && g_pCommCentral )
		g_pCommCentral->Process();
#endif // AK_OPTIMIZED
}

//-----------------------------------------------------------------------------
// Name: ProcessMsgQueue
// Desc: Process the message queue.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success:	Succeeded.
//  AK_Fail   : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ProcessMsgQueue()
{
	AKASSERT(g_pIndex);


#ifndef AK_OPTIMIZED
	//m_queueLock.Lock(); locking removed for idle performance -- values read are not critical
	// how big has the message queue grown
	m_MsgQueueActualSize = m_MsgQueue.GetActualSize();

	// how much of the queue has been filled
	AkReal32 uMsgQueuePercentageFilled = m_MsgQueue.GetPercentageUsed();
	if(m_MsgQueuePercentageFilled < uMsgQueuePercentageFilled)
	{
		m_MsgQueuePercentageFilled = uMsgQueuePercentageFilled;
	}

	//m_queueLock.Unlock();
#endif

	// Skip this if we are going to drain the message queue
	if(!m_bDrainMsgQueue)
	{
		if( m_ulWriterFlag == m_ulReaderFlag )
		{
			return AK_Success;
		}
	}

	bool bReachedEnd = false;

	m_queueLock.Lock();

	do
	{
		AkUInt32 uSizeAvail = 0, uSizeRead = 0;

		// Since we are draining the queue, bail out early
		if(m_MsgQueue.IsEmpty())
		{
			break;
		}
	
		AkUInt8 * pData =  (AkUInt8 *) m_MsgQueue.BeginReadEx( uSizeAvail );
		AKASSERT( pData );

		m_queueLock.Unlock();

		do
		{
			AkQueuedMsg * pItem = (AkQueuedMsg *) pData;

			switch ( pItem->type )
			{
			default:
				AKASSERT( !"Invalid msg queue item type!" );
				break;

			case QueuedMsgType_EndOfList:
					m_ulReaderFlag++;
					bReachedEnd = m_ulReaderFlag == m_ulWriterFlag && !m_bDrainMsgQueue;
				break;

			case QueuedMsgType_Event:
				{
					MONITOR_EVENTTRIGGERED( pItem->event.PlayingID, pItem->event.Event->ID(), pItem->event.GameObjID, pItem->event.CustomParam );

					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->event.GameObjID );

					for( CAkEvent::AkActionList::Iterator iter = pItem->event.Event->m_actions.Begin(); iter != pItem->event.Event->m_actions.End(); ++iter )
					{
						CAkAction* pAction = *iter;
						AKASSERT( pAction );

						AkPendingAction* pThisAction = NULL;

						if ( ACTION_TYPE_USE_OBJECT & pAction->ActionType() )
						{
							if ( pGameObj ) // Action requires an object
							{
								pThisAction = AkNew( g_DefaultPoolId, AkPendingAction( pGameObj ) );
							}
							else
							{
								MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObjectEvent, pItem->event.PlayingID, pItem->event.GameObjID, pItem->event.Event->ID(), false );
							}
						}
						else
						{
							pThisAction = AkNew( g_DefaultPoolId, AkPendingAction( NULL ) );
						}

						if( pThisAction )
						{
							pThisAction->TargetPlayingID = pItem->event.TargetPlayingID;
							pThisAction->pAction = pAction;
							pThisAction->UserParam.Init(pItem->event.PlayingID, pItem->event.CustomParam);

							EnqueueOrExecuteAction( pThisAction );// true tells to hold delayed items, they will be added in the delayed list later.
						}
					}
					if ( pGameObj )
					{
						pGameObj->Release();
					}
					g_pPlayingMgr->RemoveItemActiveCount( pItem->event.PlayingID );

					pItem->event.Event->Release();

					if (pItem->event.CustomParam.pExternalSrcs)
						pItem->event.CustomParam.pExternalSrcs->Release();
				}
				break;

            case QueuedMsgType_SourcePluginAction:
                {
                    UserParams local_userParams;                    
                    local_userParams.SetPlayingID(pItem->sourcePluginAction.PlayingID);

                    AkUInt32 ulPluginID = CAkEffectsMgr::GetMergedID( 
                                                        AkPluginTypeSource, 
                                                        pItem->sourcePluginAction.CompanyID, 
                                                        pItem->sourcePluginAction.PluginID 
                                                        );
                                
                    // Adding bit meaning this object wa dynamically created without the Wwise project.
                    AkUniqueID soundID = ulPluginID | AK_SOUNDENGINE_RESERVED_BIT;

                    switch( pItem->sourcePluginAction.ActionType)
                    {
                    case AkSourcePluginActionType_Play:
                        {
                            CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->sourcePluginAction.GameObjID );
                    
					        if( pGameObj )// Avoid performing if the GameObject was specified but does not exist.
					        {
								//Create a dummy effect
								CAkFxCustom* pFx = static_cast<CAkFxCustom*>(g_pIndex->m_idxFxCustom.GetPtrAndAddRef( soundID ));
								if( !pFx )
								{
									pFx = CAkFxCustom::Create( soundID );
									if ( pFx )
									{
										pFx->SetFX( ulPluginID, NULL );
									}
								}
								else
								{
									pFx->Release();
								}

                                CAkSound* pSound = static_cast<CAkSound*>(g_pIndex->GetNodePtrAndAddRef( soundID, AkNodeType_Default ));
                                if( !pSound && pFx )
                                {
									pSound = CAkSound::Create(soundID);
									if(pSound)
									{
										pSound->SetSource( pFx->ID() );
									}
                                }
                                else
                                {
                                    pSound->Release();// release only if GetPtrAndAddRef was called.
                                }
                                PlaySourceInput( soundID, pGameObj, local_userParams );
					        }
					        else
					        {
								MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->sourcePluginAction.GameObjID, AK_INVALID_UNIQUE_ID, false );
					        }

                            if ( pGameObj )
					        {
						        pGameObj->Release();
					        }
                        }
                        break;

                    case AkSourcePluginActionType_Stop:
                        {
                            CAkSound* pSound = static_cast<CAkSound*>(g_pIndex->GetNodePtrAndAddRef( soundID, AkNodeType_Default ));

	                        if( pSound )
	                        {
		                        ActionParams l_Params;
		                        l_Params.bIsFromBus = false;
		                        l_Params.bIsMasterResume = false;
		                        l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
		                        l_Params.pGameObj = NULL;
                                l_Params.playingID = pItem->sourcePluginAction.PlayingID;
		                        l_Params.transParams.TransitionTime = 0;
		                        l_Params.bIsMasterCall = false;
                                l_Params.eType = ActionParamType_Stop;

		                        pSound->ExecuteAction( l_Params );
		                        pSound->Release();
                            }
                        }
                        break;
                    }
                    
                }
                break;

			case QueuedMsgType_EventAction:
				{
					AkQueuedMsg_EventAction& rEventAction = pItem->eventAction;
					MONITOR_MSGEX( AK::Monitor::ErrorCode_ExecuteActionOnEvent, rEventAction.TargetPlayingID, rEventAction.GameObjID, AK_INVALID_UNIQUE_ID, false );

					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( rEventAction.GameObjID );
					if( pGameObj || rEventAction.GameObjID == AK_INVALID_GAME_OBJECT )// Avoid performing if the GameObject was specified but does not exist.
					{
						// Find each action play in the list, and execute the action on their target.
						for( CAkEvent::AkActionList::Iterator iter = rEventAction.Event->m_actions.Begin(); iter != rEventAction.Event->m_actions.End(); ++iter )
						{
							CAkAction* pAction = *iter;
							AKASSERT( pAction );
							
							AkActionType aType = pAction->ActionType();
							if( aType == AkActionType_Play )
							{
								CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
								pTargetNode.Attach( pAction->GetAndRefTarget() );

								if( pTargetNode )
								{
									ProcessCustomAction( 
										pTargetNode, 
										pGameObj, 
										rEventAction.eActionToExecute, 
										rEventAction.uTransitionDuration,
										rEventAction.eFadeCurve,
										rEventAction.TargetPlayingID
										);
								}
							}
							else
							{
								switch( rEventAction.eActionToExecute )
								{
								case AK::SoundEngine::AkActionOnEventType_Stop:
								case AK::SoundEngine::AkActionOnEventType_Break:
									g_pAudioMgr->StopAction( pAction->ID(), rEventAction.TargetPlayingID );
									break;

								case AK::SoundEngine::AkActionOnEventType_Pause:
									g_pAudioMgr->PauseAction( pAction->ID(), rEventAction.TargetPlayingID );
									break;

								case AK::SoundEngine::AkActionOnEventType_Resume:
									g_pAudioMgr->ResumeAction( pAction->ID(), rEventAction.TargetPlayingID );
									break;
								default:
									AKASSERT(!"Unsupported AkActionOnEventType");
									break;
								}
							}
						}
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObjectEvent, AK_INVALID_PLAYING_ID, pItem->eventAction.GameObjID, pItem->eventAction.Event->ID(), false );
					}

					if ( pGameObj )
					{
						pGameObj->Release();
					}

					pItem->eventAction.Event->Release();
				}
				break;

			case QueuedMsgType_Seek:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->seek.GameObjID );
					if( pGameObj || pItem->seek.GameObjID == AK_INVALID_GAME_OBJECT )// Avoid performing if the GameObject was specified but does not exist.
					{
						AkQueuedMsg_Seek & rSeek = pItem->seek;
						// Find each action play in the list, and execute the action on their target.
						for( CAkEvent::AkActionList::Iterator iter = rSeek.Event->m_actions.Begin(); iter != rSeek.Event->m_actions.End(); ++iter )
						{
							CAkAction* pAction = *iter;
							AKASSERT( pAction );
							
							AkActionType aType = pAction->ActionType();
							if( aType == AkActionType_Play )
							{
								CAkActionPlay* pActionPlay = static_cast<CAkActionPlay*>( pAction );
								CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( pActionPlay->ElementID(), AkNodeType_Default );

								if( pNode )
								{
									SeekActionParams l_Params;
									l_Params.bIsFromBus = false;
									l_Params.bIsMasterResume = false;
									l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
									l_Params.pGameObj = pGameObj;
									l_Params.playingID = AK_INVALID_PLAYING_ID;
									l_Params.transParams.TransitionTime = 0;
									l_Params.bIsMasterCall = false;
									l_Params.bIsSeekRelativeToDuration = pItem->seek.bIsSeekRelativeToDuration;
									l_Params.bSnapToNearestMarker = pItem->seek.bSnapToMarker;
									l_Params.iSeekTime = rSeek.iPosition;
									l_Params.eType = ActionParamType_Seek;

									// Note: seeking of pending actions is useless, counterintuitive and thus not supported

									pNode->ExecuteAction( l_Params );
									pNode->Release();
								}
							}
						}
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObjectEvent, AK_INVALID_PLAYING_ID, pItem->seek.GameObjID, pItem->seek.Event->ID(), false );
					}

					if ( pGameObj )
					{
						pGameObj->Release();
					}

					pItem->seek.Event->Release();
				}
				break;

			case QueuedMsgType_RTPC:
				/// WARNING: Do not forget to keep QueuedMsgType_RTPCWithTransition in sync.
				if ( pItem->rtpc.GameObjID == AK_INVALID_GAME_OBJECT )
				{
					// No transition.
					TransParams transParams;
					transParams.TransitionTime = 0;					
					g_pRTPCMgr->SetRTPCInternal( pItem->rtpc.ID, pItem->rtpc.Value, NULL, transParams );
				}
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->rtpc.GameObjID );
					if ( pGameObj )
					{
						// No transition.
						TransParams transParams;
						transParams.TransitionTime = 0;
						g_pRTPCMgr->SetRTPCInternal( pItem->rtpc.ID, pItem->rtpc.Value, pGameObj, transParams );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->rtpc.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_RTPCWithTransition:
				
				if ( pItem->rtpc.GameObjID == AK_INVALID_GAME_OBJECT )
				{
					g_pRTPCMgr->SetRTPCInternal( pItem->rtpcWithTransition.ID, pItem->rtpcWithTransition.Value, NULL, pItem->rtpcWithTransition.transParams );
				}
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->rtpc.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->SetRTPCInternal( pItem->rtpcWithTransition.ID, pItem->rtpcWithTransition.Value, pGameObj, pItem->rtpcWithTransition.transParams );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->rtpcWithTransition.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_State:
				g_pStateMgr->SetStateInternal( 
					pItem->setstate.StateGroupID, 
					pItem->setstate.StateID, 
					pItem->setstate.bSkipTransition,
                    pItem->setstate.bSkipExtension);
				break;

			case QueuedMsgType_Switch:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->setswitch.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->SetSwitchInternal( 
							pItem->setswitch.SwitchGroupID, 
							pItem->setswitch.SwitchStateID, 
							pGameObj );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->setswitch.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_Trigger:
				{
					if ( pItem->trigger.GameObjID == AK_INVALID_GAME_OBJECT )
						g_pStateMgr->Trigger( pItem->trigger.TriggerID, NULL );
					else
					{
						CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->trigger.GameObjID );
						if( pGameObj )
						{
							g_pStateMgr->Trigger( pItem->trigger.TriggerID, pGameObj );
							pGameObj->Release();
						}
						else
						{
							MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->trigger.GameObjID, AK_INVALID_UNIQUE_ID, false );
						}
					}
				}
				break;

			case QueuedMsgType_RegisterGameObj:
				g_pRegistryMgr->RegisterObject( pItem->reggameobj.GameObjID, pItem->reggameobj.uListenerMask, pItem->reggameobj.pMonitorData );
				break;

			case QueuedMsgType_UnregisterGameObj:
				if ( pItem->unreggameobj.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRegistryMgr->UnregisterAll();
				else
					g_pRegistryMgr->UnregisterObject( pItem->unreggameobj.GameObjID );
				break;

			case QueuedMsgType_GameObjPosition:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetPosition( 
						pItem->gameobjpos.GameObjID, 
						&(pItem->gameobjpos.Position),
						1,
						AK::SoundEngine::MultiPositionType_SingleSource
						) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjpos.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

			case QueuedMsgType_GameObjMultiPosition:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetPosition( 
						pItem->gameObjMultiPos.GameObjID, 
						pItem->gameObjMultiPos.aMultiPosition, 
						pItem->gameObjMultiPos.uNumPositions, 
						pItem->gameObjMultiPos.eMultiPositionType
						) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameObjMultiPos.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

			case QueuedMsgType_GameObjActiveListeners:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetActiveListeners( pItem->gameobjactlist.GameObjID, pItem->gameobjactlist.uListenerMask ) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjactlist.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

#if defined AK_WII
			case QueuedMsgType_GameObjActiveControllers:
				if ( g_pRegistryMgr->SetActiveControllers( pItem->gameobjactcontroller.GameObjID, pItem->gameobjactcontroller.uActiveControllerMask ) != AK_Success )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjactcontroller.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;


#endif
			case QueuedMsgType_SetSecondaryOutputVolume:
				
#if defined AK_WII
				if (pItem->secondaryoutputvolume.idDevice == AkSink_Remote)
				{
					CAkWiimoteMgr::SetWiimoteVolume( pItem->secondaryoutputvolume.iPlayer, pItem->secondaryoutputvolume.fVolume );
				}
#elif !defined( AK_3DS )
				{
					AkSinkType eSinkType = (AkSinkType)pItem->secondaryoutputvolume.idDevice;
					AkDevice* pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY(eSinkType, pItem->secondaryoutputvolume.iPlayer) );
					if (pDevice)
					{
						AKASSERT(pDevice->pFinalMix != NULL);
						pDevice->pFinalMix->SetNextVolume( AkMath::FastLinTodB( pItem->secondaryoutputvolume.fVolume ) );
					}
					else
					{
						MONITOR_ERRORMSG2( AKTEXT("Error! Could not set volume for output device."), AKTEXT("") );
					}
					
				}
#endif
				break;

			case QueuedMsgType_ListenerPosition:
				CAkListener::SetListenerPosition( pItem->listpos.uListenerIndex, pItem->listpos.Position );
				break;

			case QueuedMsgType_ListenerSpatialization:
				CAkListener::SetListenerSpatialization( 
					pItem->listspat.uListenerIndex, 
					pItem->listspat.bSpatialized, 
					pItem->listspat.bSetVolumes ? &pItem->listspat.Volumes : NULL );
				break;

			case QueuedMsgType_ListenerPipeline:
				CAkListener::SetListenerPipeline(
					pItem->listpipe.uListenerIndex, 
					pItem->listpipe.bAudio,
					pItem->listpipe.bFeedback);
				break;

			case QueuedMsgType_GameObjEnvValues:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetGameObjectAuxSendValues( 
					pItem->gameobjenvvalues.GameObjID, 
					pItem->gameobjenvvalues.EnvValues, 
					pItem->gameobjenvvalues.uNumValues ) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjenvvalues.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

			case QueuedMsgType_GameObjDryLevel:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetGameObjectOutputBusVolume( 
					pItem->gameobjdrylevel.GameObjID, 
					pItem->gameobjdrylevel.fValue ) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjdrylevel.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

			case QueuedMsgType_GameObjScalingFactor:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetGameObjectScalingFactor( 
					pItem->gameobjscalingfactor.GameObjID, 
					pItem->gameobjscalingfactor.fValue ) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjscalingfactor.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

			case QueuedMsgType_ListenerScalingFactor:
				CAkListener::SetScalingFactor(
					pItem->listenerscalingfactor.uListenerIndex, 
					pItem->listenerscalingfactor.fValue );
				break;

			case QueuedMsgType_GameObjObstruction:
				if ( AK_EXPECT_FALSE( g_pRegistryMgr->SetObjectObstructionAndOcclusion( 
					pItem->gameobjobstr.GameObjID, 
					pItem->gameobjobstr.uListenerIndex, 
					pItem->gameobjobstr.fObstructionLevel, 
					pItem->gameobjobstr.fOcclusionLevel ) != AK_Success ) )
				{
					MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->gameobjobstr.GameObjID, AK_INVALID_UNIQUE_ID, false );
				}
				break;

#ifndef AK_OPTIMIZED
			case QueuedMsgType_ResetSwitches:
				if ( pItem->resetswitches.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->ResetSwitches();
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetswitches.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->ResetSwitches( pGameObj );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->resetswitches.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_ResetRTPC:
				if ( pItem->resetrtpc.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->ResetRTPC();
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetrtpc.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->ResetRTPC( pGameObj );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->resetrtpc.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;
#endif

			case QueuedMsgType_ResetRTPCValue:
				/// WARNING: Do not forget to keep QueuedMsgType_ResetRTPCValueWithTransition in sync.
				if ( pItem->resetrtpcvalue.GameObjID == AK_INVALID_GAME_OBJECT )
				{
					// No transition.
					TransParams transParams;
					transParams.TransitionTime = 0;
					g_pRTPCMgr->ResetRTPCValue( pItem->resetrtpcvalue.ParamID, NULL, transParams );
				}
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetrtpcvalue.GameObjID );
					if ( pGameObj )
					{
						// No transition.
						TransParams transParams;
						transParams.TransitionTime = 0;
						g_pRTPCMgr->ResetRTPCValue( pItem->resetrtpcvalue.ParamID, pGameObj, transParams );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->resetrtpcvalue.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_ResetRTPCValueWithTransition:
				if ( pItem->resetrtpcvalue.GameObjID == AK_INVALID_GAME_OBJECT )
					g_pRTPCMgr->ResetRTPCValue( pItem->resetrtpcvalueWithTransition.ParamID, NULL, pItem->resetrtpcvalueWithTransition.transParams );
				else
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->resetrtpcvalue.GameObjID );
					if ( pGameObj )
					{
						g_pRTPCMgr->ResetRTPCValue( pItem->resetrtpcvalueWithTransition.ParamID, pGameObj, pItem->resetrtpcvalueWithTransition.transParams );
						pGameObj->Release();
					}
					else
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->resetrtpcvalueWithTransition.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			///////////////////////////////////////
			// Dynamic Sequence Related messages
			/////////////////////////////////////
			case QueuedMsgType_OpenDynamicSequence:
				{
					CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( pItem->opendynamicsequence.GameObjID );
					if ( pGameObj )
					{
						pItem->opendynamicsequence.pDynamicSequence->SetGameObject( pGameObj );
						pGameObj->Release();
					}
					else if ( pItem->opendynamicsequence.GameObjID != AK_INVALID_GAME_OBJECT )
					{
						MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, pItem->opendynamicsequence.GameObjID, AK_INVALID_UNIQUE_ID, false );
					}
				}
				break;

			case QueuedMsgType_DynamicSequenceCmd:
				switch ( pItem->dynamicsequencecmd.eCommand )
				{
				case AkQueuedMsg_DynamicSequenceCmd::Play:
					pItem->dynamicsequencecmd.pDynamicSequence->Play( pItem->dynamicsequencecmd.uTransitionDuration, pItem->dynamicsequencecmd.eFadeCurve );
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Pause:
					pItem->dynamicsequencecmd.pDynamicSequence->Pause( pItem->dynamicsequencecmd.uTransitionDuration, pItem->dynamicsequencecmd.eFadeCurve );
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Resume:
					pItem->dynamicsequencecmd.pDynamicSequence->Resume( pItem->dynamicsequencecmd.uTransitionDuration, pItem->dynamicsequencecmd.eFadeCurve );
					break;

				case AkQueuedMsg_DynamicSequenceCmd::ResumeWaiting:
					pItem->dynamicsequencecmd.pDynamicSequence->ResumeWaiting();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Stop:
					pItem->dynamicsequencecmd.pDynamicSequence->Stop( pItem->dynamicsequencecmd.uTransitionDuration, pItem->dynamicsequencecmd.eFadeCurve );
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Break:
					pItem->dynamicsequencecmd.pDynamicSequence->Break();
					break;

				case AkQueuedMsg_DynamicSequenceCmd::Close:
					pItem->dynamicsequencecmd.pDynamicSequence->Close();

					// Ok, we can not get rid of the playing id for which we add an active count in the OpenDynamicSequence
					g_pPlayingMgr->RemoveItemActiveCount( pItem->dynamicsequencecmd.pDynamicSequence->GetPlayingID() );

					// Release for the creation by the OpenDynamicSequence message
					pItem->dynamicsequencecmd.pDynamicSequence->Release();
					break;
				}

				// Release for enqueue item
				AKASSERT( pItem->dynamicsequencecmd.pDynamicSequence );
				pItem->dynamicsequencecmd.pDynamicSequence->Release();
				break;

			case QueuedMsgType_StopAll:
				{
					AkGameObjectID gameObjectID = pItem->stopAll.GameObjID;
					MONITOR_MSGEX( AK::Monitor::ErrorCode_StopAll, AK_INVALID_PLAYING_ID, gameObjectID, AK_INVALID_UNIQUE_ID, false );

					AkActionType actionType = gameObjectID == AK_INVALID_GAME_OBJECT ? AkActionType_Stop_ALL : AkActionType_Stop_ALL_O;
					CAkActionStop* pStopAction = CAkActionStop::Create( actionType );
					if( pStopAction )
					{
						if( gameObjectID != AK_INVALID_GAME_OBJECT )
						{
							CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( gameObjectID );
							if ( pGameObj )
							{
								AkPendingAction PendingAction( pGameObj );// dummy with no game object.
								pStopAction->Execute( &PendingAction );
								pGameObj->Release();
							}
							else
							{
								MONITOR_ERROREX( AK::Monitor::ErrorCode_UnknownGameObject, AK_INVALID_PLAYING_ID, gameObjectID, AK_INVALID_UNIQUE_ID, false );
							}
						}
						else
						{
							AkPendingAction PendingAction( NULL );// dummy with no game object.
							pStopAction->Execute( &PendingAction );
						}
						pStopAction->Release();
					}
				}
				break;

			case QueuedMsgType_StopPlayingID:
				{
					MONITOR_MSGEX( AK::Monitor::ErrorCode_StopPlayingID, pItem->stopEvent.playingID, AK_INVALID_GAME_OBJECT, AK_INVALID_UNIQUE_ID, false );

					ClearPendingItems( pItem->stopEvent.playingID );

					ActionParams l_Params;
					l_Params.bIsFromBus = false;
					l_Params.bIsMasterResume = false;
					l_Params.transParams.eFadeCurve = pItem->stopEvent.eFadeCurve;
					l_Params.eType = ActionParamType_Stop;
					l_Params.pGameObj = NULL;
					l_Params.playingID = pItem->stopEvent.playingID;
					l_Params.transParams.TransitionTime = pItem->stopEvent.uTransitionDuration;
					l_Params.bIsMasterCall = false;

					CAkBus::ExecuteMasterBusAction( l_Params );
					
				}
				break;

			case QueuedMsgType_KillBank:
				{
					CAkUsageSlot* pUsageSlot = pItem->killbank.pUsageSlot;
					AKASSERT( pUsageSlot );

					CAkURenderer::StopAllPBIs( pUsageSlot );
					pUsageSlot->Release( false );
				}
				break;
			case QueuedMsgType_AddRemovePlayerDevice:
				{
#ifdef AK_MOTION
					if (pItem->playerdevice.idDevice == AKMOTIONDEVICEID_RUMBLE)
					{

						if (pItem->playerdevice.bAdd)
						{
							//Always create the device manager (checks are done inside).  It is the only way to enable feedback.
							CAkFeedbackDeviceMgr::Create()->AddPlayerFeedbackDevice((AkUInt8)pItem->playerdevice.iPlayer, 0, pItem->playerdevice.idDevice, pItem->playerdevice.pDevice );
						}
						else
						{
							CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
							if(pMgr != NULL)
								pMgr->RemovePlayerFeedbackDevice((AkUInt8)pItem->playerdevice.iPlayer, 0, pItem->playerdevice.idDevice);
						}
					}
					else
#endif	
					{
#if !defined AK_WII && !defined AK_3DS
						//Audio device
						if (pItem->playerdevice.bAdd)
						{
							//For now, use the default audio settings.
							AkSinkType eSinkType = (AkSinkType)pItem->playerdevice.idDevice;	//TBD, use only sink type.
							AkOutputSettings settings;
							CAkLEngine::GetDefaultOutputSettings(eSinkType, settings);
							if( CAkOutputMgr::AddOutputDevice(settings, eSinkType, pItem->playerdevice.iPlayer, pItem->playerdevice.uListeners, pItem->playerdevice.pDevice) == AK_Success)
							{
								CAkOutputMgr::GetDevice(AK_MAKE_DEVICE_KEY(eSinkType, pItem->playerdevice.iPlayer))->pSink->Play();
							}
							else
							{
								MONITOR_ERRORMSG2( AKTEXT("Error! Initialization of output device failed."), AKTEXT("") );
							}
							
						}
						else
						{
							CAkOutputMgr::RemoveOutputDevice(AK_MAKE_DEVICE_KEY(pItem->playerdevice.idDevice, pItem->playerdevice.iPlayer));
						}
#endif
					}

				}
				break;
#ifdef AK_MOTION
			case QueuedMsgType_SetPlayerListener:
				{
					CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
					if(pMgr != NULL)
						pMgr->SetPlayerListener(pItem->playerlistener.iPlayer, pItem->playerlistener.iListener);
				}
				break;
			case QueuedMsgType_SetPlayerVolume:
				{
					CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
					if(pMgr != NULL)
						pMgr->SetPlayerVolume((AkUInt8)pItem->playervolume.iPlayer, pItem->playervolume.fVolume);
				}
				break;
#endif // AK_MOTION

			case QueuedMsgType_SetEffect:
				{
					CAkParameterNodeBase * pNode = g_pIndex->GetNodePtrAndAddRef( pItem->setEffect.audioNodeID, pItem->setEffect.eNodeType );
					if ( pNode )
					{
						AkNodeCategory eCat = pNode->NodeCategory();
						if ( eCat == AkNodeCategory_Bus
							|| eCat == AkNodeCategory_AuxBus
							|| eCat == AkNodeCategory_ActorMixer
							|| eCat == AkNodeCategory_RanSeqCntr
							|| eCat == AkNodeCategory_Sound
							|| eCat == AkNodeCategory_SwitchCntr
							|| eCat == AkNodeCategory_LayerCntr 
							|| eCat == AkNodeCategory_MusicTrack
							|| eCat == AkNodeCategory_MusicSegment
							|| eCat == AkNodeCategory_MusicRanSeqCntr
							|| eCat == AkNodeCategory_MusicSwitchCntr )
						{
							CAkParameterNodeBase * pParamNode = (CAkParameterNodeBase *) pNode;

							if ( pParamNode->SetFX( pItem->setEffect.uFXIndex, pItem->setEffect.shareSetID, true ) != AK_Success )
							{
								MONITOR_ERRORMSG( AKTEXT("SetEffect: set effect failed") );
							}
						}
						else
						{
							MONITOR_ERRORMSG( AKTEXT("SetEffect: audio node type not supported") );
						}
						pNode->Release();
					}
					else
					{
						MONITOR_ERRORMSG( AKTEXT("SetEffect: audio node not found") );
					}
				}
				break;

			case QueuedMsgType_SetPanningRule:
				CAkLEngine::SetPanningRule( pItem->setPanningRule.iOutputID, pItem->setPanningRule.eSinkType, pItem->setPanningRule.panRule );
				break;

			case QueuedMsgType_SetSpeakerAngles:
				{
					AKASSERT( pItem->setSpeakerAngles.pfSpeakerAngles );
					AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAKE_DEVICE_KEY( pItem->setSpeakerAngles.eSinkType, pItem->setSpeakerAngles.iOutputID ) );
					if ( pDevice )
					{
						if ( pDevice->SetSpeakerAngles( pItem->setSpeakerAngles.pfSpeakerAngles, pItem->setSpeakerAngles.uNumAngles ) != AK_Success )
						{
							MONITOR_ERRORMSG( AKTEXT("SetSpeakerAngles failed") );
						}
					}
					else
					{
						MONITOR_ERRORMSG( AKTEXT("SetSpeakerAngles: Device not found") );
					}
					AkFree( g_DefaultPoolId, pItem->setSpeakerAngles.pfSpeakerAngles );
				}
				break;

	#if !(defined( AK_WII ) && defined( AK_OPTIMIZED ))
			case QueuedMsgType_StartStopOutputCapture:
				{
					if (g_pAkSink != NULL)
					{
					if (pItem->outputCapture.szFileName != NULL)
					{
						CAkOutputMgr::StartOutputCapture(pItem->outputCapture.szFileName);
#ifdef AK_MOTION
						if (pItem->outputCapture.bCaptureMotion)
						{
							CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
							if(pMgr != NULL)
								pMgr->StartOutputCapture(pItem->outputCapture.szFileName);
						}
#endif // AK_MOTION
						AkFree(g_DefaultPoolId, pItem->outputCapture.szFileName);
					}
					else
					{
						CAkOutputMgr::StopOutputCapture();
#ifdef AK_MOTION
						CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
						if(pMgr != NULL)
							pMgr->StopOutputCapture();
#endif // AK_MOTION
						}
					}
				}
			break;
#endif

#ifdef AK_WIIU
			case QueuedMsgType_SetProcessMode:
				CAkLEngine::SetProcessMode(pItem->processMode.eStatus, pItem->processMode.msFade, pItem->processMode.eCurve, pItem->processMode.Callback, pItem->processMode.pUserData);
				break;
			case QueuedMsgType_SendMainOutputToDevice:
				CAkLEngine::SendMainOutputToDevice(pItem->mainOutputDevice.eDevice);
				break;
#endif
			}
			
			pData += pItem->size;
			uSizeRead += pItem->size;
			
			// For each event processed, increment the synchronization count.
			// for RTPC and switches too, since they can cause new playback to occur.
			CAkLEngineCmds::IncrementSyncCount();
		}
		while ( uSizeRead < uSizeAvail && !bReachedEnd );

		m_queueLock.Lock();

		m_MsgQueue.EndRead( uSizeRead );
	}
	while( !bReachedEnd );

	// If we were asked to drain the queue, make sure to reset
	m_bDrainMsgQueue = false;

	m_queueLock.Unlock();

	// We are setting this event in case multiple threads have asked the audio
	// thread to drain the queue
	if ( !AK_PERF_OFFLINE_RENDERING )
		m_hEventMgrThreadDrainEvent.Signal();

	return AK_Success;
}
//-----------------------------------------------------------------------------
// Name: ProcessPendingList
// Desc: figure out if some of those pending are ready to run.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//----------------------------------------------------------------------------- 
void CAkAudioMgr::ProcessPendingList()
{
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		// is it time to go ?
		if( (*iter).key <= m_uBufferTick)
		{
			AkPendingAction* pPendingAction = (*iter).item;
			m_mmapPending.Erase( iter );

			NotifyDelayEnded( pPendingAction );

			ProcessAction( pPendingAction );
			
			//increment the sync count for pending events too, so that they don't sync with next events.
			CAkLEngineCmds::IncrementSyncCount();
			
			iter = m_mmapPending.BeginEx();
		}
		else
        {
			break;
        }
	}
}

//-----------------------------------------------------------------------------
// Name: EnqueueOrExecuteAction
// Desc: Enqueue or execute an action.
//
// Parameters:
//	AkPendingAction* in_pActionItem : Action to enqueue.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::EnqueueOrExecuteAction( AkPendingAction* in_pActionItem )
{
	AKASSERT(in_pActionItem);
	AKASSERT(in_pActionItem->pAction);

	g_pPlayingMgr->AddItemActiveCount(in_pActionItem->UserParam.PlayingID());

	// make sure it won't get thrown away
	in_pActionItem->pAction->AddRef();

	AkUInt32 delayS = in_pActionItem->pAction->GetDelayTime();
	AkUInt32 delayTicks = delayS / AK_NUM_VOICE_REFILL_FRAMES;

	in_pActionItem->LaunchTick = m_uBufferTick;
	in_pActionItem->LaunchFrameOffset = delayS - delayTicks * AK_NUM_VOICE_REFILL_FRAMES;

	if( delayTicks == 0 )
	{
		ProcessAction( in_pActionItem );
	}
	else
	{
		// Special case for Play and Continue actions.
		// Post ahead of time to lower engine to reduce latency due to streaming.
		if ( in_pActionItem->pAction->ActionType() == AkActionType_PlayAndContinue )
		{
			AkUInt32 uNumFramesLookAhead = AkMin( delayTicks, g_settings.uContinuousPlaybackLookAhead );
			delayTicks -= uNumFramesLookAhead;

			in_pActionItem->LaunchFrameOffset = delayS - delayTicks * AK_NUM_VOICE_REFILL_FRAMES;

			if ( delayTicks == 0 )
			{
				// Action should be processed now after all. Process and bail out.
				ProcessAction( in_pActionItem ); 
				return;
			}
		}
		in_pActionItem->LaunchTick += delayTicks;
		if( m_mmapPending.Insert( in_pActionItem->LaunchTick, in_pActionItem ) == AK_Success )
		{
			MONITOR_ACTIONDELAYED( in_pActionItem->UserParam.PlayingID(), in_pActionItem->pAction->ID(), in_pActionItem->GameObjID(), delayS * 1000 / AK_CORE_SAMPLERATE, in_pActionItem->UserParam.CustomParam() );
			NotifyDelayStarted( in_pActionItem );
		}
		else
		{
			FlushAndCleanPendingAction( in_pActionItem );
		}
	}
}

void CAkAudioMgr::FlushAndCleanPendingAction( AkPendingAction* in_pPendingAction )
{
	in_pPendingAction->pAction->Release();
	AkDelete( g_DefaultPoolId, in_pPendingAction );
}

//-----------------------------------------------------------------------------
// Name: ProcessActionQueue
// Desc: Process the action queue.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ProcessAction( AkPendingAction * in_pAction ) 
{
	AKASSERT( in_pAction->pAction );

	MONITOR_ACTIONTRIGGERED( in_pAction->UserParam.PlayingID(), in_pAction->pAction->ID(), in_pAction->GameObjID(), in_pAction->UserParam.CustomParam() );

	// execute it
	in_pAction->pAction->Execute( in_pAction );

	// Important to do it AFTER calling execute, that will avoid having the Playing mgr thinking that the Playing ID is dead.
	if( in_pAction->UserParam.PlayingID() )
	{
		g_pPlayingMgr->RemoveItemActiveCount( in_pAction->UserParam.PlayingID() );
	}

	// we don't care anymore about that one
	in_pAction->pAction->Release();

	// get rid of it
	AkDelete( g_DefaultPoolId, in_pAction );
}

//-----------------------------------------------------------------------------
// Name: ProcessCustomAction
//-----------------------------------------------------------------------------
void CAkAudioMgr::ProcessCustomAction( 
		CAkParameterNodeBase* ptargetNode, 
		CAkRegisteredObj * in_pGameObj, 
		AK::SoundEngine::AkActionOnEventType in_ActionToExecute,
		AkTimeMs in_uTransitionDuration,
		AkCurveInterpolation in_eFadeCurve,
		AkPlayingID	in_PlayingID
	)
{
	if( ptargetNode )
	{
		ActionParams l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterResume = false;
		l_Params.transParams.eFadeCurve = in_eFadeCurve;
		l_Params.pGameObj = in_pGameObj;
		l_Params.playingID = in_PlayingID;
		l_Params.transParams.TransitionTime = in_uTransitionDuration;
		l_Params.bIsMasterCall = false;
		l_Params.targetNodePtr = ptargetNode;

		switch( in_ActionToExecute )
		{
		case AK::SoundEngine::AkActionOnEventType_Stop:
			g_pAudioMgr->StopPendingAction( ptargetNode, in_pGameObj, in_PlayingID );
			l_Params.eType = ActionParamType_Stop;
			break;

		case AK::SoundEngine::AkActionOnEventType_Pause:
			g_pAudioMgr->PausePendingAction( ptargetNode, in_pGameObj, true, in_PlayingID );
			l_Params.eType = ActionParamType_Pause;
			break;

		case AK::SoundEngine::AkActionOnEventType_Resume:
			g_pAudioMgr->ResumePausedPendingAction( ptargetNode, in_pGameObj, false, in_PlayingID );
			l_Params.eType = ActionParamType_Resume;
			break;

		case AK::SoundEngine::AkActionOnEventType_Break:
			g_pAudioMgr->BreakPendingAction( ptargetNode, in_pGameObj, in_PlayingID );
			l_Params.eType = ActionParamType_Break;
			break;

		default:
			AKASSERT( !"Unexpected custom action type" );
			return;
		};

		ptargetNode->ExecuteAction( l_Params );
	}
}

void CAkAudioMgr::PlaySourceInput( AkUniqueID in_Target, CAkRegisteredObj* in_pGameObj, UserParams in_userParams )
{
    CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( in_Target, AkNodeType_Default );

	if(pNode)
	{
		TransParams	Tparameters;

		Tparameters.TransitionTime = 0;
		Tparameters.eFadeCurve = AkCurveInterpolation_Linear;

		AkPBIParams pbiParams;
        
		pbiParams.eType = AkPBIParams::PBI;
        pbiParams.pInstigator = pNode;
        pbiParams.userParams = in_userParams;
		pbiParams.ePlaybackState = PB_Playing;
		pbiParams.uFrameOffset = 0;
        pbiParams.bIsFirst = true;

		pbiParams.pGameObj = in_pGameObj;

		pbiParams.pTransitionParameters = &Tparameters;
        pbiParams.pContinuousParams = NULL;
        pbiParams.sequenceID = AK_INVALID_SEQUENCE_ID;

		static_cast<CAkParameterNode*>(pNode)->Play( pbiParams );

		pNode->Release();
	}
	else
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_userParams.PlayingID(), in_pGameObj->ID(), in_userParams.CustomParam(), AkMonitorData::NotificationReason_PlayFailed, HistArray, in_Target, false, 0 );
        MONITOR_ERROREX( AK::Monitor::ErrorCode_SelectedNodeNotAvailablePlay, in_userParams.PlayingID(), in_pGameObj->ID(), in_Target, false );
	}
}

//-----------------------------------------------------------------------------
// Name: PausePendingAction
//-----------------------------------------------------------------------------
void CAkAudioMgr::PausePendingAction( CAkParameterNodeBase * in_pNodeToTarget, 
										  CAkRegisteredObj * in_GameObj,
										  bool		in_bIsMasterOnResume,
										  AkPlayingID in_PlayingID)
{
	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	// scan'em all

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;
		pAction = pThisAction->pAction;

		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach( pAction->GetAndRefTarget() );

		// is it ours ? we do pause Resume actions only if we are a master pause
		if(
			(!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ) )
			&& ( ( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{
			++(pThisAction->ulPauseCount);
		}

		++iterPaused;
	}

	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;
		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach( pAction->GetAndRefTarget() );

		// is it ours ? we do pause Resume actions only if we are a master pause
		if(
			(!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ) )
			&& ( ( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}
//--------------------------------------------------------------------------------------------
// Name: PausePendingItem
// Desc: move actions affecting an audionode (in_ulAudioNodeID) pending in the paused pending.
//
// Parameters:
//	AkUniqueID	 in_ulElementID		   :
//	CAkRegisteredObj * in_GameObj		       :
//-----------------------------------------------------------------------------
void CAkAudioMgr::PausePendingItems( AkPlayingID in_PlayingID )
{
	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	// scan'em all

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;

		// is it ours ? we do pause Resume actions only if they match our playing id
		if(	   ( pThisAction->UserParam.PlayingID() == in_PlayingID )
			&& ( pAction->ActionType() != AkActionType_Duck ) )
		{
			++(pThisAction->ulPauseCount);
		}

		++iterPaused;
	}

	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;

		// is it ours ? we do pause Resume actions only if they match our playing id
		if(	   ( pThisAction->UserParam.PlayingID() == in_PlayingID )
			&& ( pAction->ActionType() != AkActionType_Duck ) )
		{
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void CAkAudioMgr::InsertAsPaused( AkUniqueID in_ElementID, AkPendingAction* in_pPendingAction, AkUInt32 in_ulPauseCount )
{
	in_pPendingAction->PausedTick = m_uBufferTick;
	in_pPendingAction->ulPauseCount = in_ulPauseCount;

	// shove the action in the PausedPending list
	AKRESULT eResult = m_mmapPausedPending.Insert( in_ElementID, in_pPendingAction );

	if( eResult == AK_Success )
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID(), in_pPendingAction->GameObjID(), in_pPendingAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_Paused, HistArray, in_pPendingAction->pAction->ID(), false, 0 );
	}
	else
	{
		MONITOR_ERRORMSG( AKTEXT("Pending action was destroyed because a critical memory allocation failed.") );
		NotifyDelayAborted( in_pPendingAction, false );
		FlushAndCleanPendingAction( in_pPendingAction );
	}
}

void CAkAudioMgr::TransferToPending( AkPendingAction* in_pPendingAction )
{
	// offset the launch time by the pause duration
	in_pPendingAction->LaunchTick += ( m_uBufferTick - in_pPendingAction->PausedTick );

	// shove it action in the Pending list
	AKRESULT eResult = m_mmapPending.Insert( in_pPendingAction->LaunchTick, in_pPendingAction );

	if( eResult == AK_Success )
	{
		// Here we must send the resume notification anyway!, to balance the pause notification.
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pPendingAction->UserParam.PlayingID(), in_pPendingAction->GameObjID(), in_pPendingAction->UserParam.CustomParam(), AkMonitorData::NotificationReason_Resumed, HistArray, in_pPendingAction->pAction->ID(), false, 0 );
	}
	else
	{
		MONITOR_ERRORMSG( AKTEXT("Pending action was destroyed because a critical memory allocation failed.") );
		NotifyDelayAborted( in_pPendingAction, true );
		FlushAndCleanPendingAction( in_pPendingAction );
	}
}

//-----------------------------------------------------------------------------
// Name: BreakPendingAction
// Desc: Break pending actions.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::BreakPendingAction( CAkParameterNodeBase * in_pNodeToTarget, 
										  CAkRegisteredObj * in_GameObj,
										  AkPlayingID in_PlayingID )
{
	AKRESULT	eResult = AK_Success;

	CAkAction* pAction;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
			pTargetNode.Attach( pAction->GetAndRefTarget() );
			if(
				(!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ) )
				&& 
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				)
			{	
				bool l_bFlush = false;
				switch( pAction->ActionType() )
				{
				case AkActionType_PlayAndContinue:
					l_bFlush = static_cast<CAkActionPlayAndContinue*>( pPendingAction->pAction )->BreakToNode( in_pNodeToTarget, pPendingAction->GameObj(), pPendingAction );
					break;
				case AkActionType_Play:
					l_bFlush = true;
					break;
				case AkActionType_None:
				case AkActionType_Stop_E:
				case AkActionType_Stop_E_O:
					break;
				}

				if( l_bFlush )
				{
					NotifyDelayAborted( pPendingAction, false );
					iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
			pTargetNode.Attach( pAction->GetAndRefTarget() );
			if(
				( !in_pNodeToTarget || IsElementOf( in_pNodeToTarget,pTargetNode ) )
				&& 
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				&& 
				( pAction->ActionType() == AkActionType_PlayAndContinue )
				)
			{
				bool l_bFlush = static_cast<CAkActionPlayAndContinue*>( pPendingAction->pAction )->BreakToNode( in_pNodeToTarget, pPendingAction->GameObj(), pPendingAction );
				if( l_bFlush )
				{
					NotifyDelayAborted( pPendingAction, true );
					iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: StopPendingAction
// Desc: Stop pending actions.
//
// Parameters:
// AkUniqueID   in_TargetID			  :
// CAkRegisteredObj * in_GameObj            :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopPendingAction(  CAkParameterNodeBase * in_pNodeToTarget, 
										 CAkRegisteredObj * in_GameObj,
										 AkPlayingID in_PlayingID )
{
	AKRESULT	eResult = AK_Success;

	CAkAction* pAction;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
			pTargetNode.Attach( pAction->GetAndRefTarget() );
			if(
				(!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ) )
				&& 
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				&& 
				( pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			pAction = pPendingAction->pAction;
			CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
			pTargetNode.Attach( pAction->GetAndRefTarget() );
			if(
				( !in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ) )
				&& 
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				&& 
				( pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: PausePendingActionAllExcept
// Desc:
//
// Parameters:
// CAkRegisteredObj *		in_GameObj				:
// ExceptionList*	in_pExceptionList		:
//-----------------------------------------------------------------------------
void CAkAudioMgr::PausePendingActionAllExcept(	CAkRegisteredObj *	in_GameObj, 
												ExceptionList*		in_pExceptionList,
												bool				in_bIsMasterOnResume,
												AkPlayingID in_PlayingID )
{
	AkPendingAction*	pThisAction = NULL;
	CAkAction*			pAction		= NULL;

	AkMultimapPausedPending::Iterator iterPaused = m_mmapPausedPending.Begin();
	while( iterPaused != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPaused).item;
		pAction = pThisAction->pAction;

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(
			(( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&& !IsAnException( pAction, in_pExceptionList )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{	
			++(pThisAction->ulPauseCount);
		}
		++iterPaused;
	}

	// scan'em all
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(
			(( (pAction->ActionType() & ACTION_TYPE_ACTION) != ACTION_TYPE_RESUME) || in_bIsMasterOnResume )
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&& !IsAnException( pAction, in_pExceptionList )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{	
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: StopPendingActionAllExcept
// Desc:
//
// Parameters:
// CAkRegisteredObj *	  in_GameObj		    :
// ExceptionList& in_pExceptionList      :
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopPendingActionAllExcept( CAkRegisteredObj * in_GameObj,
												  ExceptionList* in_pExceptionList,
												  AkPlayingID in_PlayingID )
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if(
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				&& !IsAnException( pPendingAction->pAction, in_pExceptionList )
				&& ( pPendingAction->pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if(
				CheckObjAndPlayingID( in_GameObj, in_PlayingID, pPendingAction )
				&& !IsAnException( pPendingAction->pAction, in_pExceptionList )
				&& ( pPendingAction->pAction->ActionType() != AkActionType_Duck )
				)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ResumePausedPendingAction
// Desc: move paused pending actions(in_ulElementID) in the pending list
//
// Parameters:
//	AkUniqueID		in_ulElementID		  :
//	CAkRegisteredObj *	in_GameObj			  :
//-----------------------------------------------------------------------------
void CAkAudioMgr::ResumePausedPendingAction( CAkParameterNodeBase * in_pNodeToTarget, 
												 CAkRegisteredObj *	in_GameObj,
												 bool			in_bIsMasterOnResume,
												 AkPlayingID in_PlayingID )
{
	AkPendingAction* pThisAction;

	// if we've got it then move it

	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach( pThisAction->pAction->GetAndRefTarget() );
		if( (!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ))
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			)
		{
			if( in_bIsMasterOnResume || pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}

	ResumeNotPausedPendingAction( in_pNodeToTarget, in_GameObj, in_bIsMasterOnResume, in_PlayingID );
}

void CAkAudioMgr::ResumePausedPendingItems( AkPlayingID in_playingID )
{
	AkPendingAction* pThisAction;

	// if we've got it then move it
	
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if( pThisAction->UserParam.PlayingID() == in_playingID )
		{
			if( pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
}

void CAkAudioMgr::ResumeNotPausedPendingAction( CAkParameterNodeBase * in_pNodeToTarget,
												CAkRegisteredObj *	in_GameObj,
												bool				/*in_bIsMasterOnResume*/,
												AkPlayingID in_PlayingID)
{
	//in_bIsMasterOnResume here is unused for now, but will be useful when supporting pause counting
	AkPendingAction* pThisAction;

	// if we've got it then move it
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		CAkAction* pAction = pThisAction->pAction;
		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach( pAction->GetAndRefTarget() );
		if( (!in_pNodeToTarget || IsElementOf( in_pNodeToTarget, pTargetNode ))
			&& CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			)
		{	 
			if( pAction->ActionType() == AkActionType_PlayAndContinue )
			{
				CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
				pActionPAC->Resume();
			}
		}
		++iter;
	}
}

//-----------------------------------------------------------------------------
// Name: ResumePausedPendingActionAllExcept
// Desc: Execute behavioural side and lower engine side.
//
// Parameters:
// CAkRegisteredObj *		in_GameObj			  :
// ExceptionList&	in_pExceptionList      :
//-----------------------------------------------------------------------------
void CAkAudioMgr::ResumePausedPendingActionAllExcept(CAkRegisteredObj * in_GameObj, 
														 ExceptionList* in_pExceptionList,
														 bool			in_bIsMasterOnResume,
														 AkPlayingID in_PlayingID )
{
	AkPendingAction*	pThisAction = NULL;

	// if we've got it then move it
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if(
			CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&& !IsAnException( pThisAction->pAction, in_pExceptionList )
			)
		{	
			if( in_bIsMasterOnResume || pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
	g_pAudioMgr->ResumeNotPausedPendingActionAllExcept( in_GameObj, in_pExceptionList, in_bIsMasterOnResume, in_PlayingID );
}

void CAkAudioMgr::ResumeNotPausedPendingActionAllExcept(CAkRegisteredObj * in_GameObj, 
														ExceptionList*  in_pExceptionList,
														bool			/*in_bIsMasterOnResume*/,
														AkPlayingID in_PlayingID)
{
	//in_bIsMasterOnResume here is unused for now, but will be useful when supporting pause counting

	AkPendingAction*	pThisAction = NULL;

	// if we've got it then move it
	AkMultimapPending::Iterator iter = m_mmapPending.Begin();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		CAkAction* pAction = pThisAction->pAction;
		if(
			CheckObjAndPlayingID( in_GameObj, in_PlayingID, pThisAction )
			&&
			!IsAnException( pAction, in_pExceptionList )
			)
		{	 
			if( pAction->ActionType() == AkActionType_PlayAndContinue )
			{
				CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
				pActionPAC->Resume();
			}
		}
		++iter;
	}
}
//-----------------------------------------------------------------------------
// Name: RemovePendingAction
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemovePendingAction( CAkParameterNodeBase * in_pNodeToTarget )
{
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		AkPendingAction* pPending = (*iter).item;
		CAkAction* pAction = pPending->pAction;
		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach( pAction->GetAndRefTarget() );
		if( 
			IsElementOf( in_pNodeToTarget, pTargetNode )
			&& ( pAction->ActionType() != AkActionType_Duck )
			)
		{
			NotifyDelayAborted( pPending, false );

			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
		else
		{
			++iter;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemovePausedPendingAction
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemovePausedPendingAction( CAkParameterNodeBase * in_pNodeToTarget )
{
	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		AkPendingAction* pPending = (*iter).item;
		CAkSmartPtr<CAkParameterNodeBase> pTargetNode;
		pTargetNode.Attach(pPending->pAction->GetAndRefTarget() );
		if( IsElementOf( in_pNodeToTarget, pTargetNode ) )
		{
			NotifyDelayAborted( pPending, true );

			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
		else
		{
			++iter;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPausedPendingAction
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPausedPendingAction()
{
	if ( m_mmapPausedPending.IsInitialized() )
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			AkPendingAction* pPending = (*iter).item;

			NotifyDelayAborted( pPending, true );

			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPendingAction
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPendingAction()
{
	if ( m_mmapPending.IsInitialized() )
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			AkPendingAction* pPending = (*iter).item;
			
			NotifyDelayAborted( pPending, false );

			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveAllPreallocAndReferences
// Desc: Message queue clean-up before destroying the Audio Manager.
//		 Free command queue's pre-allocated and pre-referenced items (that is, that were allocated 
//		 or referenced by the game thread, in AkAudioLib). 
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::RemoveAllPreallocAndReferences()
{
	AkAutoLock<CAkLock> protectQueuePurge( m_queueLock );

	while(!m_MsgQueue.IsEmpty())
	{
		AkQueuedMsg * pItem = (AkQueuedMsg *) m_MsgQueue.BeginRead();
		AKASSERT( pItem );

		switch ( pItem->type )
		{
		case QueuedMsgType_RegisterGameObj:
			if ( pItem->reggameobj.pMonitorData )
			{
				MONITOR_FREESTRING( pItem->reggameobj.pMonitorData );
			}
			break;

		case QueuedMsgType_Event:
			AKASSERT( pItem->event.Event );
			g_pPlayingMgr->RemoveItemActiveCount( pItem->event.PlayingID );
			pItem->event.Event->Release();
			if (pItem->event.CustomParam.pExternalSrcs != NULL)
				pItem->event.CustomParam.pExternalSrcs->Release();
			break;

		case QueuedMsgType_Seek:
			AKASSERT( pItem->seek.Event );
			pItem->seek.Event->Release();
			break;

		case QueuedMsgType_EventAction:
			AKASSERT( pItem->eventAction.Event );
			pItem->eventAction.Event->Release();
			break;

		case QueuedMsgType_DynamicSequenceCmd:
			AKASSERT( pItem->dynamicsequencecmd.pDynamicSequence );
			if( AkQueuedMsg_DynamicSequenceCmd::Close == pItem->dynamicsequencecmd.eCommand )
			{
				g_pPlayingMgr->RemoveItemActiveCount( pItem->dynamicsequencecmd.pDynamicSequence->GetPlayingID() );
				pItem->dynamicsequencecmd.pDynamicSequence->Release();// Yes, we must release twice.
			}
			pItem->dynamicsequencecmd.pDynamicSequence->Release();
			break;
			
		/// TODO: Add cases for messages that need special cleanup.
		}

		m_MsgQueue.EndRead( pItem->size );
	}
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayStarted
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayStarted(AkPendingAction* in_pPending)
{
	NotifyDelay( in_pPending, AkMonitorData::NotificationReason_Delay_Started, false );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayAborted
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayAborted( AkPendingAction* in_pPending, bool in_bWasPaused )
{
	NotifyDelay(in_pPending, AkMonitorData::NotificationReason_Delay_Aborted, in_bWasPaused );
	g_pPlayingMgr->RemoveItemActiveCount( in_pPending->UserParam.PlayingID() );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelayEnded
// Desc:
//
// Parameters:
//	AkPendingAction* in_pPending : Point to a pending action.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelayEnded(AkPendingAction* in_pPending, bool in_bWasPaused )
{
	NotifyDelay(in_pPending, AkMonitorData::NotificationReason_Delay_Ended, in_bWasPaused );
}

//-----------------------------------------------------------------------------
// Name: NotifyDelay
// Desc: 
//
// Parameters:
//	AkPendingAction*				  in_pPending : Point to a pending action.
//	AkMonitorData::NotificationReason in_Reason   :
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::NotifyDelay(AkPendingAction* in_pPending, 
								AkMonitorData::NotificationReason in_Reason,
								bool in_bWasPaused )
{
	CAkCntrHist HistArray;
	if( in_bWasPaused )
	{
		MONITOR_OBJECTNOTIF( in_pPending->UserParam.PlayingID(), in_pPending->GameObjID(), in_pPending->UserParam.CustomParam(), AkMonitorData::NotificationReason_Pause_Aborted, HistArray, in_pPending->pAction->ID(), false, 0 );
	}

	switch ( in_pPending->pAction->ActionType() )
	{
	case AkActionType_Duck:
		// WG-4697: Don't notify if the action is a ducking action. 
		break;
	case AkActionType_PlayAndContinue:
		if(!static_cast<CAkActionPlayAndContinue*>(in_pPending->pAction)->NeedNotifyDelay() )
		{
			//Do not notify in the case of a cross-fade
			if(in_Reason == AkMonitorData::NotificationReason_Delay_Aborted)
			{
				in_Reason = AkMonitorData::NotificationReason_ContinueAborted;
			}
			else
			{
				break;
			}
		}
		//no break here
	case AkActionType_Play:
		static_cast<CAkActionPlay*>(in_pPending->pAction)->GetHistArray( HistArray );

		//no break here
	default:
		MONITOR_OBJECTNOTIF( in_pPending->UserParam.PlayingID(), in_pPending->GameObjID(), in_pPending->UserParam.CustomParam(), in_Reason, HistArray, in_pPending->pAction->ID(), false, 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Name: GetActionMatchingPlayingID
// Desc:
//
// Parameters:
//	AkPlayingID in_PlayingID
//
// Return: 
//	AkPendingAction * : Pointer to pending action.
//-----------------------------------------------------------------------------
AkPendingAction* CAkAudioMgr::GetActionMatchingPlayingID(AkPlayingID in_PlayingID)
{
	for( AkMultimapPending::Iterator iter = m_mmapPending.Begin(); iter != m_mmapPending.End(); ++iter )
	{
		if( (*iter).item->UserParam.PlayingID() == in_PlayingID )
		{
			return (*iter).item;
		}
	}
	for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
	{
		if( (*iter).item->UserParam.PlayingID() == in_PlayingID)
		{
			return (*iter).item;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Name: StopAction
// Desc:
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::StopAction(AkUniqueID in_ActionID, AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/)
{
	AKRESULT	eResult = AK_Success;

	AkPendingAction* pPending;

	for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); )
	{
		pPending = (*iter).item;

		if( pPending->pAction->ID() == in_ActionID && (in_PlayingID==AK_INVALID_PLAYING_ID||pPending->UserParam.PlayingID()==in_PlayingID) )
		{
			NotifyDelayAborted( pPending, false );
			iter = FlushPendingItem( pPending, m_mmapPending, iter );
		}
		else
			++iter;
	}

	for( AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx(); iter != m_mmapPausedPending.End(); )
	{
		pPending = (*iter).item;

		if( pPending->pAction->ID() == in_ActionID && (in_PlayingID==AK_INVALID_PLAYING_ID||pPending->UserParam.PlayingID()==in_PlayingID))
		{
			NotifyDelayAborted( pPending, true );
			iter = FlushPendingItem( pPending, m_mmapPausedPending, iter );
		}
		else
			++iter;
	}

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: PauseAction
// Desc:
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::PauseAction(AkUniqueID in_ActionID, AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/)
{
	AKRESULT eResult = AK_Success;

	AkPendingAction*	pThisAction;
	CAkAction*			pAction;

	AkMultimapPausedPending::Iterator iterPause = m_mmapPausedPending.Begin();
	while( iterPause != m_mmapPausedPending.End() )
	{
		pThisAction = (*iterPause).item;
		pAction = pThisAction->pAction;
		AKASSERT(pAction);

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(in_ActionID == pAction->ID()&& (in_PlayingID==AK_INVALID_PLAYING_ID||pThisAction->UserParam.PlayingID()==in_PlayingID))
		{	
			++(pThisAction->ulPauseCount);
		}
		++iterPause;
	}

	// scan'em all
	AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
	while( iter != m_mmapPending.End() )
	{
		pThisAction = (*iter).item;
		pAction = pThisAction->pAction;
		AKASSERT(pAction);

		// is it ours ? we don't pause pending resumes or we'll get stuck
		if(in_ActionID == pAction->ID()&& (in_PlayingID==AK_INVALID_PLAYING_ID||pThisAction->UserParam.PlayingID()==in_PlayingID))
		{	
			InsertAsPaused( pAction->ElementID(), pThisAction );
			iter = m_mmapPending.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ResumeAction
// Desc:
//
// Return: 
//	AK_Success : Succeeded.
//  AK_Fail    : Failed.
//-----------------------------------------------------------------------------
AKRESULT CAkAudioMgr::ResumeAction(AkUniqueID in_ActionID, AkPlayingID in_PlayingID /*= AK_INVALID_PLAYING_ID*/)
{
	AKRESULT eResult = AK_Success;
	AkPendingAction*	pThisAction;

	AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
	while( iter != m_mmapPausedPending.End() )
	{
		pThisAction = (*iter).item;
		if( pThisAction->pAction->ID() == in_ActionID && (in_PlayingID==AK_INVALID_PLAYING_ID||pThisAction->UserParam.PlayingID()==in_PlayingID) )
		{
			if( pThisAction->ulPauseCount == 0 )
			{
				TransferToPending( pThisAction );
				iter = m_mmapPausedPending.Erase( iter );
			}
			else
			{
				--(pThisAction->ulPauseCount);
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}
	return eResult;
}

//-----------------------------------------------------------------------------
// Name: ClearPendingItems
// Desc:
//
// Parameters:
// AkPlayingID in_PlayingID
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearPendingItems( AkPlayingID in_PlayingID )
{
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID() == in_PlayingID )
			{
				NotifyDelayAborted( pPendingAction, false );

				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID() == in_PlayingID)
			{
				NotifyDelayAborted( pPendingAction, true );

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ClearPendingItemsExemptOne
// Desc:
//
// Parameters:
//	AkPlayingID in_PlayingID
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearPendingItemsExemptOne(AkPlayingID in_PlayingID)
{
	bool bExempt = true;
	AkPendingAction* pPendingAction;
	{
		AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx();
		while( iter != m_mmapPending.End() )
		{
			pPendingAction = (*iter).item;
			if( pPendingAction->UserParam.PlayingID() == in_PlayingID )
			{
				if(bExempt)
				{
					NotifyDelayEnded( pPendingAction );
					g_pPlayingMgr->RemoveItemActiveCount( pPendingAction->UserParam.PlayingID() );
					bExempt = false;
				}
				else
				{
					NotifyDelayAborted( pPendingAction, false );
				}
				iter = FlushPendingItem( pPendingAction, m_mmapPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
	{
		AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx();
		while( iter != m_mmapPausedPending.End() )
		{
			pPendingAction = (*iter).item;
			if(pPendingAction->UserParam.PlayingID() == in_PlayingID)
			{
				if(bExempt)
				{
					// This is a special situation, the notification use true as optionnal flag, 
					// telling that the sound is unpaused and continuated in one shot
					NotifyDelayEnded( pPendingAction, true );
					g_pPlayingMgr->RemoveItemActiveCount( pPendingAction->UserParam.PlayingID() );
					bExempt = false;
				}
				else
				{
					NotifyDelayAborted( pPendingAction, true );
				}

				iter = FlushPendingItem( pPendingAction, m_mmapPausedPending, iter );
			}
			else
			{
				++iter;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ClearCrossFadeOccurence
// Desc:
//
// Parameters:
//	 CAkPBI* in_pPBIToCheck :
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkAudioMgr::ClearCrossFadeOccurence( CAkContinuousPBI* in_pPBIToCheck )
{
	CAkAction* pAction;
	//do not use in_pPBIToCheck here, only for comparaison purpose.
	for( AkMultimapPending::Iterator iter = m_mmapPending.Begin(); iter != m_mmapPending.End(); ++iter )
	{
		pAction = (*iter).item->pAction;
		if( pAction->ActionType() == AkActionType_PlayAndContinue )
		{
			static_cast<CAkActionPlayAndContinue*>( pAction )->UnsetFadeBack( in_pPBIToCheck );
		}
	}
	for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
	{
		pAction = (*iter).item->pAction;
		if(pAction->ActionType() == AkActionType_PlayAndContinue)
		{
			static_cast<CAkActionPlayAndContinue*>( pAction )->UnsetFadeBack( in_pPBIToCheck );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: IsAnException
// Desc:
//
// Parameters:
//	None.
//
// Return: 
//	true  : An exception.
//	false : Not an exception.
//-----------------------------------------------------------------------------
bool CAkAudioMgr::IsAnException( CAkAction* in_pAction, ExceptionList* in_pExceptionList )
{
	AKASSERT(in_pAction);

	if( !in_pExceptionList )
	{
		return false;
	}

	bool l_bCheckedBus = false;

	CAkParameterNodeBase* pBusNode = NULL;

	if(in_pAction->ElementID())
	{
		CAkParameterNodeBase* pNode = in_pAction->GetAndRefTarget();
		CAkParameterNodeBase* pNodeInitial = pNode;

		while(pNode != NULL)
		{
			for( ExceptionList::Iterator iter = in_pExceptionList->Begin(); iter != in_pExceptionList->End(); ++iter )
			{
				WwiseObjectID wwiseId( pNode->ID(), pNode->IsBusCategory() );
				if( *iter == wwiseId )
				{
					pNodeInitial->Release();
					return true;
				}
			}

			if( !l_bCheckedBus )
			{
				pBusNode = pNode->ParentBus();
				if(pBusNode)
				{
					l_bCheckedBus = true;
				}
			}

			pNode = pNode->Parent();
		}
		while(pBusNode != NULL)
		{
			for( ExceptionList::Iterator iter = in_pExceptionList->Begin(); iter != in_pExceptionList->End(); ++iter )
			{
				WwiseObjectID wwiseId( pBusNode->ID(), pBusNode->IsBusCategory() );
				if( (*iter) == wwiseId )
				{
					pNodeInitial->Release();
					return true;
				}
			}

			pBusNode = pBusNode->ParentBus();
		}

		if ( pNodeInitial )
			pNodeInitial->Release();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Name: IsElementOf
// Desc:
//
// Parameters:
// AkUniqueID in_TargetID	:
// AkUniqueID in_IDToCheck	:
//
// Return: 
//	true  : is element of.
//	false : not an element of.
//-----------------------------------------------------------------------------
bool CAkAudioMgr::IsElementOf( CAkParameterNodeBase * in_pNodeToTarget, CAkParameterNodeBase * in_pNodeToCheck )
{
	bool bIsElementOf = false;

	if(in_pNodeToTarget && in_pNodeToCheck)
	{
		if(in_pNodeToTarget == in_pNodeToCheck)//most situations
		{
			bIsElementOf = true;
		}
		else
		{
			CAkParameterNodeBase* pBus = in_pNodeToCheck->ParentBus();

			for( in_pNodeToCheck = in_pNodeToCheck->Parent(); in_pNodeToCheck ; in_pNodeToCheck = in_pNodeToCheck->Parent() )
			{
				if( in_pNodeToTarget == in_pNodeToCheck )
				{
					bIsElementOf = true;
					break;
				}
				if( pBus == NULL )
				{
					pBus = in_pNodeToCheck->ParentBus();
				}
			}
			if( !bIsElementOf )
			{
				//checking bus
				for( /*noinit*/ ; pBus ; pBus = pBus->ParentBus() )
				{
					if( in_pNodeToTarget == pBus )
					{
						bIsElementOf = true;
						break;
					}
				}
			}		
		}
	}

	return bIsElementOf;
}
//-----------------------------------------------------------------------------
// Name: Start
// Desc: Execute behavioural side and lower engine side.
//
// Parameters:
//	None.
//
// Return: 
//	AK_Success:	VPL executed. 
//  AK_Fail:		Failed to execute a VPL.
//-----------------------------------------------------------------------------
//Start the AudioThread
AKRESULT CAkAudioMgr::Start()
{
	if ( AK_PERF_OFFLINE_RENDERING )
		return AK_Success;

	if ( m_hEventMgrThreadDrainEvent.Init() == AK_Success )
		return m_audioThread.Start();
	return AK_Fail;
}

//Stop the AudioThread 
void CAkAudioMgr::Stop()
{
	if ( AK_PERF_OFFLINE_RENDERING )
		return;

	m_audioThread.Stop();
}

AkPendingAction::AkPendingAction( CAkRegisteredObj * in_pGameObj )
	: TargetPlayingID( AK_INVALID_PLAYING_ID )
	, pGameObj( in_pGameObj )
{
	if ( pGameObj )
		pGameObj->AddRef();
}

AkPendingAction::~AkPendingAction()
{
	if ( pGameObj )
		pGameObj->Release();
}

AkGameObjectID AkPendingAction::GameObjID() 
{ 
	return pGameObj ? pGameObj->ID() : AK_INVALID_GAME_OBJECT; 
}

void AkPendingAction::TransUpdateValue( AkIntPtr in_eTarget, AkReal32, bool in_bIsTerminated )
{
	AKASSERT( g_pAudioMgr );
	if( pAction->ActionType() == AkActionType_PlayAndContinue )
	{
		CAkActionPlayAndContinue* pActionPAC = static_cast<CAkActionPlayAndContinue*>( pAction );
		AKASSERT(g_pTransitionManager);	 

		TransitionTargets eTarget = (TransitionTargets)in_eTarget;
		switch( eTarget )
		{
		case TransTarget_Stop:
		case TransTarget_Play:
			if(in_bIsTerminated)
			{
				pActionPAC->m_PBTrans.pvPSTrans = NULL;
				pActionPAC->m_PBTrans.bIsPSTransFading = false;

				if( eTarget == TransTarget_Stop )
				{
					g_pAudioMgr->StopPending( this );
				}
			}
			break;
		case TransTarget_Pause:
		case TransTarget_Resume:
			if(in_bIsTerminated)
			{
				pActionPAC->m_PBTrans.pvPRTrans = NULL;
				pActionPAC->m_PBTrans.bIsPRTransFading = false;

				if( eTarget == TransTarget_Pause )
				{
					g_pAudioMgr->PausePending( this );
				}

				pActionPAC->SetPauseCount( 0 );
			}
			break;
		default:
			AKASSERT(!"Unsupported data type");
			break;
		}
	}
	else
	{
		// Should not happen, only PlayAndContinue Actions can have transitions.
		AKASSERT( pAction->ActionType() == AkActionType_PlayAndContinue );
	}
}

AKRESULT CAkAudioMgr::PausePending( AkPendingAction* in_pPA )
{
	AKRESULT	eResult = AK_Success;

	if( in_pPA )
	{
		AkPendingAction*	pThisAction = NULL;
		CAkAction*			pAction		= NULL;

		// scan'em all
		for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); ++iter )
		{
			pThisAction = (*iter).item;
			pAction = pThisAction->pAction;

			if( pThisAction == in_pPA )
			{	
				AkUInt32 l_ulPauseCount = 0;

				if( pAction->ActionType() == AkActionType_PlayAndContinue )
				{
					l_ulPauseCount = static_cast<CAkActionPlayAndContinue*>(pAction)->GetPauseCount() - 1;
					static_cast<CAkActionPlayAndContinue*>(pAction)->SetPauseCount( 0 );
				}

				InsertAsPaused( pAction->ElementID(), pThisAction, l_ulPauseCount );
				m_mmapPending.Erase( iter );
				return eResult;
			}
		}
		
		// scan'em all
		for( AkMultimapPausedPending::Iterator iter = m_mmapPausedPending.Begin(); iter != m_mmapPausedPending.End(); ++iter )
		{
			pThisAction = (*iter).item;
			pAction = pThisAction->pAction;

			if( pThisAction == in_pPA )
			{	
				if( pAction->ActionType() == AkActionType_PlayAndContinue )
				{
					(pThisAction->ulPauseCount) += static_cast<CAkActionPlayAndContinue*>(pAction)->GetPauseCount();
				}
				else
				{
					++(pThisAction->ulPauseCount);
				}
				return eResult;
			}
		}
	}
	return eResult;
}

AKRESULT CAkAudioMgr::StopPending( AkPendingAction* in_pPA )
{
	AKRESULT	eResult = AK_Success;

	if( in_pPA )
	{		
		for( AkMultimapPausedPending::IteratorEx iter = m_mmapPausedPending.BeginEx(); iter != m_mmapPausedPending.End(); ++iter )
		{
			if( in_pPA == (*iter).item )
			{
				NotifyDelayAborted( in_pPA, true );
				FlushPendingItem( in_pPA, m_mmapPausedPending, iter );
				break;
			}
		}
		
		for( AkMultimapPending::IteratorEx iter = m_mmapPending.BeginEx(); iter != m_mmapPending.End(); ++iter )
		{
			if( in_pPA == (*iter).item )
			{
				NotifyDelayAborted( in_pPA, false );
				FlushPendingItem( in_pPA, m_mmapPending, iter );
				break;
			}
		}
	}
	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkAudioMgr::InvalidatePendingPaths(AkUniqueID in_idElement)
{
	AkMultimapPending::Iterator it = m_mmapPending.Begin();
	while(it != m_mmapPending.End())
	{
		AkPendingAction* pAction = (*it).item;
		if( pAction->pAction->ActionType() == AkActionType_PlayAndContinue )
		{
			CAkActionPlayAndContinue *pPlayAndContinue = static_cast<CAkActionPlayAndContinue*>( pAction->pAction );
			if (in_idElement == pPlayAndContinue->GetPathInfo()->PathOwnerID)
			{
				g_pPathManager->RemovePathFromList(pPlayAndContinue->GetPathInfo()->pPBPath);
				pPlayAndContinue->GetPathInfo()->pPBPath = NULL;
				pPlayAndContinue->GetPathInfo()->PathOwnerID = AK_INVALID_UNIQUE_ID;
			}
		}
		++it;
	}

	AkMultimapPausedPending::Iterator it2 = m_mmapPausedPending.Begin();
	while(it2 != m_mmapPausedPending.End())
	{
		AkPendingAction* pAction = (*it2).item;
		if( pAction->pAction->ActionType() == AkActionType_PlayAndContinue )
		{
			CAkActionPlayAndContinue *pPlayAndContinue = static_cast<CAkActionPlayAndContinue*>( pAction->pAction );
			if (in_idElement == pPlayAndContinue->GetPathInfo()->PathOwnerID)
			{
				g_pPathManager->RemovePathFromList(pPlayAndContinue->GetPathInfo()->pPBPath);
				pPlayAndContinue->GetPathInfo()->pPBPath = NULL;
				pPlayAndContinue->GetPathInfo()->PathOwnerID = AK_INVALID_UNIQUE_ID;
			}
		}
		++it2;
	}
}
#endif

#ifdef AK_IOS
void CAkAudioMgr::HandleLossOfHardwareResponse(bool bHasTicked)
{
	if ( bHasTicked )
	{
		AKPLATFORM::PerformanceCounter(&m_timeLastBuffer);
		m_uCallsWithoutTicks = 0;
		AK_PERF_TICK_AUDIO();
	}
	else if ( !bHasTicked && (CAkLEngine::IsDeviceSuspended() || CAkLEngine::IsAudioSessionInterrupted()) )
	{
		// When device is suspended, we let sound engine clock continue.
		AKPLATFORM::PerformanceCounter(&m_timeLastBuffer);
		m_uCallsWithoutTicks = 0;
	}
	else
	{
		m_uCallsWithoutTicks++;
		
		//How much time has passed since the last tick?
		// WG-23727: Use configurable timeout to avoid crashes from various iOS hardware.
		const AkReal32 FramesPerSec = 60.f;
		const AkReal32 SecPerMs = 0.001f;
		AkUInt32 maxUnresponsitveFrames = (AkReal32) ( (AkReal32)g_PDSettings.uMaxSuspendTimeoutMs * SecPerMs * FramesPerSec );
		if (m_uCallsWithoutTicks > maxUnresponsitveFrames)
		{
			AkInt64 curTime;
			AKPLATFORM::PerformanceCounter(&curTime);
			if (AKPLATFORM::Elapsed(curTime, m_timeLastBuffer) > g_PDSettings.uMaxSuspendTimeoutMs )
			{
				//The HW hasn't requested samples for too long, assume it is dead.
				CAkLEngine::ReplaceMainSinkWithDummy();
				m_timeLastBuffer = curTime;
				m_uCallsWithoutTicks = 0;
				MONITOR_ERRORMSG("Hardware audio subsystem stopped responding.  Silent mode is enabled.");
			}
		}
	}
}
#endif // #ifdef AK_IOS
