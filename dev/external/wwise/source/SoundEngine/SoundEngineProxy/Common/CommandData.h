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
#pragma once

#ifndef AK_OPTIMIZED

#include "AkRTPC.h"
#include "AkPath.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include "AkLayerCntr.h"
#include "AkMusicStructs.h"
#include "AkAttenuationMgr.h"
#include "AkMonitorData.h"
#include "CommandDataSerializer.h"
#include "IActionProxy.h"
#include "IBusProxy.h"
#include "IEventProxy.h"
#include "IFeedbackNodeProxy.h"
#include "IFxBaseProxy.h"
#include "IParameterNodeProxy.h"
#include "IRanSeqContainerProxy.h"
#include "ISoundProxy.h"
#include "IStateProxy.h"
#include "ITrackProxy.h"
#include "IMultiSwitchProxy.h"

#include "AkPrivateTypes.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace AkMonitorData
{
	struct Watch;
}

namespace ProxyCommandData
{
	enum CommandType
	{
		TypeRendererProxy = 1,
		TypeALMonitorProxy,
		TypeStateMgrProxy,
		TypeObjectProxyStore,
		TypeObjectProxy
	};

	struct CommandData
	{
		inline CommandData() {}
		inline CommandData( CommandType in_eCommandType, AkUInt16 in_methodID )
			: m_commandType( (AkUInt16)in_eCommandType )
			, m_methodID( in_methodID )
		{
		}
		
		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_commandType;
		AkUInt16 m_methodID;

		static AkMemPoolId s_poolID;
	};

	template <class POINT>
	struct CurveData
	{
		CurveData()
			: m_bWasCurveDeserialized(false)
			, m_eScaling( AkCurveScaling_None )
			, m_pArrayConversion( NULL )
			, m_ulConversionArraySize( 0 )
		{};

		~CurveData()
		{
			if (m_bWasCurveDeserialized && m_pArrayConversion)
				AkFree(CommandData::s_poolID, m_pArrayConversion);
		}

		bool Deserialize( CommandDataSerializer &in_rSerializer )
		{
			m_bWasCurveDeserialized = true;
			return in_rSerializer.GetEnum( m_eScaling ) &&
				in_rSerializer.DeserializeArray( m_ulConversionArraySize, m_pArrayConversion);		
		}

		bool Serialize( CommandDataSerializer &in_rSerializer ) const
		{
			return in_rSerializer.PutEnum( m_eScaling ) && 
				in_rSerializer.SerializeArray( m_ulConversionArraySize, m_pArrayConversion);
		}

		bool m_bWasCurveDeserialized;
		AkCurveScaling m_eScaling;
		POINT* m_pArrayConversion;
		AkUInt32 m_ulConversionArraySize;
	};
}

