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
// AkMonitorData.h
//
// Public structures for monitoring.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MONITORDATA_H_
#define _MONITORDATA_H_

#include "AkPrivateTypes.h"
#include "AkCommon.h"
#include "AkCntrHistory.h"
#include "AkParameters.h"		// AkValueMeaning & Co
#include "AkFeedbackStructs.h"

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/Tools/Common/AkMonitorError.h>

#include <stddef.h> // for offsetof

#define MONITOR_MSG_MAXLENGTH 80

#define MONITORDATAITEMBASESIZE ( offsetof( AkMonitorData::MonitorDataItem, objectData ) )

#define SIZEOF_MEMBER(s,m)   sizeof(((s *)0)->m)
#define SIZEOF_MONITORDATA(_x)	( MONITORDATAITEMBASESIZE + SIZEOF_MEMBER( AkMonitorData::MonitorDataItem, _x ) )
#define SIZEOF_MONITORDATA_TO(_STRUCT_MEMBER_) ( offsetof(AkMonitorData::MonitorDataItem, _STRUCT_MEMBER_ ) )

namespace AkMonitorData
{
	enum MonitorDataType
	{
		MonitorDataTimeStamp = 0,
		MonitorDataObject,
		MonitorDataState,
		MonitorDataParamChanged,
		MonitorDataBank,
		MonitorDataEventTriggered,	
		MonitorDataActionDelayed,	
		MonitorDataActionTriggered,	
		MonitorDataBusNotif,			
		MonitorDataSetParam,			
		MonitorDataAudioPerf,		
		MonitorDataGameObjPosition,	
		MonitorDataObjRegistration,	
		MonitorDataPath,				
		MonitorDataSwitch,			
		MonitorDataPluginTimer,		
		MonitorDataMemoryPool,		
		MonitorDataMemoryPoolName,	
		MonitorDataStreaming,		
		MonitorDataStreamsRecord,	
		MonitorDataDevicesRecord,		
		MonitorDataPipeline,			
		MonitorDataEnvironment,		
		MonitorDataListeners,		
		MonitorDataObsOcc,			
		MonitorDataMarkers,			
		MonitorDataOutput,			
		MonitorDataSegmentPosition,	
		MonitorDataControllers,		
		MonitorDataRTPCValues,		
		MonitorDataErrorCode,		
		MonitorDataMessageCode,		
		MonitorDataPrepare,			
		MonitorDataSoundBank,		
		MonitorDataMedia,			
		MonitorDataEvent,			
		MonitorDataGameSync,			
		MonitorDataFeedback,			
		MonitorDataFeedbackDevices,	
		MonitorDataFeedbackGameObjs,
		MonitorDataErrorString,		
		MonitorDataMessageString,	
		MonitorDataResolveDialogue,	
		MonitorDataMusicTransition,	
		MonitorDataPlugin,			
		MonitorDataExternalSource,	
		MonitorDataStreamingDevice,	
		MonitorDataMeter,			
		MonitorDataSends,			
		MonitorDataLoudnessMeter,
		MonitorDataPlatformSinkType,

		MonitorDataEndOfItems // KEEP THIS AT END: last value, not a data type
	};

	typedef AkUInt64 MaskType;
	const MaskType AllMonitorData = 0xFFFFFFFFFFFFFFFFULL;

	#define AKMONITORDATATYPE_TOMASK(_datatype) ((AkMonitorData::MaskType)1<<_datatype)

	enum NotificationReason
	{
		NotificationReason_None						= 0,	// No apparent reason
		NotificationReason_Stopped					= 1,	// A stop command was executed
		NotificationReason_StoppedAndContinue		= 2,	// A stop command was executed on something that is not the last of its serie
		NotificationReason_Paused					= 3,	// A pause command was executed
		NotificationReason_Resumed					= 4,	// A resume command was executed
		NotificationReason_Pause_Aborted			= 5,	// A paused sound was stopped
		NotificationReason_EndReached				= 6,	// The sound reached the end and stopped by itself
		NotificationReason_EndReachedAndContinue	= 7,	// The sound reached the end, but another sound is/was/will be launched
		NotificationReason_Play						= 9,	// A PBI just been created and a sound start playing
		NotificationReason_PlayContinue				= 10,	// Specific play following a EndReachedAndContinue
		NotificationReason_Delay_Started			= 11,	// There will be a delay before the next play
		NotificationReason_Delay_Ended				= 12,
		NotificationReason_Delay_Aborted			= 13,	// Signify that a pending action was destroyed without being executed
		NotificationReason_Fade_Started				= 14,	// Sound started fading in
		NotificationReason_Fade_Completed			= 15,	// Sound completed the fade in
		NotificationReason_Fade_Aborted				= 16,	// Sound terminated before transition completed

