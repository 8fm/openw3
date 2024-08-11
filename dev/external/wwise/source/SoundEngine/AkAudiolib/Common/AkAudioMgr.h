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
// AkAudioMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIO_MGR_H_
#define _AUDIO_MGR_H_

#include "AkChunkRing.h"
#include "AkMultiKeyList.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkActionExcept.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include "AkTransition.h"
#include "AkMonitorData.h"			// NotificationReason
#include "AkMonitor.h"
#include "PrivateStructures.h"
#include "AkAudioThread.h"
#include "AkManualEvent.h"

class CAkPBI;
class CAkContinuousPBI;
class CAkEvent;
class CAkDynamicSequence;
class CAkRegisteredObj;

extern AkPlatformInitSettings g_PDSettings;

using namespace AKPLATFORM;

enum AkQueuedMsgType
{
    QueuedMsgType_EndOfList,
	QueuedMsgType_Event,
	QueuedMsgType_RTPC,
	QueuedMsgType_RTPCWithTransition,
	QueuedMsgType_ResetRTPC,
	QueuedMsgType_ResetRTPCValue,
	QueuedMsgType_ResetRTPCValueWithTransition,
	QueuedMsgType_State,
	QueuedMsgType_Switch,
	QueuedMsgType_ResetSwitches,
	QueuedMsgType_Trigger,
	QueuedMsgType_RegisterGameObj,
	QueuedMsgType_UnregisterGameObj,
	QueuedMsgType_GameObjPosition,
	QueuedMsgType_GameObjActiveListeners,
	QueuedMsgType_GameObjActiveControllers,
	QueuedMsgType_ListenerPosition,
	QueuedMsgType_ListenerSpatialization,
	QueuedMsgType_GameObjEnvValues,
	QueuedMsgType_GameObjDryLevel,
	QueuedMsgType_GameObjObstruction,

	QueuedMsgType_SetSecondaryOutputVolume,

	QueuedMsgType_OpenDynamicSequence,
	QueuedMsgType_DynamicSequenceCmd,

	QueuedMsgType_KillBank,
	QueuedMsgType_StopAll,

	QueuedMsgType_ListenerPipeline,

	QueuedMsgType_AddRemovePlayerDevice,
#ifdef AK_MOTION
	QueuedMsgType_SetPlayerListener,
	QueuedMsgType_SetPlayerVolume,
#endif // AK_MOTION

	QueuedMsgType_StopPlayingID,
	QueuedMsgType_EventAction,
	QueuedMsgType_GameObjScalingFactor,
	QueuedMsgType_GameObjMultiPosition,
	QueuedMsgType_ListenerScalingFactor,
	QueuedMsgType_Seek,
    QueuedMsgType_SourcePluginAction,
	QueuedMsgType_StartStopOutputCapture,

	QueuedMsgType_SetEffect,
	QueuedMsgType_SetPanningRule,
	QueuedMsgType_SetSpeakerAngles,
	QueuedMsgType_SetProcessMode, 
	QueuedMsgType_SendMainOutputToDevice
};

struct AkQueuedMsg_EventBase
{
	AkGameObjectID		GameObjID;		// Associated game object
	AkPlayingID			PlayingID;		// Playing ID
	AkPlayingID			TargetPlayingID;// Playing ID affected, not current!
	AkCustomParamType	CustomParam;
};

struct AkQueuedMsg_Event
	: public AkQueuedMsg_EventBase
{
	CAkEvent*			Event;			// Pointer to the event
};

enum AkSourcePluginActionType
{
    AkSourcePluginActionType_Play,
    AkSourcePluginActionType_Stop
};

struct AkQueuedMsg_SourcePluginAction
	: public AkQueuedMsg_EventBase
{
	AkUInt32			        PluginID;		// ID of the plug-in to instanciate.
    AkUInt32                    CompanyID;      // Company ID of the plug-i to instanciate.
    AkSourcePluginActionType    ActionType;
};

struct AkQueuedMsg_EventAction
{
	CAkEvent*			Event;				// Pointer to the event
	AkGameObjectID		GameObjID;			// Associated game object
	AK::SoundEngine::AkActionOnEventType eActionToExecute;	// Action to be executed on the event
	AkTimeMs			uTransitionDuration;
	AkCurveInterpolation eFadeCurve;
	AkPlayingID			TargetPlayingID;
};

