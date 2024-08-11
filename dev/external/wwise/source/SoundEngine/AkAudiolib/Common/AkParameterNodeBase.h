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
// AkParameterNodeBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _BASE_PARAMETER_NODE_H_
#define _BASE_PARAMETER_NODE_H_

#include "AkPBIAware.h"
#include "AkSIS.h"
#include "AkActionExcept.h"
#include "ITransitionable.h"
#include "AkParameters.h"
#include "AkPropBundle.h"
#include "AkCommon.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkBitArray.h"
#include "AkMutedMap.h"
#include "AkConversionTable.h"
#include "AkKeyArray.h"
#include "AkRTPCMgr.h"
#ifdef AK_MOTION
#include "AkFeedbackStructs.h"
#endif // AK_MOTION
#include <math.h>
#include "AudiolibLimitations.h"
#include "ActivityChunk.h"

struct AkSoundParams;
class CAkSIS;
class CAkState;
class AkActivityChunk;

enum AkNodeCategory
{
	AkNodeCategory_Bus				= 0,	// The node is a bus
	AkNodeCategory_ActorMixer		= 1,	// The node is an actro-mixer
	AkNodeCategory_RanSeqCntr		= 2,	// The node is a Ran/Seq container
	AkNodeCategory_Sound			= 3,	// The node is a sound
	AkNodeCategory_SwitchCntr		= 4,	// The node is a Switch container
	AkNodeCategory_LayerCntr		= 5,	// The node is a Layer container


    AkNodeCategory_MusicTrack		= 6,	// The node is a Music Track
    AkNodeCategory_MusicSegment		= 7,	// The node is a Music Segment
    AkNodeCategory_MusicRanSeqCntr	= 8,	// The node is a Music Multi-Sequence container
    AkNodeCategory_MusicSwitchCntr	= 9,	// The node is a Music Switch container
	AkNodeCategory_FeedbackBus		= 10,	// The node is a feedback device bus
	AkNodeCategory_FeedbackNode		= 11,	// The node is a feedback multi-source node
	AkNodeCategory_AuxBus			= 12,	// The node is an Auxilliary bus node

	AkNodeCategory_None				= 1000
};

struct ActionParams
{
	ActionParamType eType;
	CAkRegisteredObj * pGameObj;
	AkPlayingID		playingID;
	TransParams     transParams;
	bool			bIsFromBus;
	bool			bIsMasterCall;
	bool			bIsMasterResume;
	CAkParameterNodeBase* targetNodePtr; // usually not initialised uselessly
};

/// REVIEW: Using action params in "generic" ExecuteAction().
struct SeekActionParams : public ActionParams
{
	union
	{
		AkTimeMs		iSeekTime;
		AkReal32		fSeekPercent;
	};
	AkUInt8			bIsSeekRelativeToDuration	:1;
	AkUInt8			bSnapToNearestMarker		:1;
};

struct ActionParamsExcept
{
	ActionParamType eType;
	ExceptionList*  pExeceptionList;
	CAkRegisteredObj * pGameObj;
	AkPlayingID		playingID;
    TransParams     transParams;
	bool			bIsFromBus;
	bool			bIsMasterResume;
};

/// REVIEW: Using action params in "generic" ExecuteAction().
struct SeekActionParamsExcept : public ActionParamsExcept
{
	union
	{
		AkTimeMs		iSeekTime;
		AkReal32		fSeekPercent;
	};
	AkUInt8			bIsSeekRelativeToDuration	:1;
	AkUInt8			bSnapToNearestMarker		:1;
};

struct NotifParams
{
	AkRTPC_ParameterID	eType;
	CAkRegisteredObj * pGameObj;
	bool			bIsFromBus;
	void*			pExceptObjects;
	AkReal32        fValue;
};

struct AkStateGroupUpdate
{
	AkUniqueID ulGroupID;
	AkUInt32 ulStateCount;
	AkSyncType eSyncType;
};

struct AkStateUpdate
{
	AkUniqueID ulStateInstanceID;//StateInstanceID to be added
	AkStateID ulStateID;//StateID
};

struct AkEffectUpdate
{
	AkUniqueID ulEffectID;
	AkUInt8 uiIndex;
	bool bShared;
};

typedef void( *AkForAllPBIFunc )(
	CAkPBI * in_pPBI,
	CAkRegisteredObj * in_pGameObj,
	void * in_pCookie 
	);