		NotificationReason_EnterSwitchContPlayback	= 20,// Notif sent to tell there is a continuous playback on a switch container
		NotificationReason_ExitSwitchContPlayback	= 21,// Notif sent to tell there is a continuous playback on a switch container that ended
		NotificationReason_PauseSwitchContPlayback	= 22,// Notif sent to tell there is a continuous playback on a switch container that paused
		NotificationReason_ResumeSwitchContPlayback	= 23,// Notif sent to tell there is a continuous playback on a switch container that resumed
		NotificationReason_PauseSwitchContPlayback_Aborted = 24,// Notif sent to tell there is a continuous playback on a switch container that was stopped while paused

		NotificationReason_NothingToPlay			= 25,// Notif sent to tell that the specified event found nothing to play on a switch container

		NotificationReason_EventEndReached			= 26,// Notif sent to tell that the specified event is finished playing
		NotificationReason_EventMarker				= 27,// Notif sent to tell that a marker was reached while playing the specified event

		NotificationReason_Seek						= 28,// Notif sent when a seek action is being executed
		NotificationReason_SeekPercent				= 29,// Notif sent when a seek action (using a percentage) is being executed

		NotificationReason_PlayFailed,						// Notification meaning the play asked was not done for an out of control reason
															// For example, if The Element has a missing source file.
		NotificationReason_ContinueAborted,					// The continuity was breached by a no source found in the next item to play
		NotificationReason_PlayFailedLimit,					// Play failed due to playback limit
		NotificationReason_PlayFailedGlobalLimit,			// Play failed due to global playback limit
		NotificationReason_PlayFailedMemoryThreshold,		// Play failed due to memory threshold
		NotificationReason_ContinueAbortedLimit,			// Continue aborted due to playback limit
		NotificationReason_ContinueAbortedGlobalLimit,		// Continue aborted due to Global playback limit
		NotificationReason_ContinueAbortedMemoryThreshold,	// Continue aborted due to memory threshold

		NotificationReason_StoppedLimit,					// Stopped due to playback limit
		NotificationReason_StoppedGlobalLimit,				// Stopped due to Global playback limit
		NotificationReason_StoppedMemoryThreshold,			// Stopped due to memory threshold
		NotificationReason_KilledVolumeUnderThreshold,		// Voice killed due to volume under threshold
		NotificationReason_VirtualNoListener,				// Voice has no listener, it will use the virtual behavior.


		//////////////////////////////////////////////
		// Notifications used in the AkParamsChanged Notification
		//////////////////////////////////////////////
		NotificationReason_Muted			= 100,		// Element is muted
		NotificationReason_Unmuted			= 101,		// Element is unmuted
		NotificationReason_PitchChanged		= 102,		// Element Pitch changed
		NotificationReason_VolumeChanged	= 103,		// Element Volume Changed

		NotificationReason_LPFChanged		= 105,		// Element LPF Changed
		NotificationReason_BypassFXChanged	= 106,		// Element BypassFX Changed
		NotificationReason_FeedbackVolumeChanged	= 107, // Element Feedback Volume Changed

		NotificationReason_BankLoadRequestReceived		= 501,
		NotificationReason_BankUnloadRequestReceived	= 502,
		NotificationReason_BankLoaded					= 503,
		NotificationReason_BankUnloaded					= 504,
		NotificationReason_BankLoadFailed				= 507,
		NotificationReason_BankUnloadFailed				= 508,
		NotificationReason_ErrorWhileLoadingBank		= 509,
		NotificationReason_InsuficientSpaceToLoadBank	= 510,
		NotificationReason_BankAlreadyLoaded			= 511,
		NotificationReason_ClearAllBanksRequestReceived	= 512,


		NotificationReason_PrepareEventRequestReceived  = 601,
		NotificationReason_UnPrepareEventRequestReceived= 602,
		NotificationReason_ClearPreparedEventsRequested	= 603,
		NotificationReason_PrepareGameSyncRequested		= 604,
		NotificationReason_UnPrepareGameSyncRequested	= 605,
		NotificationReason_PrepareGameSyncSuccess		= 606,
		NotificationReason_PrepareGameSyncFailure		= 607,
		NotificationReason_UnPrepareGameSyncSuccess		= 608,
		NotificationReason_UnPrepareGameSyncFailure		= 609,
		NotificationReason_EventPrepareSuccess			= 610,
		NotificationReason_EventPrepareFailure			= 611,
		NotificationReason_EventUnPrepareSuccess		= 612,
		NotificationReason_EventUnPrepareFailure		= 613,
		NotificationReason_PrepareBankRequested			= 614,
		NotificationReason_UnPrepareBankRequested		= 615,