struct AkQueuedMsg_Rtpc
{
	AkRtpcID		ID;
	AkRtpcValue		Value;
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_RtpcWithTransition : public AkQueuedMsg_Rtpc
{
	TransParams		transParams;
};

struct AkQueuedMsg_State
{
	AkStateGroupID  StateGroupID;
	AkStateID		StateID;
	bool			bSkipTransition;
    bool			bSkipExtension;
};

struct AkQueuedMsg_Switch
{
	AkGameObjectID	GameObjID;		// Associated game object
	AkSwitchGroupID SwitchGroupID;
	AkSwitchStateID SwitchStateID;
};

struct AkQueuedMsg_Trigger
{
	AkGameObjectID	GameObjID;		// Associated game object
	AkTriggerID		TriggerID;
};

struct AkQueuedMsg_RegisterGameObj
{
	AkGameObjectID	GameObjID;
	AkUInt32		uListenerMask;
	void *			pMonitorData; // Monitor data, allocated in game thread, to be used in audio thread if registration successful
};

struct AkQueuedMsg_UnregisterGameObj
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_GameObjPosition
{
	AkGameObjectID	GameObjID;
	AkSoundPosition Position;
};

struct AkQueuedMsg_GameObjMultiplePosition
{
	AkGameObjectID						GameObjID;
	AkUInt16							uNumPositions;
	AK::SoundEngine::MultiPositionType	eMultiPositionType;
	AkSoundPosition						aMultiPosition[1];
};

struct AkQueuedMsg_GameObjActiveListeners
{
	AkGameObjectID	GameObjID;
	AkUInt32		uListenerMask;
};

struct AkQueuedMsg_GameObjActiveControllers
{
	AkGameObjectID	GameObjID;
	AkUInt32		uActiveControllerMask;
};

struct AkQueuedMsg_ListenerPosition
{
	AkUInt32		uListenerIndex;
	AkListenerPosition Position;
};

struct AkQueuedMsg_ListenerSpatialization
{
	AkUInt32		uListenerIndex;
	AkSpeakerVolumes Volumes;
	bool            bSpatialized;
	bool            bSetVolumes;
};

struct AkQueuedMsg_ListenerPipeline
{
	AkUInt32		uListenerIndex;
	bool			bAudio;
	bool			bFeedback;
};

struct AkQueuedMsg_SetPlayerListener
{
	AkUInt8		iPlayer;
	AkUInt8		iListener;
	AkUInt16	DummyForAlignment;	//Remove if size becomes a multiple of 4.  See AkChunkRing.cpp line 235
};

struct AkQueuedMsg_SetPlayerVolume
{
	AkReal32	fVolume;
	AkUInt32	iPlayer;	//32 bits for alignment.  See AkChunkRing.cpp line 235
};

struct AkQueuedMsg_AddRemovePlayerDevice
{
	AkUInt32	uListeners;
	void *		pDevice;
	AkUInt32	idDevice;
	AkUInt32	iPlayer;
	bool		bAdd;
};

struct AkQueuedMsg_GameObjEnvValues
{
	AkGameObjectID	GameObjID;
	AkUInt32		uNumValues;
	AkAuxSendValue EnvValues[AK_MAX_AUX_PER_OBJ];
};

struct AkQueuedMsg_GameObjDryLevel
{
	AkGameObjectID	GameObjID;
	AkReal32		fValue;
};

struct AkQueuedMsg_GameObjScalingFactor
{
	AkGameObjectID	GameObjID;
	AkReal32		fValue;
};

struct AkQueuedMsg_ListenerScalingFactor
{
	AkUInt32		uListenerIndex;
	AkReal32		fValue;
};

struct AkQueuedMsg_Seek
{
	CAkEvent*		Event;
	AkGameObjectID	GameObjID;
	union
	{
		AkTimeMs		iPosition;
		AkReal32		fPercent;
	};
	bool			bIsSeekRelativeToDuration;
	bool			bSnapToMarker;
};

struct AkQueuedMsg_GameObjObstruction
{
	AkGameObjectID	GameObjID;
	AkUInt32		uListenerIndex;
	AkReal32		fObstructionLevel;
	AkReal32		fOcclusionLevel;
};

struct AkQueuedMsg_ResetSwitches
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_ResetRTPC
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_ResetRTPCValue
{
	AkUInt32		ParamID;
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_ResetRtpcValueWithTransition : public AkQueuedMsg_ResetRTPCValue
{
	TransParams		transParams;
};

struct AkQueuedMsg_SetSecondaryOutputVolume
{
	AkUInt32	idDevice;
	AkUInt32	iPlayer;
	AkReal32	fVolume;
};

struct AkQueuedMsg_OpenDynamicSequence
	: public AkQueuedMsg_EventBase
{
	CAkDynamicSequence*	pDynamicSequence;
};

class CAkUsageSlot;

struct AkQueuedMsg_KillBank
{
	CAkUsageSlot* pUsageSlot;
};

struct AkQueuedMsg_StopAll
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_StopPlayingID
{
	AkPlayingID				playingID;
	AkTimeMs				uTransitionDuration;
	AkCurveInterpolation	eFadeCurve;
};

struct AkQueuedMsg_DynamicSequenceCmd
{
	enum Command
	{
		Play,
		Pause,
		Resume,
		Close,
		Stop,
		Break,
		ResumeWaiting
	};

	CAkDynamicSequence*			pDynamicSequence;
	Command						eCommand;
	AkTimeMs					uTransitionDuration;
	AkCurveInterpolation		eFadeCurve;
};

struct AkQueuedMsg_StartStopCapture
{
	AkOSChar *szFileName;
	bool bCaptureMotion;
};

struct AkQueuedMsg_SetEffect
{
	AkUniqueID	audioNodeID;
	AkUInt32	uFXIndex;
	AkUniqueID	shareSetID;
	AkNodeType  eNodeType;
};

struct AkQueuedMsg_SetPanningRule
{
	AkUInt32 		iOutputID;
	AkSinkType 		eSinkType;
	AkPanningRule	panRule;
};

struct AkQueuedMsg_SetSpeakerAngles
{
	AkReal32 *		pfSpeakerAngles;
	AkUInt32		uNumAngles;
	AkSinkType 		eSinkType;
	AkUInt32 		iOutputID;
};

struct AkQueuedMsg_SetProcessMode
{
	AkUInt32 eStatus;
	AkUInt32 msFade;
	AkCurveInterpolation eCurve;
	void(*Callback)(void*);
	void *pUserData;
};

#ifdef AK_WIIU
struct AkQueuedMsg_SendMainOutputToDevice
{
	AkOutputSelector eDevice;
};
#endif

struct AkQueuedMsg
{
	AkQueuedMsg( AkUInt16 in_type )
		:type( in_type )
	{}

	AkUInt16 size;
	AkUInt16 type;

	union
	{
        AkQueuedMsg_Event	event;
		AkQueuedMsg_Rtpc	rtpc;
		AkQueuedMsg_RtpcWithTransition	rtpcWithTransition;
		AkQueuedMsg_State   setstate;
		AkQueuedMsg_Switch	setswitch;
		AkQueuedMsg_Trigger trigger;
		AkQueuedMsg_RegisterGameObj reggameobj;
		AkQueuedMsg_UnregisterGameObj unreggameobj;
		AkQueuedMsg_GameObjPosition gameobjpos;
		AkQueuedMsg_GameObjMultiplePosition gameObjMultiPos;
		AkQueuedMsg_GameObjActiveListeners gameobjactlist;
		AkQueuedMsg_GameObjActiveControllers gameobjactcontroller;
		AkQueuedMsg_ListenerPosition listpos;
		AkQueuedMsg_ListenerSpatialization listspat;
		AkQueuedMsg_GameObjEnvValues gameobjenvvalues;
		AkQueuedMsg_GameObjDryLevel gameobjdrylevel;
		AkQueuedMsg_GameObjObstruction gameobjobstr;
		AkQueuedMsg_ResetSwitches resetswitches;
		AkQueuedMsg_ResetRTPC resetrtpc;
		AkQueuedMsg_ResetRTPCValue resetrtpcvalue;
		AkQueuedMsg_ResetRtpcValueWithTransition resetrtpcvalueWithTransition;
		AkQueuedMsg_SetSecondaryOutputVolume secondaryoutputvolume;
		AkQueuedMsg_OpenDynamicSequence opendynamicsequence;
		AkQueuedMsg_DynamicSequenceCmd dynamicsequencecmd;
		AkQueuedMsg_KillBank killbank;
		AkQueuedMsg_ListenerPipeline listpipe;
		AkQueuedMsg_SetPlayerListener playerlistener;
		AkQueuedMsg_SetPlayerVolume playervolume;
		AkQueuedMsg_AddRemovePlayerDevice playerdevice;
		AkQueuedMsg_StopAll stopAll;
		AkQueuedMsg_StopPlayingID stopEvent;
		AkQueuedMsg_EventAction eventAction;
		AkQueuedMsg_GameObjScalingFactor gameobjscalingfactor;
		AkQueuedMsg_ListenerScalingFactor listenerscalingfactor;
		AkQueuedMsg_Seek seek;
        AkQueuedMsg_SourcePluginAction sourcePluginAction;
		AkQueuedMsg_StartStopCapture outputCapture;
		AkQueuedMsg_SetEffect setEffect;
		AkQueuedMsg_SetPanningRule setPanningRule;
		AkQueuedMsg_SetSpeakerAngles setSpeakerAngles;
		AkQueuedMsg_SetProcessMode processMode;
#ifdef AK_WIIU
		AkQueuedMsg_SendMainOutputToDevice mainOutputDevice;
#endif
	};

#define DCL_MSG_OBJECT_SIZE( _NAME_, _OBJECT_ ) \
static AkUInt16 _NAME_()

	static AkUInt16 Sizeof_EndOfList();
	static AkUInt16 Sizeof_GameObjMultiPositionBase();

	DCL_MSG_OBJECT_SIZE( Sizeof_Event,						AkQueuedMsg_Event );
	DCL_MSG_OBJECT_SIZE( Sizeof_Rtpc,						AkQueuedMsg_Rtpc );
	DCL_MSG_OBJECT_SIZE( Sizeof_RtpcWithTransition,			AkQueuedMsg_RtpcWithTransition );
	DCL_MSG_OBJECT_SIZE( Sizeof_State,						AkQueuedMsg_State );
	DCL_MSG_OBJECT_SIZE( Sizeof_Switch,						AkQueuedMsg_Switch );
	DCL_MSG_OBJECT_SIZE( Sizeof_Trigger,					AkQueuedMsg_Trigger );
	DCL_MSG_OBJECT_SIZE( Sizeof_RegisterGameObj,			AkQueuedMsg_RegisterGameObj );
	DCL_MSG_OBJECT_SIZE( Sizeof_UnregisterGameObj,			AkQueuedMsg_UnregisterGameObj );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjPosition,			AkQueuedMsg_GameObjPosition );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjActiveListeners, 	AkQueuedMsg_GameObjActiveListeners );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjActiveControllers,	AkQueuedMsg_GameObjActiveControllers );
	DCL_MSG_OBJECT_SIZE( Sizeof_ListenerPosition,			AkQueuedMsg_ListenerPosition );
	DCL_MSG_OBJECT_SIZE( Sizeof_ListenerSpatialization, 	AkQueuedMsg_ListenerSpatialization );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjEnvValues,			AkQueuedMsg_GameObjEnvValues );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjDryLevel,			AkQueuedMsg_GameObjDryLevel );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjObstruction, 		AkQueuedMsg_GameObjObstruction );
	DCL_MSG_OBJECT_SIZE( Sizeof_ResetSwitches,				AkQueuedMsg_ResetSwitches );
	DCL_MSG_OBJECT_SIZE( Sizeof_ResetRTPC,					AkQueuedMsg_ResetRTPC );
	DCL_MSG_OBJECT_SIZE( Sizeof_ResetRTPCValue,				AkQueuedMsg_ResetRTPCValue );
	DCL_MSG_OBJECT_SIZE( Sizeof_ResetRTPCValueWithTransition,	AkQueuedMsg_ResetRtpcValueWithTransition );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetSecondaryOutputVolume,			AkQueuedMsg_SetSecondaryOutputVolume );
	DCL_MSG_OBJECT_SIZE( Sizeof_OpenDynamicSequence,		AkQueuedMsg_OpenDynamicSequence );
	DCL_MSG_OBJECT_SIZE( Sizeof_DynamicSequenceCmd, 		AkQueuedMsg_DynamicSequenceCmd );
	DCL_MSG_OBJECT_SIZE( Sizeof_KillBank,					AkQueuedMsg_KillBank );
	DCL_MSG_OBJECT_SIZE( Sizeof_StopAll,					AkQueuedMsg_StopAll );
	DCL_MSG_OBJECT_SIZE( Sizeof_ListenerPipeline,			AkQueuedMsg_ListenerPipeline );
	DCL_MSG_OBJECT_SIZE( Sizeof_ListenersToDeviceRouting,	AkQueuedMsg_ListenersToDeviceRouting );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetPlayerListener,			AkQueuedMsg_SetPlayerListener );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetPlayerVolume,			AkQueuedMsg_SetPlayerVolume );
	DCL_MSG_OBJECT_SIZE( Sizeof_AddRemovePlayerDevice,		AkQueuedMsg_AddRemovePlayerDevice );
	DCL_MSG_OBJECT_SIZE( Sizeof_StopPlayingID,				AkQueuedMsg_StopPlayingID );
	DCL_MSG_OBJECT_SIZE( Sizeof_EventAction,				AkQueuedMsg_EventAction );
	DCL_MSG_OBJECT_SIZE( Sizeof_GameObjScalingFactor,		AkQueuedMsg_GameObjScalingFactor );
	DCL_MSG_OBJECT_SIZE( Sizeof_ListenerScalingFactor,		AkQueuedMsg_ListenerScalingFactor );
	DCL_MSG_OBJECT_SIZE( Sizeof_Seek,						AkQueuedMsg_Seek );
    DCL_MSG_OBJECT_SIZE( Sizeof_PlaySourcePlugin,		    AkQueuedMsg_SourcePluginAction );
	DCL_MSG_OBJECT_SIZE( Sizeof_StartStopCapture,			AkQueuedMsg_StartStopCapture );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetEffect,					AkQueuedMsg_SetEffect );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetPanningRule,				AkQueuedMsg_SetPanningRule );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetSpeakerAngles,			AkQueuedMsg_SetSpeakerAngles );
	DCL_MSG_OBJECT_SIZE( Sizeof_SetProcessMode,				AkQueuedMsg_SetProcessMode );