#define AK_ForwardToBusType_Normal (1)
#define AK_ForwardToBusType_Motion (2)
#define AK_ForwardToBusType_ALL (AK_ForwardToBusType_Normal | AK_ForwardToBusType_Motion)

struct CounterParameters
{
	CounterParameters()
		: fPriority( AK_MIN_PRIORITY )
		, pGameObj( NULL )
		, pAMLimiter( NULL )
		, pBusLimiter( NULL )
		, uiFlagForwardToBus( AK_ForwardToBusType_ALL )
		, ui16NumKicked( 0 )
		, bMaxConsidered( false )
		, bAllowKick( true )
	{}

	AkReal32 fPriority;
	CAkRegisteredObj* pGameObj;
	CAkLimiter* pAMLimiter;
	CAkLimiter* pBusLimiter;
	AkUInt16 uiFlagForwardToBus;
	AkUInt16 ui16NumKicked;
	bool bMaxConsidered;
	bool bAllowKick;
};

class CAkParameterNodeBase : public CAkPBIAware
{
protected:
	CAkParameterNodeBase(AkUniqueID in_ulID = 0);
private:
	void FlushStateTransitions();
	
public:
    virtual ~CAkParameterNodeBase();
	virtual void OnPreRelease(){FlushStateTransitions();}//( WG-19178 )

	//////////////////////////////////////////////////////////////////////
	// AkSyncType tools
	typedef AkArray<AkSyncType, const AkSyncType, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof( AkSyncType )> StateSyncArray;

	class CAkStateSyncArray
	{
	public:
		StateSyncArray&		GetStateSyncArray(){ return m_StateSyncArray; }

		void				Term(){ m_StateSyncArray.Term(); }
		void				RemoveAllSync() { m_StateSyncArray.RemoveAll(); }

	private:
		StateSyncArray		m_StateSyncArray;
	};
	//////////////////////////////////////////////////////////////////////

	AKRESULT Init()
	{ 
		//Initializing m_bIsBusCategory here and storing it inside a member.
		//must be kept as a member here as the destructor needs to know its type to call RemoveChild on parent and RemoveFromIndex.
		AkNodeCategory eCategory = NodeCategory();
		m_bIsBusCategory = 
			   eCategory == AkNodeCategory_Bus
			|| eCategory == AkNodeCategory_AuxBus
			|| eCategory == AkNodeCategory_FeedbackBus;

		AddToIndex();
		return AK_Success; 
	}

	// Adds the Node in the General indes
	void AddToIndex();

	// Removes the Node from the General indes
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// Set the parent of the AudioNode 
    //
    // Return - CAkParameterNodeBase* - true if Element is playing, else false
	virtual void Parent(CAkParameterNodeBase* in_pParent);

	CAkParameterNodeBase* Parent() const { return m_pParentNode; }

	virtual void ParentBus(CAkParameterNodeBase* in_pParentBus);

	CAkParameterNodeBase* ParentBus() const { return m_pBusOutputNode; }

	// Notify the Children that the game object was unregistered
	virtual void Unregister(
		CAkRegisteredObj * in_pGameObj
		);

	virtual AKRESULT AddChildInternal( CAkParameterNodeBase* in_pChild );

	AKRESULT AddChildByPtr( CAkParameterNodeBase* in_pChild )
	{
		in_pChild->AddRef(); //AddChildInternal Will hold the ref and release in case of error.
		return AddChildInternal( in_pChild );
	}

	virtual AKRESULT AddChild( WwiseObjectIDext in_ulID )
	{
		if(!in_ulID.id)
		{
			return AK_InvalidID;
		}
		
		CAkParameterNodeBase* pAudioNode = g_pIndex->GetNodePtrAndAddRef( in_ulID.id, AkNodeType_Default );
		if ( !pAudioNode )
			return AK_IDNotFound;

		return AddChildInternal( pAudioNode );
	}

    virtual void RemoveChild(
        CAkParameterNodeBase* in_pChild  // Child to remove
		);

	virtual void RemoveChild( WwiseObjectIDext /*in_ulID*/ ){}

	virtual void GetChildren( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos, AkUInt32& index_out, AkUInt32 iDepth );

	//Return the Node Category
	//
	// Return - AkNodeCategory - Type of the node
	virtual AkNodeCategory NodeCategory() = 0;

	bool IsBusCategory()
	{
		return m_bIsBusCategory;
	}