		NotificationReason_MusicSegmentStarted			= 801,
		NotificationReason_MusicSegmentEnded			= 802,

		NotificationReason_MusicTransitionScheduled		= 803,
		NotificationReason_MusicTransitionNoTransNeeded	= 804,
		NotificationReason_MusicTransitionReverted		= 805,
		NotificationReason_MusicTransitionResolved		= 806,
		NotificationReason_StingerScheduled				= 807,
		NotificationReason_StingerRescheduled			= 808,

		NotificationReason_PlayCountBit					= 0x1000000, // Notification should update related WAL play count
		NotificationReason_AllBits						= 0xF000000 // Mask of the above
	};

	enum BusNotification
	{
		BusNotification_None		= 0,
		BusNotification_Ducked		= 1,
		BusNotification_Unducked	= 2,
		BusNotification_FXBypass	= 3,
		BusNotification_Muted		= 5,
		BusNotification_Unmuted		= 6
	};

	struct ObjRegistrationMonitorData
	{
		bool			isRegistered;
		AkWwiseGameObjectID	gameObjPtr;
		AkUInt16        wStringSize;   // includes terminating NULL
		char			szName[1];     // variable string size
	};

	struct BusNotifMonitorData
	{
		AkUniqueID		busID;
		BusNotification notifReason;
		AkUInt8			bitsFXBypass;	// for FX bypass usage only
		AkUInt8			bitsMask;		// for FX bypass usage only
	};

	struct TimeStampData
	{
		AkTimeMs		timeStamp;
	};

	struct ObjectMonitorData
	{
		AkPlayingID			playingID;
		AkWwiseGameObjectID	gameObjPtr;
		NotificationReason	eNotificationReason;
		AkCntrHistArray		cntrHistArray;		
		AkCustomParamType	customParam;
		WwiseObjectIDext	targetObjectID;
		AkTimeMs			timeValue;		// "Fade time" with NotificationReason_Fade_Started, "Seek time" with NotificationReason_Seek
		AkUniqueID			playlistItemID;	// Actually used by MusicEngine to send playlist item ID.
	};
#define UNKNOWN_FADE_TIME AK_UINT_MAX

	struct MarkersMonitorData
	{
		AkPlayingID			playingID;
		AkWwiseGameObjectID	gameObjPtr;
		NotificationReason	eNotificationReason;
		AkCntrHistArray		cntrHistArray;		
		AkCustomParamType	customParam;
		AkUniqueID			targetObjectID;
		AkUInt16			wStringSize;   // includes terminating NULL
		char				szLabel[1];     // variable string size
	};

	struct StateMonitorData
	{
		AkStateGroupID	stateGroupID;
		AkStateID		stateFrom;
		AkStateID		stateTo;
	};

	struct SwitchMonitorData
	{
		AkSwitchGroupID	switchGroupID;
		AkSwitchStateID	switchState;
		AkWwiseGameObjectID	gameObj;
	};

	struct ParamChangedMonitorData
	{
		NotificationReason	eNotificationReason;
		AkWwiseGameObjectID	gameObjPtr;
		WwiseObjectIDext	elementID;
	};

	struct SetParamMonitorData
	{
		NotificationReason	eNotificationReason;
		AkWwiseGameObjectID	gameObjPtr;
		WwiseObjectIDext	elementID;
		AkReal32			fTarget;
		AkValueMeaning      valueMeaning;	// Offset or absolute
		AkTimeMs			transitionTime;
	};

	struct ErrorMonitorData1
	{
		AkPlayingID			playingID;
		AkWwiseGameObjectID	gameObjID;
		AK::Monitor::ErrorCode	eErrorCode;
		AkUInt32			uParam1;
		WwiseObjectIDext	soundID;
	};

	struct ActionTriggeredMonitorData
	{
		AkPlayingID			playingID;
		AkUniqueID			actionID;
		AkWwiseGameObjectID	gameObjPtr;
		AkCustomParamType	customParam;
	};

	struct ActionDelayedMonitorData : public ActionTriggeredMonitorData
	{
		AkTimeMs		delayTime;
	};