#ifdef AK_WIIU
	DCL_MSG_OBJECT_SIZE( Sizeof_SendMainOutputToDevice,		AkQueuedMsg_SendMainOutputToDevice );
#endif
};

// all what we need to start an action at some later time
struct AkPendingAction : public ITransitionable 
							
{
	CAkAction*			pAction;				// the action itself
	AkUInt32			LaunchTick;				// when it should run
	AkUInt32			LaunchFrameOffset;		// frame offset
	AkUInt32			PausedTick;				// when it got paused
	UserParams			UserParam;
	AkUInt32			ulPauseCount;			// Pause Count
	AkPlayingID			TargetPlayingID;

	AkPendingAction( CAkRegisteredObj * in_pGameObj );
	virtual ~AkPendingAction();

	AkGameObjectID GameObjID();
	CAkRegisteredObj * GameObj() { return pGameObj; }

	virtual void TransUpdateValue( AkIntPtr in_eTargetType, AkReal32 in_fValue, bool in_bIsTerminated );

private:
	CAkRegisteredObj	* pGameObj;	// Game object pointer, made private to enforce refcounting
};

class CAkAudioMgr
{
	friend class CAkAudioThread;

public:
	CAkAudioMgr();
	~CAkAudioMgr();

	AKRESULT	Init();
	void		Term();