	// IsPlayable main purpose is to help identify easy to see dead end playback.
	// In case of doubt, return true.
	// Sounds should check if they have a source.
	virtual bool IsPlayable(){ return true; }

	AKRESULT Stop( 
		CAkRegisteredObj * in_pGameObj = NULL, 
		AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID, 
		AkTimeMs in_uTransitionDuration = 0,
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	AKRESULT Pause( 
		CAkRegisteredObj * in_pGameObj = NULL, 
		AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID, 
		AkTimeMs in_uTransitionDuration = 0,
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	AKRESULT Resume( 
		CAkRegisteredObj * in_pGameObj = NULL, 
		AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID, 
		AkTimeMs in_uTransitionDuration = 0,
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction ) = 0;
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction ) = 0;
	virtual AKRESULT ExecuteActionExceptParentCheck( ActionParamsExcept& in_rAction ) = 0;

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj, CAkParameterNodeBase* in_pNodePtr, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID ) = 0;

	virtual void ParamNotification( NotifParams& in_rParams ) = 0;
	virtual void PriorityNotification( NotifParams& in_rParams );

	virtual void Notification(
		AkRTPC_ParameterID in_ParamID, 
		AkReal32 in_fValue,						// Param variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	void PriorityNotification(
		AkReal32 in_Priority,					// Priority variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);


	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkReal32		in_fMuteRatio,	// New muting ratio
		AkMutedMapItem& in_rMutedItem,	// Node identifier (instigator's unique key)
		bool			in_bIsFromBus = false
		)=0;

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkReal32 in_fMuteRatio,			// New muting ratio
		CAkRegisteredObj * in_pGameObj,	// Target Game Object
		AkMutedMapItem& in_rMutedItem,	// Node identifier (instigator's unique key)
		bool in_bPrioritizeGameObjectSpecificItems = false
		)=0;

	// Notify the children PBI that a change int the positioning parameters occured from RTPC
	virtual void PropagatePositioningNotification(
		AkReal32			in_RTPCValue,	// Value
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		)=0;

	void PosSetPannerEnabled( bool in_bIsPannerEnabled );
	void SetPositioningEnabled( bool in_bIsPosEnabled );
	bool IsPositioningEnabled()//required only for busses.
	{
		return m_bPositioningEnabled;
	}

	virtual void ForAllPBI( 
		AkForAllPBIFunc in_funcForAll,
		CAkRegisteredObj * in_pGameObj,
		void * in_pCookie ) = 0;

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_pGameObj = NULL,
		void* in_pExceptArray = NULL
		) = 0;

	virtual void UpdateBusBypass( AkRTPC_ParameterID /*in_ParamID*/ ){}

#ifndef AK_OPTIMIZED
	virtual void InvalidatePaths() = 0;