	struct EventTriggeredMonitorData
	{
		AkPlayingID			playingID;
		AkUniqueID			eventID;
		AkWwiseGameObjectID	gameObjPtr;
		AkCustomParamType	customParam;
	};

	struct BankMonitorData
	{
		NotificationReason	eNotificationReason;
		AkUniqueID			bankID;
		AkUniqueID			languageID;
		AkUInt32			uFlags;
		AkUInt16			wStringSize;   // includes terminating NULL
		char				szBankName[1]; // variable string size
	};

	struct PrepareMonitorData
	{
		NotificationReason	eNotificationReason;
		AkUInt32			gamesyncIDorEventID;// could use union instead, but would have to add additionnal codes in the ommand data serializer
		AkUInt32			groupID;
		AkGroupType			groupType;
		AkUInt32			uNumEvents;
	};

	struct AkAudioPerfTimers		// Spent time during timing period, in millisecs
	{
		AkReal32 fInterval;			// Length of timing period, in millisecs

		AkReal32 fAudioThread;		// AudioMgr thread (upper engine)
	};

	struct AudioPerfMonitorData
	{
		AkUInt16 numFadeTransitionsUsed;
		AkUInt16 maxFadeNumTransitions;
		AkUInt16 numStateTransitionsUsed;
		AkUInt16 maxStateNumTransitions;
		AkUInt16 numRegisteredObjects;
		AkUInt16 numPlayingIDs;
		AkUInt32 uCommandQueueActualSize;		// size of queue in bytes
		AkReal32 fCommandQueuePercentageUsed;	// percentage of queue used
		AkReal32 fDSPUsage;

		// Banks memory usage, for bank profiling.
		AkUInt32 uNumPreparedEvents;
		AkUInt32 uTotalMemBanks;
		AkUInt32 uTotalPreparedMemory;
		AkUInt32 uTotalMediaMemmory;

		// Perf timers
		AkAudioPerfTimers timers;
	};

	struct GameObjPosition
	{
		AkWwiseGameObjectID gameObjID;
		AkSoundPosition position;
	};

	struct ListenerPosition
	{
		AkUInt32 uIndex;
		AkListenerPosition position;
	};

	struct GameObjPositionMonitorData
	{
		AkUInt32 ulNumGameObjPositions;
		AkUInt32 ulNumListenerPositions;

		union Position
		{
			GameObjPosition gameObjPosition;
			ListenerPosition listenerPosition;
		};

		Position positions[1];
	};

	struct DebugMonitorData
	{
		AkUInt16		wStringSize;   // includes terminating NULL
		AkUtf16			szMessage[1];  // variable string size
	};

	enum AkPathEvent
	{
		AkPathEvent_Undefined		= 0,
		AkPathEvent_ListStarted		= 1,
		AkPathEvent_VertexReached	= 2
	};

	struct PathMonitorData
	{
		AkPlayingID	playingID;
		AkUniqueID	ulUniqueID;
		AkPathEvent	eEvent;
		AkUInt32		ulIndex;
	};

	struct PluginTimerData
	{
        AkUInt32	uiPluginID;
		AkReal32		fMillisecs;
		AkUInt32	uiNumInstances;
	};

	struct PluginTimerMonitorData
	{
		AkReal32 		fInterval;	   // Length of timing period, in millisecs
		AkUInt32         ulNumTimers;
		PluginTimerData pluginData[1]; // Variable array of size ulNumTimers
	};

    typedef AK::MemoryMgr::PoolStats MemoryPoolData;
	struct MemoryMonitorData
	{
        AkUInt32        ulNumPools;
		MemoryPoolData poolData[1]; // Variable array of size ulNumPools
	};

    struct MemoryPoolNameMonitorData
	{
		AkUInt32		ulPoolId;			// ID of pool
		AkUInt16		wStringSize;       // includes terminating NULL
		AkUtf16			szName[1]; // max 63 caracters + ending caracter
	};

    // To be sent at connect time.
    typedef AkDeviceDesc DeviceRecordMonitorData;

    // To be sent when a stream is created.
    typedef AkStreamRecord StreamRecord;
    struct StreamRecordMonitorData     
    {
        AkUInt32        ulNumNewRecords; // Number of records.
        StreamRecord   streamRecords[1];// Stream records.
    };

    // Periodic stream stats.
	typedef AkDeviceData DeviceData;
	struct StreamDeviceMonitorData
    {
    	AkReal32 	   fInterval;		// Length of timing period, in millisecs
        AkUInt32       ulNumDevices;    // Number of streaming devices
        DeviceData     deviceData[1];   // Variable array of size ulNumDevices
    };