	//Start the AudioThread
	AKRESULT Start();

	//Stop the AudioThread 
	void Stop();

	//Enqueues an End-of-list item if there are events to process and wakes up audio thread.
	AKRESULT RenderAudio();
	
	AkForceInline void WakeupEventsConsumer() { return m_audioThread.WakeupEventsConsumer(); }

	//Add the specified item
	AKRESULT Enqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize );

	inline void IncrementBufferTick();

	AkUInt32 GetBufferTick(){ return m_uBufferTick; }

	template< typename T >
	typename T::IteratorEx FlushPendingItem( AkPendingAction* in_pPA, T & in_List, typename T::IteratorEx in_Iter )
	{
		typename T::IteratorEx itNext = in_List.Erase( in_Iter );
		in_pPA->pAction->Release();
		AkDelete( g_DefaultPoolId, in_pPA );
		return itNext;
	}

	AKRESULT BreakPendingAction( CAkParameterNodeBase * in_pNodeToTarget, CAkRegisteredObj * in_GameObj, AkPlayingID in_PlayingID );

	AKRESULT StopPendingAction(  CAkParameterNodeBase * in_pNodeToTarget, CAkRegisteredObj * in_GameObj, AkPlayingID in_PlayingID );

	AKRESULT StopPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, AkPlayingID in_PlayingID );

	// move actions from the pending list to the paused pending list
	void PausePendingAction(CAkParameterNodeBase * in_pNodeToTarget, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );
	void PausePendingItems( AkPlayingID in_PlayingID );

	// move actions from the paused pending list to the pending list
	void ResumePausedPendingAction( CAkParameterNodeBase * in_pNodeToTarget, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );
	void ResumePausedPendingItems( AkPlayingID in_playingID );

	// Cancel eventual pause transition in the pending list( not paused pending)
	void ResumeNotPausedPendingAction(CAkParameterNodeBase * in_pNodeToTarget, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );

	void PausePendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );

	void ResumePausedPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );

	// Cancel eventual pause transition in the pending list( not paused pending)
	void ResumeNotPausedPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume, AkPlayingID in_PlayingID );

	// remove some actions from the pending list
	void RemovePendingAction( CAkParameterNodeBase * in_pNodeToTarget );

	// remove some actions from the paused pending list
	void RemovePausedPendingAction( CAkParameterNodeBase * in_pNodeToTarget );

	AKRESULT PausePending( AkPendingAction* in_pPA );
	AKRESULT StopPending( AkPendingAction* in_pPA );

	// figure out if anything has to come out of this list
	void ProcessPendingList();

	void RemoveAllPausedPendingAction();
	void RemoveAllPendingAction();
	void RemoveAllPreallocAndReferences();

	void InsertAsPaused( AkUniqueID in_ElementID, AkPendingAction* in_pPendingAction, AkUInt32 in_ulPauseCount = 0 );
	void TransferToPending( AkPendingAction* in_pPendingAction );

	// This function return a pointer to the first action that is pending that corresponds to the
	// specified Playing ID. This is used to Recycle the content of a playing instance that
	// currently do not have PBI.
	//
	// Return - AkPendingAction* - pointer to the first action that is pending that corresponds to the
	// specified Playing ID. NULL if not found.
	AkPendingAction* GetActionMatchingPlayingID(
		AkPlayingID in_PlayingID	// Searched Playing ID
		);

	// Force the destruction of pending items associated to the specified PlayingID.
	void ClearPendingItems(
		AkPlayingID in_PlayingID
		);

	// Force the destruction of pending items associated to the specified PlayingID, but one of them do not return Abort
	void ClearPendingItemsExemptOne(
		AkPlayingID in_PlayingID
		);

	void ClearCrossFadeOccurence(
		CAkContinuousPBI* in_pPBIToCheck
		);

	// Execute an action or enqueue it in the pending queue depending if there is a delay
	void EnqueueOrExecuteAction(
		AkPendingAction* in_pActionItem
		);

	AKRESULT StopAction(AkUniqueID in_ActionID, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID);
	AKRESULT PauseAction(AkUniqueID in_ActionID, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID);
	AKRESULT ResumeAction(AkUniqueID in_ActionID,AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID);

	// the total size of the queue (in bytes)
	AkUInt32 GetActualQueueSize() {return m_MsgQueueActualSize;}
	AkUInt32 GetMaximumMsgSize() {return m_MsgQueue.GetChunkSize();}
	