#endif

	virtual bool Has3DParams() { return false; }
	bool OverridesPositioning() { return m_bPositioningInfoOverrideParent != 0; }
	virtual bool GetMaxRadius( AkReal32 & /*out_fRadius*/ ){ return false; }

	// Get2DParams returns true if using RTPC
	bool Get2DParams( CAkRegisteredObj * in_GameObj, BaseGenParams* in_pBasePosParams );
	bool Get3DPanning( CAkRegisteredObj * in_GameObj, AkVector & out_posPan );

	static bool IsException( CAkParameterNodeBase* in_pNode, ExceptionList& in_rExceptionList );

	// Gets the Next Mixing Bus associated to this node
	//
	// RETURN - CAkBus* - The next mixing bus pointer.
	//						NULL if no mixing bus found
	virtual CAkBus* GetMixingBus();

	virtual CAkBus* GetLimitingBus();

	CAkBus* GetControlBus();

	virtual void UpdateFx(
		AkUInt32	   	in_uFXIndex
		) = 0;

	// Used to increment/decrement the playcount used for notifications and ducking
	virtual AKRESULT IncrementPlayCount(
		CounterParameters& io_params
		) = 0;

	virtual void DecrementPlayCount(
		CounterParameters& io_params
		) = 0;

	virtual void IncrementVirtualCount( 
		CounterParameters& io_params
		) = 0;

	virtual void DecrementVirtualCount( 
		CounterParameters& io_params
		) = 0;

	virtual bool IncrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );
	virtual void DecrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );

	virtual AKRESULT PrepareData() = 0;
	virtual void UnPrepareData() = 0;

	static AKRESULT PrepareNodeData( AkUniqueID in_NodeID );
	static void UnPrepareNodeData( AkUniqueID in_NodeID );

	virtual AKRESULT GetAudioParameters(
		AkSoundParamsEx &out_Parameters,			// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		) = 0;

	// pause (in_bPause = true) or resume (in_bPause = false) the transitions, if any
	virtual void PauseTransitions(bool in_bPause);

	// Set a runtime property value (SIS)
	virtual void SetAkProp(
		AkPropID in_eProp, 
		CAkRegisteredObj * in_pGameObj,		// Game object associated to the action
		AkValueMeaning in_eValueMeaning,	// Target value meaning
		AkReal32 in_fTargetValue = 0,		// Target value
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs in_lTransitionTime = 0
		) = 0;

	// Reset a runtime property value (SIS)
	virtual void ResetAkProp(
		AkPropID in_eProp, 
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs in_lTransitionTime = 0
		) = 0;

	// Mute the element
	virtual void Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;
	
	// Unmute the element for the specified game object
	virtual void Unmute(
		CAkRegisteredObj *	in_pGameObj,					//Game object associated to the action
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Un-Mute the element(per object and main)
	virtual void UnmuteAll(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// starts a transition on a given SIS
	void StartSISTransition(
		CAkSIS * in_pSIS,
		AkPropID in_ePropID,
		AkReal32 in_fTargetValue,
		AkValueMeaning in_eValueMeaning,
		AkCurveInterpolation in_eFadeCurve,
		AkTimeMs in_lTransitionTime );

	// starts a mute transition on a given SIS
	void StartSisMuteTransitions(CAkSIS*		in_pSIS,
								AkReal32		in_fTargetValue,
								AkCurveInterpolation		in_eFadeCurve,
								AkTimeMs		in_lTransitionTime);

	virtual void RecalcNotification();

///////////////////////////////////////////////////////////////////////////
//  STATES
///////////////////////////////////////////////////////////////////////////

	AkStateGroupChunk* AddStateGroup(AkStateGroupID in_ulStateGroupID, bool in_bNotify = true);

	//Removes the associated channel of this node
	//After thet the channel Do not respond to any state channel untill a new channel is set
	void RemoveStateGroup(AkStateGroupID in_ulStateGroupID, bool in_bNotify = true);

	void RemoveStateGroups(bool in_bNotify = true);

	// Return if the States modifications as to be calculated
	bool UseState() const;

	// Sets the Use State flag
	void UseState(
		bool in_bUseState // Is using state
		);

	// Add a state to the sound object
    //
    // Return - AKRESULT - AK_Success if everything succeeded
	AKRESULT AddState(
		AkStateGroupID in_ulStateGroupID,
		AkUniqueID in_ulStateInstanceID,//StateInstanceID to be added
		AkStateID in_ulStateID//StateID
		);

	// Remove the specified State
	void RemoveState(
		AkStateGroupID in_ulStateGroupID,
		AkStateID in_ulStateID //SwitchState
		);

	AKRESULT UpdateStateGroups(AkUInt32 in_uGroups, AkStateGroupUpdate* in_pGroups, AkStateUpdate* in_pUpdates);

	// This function is called on the ParameterNode from the state it is using to signify that the
	// Currently in use state settings were modified and that existing PBIs have to be informed
	virtual void NotifyStateParametersModified();

	void SetMaxReachedBehavior( bool in_bKillNewest );
	void SetOverLimitBehavior( bool in_bUseVirtualBehavior );
	void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance );
	void SetIsGlobalLimit( bool in_bIsGlobalLimit );
	virtual void ApplyMaxNumInstances( AkUInt16 in_u16MaxNumInstance, CAkRegisteredObj* in_pGameObj = NULL, void* in_pExceptArray = NULL, bool in_bFromRTPC = false ) = 0;
	void SetMaxNumInstOverrideParent( bool in_bOverride );
	void SetVVoicesOptOverrideParent( bool in_bOverride );

	PriorityInfo GetPriority( CAkRegisteredObj * in_GameObjPtr );

	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax );
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax );
	virtual AkPropValue * FindCustomProp( AkUInt32 in_uPropID );

	void SetPriorityApplyDistFactor( bool in_bApplyDistFactor );
	void SetPriorityOverrideParent( bool in_bOverrideParent );

	// Set or replace an FX in the Node.
	// The emplacement is important
	AKRESULT SetFX( 
		AkUInt32 in_uFXIndex,				// Position of the FX in the array
		AkUniqueID in_uID,					// Unique id of CAkFxShareSet or CAkFxCustom
		bool in_bShareSet					// Shared?
		);

	AKRESULT RemoveFX( 
		AkUInt32 in_uFXIndex					// Position of the FX in the array
		);

	AKRESULT UpdateEffects(AkUInt32 in_uCount, AkEffectUpdate* in_pUpdates);

	AKRESULT RenderedFX(
		AkUInt32		in_uFXIndex,
		bool			in_bRendered
		);

	AKRESULT MainBypassFX(
		AkUInt32		in_bitsFXBypass,
		AkUInt32        in_uTargetMask = 0xFFFFFFFF
		);

	virtual void ResetFXBypass( 
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		) = 0;

	void BypassFX(
		AkUInt32			in_bitsFXBypass,
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj = NULL,
		bool			in_bIsFromReset = false
		);

	void ResetBypassFX(
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj = NULL
		);

	virtual void GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj = NULL
		) = 0;

	virtual void GetFXDataID(
		AkUInt32	in_uFXIndex,
		AkUInt32	in_uDataIndex,
		AkUInt32&	out_rDataID
		) = 0;

	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj ) = 0;

	virtual AKRESULT SetParamComplexFromRTPCManager( 
		void * in_pToken,
		AkUInt32 in_Param_id, 
		AkRtpcID in_RTPCid,
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj = NULL,
		void* in_pGameObjExceptArray = NULL // Actually a GameObjExceptArray pointer
		);

	// Register a given parameter to an RTPC ID
	void SetRTPC(
		AkRtpcID			in_RTPC_ID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID,
		AkCurveScaling		in_eScaling,
		AkRTPCGraphPoint*	in_pArrayConversion = NULL,		// NULL if none
		AkUInt32			in_ulConversionArraySize = 0	// 0 if none
		);

	virtual void UnsetRTPC(
		AkRTPC_ParameterID in_ParamID,
		AkUniqueID in_RTPCCurveID
		);
	
	void SetStateSyncType( AkStateGroupID in_stateGroupID, AkUInt32/*AkSyncType*/ in_eSyncType );

	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes, bool in_bBusChecked = false );

	// return true means we are done checking
	bool CheckSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

	virtual bool ParamOverriden( AkRTPC_ParameterID /* in_ParamID */ )
	{
		return false;
	}

	virtual bool ParamMustNotify( AkRTPC_ParameterID /* in_ParamID */ )
	{
		return true;
	}

	virtual void PosSetPositioningType( bool, bool, AkPannerType, AkPositionSourceType ){}

	// HDR
	inline bool IsHdrBus() { return m_bIsHdrBus; }
	inline bool IsInHdrHierarchy()
	{
		if ( IsHdrBus() )
			return true;
		else if ( ParentBus() )
			return ParentBus()->IsInHdrHierarchy();
		else if ( Parent() )
			return Parent()->IsInHdrHierarchy();
		else
			return false;
	}

	void SetHdrBus( bool in_bIsHdrBus ) { m_bIsHdrBus = in_bIsHdrBus; }