    typedef AkStreamData StreamData;
    struct StreamingMonitorData
    {
    	AkReal32 	   fInterval;		// Length of timing period, in millisecs
        AkUInt32       ulNumStreams;    // Number of streams open
        StreamData     streamData[1];   // Variable array of size ulNumStreams
    };

	struct EnvPacket
	{
		AkWwiseGameObjectID	gameObjID;
		AkReal32			fDryValue;
		AkAuxSendValue	environments[AK_MAX_GAME_DEFINED_AUX_PER_OBJ_PROFILER];
	};

	struct ObsOccPacket
	{
		AkWwiseGameObjectID	gameObjID;
		AkReal32			fObsValue[AK_NUM_LISTENERS];
		AkReal32			fOccValue[AK_NUM_LISTENERS];
	};

	// Periodic Environment Info.
    struct EnvironmentMonitorData
    {
        AkUInt32		ulNumEnvPacket;	// Number of Environment Packets
        EnvPacket	envPacket[1];	// Variable array of size ulNumEnvPacket
    };

    struct ObsOccMonitorData
    {
        AkUInt32			ulNumPacket;		// Number of Obs/Occ Packets
        ObsOccPacket	obsOccPacket[1];	// Variable array of size ulNumPacket
    };

	struct SpeakerVolumes
	{
		AkReal32 fFrontLeft;				///< Front-Left volume
		AkReal32 fFrontRight;				///< Front-Right volume
		AkReal32 fCenter;					///< Center volume
		AkReal32 fLfe;						///< LFE volume
		AkReal32 fRearLeft;					///< Rear-Left volume
		AkReal32 fRearRight;				///< Rear-Right volume
		AkReal32 fSideLeft;					///< Side-Left volume
		AkReal32 fSideRight;				///< Side-Right volume
	};

	struct ListenerPacket
	{
		SpeakerVolumes		VolumeOffset;	// per-speaker volume offset
		AkUInt8				iMotionPlayer ;	// Players associated to the listener for motion purposes (bitfield). 
		bool				bMotion		  ;	// true if it receives motion
		bool				bSpatialized  ;	// false: attenuation only
	};

	struct GameObjectListenerMaskPacket
	{
		AkWwiseGameObjectID  gameObject;
		AkUInt32        uListenerMask;
	};

	struct ListenerMonitorData
	{
		ListenerPacket	listeners[AK_NUM_LISTENERS];
		AkUInt32			ulNumGameObjMask;	// Number of GameObjectListenerMaskPacket Packets
		GameObjectListenerMaskPacket gameObjMask[1];
	};

	struct ControllerPacket
	{
		AkReal32	Volume;		// per-controller volume
		bool		bIsActive;	// per-controller activity
	};

	struct GameObjectControllerMaskPacket
	{
		AkWwiseGameObjectID  gameObject;
		AkUInt32        uControllerMask; // Bits 0 to 3 stands for controller 0 to 3
	};

#define AK_MAX_NUM_CONTROLLER_MONITORING 4
	struct ControllerMonitorData
	{
		ControllerPacket	controllers[AK_MAX_NUM_CONTROLLER_MONITORING];
		AkUInt32			ulNumGameObjMask;	// Number of GameObjectControllerMaskPacket Packets
		GameObjectControllerMaskPacket gameObjMask[1];
	};

	struct RTPCValuesPacket
	{
		AkUniqueID		rtpcID;	
		AkWwiseGameObjectID  gameObjectID;
		AkReal32		value;	
		bool			bHasValue;
	};

	struct RTPCValuesMonitorData
	{
		AkUInt32 ulNumRTPCValues;
		RTPCValuesPacket rtpcValues[1];
	};

	struct PipelineData_Bus
	{
        AkUniqueID			mixBusID;
        AkUniqueID			mixBusParentID;
		AkOutputDeviceID	deviceID;
		AkUInt32			channelMask;
		AkReal32			fBusVolume;			// Bus gain. dB.
		AkReal32			fDownstreamGain;	// Gain from current bus down to output. dB.
		AkPluginID			fxID[AK_NUM_EFFECTS_PROFILER];		// Bus insert effect.
		AkUInt16			uVoiceCount;
		bool				bFeedback;
	};