namespace RendererProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};


	struct ExecuteActionOnEvent : public CommandData
	{
		ExecuteActionOnEvent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_eventID;
		AK::SoundEngine::AkActionOnEventType m_eActionType;
        AkWwiseGameObjectID m_gameObjectID;
		AkTimeMs m_uTransitionDuration;
		AkCurveInterpolation m_eFadeCurve;

		DECLARE_BASECLASS( CommandData );
	};

	struct PostEvent : public CommandData
	{
		PostEvent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_eventID;
		AkWwiseGameObjectID m_gameObjectPtr;
		AkUInt32 m_cookie;

		DECLARE_BASECLASS( CommandData );
	};

	struct RegisterGameObj : public CommandData
	{
		RegisterGameObj();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectPtr;
		char* m_pszObjectName;

		DECLARE_BASECLASS( CommandData );
	};

	struct UnregisterGameObj : public CommandData
	{
		UnregisterGameObj();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectPtr;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetActiveListeners : public CommandData
	{
		SetActiveListeners();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectID;
		AkUInt32 m_uListenerMask;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetAttenuationScalingFactor : public CommandData
	{
		SetAttenuationScalingFactor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectID;
		AkReal32 m_fAttenuationScalingFactor;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetPanningRule : public CommandData
	{
		SetPanningRule();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPanningRule m_panningRule;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetMultiplePositions : public CommandData
	{
		SetMultiplePositions();
		~SetMultiplePositions();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectPtr;
		AkSoundPosition * m_pPositions;
		AkUInt32 m_numPositions;
		AK::SoundEngine::MultiPositionType m_eMultiPositionType;

		DECLARE_BASECLASS( CommandData );
	private:
		bool m_bWasDeserialized;
	};

	struct SetRelativePosition : public CommandData
	{
		SetRelativePosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSoundPosition m_position;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetPosition : public CommandData
	{
		SetPosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectPtr;
		AkSoundPosition m_position;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetListenerPosition : public CommandData
	{
		SetListenerPosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkListenerPosition m_position;
		AkUInt32 m_ulIndex;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetListenerScalingFactor : public CommandData
	{
		SetListenerScalingFactor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_ulIndex;
		AkReal32 m_fAttenuationScalingFactor;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetListenerSpatialization : public CommandData
	{
		SetListenerSpatialization();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uIndex;
		bool m_bSpatialized;
		bool m_bUseVolumeOffsets;		// Use to know if the AkSpeakerVolumes structure should be used.
		AkSpeakerVolumes m_volumeOffsets;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetRTPC : public CommandData
	{
		SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCid;
		AkReal32 m_value;
		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetDefaultRTPCValue : public CommandData
	{
		SetDefaultRTPCValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCid;
		AkReal32 m_defaultValue;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetRTPC : public CommandData
	{
		ResetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetRTPCValue : public CommandData
	{
		ResetRTPCValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCid;
		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetSwitch : public CommandData
	{
		SetSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchGroupID m_switchGroup;
		AkSwitchStateID m_switchState;
		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct PostTrigger : public CommandData
	{
		PostTrigger();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTriggerID m_trigger;
		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetSwitches : public CommandData
	{
		ResetSwitches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetAllStates : public CommandData
	{
		ResetAllStates();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetRndSeqCntrPlaylists : public CommandData
	{
		ResetRndSeqCntrPlaylists();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameObjectAuxSendValues : public CommandData
	{
		SetGameObjectAuxSendValues();
		~SetGameObjectAuxSendValues();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectID; 
		AkAuxSendValue* m_aEnvironmentValues;
		AkUInt32 m_uNumEnvValues;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameObjectOutputBusVolume : public CommandData
	{
		SetGameObjectOutputBusVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjectID;
		AkReal32 m_fControlValue;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetObjectObstructionAndOcclusion : public CommandData
	{
		SetObjectObstructionAndOcclusion();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_ObjectID;
		AkUInt32 m_uListener;
		AkReal32 m_fObstructionLevel;
		AkReal32 m_fOcclusionLevel;

		DECLARE_BASECLASS( CommandData );
	};
	
	struct SetObsOccCurve :  public CommandData, public ProxyCommandData::CurveData<AkRTPCGraphPoint>
	{
		SetObsOccCurve();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt32 m_curveXType;
		AkInt32 m_curveYType;

	private:
	
		DECLARE_BASECLASS( CommandData );
	};

	struct SetObsOccCurveEnabled :  public CommandData
	{
		SetObsOccCurveEnabled();
		~SetObsOccCurveEnabled();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt32 m_curveXType;
		AkInt32 m_curveYType;
		bool m_bEnabled;
	
		DECLARE_BASECLASS( CommandData );
	};

	struct AddSwitchRTPC : public CommandData
	{
		AddSwitchRTPC();
		~AddSwitchRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uSwitchGroup;
		AkRtpcID m_RTPC_ID;
		AkUInt32 m_uArraySize;
		AkRTPCGraphPointInteger* m_pArrayConversion;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveSwitchRTPC :  public CommandData
	{
		RemoveSwitchRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uSwitchGroup;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetVolumeThreshold :  public CommandData
	{
		SetVolumeThreshold();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fVolumeThreshold;

		DECLARE_BASECLASS( CommandData );
	};
	
	struct SetMaxNumVoicesLimit :  public CommandData
	{
		SetMaxNumVoicesLimit();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_maxNumberVoices;

		DECLARE_BASECLASS( CommandData );
	};

	struct PostMsgMonitor : public CommandData
	{
		PostMsgMonitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUtf16* m_pszMessage;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopAll :  public CommandData
	{
		StopAll();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_GameObjPtr;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopPlayingID :  public CommandData
	{
		StopPlayingID();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPlayingID m_playingID;
		AkTimeMs m_uTransitionDuration;
		AkCurveInterpolation m_eFadeCurve;

		DECLARE_BASECLASS( CommandData );
	};
}

namespace ALMonitorProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct Monitor : public CommandData
	{
		Monitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkMonitorData::MaskType m_uWhatToMonitor;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopMonitor : public CommandData
	{
		StopMonitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct SetMeterWatches : public CommandData
	{
		SetMeterWatches();
		~SetMeterWatches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uiWatchCount;
		AkMonitorData::MeterWatch* m_pWatches;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetWatches : public CommandData
	{
		SetWatches();
		~SetWatches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uiWatchCount;
		AkMonitorData::Watch* m_pWatches;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameSyncWatches : public CommandData
	{
		SetGameSyncWatches();
		~SetGameSyncWatches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uiWatchCount;
		AkUniqueID* m_pWatches;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};
}

namespace StateMgrProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct AddStateGroup : public CommandData
	{
		AddStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveStateGroup : public CommandData
	{
		RemoveStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct AddStateTransition : public CommandData
	{
		AddStateTransition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID1;
		AkStateID m_stateID2;
		AkTimeMs m_transitionTime;
		bool m_bIsShared;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveStateTransition : public CommandData
	{
		RemoveStateTransition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID1;
		AkStateID m_stateID2;
		bool m_bIsShared;

		DECLARE_BASECLASS( CommandData );
	};

	struct ClearStateTransitions : public CommandData
	{
		ClearStateTransitions();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetDefaultTransitionTime : public CommandData
	{
		SetDefaultTransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkTimeMs m_transitionTime;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetState : public CommandData
	{
		SetState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID;

		DECLARE_BASECLASS( CommandData );
	};

	struct GetState : public CommandData
	{
		GetState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};
}


namespace ObjectProxyStoreCommandData
{
	enum MethodIDs
	{
		MethodCreate = 1,
		MethodRelease
	};

	enum ObjectType
	{
		TypeSound = 1,
		TypeEvent,
		TypeDialogueEvent,
		TypeAction,
		TypeCustomState,
		TypeRanSeqContainer,
		TypeSwitchContainer,
		TypeActorMixer,
		TypeBus,
		TypeAuxBus,
		TypeLayerContainer,
		TypeLayer,
		TypeMusicTrack,
		TypeMusicSegment,
		TypeMusicRanSeq,
		TypeMusicSwitch,
		TypeAttenuation,
		TypeFeedbackBus,
		TypeFeedbackNode,
		TypeFxShareSet,
		TypeFxCustom
	};

	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_proxyInstancePtr;
		AkUniqueID m_objectID;

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct Create : public CommandData
	{
		Create();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		ObjectType m_eObjectType;
		AkActionType m_actionType;	// Optional

		DECLARE_BASECLASS( CommandData );
	};

	struct Release : public CommandData
	{
		Release();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};
}

namespace ObjectProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		inline CommandData() {}
		inline CommandData( AkUInt16 in_methodID ) : ProxyCommandData::CommandData( ProxyCommandData::TypeObjectProxy, in_methodID ) {}

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_proxyInstancePtr;
		AkUniqueID m_objectID;

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	template< AkUInt32 METHOD_ID >
	struct CommandData0 : public CommandData
	{
		inline CommandData0() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData0( IObjectProxy * in_pProxy ) : ObjectProxyCommandData::CommandData( METHOD_ID )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}
	};

	template< AkUInt32 METHOD_ID, class PARAM1 >
	struct CommandData1 : public CommandData
	{
		inline CommandData1() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData1( IObjectProxy * in_pProxy, const PARAM1 & in_param1 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 );
		}

		PARAM1 m_param1;
	};

	template< AkUInt32 METHOD_ID, class PARAM1, class PARAM2 >
	struct CommandData2 : public CommandData
	{
		inline CommandData2() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData2( IObjectProxy * in_pProxy, const PARAM1 & in_param1, const PARAM2 & in_param2 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
			, m_param2( in_param2 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 )
				&& in_rSerializer.Put( m_param2 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 )
				&& in_rSerializer.Get( m_param2 );
		}

		PARAM1 m_param1;
		PARAM2 m_param2;
	};

	template< AkUInt32 METHOD_ID, class PARAM1, class PARAM2, class PARAM3 >
	struct CommandData3 : public CommandData
	{
		inline CommandData3() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData3( IObjectProxy * in_pProxy, const PARAM1 & in_param1, const PARAM2 & in_param2, const PARAM3 & in_param3 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
			, m_param2( in_param2 )
			, m_param3( in_param3 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 )
				&& in_rSerializer.Put( m_param2 )
				&& in_rSerializer.Put( m_param3 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 )
				&& in_rSerializer.Get( m_param2 )
				&& in_rSerializer.Get( m_param3 );
		}

		PARAM1 m_param1;
		PARAM2 m_param2;
		PARAM3 m_param3;
	};

	template< AkUInt32 METHOD_ID, class PARAM1, class PARAM2, class PARAM3, class PARAM4 >
	struct CommandData4 : public CommandData
	{
		inline CommandData4() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData4( IObjectProxy * in_pProxy, const PARAM1 & in_param1, const PARAM2 & in_param2, const PARAM3 & in_param3, const PARAM4 & in_param4 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
			, m_param2( in_param2 )
			, m_param3( in_param3 )
			, m_param4( in_param4 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 )
				&& in_rSerializer.Put( m_param2 )
				&& in_rSerializer.Put( m_param3 )
				&& in_rSerializer.Put( m_param4 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 )
				&& in_rSerializer.Get( m_param2 )
				&& in_rSerializer.Get( m_param3 )
				&& in_rSerializer.Get( m_param4 );
		}

		PARAM1 m_param1;
		PARAM2 m_param2;
		PARAM3 m_param3;
		PARAM4 m_param4;
	};

	template< AkUInt32 METHOD_ID, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5 >
	struct CommandData5 : public CommandData
	{
		inline CommandData5() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData5( IObjectProxy * in_pProxy, const PARAM1 & in_param1, const PARAM2 & in_param2, const PARAM3 & in_param3, const PARAM4 & in_param4, const PARAM5 & in_param5 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
			, m_param2( in_param2 )
			, m_param3( in_param3 )
			, m_param4( in_param4 )
			, m_param5( in_param5 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 )
				&& in_rSerializer.Put( m_param2 )
				&& in_rSerializer.Put( m_param3 )
				&& in_rSerializer.Put( m_param4 )
				&& in_rSerializer.Put( m_param5 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 )
				&& in_rSerializer.Get( m_param2 )
				&& in_rSerializer.Get( m_param3 )
				&& in_rSerializer.Get( m_param4 )
				&& in_rSerializer.Get( m_param5 );
		}

		PARAM1 m_param1;
		PARAM2 m_param2;
		PARAM3 m_param3;
		PARAM4 m_param4;
		PARAM5 m_param5;
	};

	template< AkUInt32 METHOD_ID, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6 >
	struct CommandData6 : public CommandData
	{
		inline CommandData6() : ObjectProxyCommandData::CommandData( METHOD_ID ) {}

		inline CommandData6( IObjectProxy * in_pProxy, const PARAM1 & in_param1, const PARAM2 & in_param2, const PARAM3 & in_param3, const PARAM4 & in_param4, const PARAM5 & in_param5, const PARAM6 & in_param6 ) 
			: ObjectProxyCommandData::CommandData( METHOD_ID )
			, m_param1( in_param1 )
			, m_param2( in_param2 )
			, m_param3( in_param3 )
			, m_param4( in_param4 )
			, m_param5( in_param5 )
			, m_param6( in_param6 )
		{
			m_proxyInstancePtr = (AkUInt32)(AkUIntPtr) in_pProxy;
			m_objectID = in_pProxy->GetID();
		}

		inline bool Serialize( CommandDataSerializer& in_rSerializer ) const
		{
			return ObjectProxyCommandData::CommandData::Serialize( in_rSerializer )
				&& in_rSerializer.Put( m_param1 )
				&& in_rSerializer.Put( m_param2 )
				&& in_rSerializer.Put( m_param3 )
				&& in_rSerializer.Put( m_param4 )
				&& in_rSerializer.Put( m_param5 )
				&& in_rSerializer.Put( m_param6 );
		}

		inline bool Deserialize( CommandDataSerializer& in_rSerializer )
		{
			return ObjectProxyCommandData::CommandData::Deserialize( in_rSerializer )
				&& in_rSerializer.Get( m_param1 )
				&& in_rSerializer.Get( m_param2 )
				&& in_rSerializer.Get( m_param3 )
				&& in_rSerializer.Get( m_param4 )
				&& in_rSerializer.Get( m_param5 )
				&& in_rSerializer.Get( m_param6 );
		}

		PARAM1 m_param1;
		PARAM2 m_param2;
		PARAM3 m_param3;
		PARAM4 m_param4;
		PARAM5 m_param5;
		PARAM6 m_param6;
	};
}

namespace ParameterableProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodAddChild, 
		WwiseObjectIDext // AkUniqueID and type
	> AddChild;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodRemoveChild, 
		WwiseObjectIDext // AkUniqueID and type
	> RemoveChild;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IParameterableProxy::MethodSetAkPropF,
		AkUInt32, // AkPropID
		AkReal32,
		AkReal32,
		AkReal32
	> SetAkPropF;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IParameterableProxy::MethodSetAkPropI,
		AkUInt32, // AkPropID
		AkInt32,
		AkInt32,
		AkInt32
	> SetAkPropI;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodPriorityApplyDistFactor, 
		bool 
	> PriorityApplyDistFactor;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodPriorityOverrideParent, 
		bool 
	> PriorityOverrideParent;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodAddStateGroup, 
		AkStateGroupID 
	> AddStateGroup;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodRemoveStateGroup, 
		AkStateGroupID 
	> RemoveStateGroup;

	struct UpdateStateGroups : public ObjectProxyCommandData::CommandData0<IParameterableProxy::MethodUpdateStateGroups>
	{
		UpdateStateGroups();
		UpdateStateGroups(IObjectProxy * in_pProxy);
		~UpdateStateGroups();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uGroups;
		AkStateGroupUpdate * m_pGroups;		
		AkStateUpdate *m_pStates;

		bool m_bDeserialized;

	private:

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData0<IParameterableProxy::MethodUpdateStateGroups> );
	};
	
	typedef ObjectProxyCommandData::CommandData3
	< 
		IParameterableProxy::MethodAddState,
		AkStateGroupID,
		AkUniqueID, 
		AkStateID 
	> AddState;
	
	typedef ObjectProxyCommandData::CommandData2
	< 
		IParameterableProxy::MethodRemoveState,
		AkStateGroupID,
		AkStateID 
	> RemoveState;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodUseState, 
		bool
	> UseState;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IParameterableProxy::MethodSetStateSyncType,
		AkStateGroupID,
		AkUInt32
	> SetStateSyncType;

	typedef ObjectProxyCommandData::CommandData3
	<
		IParameterableProxy::MethodSetFX,
		AkUInt32,
		AkUniqueID,
		bool
	> SetFX;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodBypassAllFX, 
		bool
	> BypassAllFX;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IParameterableProxy::MethodBypassFX, 
		AkUInt32,
		bool
	> BypassFX;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodRemoveFX, 
		AkUInt32
	> RemoveFX;

	struct UpdateEffects : public ObjectProxyCommandData::CommandData0<IParameterableProxy::MethodUpdateEffects>
	{
		UpdateEffects();
		UpdateEffects(IObjectProxy * in_pProxy);
		~UpdateEffects();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkEffectUpdate *m_pUpdates;
		AkUInt32 m_uCount;
		bool m_bDeserialized;

	private:

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData0<IParameterableProxy::MethodUpdateEffects> );
	};

	struct SetRTPC : public ObjectProxyCommandData::CommandData, public ProxyCommandData::CurveData<AkRTPCGraphPoint>
	{
		SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

	private:

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData2
	<
		IParameterableProxy::MethodUnsetRTPC,
		AkUInt32,
		AkUniqueID
	> UnsetRTPC;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodMonitoringSolo, 
		bool 
	> MonitoringSolo;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterableProxy::MethodMonitoringMute, 
		bool 
	> MonitoringMute;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IParameterNodeProxy::MethodPosSetPositioningType,
		bool,
		bool,
		AkUInt8,
		AkUInt8
	> PosSetPositioningType;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetPannerEnabled,
		bool
	> PosSetPannerEnabled;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetPositioningEnabled,
		bool
	> SetPositioningEnabled;
}

namespace ParameterNodeProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetSpatializationEnabled,
		bool
	> PosSetSpatializationEnabled;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetAttenuationID,
		AkUniqueID
	> PosSetAttenuationID;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetIsPositionDynamic,
		bool
	> PosSetIsPositionDynamic;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetFollowOrientation,
		bool
	> PosSetFollowOrientation;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetPathMode,
		AkUInt32
	> PosSetPathMode;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetIsLooping,
		bool
	> PosSetIsLooping;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodPosSetTransition,
		AkTimeMs
	> PosSetTransition;

	struct PosSetPath : public ObjectProxyCommandData::CommandData
	{
		PosSetPath();
		~PosSetPath();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPathVertex* m_pArrayVertex;
		AkUInt32 m_ulNumVertices;
		AkPathListItemOffset* m_pArrayPlaylist;
		AkUInt32 m_ulNumPlaylistItem;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData4
	< 
		IParameterNodeProxy::MethodPosUpdatePathPoint,
		AkUInt32,
		AkUInt32,
		AkVector,
		AkTimeMs
	> PosUpdatePathPoint;

	typedef ObjectProxyCommandData::CommandData3
	< 
		IParameterNodeProxy::MethodPosSetPathRange,
		AkUInt32,
		AkReal32,
		AkReal32
	> PosSetPathRange;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodOverrideFXParent,
		bool
	> OverrideFXParent;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetBelowThresholdBehavior,
		AkUInt32
	> SetBelowThresholdBehavior;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetMaxNumInstancesOverrideParent,
		bool
	> SetMaxNumInstancesOverrideParent;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetVVoicesOptOverrideParent,
		bool
	> SetVVoicesOptOverrideParent;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetMaxNumInstances,
		AkUInt16
	> SetMaxNumInstances;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetIsGlobalLimit,
		bool
	> SetIsGlobalLimit;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetMaxReachedBehavior,
		bool
	> SetMaxReachedBehavior;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetOverLimitBehavior,
		bool
	> SetOverLimitBehavior;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetVirtualQueueBehavior,
		AkUInt32
	> SetVirtualQueueBehavior;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IParameterNodeProxy::MethodSetAuxBusSend,
		AkUniqueID,
		AkUInt32
	> SetAuxBusSend;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetOverrideGameAuxSends,
		bool
	> SetOverrideGameAuxSends;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetUseGameAuxSends,
		bool
	> SetUseGameAuxSends;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetOverrideUserAuxSends,
		bool
	> SetOverrideUserAuxSends;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetOverrideHdrEnvelope,
		bool
	> SetOverrideHdrEnvelope;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetOverrideAnalysis,
		bool
	> SetOverrideAnalysis;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetNormalizeLoudness,
		bool
	> SetNormalizeLoudness;
	
	typedef ObjectProxyCommandData::CommandData1
	< 
		IParameterNodeProxy::MethodSetEnableEnvelope,
		bool
	> SetEnableEnvelope;
}

namespace BusProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetMaxNumInstancesOverrideParent,
		bool
	> SetMaxNumInstancesOverrideParent;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetMaxNumInstances,
		AkUInt16
	> SetMaxNumInstances;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetMaxReachedBehavior,
		bool
	> SetMaxReachedBehavior;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetOverLimitBehavior,
		bool
	> SetOverLimitBehavior;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetRecoveryTime,
		AkTimeMs
	> SetRecoveryTime;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetMaxDuckVolume,
		AkReal32
	> SetMaxDuckVolume;

	typedef ObjectProxyCommandData::CommandData6
	< 
		IBusProxy::MethodAddDuck,
		AkUniqueID,
		AkVolumeValue,
		AkTimeMs,
		AkTimeMs,
		AkUInt32,
		AkUInt32
	> AddDuck;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodRemoveDuck,
		AkUniqueID
	> RemoveDuck;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IBusProxy::MethodRemoveAllDuck
	> RemoveAllDuck;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IBusProxy::MethodSetAsBackgroundMusic
	> SetAsBackgroundMusic;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IBusProxy::MethodUnsetAsBackgroundMusic
	> UnsetAsBackgroundMusic;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodEnableWiiCompressor,
		bool
	> EnableWiiCompressor;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodChannelConfig,
		AkUInt32
	> ChannelConfig;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetHdrBus,
		bool
	> SetHdrBus;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetHdrReleaseMode,
		bool
	> SetHdrReleaseMode;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IBusProxy::MethodSetHdrCompressorDirty
	> SetHdrCompressorDirty;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IBusProxy::MethodSetMasterBus, 
		AkUInt32
	> SetMasterBus;
}

namespace SoundProxyCommandData
{
	struct SetSource : public ObjectProxyCommandData::CommandData
	{
		SetSource();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		char* m_pszSourceName;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData1
	< 
		ISoundProxy::MethodSetSource_Plugin,
		AkUniqueID
	> SetSource_Plugin;

	typedef ObjectProxyCommandData::CommandData1
	< 
		ISoundProxy::MethodIsZeroLatency, 
		bool
	> IsZeroLatency;
}

namespace TrackProxyCommandData
{
	struct SetPlayList : public ObjectProxyCommandData::CommandData
	{
		SetPlayList();
		~SetPlayList();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32		m_uNumPlaylistItem;
		AkTrackSrcInfo* m_pArrayPlaylistItems;
		AkUInt32		m_uNumSubTrack;


	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddClipAutomation : public ObjectProxyCommandData::CommandData
	{
		AddClipAutomation();
		~AddClipAutomation();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32				m_uClipIndex;
		AkClipAutomationType	m_eAutomationType;
		AkRTPCGraphPoint *		m_pArrayPoints;
		AkUInt32				m_uNumPoints;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData1
	< 
		ITrackProxy::MethodIsStreaming,
		bool
	> IsStreaming;

	typedef ObjectProxyCommandData::CommandData1
	< 
		ITrackProxy::MethodIsZeroLatency,
		bool
	> IsZeroLatency;

	typedef ObjectProxyCommandData::CommandData1
	< 
		ITrackProxy::MethodLookAheadTime,
		AkTimeMs
	> LookAheadTime;

	typedef ObjectProxyCommandData::CommandData1
	< 
		ITrackProxy::MethodSetMusicTrackRanSeqType,
		AkUInt32
	> SetMusicTrackRanSeqType;
}

namespace EventProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IEventProxy::MethodAdd,
		AkUniqueID
	> Add;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IEventProxy::MethodRemove,
		AkUniqueID
	> Remove;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IEventProxy::MethodClear
	> Clear;
}

namespace MultiSwitchProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData4
	< 
		IMultiSwitchProxy::MethodSetAkPropF,
		AkUInt32, // AkPropID
		AkReal32,
		AkReal32,
		AkReal32
	> SetAkPropF;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IMultiSwitchProxy::MethodSetAkPropI,
		AkUInt32, // AkPropID
		AkInt32,
		AkInt32,
		AkInt32
	> SetAkPropI;

	struct SetDecisionTree : public ObjectProxyCommandData::CommandData
	{
		SetDecisionTree();
		~SetDecisionTree();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		void*       m_pData;
        AkUInt32    m_uSize;
		AkUInt32	m_uDepth;

	private:
		bool m_bWasDeserialized;
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetArguments : public ObjectProxyCommandData::CommandData
	{
		SetArguments();
		~SetArguments();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32*		m_pArgs;
		AkUInt8*		m_pGroupTypes;
		AkUInt32		m_uNumArgs;

	private:
		bool m_bWasDeserialized;
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace ActionProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionProxy::MethodSetElementID,
		WwiseObjectIDext
	> SetElementID;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IActionProxy::MethodSetAkPropF,
		AkUInt32, // AkPropID
		AkReal32,
		AkReal32,
		AkReal32
	> SetAkPropF;

	typedef ObjectProxyCommandData::CommandData4
	< 
		IActionProxy::MethodSetAkPropI,
		AkUInt32, // AkPropID
		AkInt32,
		AkInt32,
		AkInt32
	> SetAkPropI;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionProxy::MethodCurveType,
		AkUInt32 // AkCurveInterpolation
	> CurveType;
}

namespace ActionExceptProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionExceptProxy::MethodAddException,
		WwiseObjectIDext
	> AddException;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionExceptProxy::MethodRemoveException,
		WwiseObjectIDext
	> RemoveException;

	typedef ObjectProxyCommandData::CommandData0
	< 
		IActionExceptProxy::MethodRemoveException
	> ClearExceptions;
}

namespace ActionPauseProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionPauseProxy::MethodIncludePendingResume,
		bool
	> IncludePendingResume;
}

namespace ActionResumeProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionResumeProxy::MethodIsMasterResume,
		bool
	> IsMasterResume;
}

namespace ActionSetAkPropProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData4
	< 
		IActionSetAkPropProxy::MethodSetValue,
		AkReal32,
		AkUInt32, // AkValueMeaning
		AkReal32,
		AkReal32
	> SetValue;
}

namespace ActionSetStateProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionSetStateProxy::MethodSetGroup,
		AkStateGroupID
	> SetGroup;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionSetStateProxy::MethodSetTargetState,
		AkStateID
	> SetTargetState;
}

namespace ActionSetSwitchProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionSetSwitchProxy::MethodSetSwitchGroup,
		AkSwitchGroupID
	> SetSwitchGroup;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionSetSwitchProxy::MethodSetTargetSwitch,
		AkSwitchStateID
	> SetTargetSwitch;
}

namespace ActionSetGameParameterProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData4
	< 
		IActionSetGameParameterProxy::MethodSetValue,
		AkPitchValue,
		AkUInt32, // AkValueMeaning
		AkPitchValue,
		AkPitchValue
	> SetValue;
}

namespace ActionUseStateProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionUseStateProxy::MethodUseState,
		bool
	> UseState;
}

namespace ActionBypassFXProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionBypassFXProxy::MethodBypassFX,
		bool
	> BypassFX;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IActionBypassFXProxy::MethodSetBypassTarget,
		bool,
		AkUInt8
	> SetBypassTarget;
}

namespace ActionSeekProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData3
	< 
		IActionSeekProxy::MethodSetSeekPositionPercent,
		AkReal32,
		AkReal32,
		AkReal32
	> SetSeekPositionPercent;

	typedef ObjectProxyCommandData::CommandData3
	< 
		IActionSeekProxy::MethodSetSeekPositionTimeAbsolute,
		AkTimeMs,
		AkTimeMs,
		AkTimeMs
	> SetSeekPositionTimeAbsolute;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IActionSeekProxy::MethodSetSeekToNearestMarker,
		bool
	> SetSeekToNearestMarker;
}

namespace StateProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData2
	< 
		IStateProxy::MethodSetAkProp,
		AkUInt32, // AkPropID
		AkReal32
	> SetAkProp;
}

namespace AttenuationProxyCommandData
{
	struct SetAttenuationParams : public ObjectProxyCommandData::CommandData
	{
		SetAttenuationParams();
		~SetAttenuationParams();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool LoadRTPCArray( CommandDataSerializer &in_rSerializer, AkUInt32 &out_rNumRTPC, AkWwiseRTPCreg *& out_pRTPCArray );
		bool LoadCurveArray( CommandDataSerializer &in_rSerializer, AkUInt32 &out_uNumCurves, AkWwiseGraphCurve*& out_pCurves );
		
		AkWwiseAttenuation m_Params;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace RanSeqContainerProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodMode,
		AkUInt32 // AkContainerMode
	> Mode;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodIsGlobal,
		bool
	> IsGlobal;

    struct SetPlaylist : public ObjectProxyCommandData::CommandData
	{
		SetPlaylist();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		void*       m_pvListBlock;
        AkUInt32    m_ulParamBlockSize;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodResetPlayListAtEachPlay,
		bool
	> ResetPlayListAtEachPlay;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodRestartBackward,
		bool
	> RestartBackward;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodContinuous,
		bool
	> Continuous;

	typedef ObjectProxyCommandData::CommandData3
	< 
		IRanSeqContainerProxy::MethodForceNextToPlay,
		AkInt16,
		AkWwiseGameObjectID,
		AkPlayingID
	> ForceNextToPlay;

	typedef ObjectProxyCommandData::CommandData3
	< 
		IRanSeqContainerProxy::MethodForceNextToPlay,
		AkInt16,
		AkWwiseGameObjectID,
		AkPlayingID
	> ForceNextToPlay;

	struct NextToPlay : public ObjectProxyCommandData::CommandData
	{
		NextToPlay();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseGameObjectID m_gameObjPtr;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodRandomMode,
		AkUInt32 // AkRandomMode
	> RandomMode;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodAvoidRepeatingCount,
		AkUInt16
	> AvoidRepeatingCount;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IRanSeqContainerProxy::MethodSetItemWeight_withID,
		AkUniqueID,
		AkUInt32
	> SetItemWeight_withID;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IRanSeqContainerProxy::MethodSetItemWeight_withPosition,
		AkUInt16,
		AkUInt32
	> SetItemWeight_withPosition;

	typedef ObjectProxyCommandData::CommandData5
	< 
		IRanSeqContainerProxy::MethodLoop,
		bool,
		bool,
		AkInt16,
		AkInt16,
		AkInt16
	> Loop;

	typedef ObjectProxyCommandData::CommandData1
	< 
		IRanSeqContainerProxy::MethodTransitionMode,
		AkUInt32 // AkTransitionMode
	> TransitionMode;

	typedef ObjectProxyCommandData::CommandData3
	< 
		IRanSeqContainerProxy::MethodTransitionTime,
		AkTimeMs,
		AkTimeMs,
		AkTimeMs 
	> TransitionTime;
}

namespace SwitchContainerProxyCommandData
{
	struct SetSwitchGroup : public ObjectProxyCommandData::CommandData
	{
		SetSwitchGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_ulGroup;
		AkGroupType m_eGroupType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetDefaultSwitch : public ObjectProxyCommandData::CommandData
	{
		SetDefaultSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ClearSwitches : public ObjectProxyCommandData::CommandData
	{
		ClearSwitches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddSwitch : public ObjectProxyCommandData::CommandData
	{
		AddSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchStateID m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveSwitch : public ObjectProxyCommandData::CommandData
	{
		RemoveSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchStateID m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddNodeInSwitch : public ObjectProxyCommandData::CommandData
	{
		AddNodeInSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;
		AkUniqueID m_nodeID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveNodeFromSwitch : public ObjectProxyCommandData::CommandData
	{
		RemoveNodeFromSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;
		AkUniqueID m_nodeID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetContinuousValidation : public ObjectProxyCommandData::CommandData
	{
		SetContinuousValidation();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsContinuousCheck;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetContinuePlayback : public ObjectProxyCommandData::CommandData
	{
		SetContinuePlayback();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		bool m_bContinuePlayback;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFadeInTime : public ObjectProxyCommandData::CommandData
	{
		SetFadeInTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkTimeMs m_time;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFadeOutTime : public ObjectProxyCommandData::CommandData
	{
		SetFadeOutTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkTimeMs m_time;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetOnSwitchMode : public ObjectProxyCommandData::CommandData
	{
		SetOnSwitchMode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkOnSwitchMode m_bSwitchMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetIsFirstOnly : public ObjectProxyCommandData::CommandData
	{
		SetIsFirstOnly();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		bool m_bIsFirstOnly;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace LayerContainerProxyCommandData
{
	//
	// AddLayer
	//

	struct AddLayer : public ObjectProxyCommandData::CommandData
	{
		AddLayer();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_LayerID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// RemoveLayer
	//

	struct RemoveLayer : public ObjectProxyCommandData::CommandData
	{
		RemoveLayer();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_LayerID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace LayerProxyCommandData
{
	//
	// SetRTPC
	//

	struct SetRTPC : public ObjectProxyCommandData::CommandData, public ProxyCommandData::CurveData<AkRTPCGraphPoint>
	{
		SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

	private:

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// UnsetRTPC
	//

	struct UnsetRTPC : public ObjectProxyCommandData::CommandData
	{
		UnsetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// SetChildAssoc
	//

	struct SetChildAssoc : public ObjectProxyCommandData::CommandData
	{
		SetChildAssoc();
		~SetChildAssoc();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_ChildID;
		AkRTPCGraphPoint* m_pCrossfadingCurve;
		AkUInt32 m_ulCrossfadingCurveSize;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// UnsetChildAssoc
	//

	struct UnsetChildAssoc : public ObjectProxyCommandData::CommandData
	{
		UnsetChildAssoc();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_ChildID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// SetCrossfadingRTPC
	//

	struct SetCrossfadingRTPC : public ObjectProxyCommandData::CommandData
	{
		SetCrossfadingRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_rtpcID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace MusicNodeProxyCommandData
{
	struct MeterInfo : public ObjectProxyCommandData::CommandData
	{
		MeterInfo();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsOverrideParent;
		AkMeterInfo m_MeterInfo;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetStingers : public ObjectProxyCommandData::CommandData
	{
		SetStingers();
		~SetStingers();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		CAkStinger*	m_pStingers;
		AkUInt32	m_NumStingers;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace MusicTransAwareProxyCommandData
{
	struct SetRules : public ObjectProxyCommandData::CommandData
	{
		SetRules();
		~SetRules();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_NumRules;
		AkWwiseMusicTransitionRule* m_pRules;
	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace MusicRanSeqProxyCommandData
{
	struct SetPlayList : public ObjectProxyCommandData::CommandData
	{
		SetPlayList();
		~SetPlayList();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_NumItems;
		AkMusicRanSeqPlaylistItem* m_pArrayItems;
	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace MusicSwitchProxyCommandData
{
	struct ContinuePlayback : public ObjectProxyCommandData::CommandData
	{
		ContinuePlayback();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bContinuePlayback;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace MusicSegmentProxyCommandData
{
	struct Duration : public ObjectProxyCommandData::CommandData
	{
		Duration();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal64 m_fDuration;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct StartPos : public ObjectProxyCommandData::CommandData
	{
		StartPos();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal64 m_fStartPos;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMarkers : public ObjectProxyCommandData::CommandData
	{
		SetMarkers();
		~SetMarkers();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkMusicMarkerWwise*     m_pArrayMarkers;
		AkUInt32                m_ulNumMarkers;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
}

namespace FeedbackNodeProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData3
	< 
		IFeedbackNodeProxy::MethodAddPluginSource, 
		AkUniqueID,
		AkUInt16,
		AkUInt16
	> AddPluginSource;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IFeedbackNodeProxy::MethodSetSourceVolumeOffset, 
		AkUniqueID,
		AkReal32
	> SetSourceVolumeOffset;
}

namespace FxBaseProxyCommandData
{
	typedef ObjectProxyCommandData::CommandData1
	< 
		IFxBaseProxy::MethodSetFX, 
		AkPluginID
	> SetFX;

	typedef ObjectProxyCommandData::CommandData2
	< 
		IFxBaseProxy::MethodSetFXParam, 
		AkFXParamBlob, // Placed first due to align-4 constraint on the blob
		AkPluginParamID
	> SetFXParam;

	struct SetRTPC : public ObjectProxyCommandData::CommandData, public ProxyCommandData::CurveData<AkRTPCGraphPoint>
	{
		SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

	private:

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	typedef ObjectProxyCommandData::CommandData2
	< 
		IFxBaseProxy::MethodUnsetRTPC, 
		AkUInt32,
		AkUniqueID
	> UnsetRTPC;
}
#endif // #ifndef AK_OPTIMIZED