#ifdef AK_MOTION
	// Feedback support
	// Get the compounded feedback parameters.  There is currenly only the volume.
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetFeedbackParameters( 
		AkFeedbackParams &io_Params,			// Parameters
		CAkSource* in_pSource,
		CAkRegisteredObj * in_GameObjPtr,			// Game object associated to the query
		bool in_bDoBusCheck = true );

	// Set the feedback volume
	virtual void SetFeedbackVolume(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// Volume target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	virtual void FeedbackParentBus(CAkFeedbackBus* in_pParent);
	virtual CAkFeedbackBus* FeedbackParentBus();
	CAkFeedbackBus* GetFeedbackParentBusOrDefault();
	virtual AkVolumeValue GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr );
	virtual AkLPFType GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr );

#endif // AK_MOTION

	// Monitoring solo/mute (profiling only)
#ifndef AK_OPTIMIZED
	virtual void MonitoringSolo( bool in_bSolo );
	virtual void MonitoringMute( bool in_bMute );

	void _MonitoringSolo( bool in_bSolo, AkUInt32& io_ruSoloCount );
	void _MonitoringMute( bool in_bMute, AkUInt32& io_ruMuteCount );

	inline static void ResetMonitoringMuteSolo() 
	{ 
		g_uSoloCount = 0;
		g_uMuteCount = 0;
		g_uSoloCount_bus = 0;
		g_uMuteCount_bus = 0;
		g_bIsMonitoringMuteSoloDirty = false; 
	}
	inline static void SetMonitoringMuteSoloDirty() { g_bIsMonitoringMuteSoloDirty = true; }
	static bool IsRefreshMonitoringMuteSoloNeeded();	// Call this within global lock to know if Renderer should refresh solo/mutes.

	inline static bool IsMonitoringSoloActive() { return ( g_uSoloCount > 0 ); }
	inline static bool IsMonitoringMuteSoloActive() { return ( g_uSoloCount > 0 || g_uMuteCount > 0 ); }

	// Fetch monitoring mute/solo state from hierarchy, starting from leaves.
	virtual void GetMonitoringMuteSoloState( 
		bool in_bCheckBus,	// Pass true. When an overridden bus is found, it is set to false.
		bool & io_bSolo,	// Pass false. Bit is OR'ed against each node of the signal flow.
		bool & io_bMute		// Pass false. Bit is OR'ed against each node of the signal flow.
		);