	struct PipelineData_Voice
	{
        AkWwiseGameObjectID	gameObjID;
		AkPlayingID			playingID;
        AkUniqueID			soundID;
        AkUniqueID			mixBusID;
		AkUniqueID			feedbackMixBusID;

		AkPluginID			fxID[AK_NUM_EFFECTS_PROFILER];	// Sound insert effect.
        
        AkReal32        fBaseVolume;        // Voice volume (hierarchy). Includes HDR attenuation is applicable. dB.
		AkReal32		fMaxVolume;			// Max voice volume, including bus gains, among all signal paths. dB.
		AkReal32		fHdrWindowTop;		// HDR window top for this voice. dB SPL.
		AkReal32		fEnvelope;			// Current analyzed envelope value, in dB. 0 dB if unavailable.
		AkReal32		fNormalizationGain;	// Normalization gain (loudness normalization and make-up gain). dB. 
		AkReal32		fOutputBusVolume[ AK_MAX_NUM_CHANNELS ]; // entire dry path, per output channel
		AkReal32		fBaseLPF;			// Hierarchy LPF
		AkReal32		fVoiceLPF;			// max( base, occlusion) 
		AkReal32		fOutputBusLPF;		// max (out.bus.lpf, obstruction)
        AkPriority      priority;
        bool            bIsStarted;
		bool            bIsVirtual;
		bool			bIsForcedVirtual;

		// Game-Defined
		AkReal32 fGameAuxSendVolume;
	};

    struct PipelineData
    {
		// A unique ID in time: 0 means BUS, else VOICE
		AkUniqueID		pipelineID;

		union
		{
			PipelineData_Bus bus;
			PipelineData_Voice voice;
		};
	};

	struct PipelineDevMap
	{
		AkUniqueID			pipelineID;
		AkUniqueID			mixBusID;
		AkOutputDeviceID	deviceID;
		AkReal32			fMaxVolume;
		AkReal32			fOutputBusVolume[ AK_MAX_NUM_CHANNELS ]; // entire dry path, per output channel
	};

    struct PipelineMonitorData
    {
		AkUInt16        numPipelineData;
        AkUInt16        numPipelineDevMap;
		AkUInt32		placeholder;
		// Data that follows is mapped starting at placeholder above.
        //PipelineData		pipelineData[];		// Variable array of size numPipelineData
		//PipelineDevMap	pipelineDevMap[];	// Variable array of size numPipelineDevMap
    };

	struct OutputMonitorData
	{
		AkReal32		fPeak;
		AkReal32		fOffset;
		AkReal32		fRMS;
	};

	struct SegmentPositionData
	{
		AkReal64			f64Position;
		AkPlayingID			playingID;
		AkUniqueID			segmentID;
		AkCustomParamType	customParam;
	};

	struct SegmentPositionMonitorData
	{
		AkUInt32			numPositions;
		SegmentPositionData positions[1];
	};

	/////////////////////////////////////////////
	// Bank profiling notifications
	/////////////////////////////////////////////
	struct LoadedSoundBankMonitorData
	{
		AkBankID	bankID;
		AkMemPoolId memPoolID;
		AkUInt32	uBankSize;
		AkUInt32	uNumIndexableItems;
		AkUInt32	uNumMediaItems;
		bool		bIsExplicitelyLoaded;
		bool		bIsDestroyed;
	};

	struct MediaPreparedMonitorData
	{
		struct BankMedia
		{
			AkBankID	bankID; // Invalid ID means prepared
			AkUInt32	uMediaSize;
		};

		AkUniqueID	uMediaID;

		AkUInt32	uArraySize;// If zero, means not prepared anymore.
		BankMedia	bankMedia[1];// Invalid ID means prepared
	};

	struct EventPreparedMonitorData
	{
		AkUniqueID	eventID;
		AkUInt32	uRefCount;
	};

	struct GameSyncMonitorData
	{
		AkUniqueID	groupID;
		AkUniqueID	syncID;
		AkGroupType eSyncType;	// enum either AkGroupType_Switch or AkGroupType_State
		bool		bIsEnabled; // true = ON, false = OFF
	};
	/////////////////////////////////////////////

	struct FeedbackMonitorData
	{
		AkAudioPerfTimers	timer;
		AkReal32			fPeak;
	};

	struct GameObjectPlayerMaskPacket
	{
		AkWwiseGameObjectID  gameObject;
		AkUInt32        uPlayerMask; // Bits 0 to 3 stands for controller 0 to 3
	};

	struct FeedbackGameObjMonitorData
	{
		AkUInt32	ulNumGameObjMask;						// Number of GameObjectPlayerMaskPacket Packets
		GameObjectPlayerMaskPacket gameObjInfo[1];
	};