#ifndef AK_OPTIMIZED
	// 0.0 -> 1.0 (returns the greatest value since the previous query)
	AkReal32 GetPercentageQueueFilled() 
	{AkReal32 tmp = m_MsgQueuePercentageFilled; m_MsgQueuePercentageFilled = 0.0f; return tmp;}	

	void InvalidatePendingPaths(AkUniqueID in_idElement);
#endif

	AkThreadID GetThreadID() { return m_audioThread.GetAudioThreadID(); }
	CAkAudioThread & GetAudioThread() { return m_audioThread; }
private:

	void Perform();

	// Add an action in the delayed action queue
	
	void FlushAndCleanPendingAction( 
		AkPendingAction* in_pPendingAction 
		);

	CAkLock m_queueLock;

	// the messages we get from the game side
	AkSparseChunkRing m_MsgQueue;

	// when set drain the message queue
	bool m_bDrainMsgQueue;

	// actual size of the message queue
	AkUInt32 m_MsgQueueActualSize;

#ifndef AK_OPTIMIZED
	// how much of the queue is filled
	AkReal32 m_MsgQueuePercentageFilled;
#endif

	// the things that are not ready to be done
	typedef CAkMultiKeyList<AkUInt32, AkPendingAction*, AkAllocAndKeep> AkMultimapPending;
	AkMultimapPending m_mmapPending;

	// the things that are not ready to be done that got paused
	typedef CAkMultiKeyList<AkUniqueID, AkPendingAction*, AkAllocAndKeep> AkMultimapPausedPending;
	AkMultimapPausedPending m_mmapPausedPending;

	// Time actually in use by the Audio Mgr
	AkUInt32 m_uBufferTick;

	AkUInt32 m_ulWriterFlag; //Counter incremented at everytime the RenderAudio() function is called
	AkUInt32 m_ulReaderFlag; //Counter incremented at everytime the audiothread reaches an EOL element in the queue

	// Take all the Events in the Event queue untill it reaches 
	// the End Of List flag and process them all
	//
	// Return - AkResult - Ak_Success if succeeded
	AKRESULT ProcessMsgQueue();

	// execute those ready
	void ProcessAction( AkPendingAction * in_pAction );

	// execute a specific custom action
	void ProcessCustomAction( 
		CAkParameterNodeBase* ptargetNode, 
		CAkRegisteredObj * in_pGameObj, 
		AK::SoundEngine::AkActionOnEventType in_ActionToExecute,
		AkTimeMs in_uTransitionDuration,
		AkCurveInterpolation in_eFadeCurve,
		AkPlayingID	in_PlayingID
		);

    // play a specified target
	void PlaySourceInput(
		AkUniqueID in_Target, 
		CAkRegisteredObj * in_pGameObj,
        UserParams in_userParams
		);

	// Monitor notif
	void NotifyDelayStarted(
		AkPendingAction* in_pPending//Pointer to the pending action that was delayed
		);

	// Monitor notif
	void NotifyDelayAborted(
		AkPendingAction* in_pPending, //Pointer to the pending action that was aborted
		bool in_bWasPaused
		);

	// Monitor notif
	void NotifyDelayEnded(
		AkPendingAction* in_pPending, //Pointer to the pending action that just ended
		bool in_bWasPaused = false
		);

	void NotifyDelay(
		AkPendingAction* in_pPending, 
		AkMonitorData::NotificationReason in_Reason,
		bool in_bWasPaused
		);

	//Internal tool, allow to know it the specified action should be excepted by the call.
	bool IsAnException( CAkAction* in_pAction, ExceptionList* in_pExceptionList );

	bool IsElementOf( CAkParameterNodeBase * in_pNodeToTarget, CAkParameterNodeBase * in_pNodeToCheck );
	AKRESULT LockedEnqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize );
	
#ifdef AK_IOS
	void HandleLossOfHardwareResponse(bool bHasTicked);
#endif // #ifdef AK_IOS
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

	CAkAudioThread	m_audioThread;
	AkManualEvent	m_hEventMgrThreadDrainEvent;
	AkInt64			m_timeLastBuffer;
	AkUInt32		m_uCallsWithoutTicks;
};

extern CAkAudioMgr* g_pAudioMgr;
#endif