#endif

	AkForceInline bool HasRTPC(AkRTPC_ParameterID in_ePropID) {return m_RTPCBitArray.IsSet(in_ePropID);}

protected:

	// Entry point for positioning change notifications. A node must not notify unless positioning 
	// is defined at its own level. See WG-22498.
	virtual void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// Value
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		)
	{
		if ( OverridesPositioning() )
			PropagatePositioningNotification( in_RTPCValue, in_ParameterID, in_GameObj, in_pExceptArray );
	}

	virtual AKRESULT SetInitialParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize ) = 0;
	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly ) = 0;
	AKRESULT SetInitialRTPC(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );	

	AKRESULT ReadStateChunk( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	AKRESULT SetNodeBaseParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );
	virtual AKRESULT SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAuxParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	AKRESULT ReadFeedbackInfo(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize);

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj ) = 0;

	virtual CAkRTPCMgr::SubscriberType GetRTPCSubscriberType() const;

	void GetAudioStateParams( AkSoundParams &io_Parameters, AkUInt32 in_uParamSelect );

	// Helper
	AKRESULT GetNewResultCodeForVirtualStatus( AKRESULT in_prevError, AKRESULT in_NewError )
	{
		switch( in_NewError )
		{
		case AK_Success:
			break;

		case AK_MustBeVirtualized:
			if( in_prevError == AK_Success )
				in_prevError = AK_MustBeVirtualized;
			break;

		default:
			// Take the new error code
			in_prevError = in_NewError;
			break;
		}

		return in_prevError;
	}

	AkStateGroupChunk* GetStateGroupChunk( AkStateGroupID in_ulStateGroupID );

	static AkForceInline void ApplySIS( const CAkSIS & in_rSIS, AkPropID in_ePropID, AkReal32 & io_value )
	{
		AkSISValue * pValue = in_rSIS.m_values.FindProp( in_ePropID );
		if ( pValue )
			io_value += pValue->fValue;
	}

//members
protected:
	typedef AkListBareLight<AkStateGroupChunk, AkListBareLightNextInNode> StateList;

	StateList m_states;

	CAkSIS*			m_pGlobalSIS;

	struct FXStruct
	{
		bool bRendered;
		bool bShareSet;
		AkUniqueID id;
	};

	struct FXChunk
	{
		FXChunk();
		~FXChunk();

		FXStruct aFX[ AK_NUM_EFFECTS_PER_OBJ ];
		AkUInt8 bitsMainFXBypass; // original bypass params | 0-3 is effect-specific, 4 is bypass all
	};

	FXChunk * m_pFXChunk;

public:
	bool PriorityOverrideParent(){ return m_bPriorityOverrideParent; }

	AkUInt16	 	GetMaxNumInstances( CAkRegisteredObj * in_GameObjPtr = NULL );
	bool			IsMaxNumInstancesActivated(){ return GetMaxNumInstances() != 0; }