	struct FeedbackDeviceIDMonitorData
	{
		AkUInt16 usCompanyID;
		AkUInt16 usDeviceID;
		AkUInt8	 ucPlayerActive;					//Bitfield to tell which player are active.  4 bits, one per player.
	};

	struct FeedbackDevicesMonitorData
	{
		AkUInt16 usDeviceCount;
		FeedbackDeviceIDMonitorData deviceIDs[1];	// Dynamic array of device IDs
	};

	/////////////////////////////////////////////
	// Music engine notifications
	/////////////////////////////////////////////
	struct MusicTransitionMonitorData
	{
		AkPlayingID			playingID;
		AkWwiseGameObjectID	gameObj;
		NotificationReason	eNotificationReason;
		AkUInt32			uTransitionRuleIndex;	// 0-based rule index (Wwise display starts with 1)
		AkUniqueID			musicSwitchContainer;	// Switch container for switch transitions.
		AkUniqueID			nodeSrcID;				// Switch transitions: source node; Stingers: source node.
		AkUniqueID			nodeDestID;				// Switch transitions: destination node; Stingers: stinger node.
		AkUniqueID			segmentSrc;		// Switch transitions: source segment (child or descendant); Stingers: host segment.
		AkUniqueID			segmentDest;
		AkTimeMs			time;
		AkUniqueID			cueSrc;			// Source cue ID
		AkUniqueID			cueDest;		// Destination cue ID
	};

	// Custom data upload for plugins
	struct PluginMonitorData
	{
		AkUniqueID			audioNodeID;// Owner node ID.
		AkPluginID			pluginID;	// Effect unique type ID. 
		AkUInt32			uFXIndex;	// Effect's index.
		AkUInt32			uDataSize;	// Size of data buffer.
		AkUInt8				arBytes[1];	// Variable bytes array of size uDataSize
	};

	enum DialogueWeightedDecisionType
	{
		DialogueWeightedDecisionType_NotWeighted = 0,
		DialogueWeightedDecisionType_100 = 1,
		DialogueWeightedDecisionType_1to99 = 2,
		DialogueWeightedDecisionType_0 = 3,
		DialogueWeightedDecisionType_NoCandidate = 4
	};

	struct CommonDialogueMonitorData
	{
		AkUniqueID			idDialogueEvent;
		AkUniqueID			idObject;
		AkUInt32			uPathSize;
		AkPlayingID			idSequence;
		AkUInt16			uRandomChoice;
		AkUInt16			uTotalProbability;
		AkUInt8				uWeightedDecisionType;	// DialogueWeightedDecisionType
		AkUInt32			uWeightedPossibleCount;
		AkUInt32			uWeightedTotalCount;
		AkArgumentValueID	aPath[1];						// Variable-sized
	};

	struct ExternalSourceMonitorData
	{
		AkWwiseGameObjectID  idGameObj;
		AkUniqueID		idSource;
		AkPlayingID		idPlay;
		AkUInt16		wStringSize;   // includes terminating NULL
		AkUtf16			szUtf16File[1];  // variable string size
	};

	struct BusMeterData
	{
		AkUniqueID idBus;		// bus short ID
		AkUInt8 uDataType;		// enum BusMeterDataType
		AkUInt16 uChannelMask;	// Number of channel in aChannels
		AkReal32 aChannels[ AK_MAX_NUM_CHANNELS ];		// meter data per channel
	};

	struct MeterData
	{
		AkUInt16	uNumBusses;	// Number of busses
		BusMeterData busMeters[1];  // Dynamic array of busses
	};

	struct PlatformSinkTypeData
	{
		AkUInt16	uSinkType;
		AkUInt16	uBufSize;	// Size of szName buffer
		char		szNameBuf[1]; // Contains null-terminated string; may have garbage after NULL char
	};

	enum SendType
	{
		SendType_UserDefinedSend0 = 0,
		SendType_UserDefinedSend1,
		SendType_UserDefinedSend2,
		SendType_UserDefinedSend3,
		SendType_GameDefinedSend0,
		SendType_GameDefinedSend1,
		SendType_GameDefinedSend2,
		SendType_GameDefinedSend3
	};

	struct SendsData
	{
		AkUniqueID			pipelineID;
        AkWwiseGameObjectID	gameObjID;
        AkUniqueID			soundID;

		// Send data
		AkReal32 fVolume;
		AkUniqueID auxBusID;