protected:

	// out_fValue will be offset
	AkForceInline void GetPropAndRTPC( AkReal32& out_fValue, AkPropID in_propId, CAkRegisteredObj * in_GameObjPtr, AkReal32 in_fDefaultValue = 0.0f )
	{
		out_fValue += m_props.GetAkProp( in_propId, in_fDefaultValue ).fValue;
		AkRTPC_ParameterID rtpcId = g_AkPropRTPCID[ in_propId ];
		if( m_RTPCBitArray.IsSet( rtpcId ) )
		{
			out_fValue += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), rtpcId, in_GameObjPtr );
		}
	}

	// out_fValue will be overriden exclusively
	AkForceInline void GetPropAndRTPCExclusive( AkReal32& out_fValue, AkPropID in_propId, CAkRegisteredObj * in_GameObjPtr )
	{
		AkRTPC_ParameterID rtpcId = g_AkPropRTPCID[ in_propId ];
		if( m_RTPCBitArray.IsSet( rtpcId ) )
		{
			out_fValue = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), rtpcId, in_GameObjPtr );
		}
		else
		{
			out_fValue = m_props.GetAkProp( in_propId, g_AkPropDefault[ in_propId ].fValue ).fValue;
		}
	}

	/////////////////////////////////////////////////////

	bool EnableActivityChunk( AkUInt16 in_flagForwardToBus )
	{
		if( !IsActivityChunkEnabled() )
		{
			m_pActivityChunk = AkNew( g_DefaultPoolId, AkActivityChunk( GetMaxNumInstances(), m_bIsGlobalLimit, DoesKillNewest(), m_bUseVirtualBehavior ) );
			if( m_pActivityChunk )
				return OnNewActivityChunk( in_flagForwardToBus );
			else
				return false;
		}
		return true;
	}

	virtual bool OnNewActivityChunk( AkUInt16 in_flagForwardToBus );
	bool SetFastActive( CAkParameterNodeBase* in_pChild, AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );
	void UnsetFastActive( CAkParameterNodeBase* in_pChild );

	bool IsActivityChunkEnabled() const
	{
		return m_pActivityChunk != NULL;
	}

	void DeleteActivityChunk();

	AkUInt16 GetPlayCount() const
	{ 
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->GetPlayCount();
		else
			return 0; 
	}

	AkUInt16 GetActivityCount()
	{ 
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->GetActivityCount();
		else
			return 0; 
	}

	AkUInt16 GetPlayCountValid()
	{ 
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->GetPlayCountValid();
		else
			return 0; 
	}

	AkUInt16 GetVirtualCountValid()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->GetVirtualCountValid();
		else
			return 0; 
	}

	AKRESULT IncrementPlayCountValue( AkUInt16 in_flagForwardToBus );
	void DecrementPlayCountValue();
	bool IncrementActivityCountValue( AkUInt16 in_flagForwardToBus );
	void DecrementActivityCountValue();

	void IncrementPlayCountValid()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->IncrementPlayCountValid();
	}

	void DecrementPlayCountValid()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->DecrementPlayCountValid();
	}

	void IncrementVirtualCountValid()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->IncrementVirtualCountValid();
	}
	void DecrementVirtualCountValid()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->DecrementVirtualCountValid();
	}

	void UpdateMaxNumInstanceGlobal( AkUInt16 in_u16LastMaxNumInstanceForRTPC )
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->UpdateMaxNumInstanceGlobal(in_u16LastMaxNumInstanceForRTPC );
	}

	void AddPBI( CAkPBI* in_pPBI )
	{
		AKASSERT( IsActivityChunkEnabled() );

		m_pActivityChunk->m_listPBI.AddFirst( in_pPBI );
	}

	void RemovePBI( CAkPBI* in_pPBI )
	{
		AKASSERT( IsActivityChunkEnabled() );

		m_pActivityChunk->m_listPBI.Remove( in_pPBI );
		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}

public:
	bool IsGlobalLimit()
	{
		if( IsActivityChunkEnabled() )
			return m_pActivityChunk->IsGlobalLimit();
		return true;
	}

protected:

	AKRESULT IncrementPlayCountGlobal( AkReal32 in_fPriority, AkUInt16& io_ui16NumKickedOrRevived, CAkLimiter*& io_pLimiter );
	void DecrementPlayCountGlobal();
	void IncrementVirtualCountGlobal(){ IncrementVirtualCountValid(); }
	void DecrementVirtualCountGlobal( AkUInt16& io_ui16NumKickedOrRevived, bool in_bAllowKick );

	AKRESULT IncrementPlayCountGameObject( AkReal32 in_fPriority, AkUInt16& io_ui16NumKickedOrRevived, CAkRegisteredObj* in_pGameObj, CAkLimiter*& io_pLimiter );
	void DecrementPlayCountGameObject( CAkRegisteredObj* in_pGameObj );
	void IncrementVirtualCountGameObject( CAkRegisteredObj* in_pGameObj );
	void DecrementVirtualCountGameObject( AkUInt16& io_ui16NumKickedOrRevived, bool in_bAllowKick, CAkRegisteredObj* in_pGameObj );

public:
	bool IsPlaying() const { return ( GetPlayCount() != 0 ); } // This function was NOT created as an helper to get information about if something is playing, 
														// but instead have been created to avoid forwarding uselessly notifications trough the tree.

	bool IsActiveOrPlaying(){ return IsPlaying() || IsActive(); }
	bool IsActive(){ return GetActivityCount() != 0; }

	bool DoesKillNewest(){ return m_bKillNewest; }

	/////////////////////////////////////////////////////

	AkActivityChunk*	m_pActivityChunk;
protected:
	CAkParameterNodeBase*	m_pParentNode;		// Hirc Parent (optional), bus always have NULL
	CAkParameterNodeBase*	m_pBusOutputNode;	// Bus Parent (optional)

	AkPropBundle<AkPropValue> m_props;
	
	AkUInt16		m_u16MaxNumInstance;				// Zero being no max.

	AkUInt8			m_bKillNewest					:1;
	AkUInt8			m_bUseVirtualBehavior			:1;
	AkUInt8			m_bIsVVoicesOptOverrideParent	:1;
	AkUInt8			m_bIsMaxNumInstOverrideParent	:1;
	AkUInt8			m_bIsGlobalLimit				:1;

	AkUInt8			m_bPriorityApplyDistFactor	: 1;
	AkUInt8			m_bPriorityOverrideParent	: 1;
	AkUInt8			m_bUseState					: 1;	// Enable and disable the use of the state
	AkUInt8			m_bIsInDestructor			: 1;
	AkUInt8			m_bIsBusCategory			: 1;

	////////////////////////////////////////////////////////////////////////
	// Settings from CAkBus
	// Putting it in the base class allows saving significant memory
	// while having no real drawback.
	AkUInt8			m_bPositioningEnabled		: 1;
	AkUInt8			m_bIsHdrBus					: 1;
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	// Settings from CAkParameterNode
	// Putting it in the base class allows saving significant memory
	// while having no real drawback.
	AkUInt8					m_bPositioningInfoOverrideParent	:1;
	AkUInt8					m_bPositioningEnablePanner			:1;
	AkUInt8					m_bIsFXOverrideParent				:1;
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	// Settings from CAkSwitchCntr
	// Putting it in the base class allows saving significant memory
	// while having no real drawback.
	AkUInt8					m_bIsContinuousValidation			:1;	// Is the validation continuous
	////////////////////////////////////////////////////////////////////////

	// Monitoring mute/solo.
#ifndef AK_OPTIMIZED
	AkUInt8			m_bIsSoloed	:1;		// UI explicitly set this node to SOLO.
	AkUInt8			m_bIsMuted	:1;		// UI explicitly set this node to MUTE.
	static AkUInt32	g_uSoloCount;		// Total number of nodes set to SOLO.
	static AkUInt32	g_uMuteCount;		// Total number of nodes set to MUTE.
	static AkUInt32	g_uSoloCount_bus;		// Total number of nodes set to SOLO.
	static AkUInt32	g_uMuteCount_bus;		// Total number of nodes set to MUTE.
	static bool		g_bIsMonitoringMuteSoloDirty;	// Set to true when property changes, to force an update by the renderer.
#endif

	CAkBitArray<AkUInt64>	m_RTPCBitArray;

#ifdef AK_MOTION
	// Feedback information.  This is an optional structure which should be present
	// only if there is information to be kept.  This means either: 
	// a) the object is connected to a feedback bus
	// b) the user set a feedback volume (even if not connected directly, children could be affected)
	struct AkFeedbackInfo
	{
		AkFeedbackInfo() : m_pFeedbackBus( NULL ) {}
		CAkFeedbackBus*	m_pFeedbackBus;		// Output bus
	};

	AkFeedbackInfo* m_pFeedbackInfo;
#endif // AK_MOTION
};

#endif // _PARAMETER_NODE_BASE_H_