		SendType eSendType;
	};

	struct SendsMonitorData
	{
		AkUInt32    ulNumSends;
		SendsData	sends[1]; // Variable array of size ulNumSends
	};

	struct MonitorDataItem
	{
		AkUInt8 eDataType; // MonitorDataType

		union
		{
			TimeStampData				timeStampData;
			ObjectMonitorData			objectData;			// Notif
			StateMonitorData			stateData;			// Notif
			SwitchMonitorData			switchData;			// Notif
			ParamChangedMonitorData		paramChangedData;	// Notif
			MarkersMonitorData			markersData;		// Notif
			
			SetParamMonitorData			setParamData;		// Log only
			ActionTriggeredMonitorData	actionTriggeredData;// Log only
			ActionDelayedMonitorData	actionDelayedData;	// Log only
			EventTriggeredMonitorData	eventTriggeredData;	// Log only

			BankMonitorData				bankData;			// Bank loading
			PrepareMonitorData			prepareData;		// PrepareEvent/PrepareGameSync

			BusNotifMonitorData			busNotifData;		// Bus notification

			AudioPerfMonitorData		audioPerfData;		// Performance information

			GameObjPositionMonitorData	gameObjPositionData;// Game Object position in 3D

			ObjRegistrationMonitorData	objRegistrationData;// Log helper
	
			ErrorMonitorData1			errorData1;			// Error

			DebugMonitorData			debugData;			// Audiolib User Message

			PathMonitorData				pathData;

			PluginTimerMonitorData      pluginTimerData;

			MemoryMonitorData           memoryData;
			MemoryPoolNameMonitorData   memoryPoolNameData;

			EnvironmentMonitorData		environmentData;
			SendsMonitorData			sendsData;

			ListenerMonitorData			listenerData;

			ObsOccMonitorData			obsOccData;

            StreamingMonitorData  		streamingData;
			StreamDeviceMonitorData		streamingDeviceData;
            StreamRecordMonitorData     streamRecordData;
            DeviceRecordMonitorData   	deviceRecordData;

			PipelineMonitorData         pipelineData;
			OutputMonitorData			outputData;
			SegmentPositionMonitorData	segmentPositionData;

			ControllerMonitorData		controllerData;
			RTPCValuesMonitorData		rtpcValuesData;

			LoadedSoundBankMonitorData	loadedSoundBankData;
			MediaPreparedMonitorData	mediaPreparedData;
			EventPreparedMonitorData	eventPreparedData;
			GameSyncMonitorData			gameSyncData;
			FeedbackMonitorData			feedbackData;
			FeedbackGameObjMonitorData	feedbackGameObjData;
			FeedbackDevicesMonitorData	feedbackDevicesData;
			CommonDialogueMonitorData	commonDialogueData;

			MusicTransitionMonitorData	musicTransData;
			PluginMonitorData			pluginMonitorData;

			ExternalSourceMonitorData	templateSrcData;

			MeterData					meterData;

			PlatformSinkTypeData		platformSinkTypeData;
		};

	private:
		MonitorDataItem & operator=( const MonitorDataItem & /*in_other*/ )
		{
			// Cannot copy this struct with operator= because of variable size elements ( pluginTimerData, etc. )
			return *this;
		}
	};

	// Return the real size of a MonitorDataItem ( for allocation, copy purpose )
	AKSOUNDENGINE_API AkUInt32 RealSizeof( const MonitorDataItem & in_rItem );

	// The watches define the game objects and listeners that are monitored by the game engine
	// There are several watch types
	enum WatchType
	{
		// NOTE: don't change the IDs because those values are persisted
		WatchType_Unknown = -1,
		WatchType_GameObjectName = 0,
		WatchType_GameObjectID = 1,
		WatchType_ListenerID = 2
	};

	// WARNING: This structure is persisted into the registry
	// If the content of it is modified, update MonitoringMgrUserPref::k_lWatchVersion
	struct Watch
	{
		Watch(): eType( WatchType_Unknown ), wNameSize( 0 ){ ID.gameObjectID = 0; szName[0] = 0; }
		WatchType eType;
		union 
		{
			AkWwiseGameObjectID gameObjectID;
			AkUInt32 uiListenerID;
		} ID;
		AkUInt16 wNameSize;
		char szName[128];
	};

	struct MeterWatch
	{
		AkUniqueID busID;
		AkUInt8 uBusMeterDataTypes; // bitmask of BusMeterDataType
	};
}

#endif	// _MONITORDATA_H_
