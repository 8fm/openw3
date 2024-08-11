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
// AkBus.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _BUS_H_
#define _BUS_H_

#include "AkActiveParent.h"
#include "AkDuckItem.h"
#include "AkKeyList.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkBusCtx.h"

extern CAkBusCtx g_MasterBusCtx;
extern CAkBusCtx g_SecondaryMasterBusCtx;

// class corresponding to a bus
//
// Author:  alessard
class CAkBus : public CAkActiveParent<CAkParameterNodeBase>
{
public:

	enum AkDuckingState
	{
		DuckState_OFF,
		DuckState_ON,
		DuckState_PENDING//Waiting for the notification before unducking
#define DUCKING_STATE_NUM_STORAGE_BIT 3
	};

	struct AkDuckInfo
	{
		AkVolumeValue DuckVolume;
		AkTimeMs FadeOutTime;
		AkTimeMs FadeInTime;
		AkCurveInterpolation FadeCurve;
		AkPropID TargetProp;
		bool operator ==(AkDuckInfo& in_Op)
		{
			return ( (DuckVolume == in_Op.DuckVolume) 
				&& (FadeOutTime == in_Op.FadeOutTime) 
				&& (FadeInTime == in_Op.FadeInTime) 
				&& (FadeCurve == in_Op.FadeCurve) 
				&& (TargetProp == in_Op.TargetProp)
				);
		}
	};

	//Thread safe version of the constructor
	static CAkBus* Create(AkUniqueID in_ulID = 0);

	// Check if the specified child can be connected
    //
    // Return - bool -	AK_NotCompatible
	//					AK_Succcess
	//					AK_MaxReached
    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode  // Audio node ID to connect on
        );

	virtual void ParentBus(CAkParameterNodeBase* in_pParent);
	using CAkActiveParent<CAkParameterNodeBase>::ParentBus;
	virtual void Parent(CAkParameterNodeBase* in_pParent);
	using CAkActiveParent<CAkParameterNodeBase>::Parent;

	virtual AKRESULT AddChildInternal( CAkParameterNodeBase* in_pChild );

	virtual AKRESULT AddChild( WwiseObjectIDext in_ulID );

	virtual void RemoveChild(
        CAkParameterNodeBase* in_pChild
		);

	virtual void RemoveChild( WwiseObjectIDext in_ulID );

	virtual AkNodeCategory NodeCategory();	

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );

	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	virtual AKRESULT ExecuteActionExceptParentCheck( ActionParamsExcept& in_rAction ){ return ExecuteActionExcept( in_rAction ); }

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj , CAkParameterNodeBase* in_NodePtr, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ );

	virtual void PriorityNotification( NotifParams& in_rParams );

	virtual void ForAllPBI( 
		AkForAllPBIFunc in_funcForAll,
		CAkRegisteredObj * in_pGameObj,
		void * in_pCookie );

	virtual void PropagatePositioningNotification(
		AkReal32			in_RTPCValue,	// 
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a Positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		);

	virtual AKRESULT GetAudioParameters(
		AkSoundParamsEx &out_Parameters,	// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		);

	AkVolumeValue GetBusEffectiveVolume( BusVolumeType in_VolumeType, AkPropID in_eProp );

	// Set a runtime property value (SIS)
	virtual void SetAkProp(
		AkPropID in_eProp, 
		CAkRegisteredObj * in_pGameObj,		// Game object associated to the action
		AkValueMeaning in_eValueMeaning,	// Target value meaning
		AkReal32 in_fTargetValue = 0,		// Target value
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs in_lTransitionTime = 0
		);

	using CAkActiveParent<CAkParameterNodeBase>::SetAkProp;

	// Reset a runtime property value (SIS)
	virtual void ResetAkProp(
		AkPropID in_eProp, 
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs in_lTransitionTime = 0
		);

	// Mute the element
	virtual void Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);
	
	// Unmute the element for the specified game object
	virtual void Unmute(
		CAkRegisteredObj *	in_pGameObj,					//Game object associated to the action
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Un-Mute the element(per object and main)
	virtual void UnmuteAll(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	virtual void SetAkProp( 
		AkPropID in_eProp, 
		AkReal32 in_fValue, 
		AkReal32 in_fMin, 
		AkReal32 in_fMax 
		);

	virtual void ParamNotification( NotifParams& in_rParams );

	virtual void MuteNotification(
		AkReal32 in_fMuteLevel,
		AkMutedMapItem& in_rMutedItem,
		bool			in_bIsFromBus = false
		);

	using CAkActiveParent<CAkParameterNodeBase>::MuteNotification; //Removes a warning on 3DS, WiiU and Vita compilers.

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_GameObj,
		void*	in_pExceptArray = NULL
		);

	void SetRecoveryTime( AkUInt32 in_RecoveryTime );

	void SetMaxDuckVolume( AkReal32 in_fMaxDuckVolume );

	AKRESULT AddDuck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_TargetProp
		);

	AKRESULT RemoveDuck(
		AkUniqueID in_BusID
		);

	AKRESULT RemoveAllDuck();

	void Duck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_PropID
		);

	void Unduck(
		AkUniqueID in_BusID,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_PropID
		);

	void PauseDuck(
		AkUniqueID in_BusID
		);

	virtual void ApplyMaxNumInstances( 
		AkUInt16 in_u16MaxNumInstance, 
		CAkRegisteredObj* in_pGameObj = NULL,
		void* in_pExceptArray = NULL,
		bool in_bFromRTPC = false
		);

	virtual AKRESULT IncrementPlayCount( 
		CounterParameters& io_params 
		);

	virtual void DecrementPlayCount( 
		CounterParameters& io_params
		);

	virtual void IncrementVirtualCount( 
		CounterParameters& io_params
		);

	virtual void DecrementVirtualCount( 
		CounterParameters& io_params
		);
		
	virtual bool IncrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );
	virtual void DecrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );

	bool IsOrIsChildOf( CAkParameterNodeBase * in_pNodeToTest );

	void DuckNotif();

	bool HasEffect();
	bool IsMixingBus();

	bool IsTopBus();

	static CAkBus *GetPrimaryMasterBusAndAddRef();
	static CAkBus *GetSecondaryMasterBusAndAddRef();
	
	static void ExecuteMasterBusAction( ActionParams& in_params )
	{
		CAkBus * pPriMast = GetPrimaryMasterBusAndAddRef();
		if (pPriMast)
		{
			pPriMast->ExecuteAction( in_params );
			pPriMast->Release();
		}

		CAkBus * pSecMast = GetSecondaryMasterBusAndAddRef();
		if (pSecMast)
		{
			pSecMast->ExecuteAction( in_params );
			pSecMast->Release();
		}
	}

	static void ExecuteMasterBusActionExcept( ActionParamsExcept& in_params )
	{
		CAkBus * pPriMast = GetPrimaryMasterBusAndAddRef();
		if (pPriMast)
		{
			pPriMast->ExecuteActionExcept( in_params );
			pPriMast->Release();
		}

		CAkBus * pSecMast = GetSecondaryMasterBusAndAddRef();
		if (pSecMast)
		{
			pSecMast->ExecuteActionExcept( in_params );
			pSecMast->Release();
		}
	}

	
	static void ClearMasterBus();

	void CheckDuck();

	AkVolumeValue GetDuckedVolume( AkPropID in_eProp );

	void ChannelConfig( AkUInt32 in_uConfig );
	inline AkUInt32 ChannelConfig() { return m_uChannelConfig; }

	// Gets the Next Mixing Bus associated to this node
	//
	// RETURN - CAkBus* - The next mixing bus pointer.
	//						NULL if no mixing bus found
	virtual CAkBus* GetMixingBus();

	virtual CAkBus* GetLimitingBus();

	virtual void UpdateFx(
		AkUInt32 in_uFXIndex
		);

	virtual void GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj = NULL
		);

	virtual void GetFXDataID(
		AkUInt32	in_uFXIndex,
		AkUInt32	in_uDataIndex,
		AkUInt32&	out_rDataID
		);

	bool GetBypassFX( AkUInt32 in_uFXIndex );
	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj = NULL );
	virtual void UpdateBusBypass( AkRTPC_ParameterID in_ParamID );


	virtual void ResetFXBypass( 
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

	virtual AKRESULT SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize);
	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );

	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

	static void EnableHardwareCompressor( bool in_Enable );

	virtual AkUInt16 Children()
	{
		return static_cast<AkUInt16>( m_mapChildId.Length() + m_mapBusChildId.Length() );
	}

	void UpdateVoiceVolumes();

	AkForceInline AkReal32 GetControlBusVolume()
	{
		if( !m_bControBusVolumeUpdated )
		{
			UpdateVoiceVolumes();
		}
		return m_fEffectiveBusVolume;
	}

	AkForceInline AkReal32 GetVoiceVolume()
	{
		if( !m_bControBusVolumeUpdated )
		{
			UpdateVoiceVolumes();
		}
		return m_fEffectiveVoiceVolume;
	}

	virtual void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// Value
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		)
	{
		if ( m_bPositioningEnabled )
			PropagatePositioningNotification( in_RTPCValue, in_ParameterID, in_GameObj, in_pExceptArray );
	}

	// HDR
	//
	inline void SetHdrCompressorDirty()
	{ 
		// Force recomputation of coefficients and clear memory, and re-cache in lower engine.
		m_bHdrGainComputerDirty = true;
		m_bHdrReleaseTimeDirty = true;	
	}

	// Set up the global variables for accessing the top of the bus hierarchy. 
	inline void SetMasterBus(AkUInt32 in_uMasterBusType)
	{ 
		if ( in_uMasterBusType == 0 /*MasterBusType::MainMaster*/ )
		{
			g_MasterBusCtx.SetBus(this);
			if (!IsInMainHierarchy())
				_SetInMainHierarchy(true);
		}
		else if (in_uMasterBusType == 1 /*MasterBusType::SecondaryMaster*/ )
		{
			g_SecondaryMasterBusCtx.SetBus(this);
			if (IsInMainHierarchy())
				_SetInMainHierarchy(false);
		}
	}

	inline void SetHdrReleaseMode( bool in_bHdrReleaseModeExponential ) 
	{ 
		m_bHdrReleaseModeExponential = in_bHdrReleaseModeExponential;
		m_bHdrReleaseTimeDirty = true;	// Force recomputation of coefficients and clear memory.
	}

	// Returns true if values were dirty.
	inline bool GetHdrGainComputer(
		AkReal32 & out_fHdrThreshold,
		AkReal32 & out_fRatio
		)
	{
		// Can change because of live edit. Will be honored on next play. AKASSERT( IsHdrBus() );
		GetPropAndRTPCExclusive( out_fHdrThreshold, AkPropID_HDRBusThreshold, NULL );
		GetPropAndRTPCExclusive( out_fRatio, AkPropID_HDRBusRatio, NULL );
		bool bGainComputerDirty = m_bHdrGainComputerDirty;
		m_bHdrGainComputerDirty = false;
		return bGainComputerDirty;
	}

	// Returns true if values were dirty.
	inline bool GetHdrBallistics(
		AkReal32 & out_fReleaseTime,
		bool & out_bReleaseModeExponential		
		)
	{
		GetPropAndRTPCExclusive( out_fReleaseTime, AkPropID_HDRBusReleaseTime, NULL );
		out_bReleaseModeExponential = m_bHdrReleaseModeExponential;
		bool bReleaseTimeDirty = m_bHdrReleaseTimeDirty;
		m_bHdrReleaseTimeDirty = false;
		return bReleaseTimeDirty;
	}

	// Lower engine calls this when HDR window top has been calculated.
	void NotifyHdrWindowTop( AkReal32 in_fWindowTop );

#ifndef AK_OPTIMIZED
	virtual void MonitoringSolo( bool in_bSolo );
	virtual void MonitoringMute( bool in_bMute );

	inline static bool IsMonitoringSoloActive_bus() { return ( g_uSoloCount_bus > 0 ); }
	inline static bool IsMonitoringMuteSoloActive_bus() { return ( g_uSoloCount_bus > 0 || g_uMuteCount_bus > 0 ); }

	// Fetch monitoring mute/solo state from hierarchy, starting from leaves.
	virtual void GetMonitoringMuteSoloState(
		bool in_bCheckBus,	// Pass true. When an overridden bus is found, it is set to false. 
		bool & io_bSolo,	// Pass false. Bit is OR'ed against each node of the signal flow.
		bool & io_bMute		// Pass false. Bit is OR'ed against each node of the signal flow.
		);

	void RefreshMonitoringMute();
	inline bool IsMonitoringMute() { return m_bIsMonitoringMute; }
#endif

	bool IsInMainHierarchy() const { return m_bMainOutputHierarchy ;}
	void _SetInMainHierarchy( bool in_bIsInMainHierarchy );

protected:

	class ChildrenIterator
	{
	public:
		ChildrenIterator( AkMapChildID& in_First, AkMapChildID& in_Second )
			:m_pCurrentlist( &in_First )
			,m_pSecondlist( &in_Second )
		{
			m_iter = in_First.Begin();
			SwitchIfEnd();
		}

		inline ChildrenIterator& operator++()
		{
			++m_iter;
			SwitchIfEnd();
			return *this;
		}

		inline bool End()
		{
			 return m_iter == m_pCurrentlist->End();
		}

		/// Operator *.
		inline CAkParameterNodeBase* operator*()
		{
			return (*m_iter);
		}

	private:
		inline void SwitchIfEnd()
		{
			// Switch to next list.
			if( m_iter == m_pCurrentlist->End() )
			{
				if( m_pCurrentlist != m_pSecondlist )
				{
					m_pCurrentlist = m_pSecondlist;
					m_iter = m_pCurrentlist->Begin();
				}
			}
		}

		AkMapChildID::Iterator m_iter;
		AkMapChildID* m_pCurrentlist;
		AkMapChildID* m_pSecondlist;
	};

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj = NULL );
	virtual void RecalcNotification();

	virtual CAkRTPCMgr::SubscriberType GetRTPCSubscriberType() const;

	// Constructors
    CAkBus(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkBus();

	AKRESULT Init();

	void StartDucking();
	void StopDucking();

	virtual AKRESULT SetInitialParams(AkUInt8*& pData, AkUInt32& ulDataSize );

	void StartDuckTransitions(CAkDuckItem*		in_pDuckItem,
								AkReal32		in_fTargetValue,
								AkValueMeaning	in_eValueMeaning,
								AkCurveInterpolation	in_eFadeCurve,
								AkTimeMs		in_lTransitionTime,
								AkPropID		in_ePropID);

	AKRESULT RequestDuckNotif();

	void UpdateDuckedBus();

	AkMapChildID m_mapBusChildId; // List of nodes connected to this one

	AkUInt32 m_RecoveryTime; // Recovery time in output samples
	AkUInt32 m_uChannelConfig;

	AkReal32 m_fMaxDuckVolume;

	typedef CAkKeyList<AkUniqueID, AkDuckInfo, AkAllocAndFree> AkToDuckList;
	AkToDuckList m_ToDuckList;

	typedef CAkKeyList<AkUniqueID, CAkDuckItem, AkAllocAndFree> AkDuckedVolumeList;
	AkDuckedVolumeList m_DuckedVolumeList;
	AkDuckedVolumeList m_DuckedBusVolumeList;

	AkReal32 m_fEffectiveBusVolume;//in dB
	AkReal32 m_fEffectiveVoiceVolume;//in dB
	AkUInt8 m_bControBusVolumeUpdated :1;
#ifndef AK_OPTIMIZED
	AkUInt8	m_bIsMonitoringMute			:1;	// Should not be heard when muted
#endif

	virtual bool OnNewActivityChunk( AkUInt16 in_flagForwardToBus )
	{
		bool bRet = CAkParameterNodeBase::OnNewActivityChunk( in_flagForwardToBus );

#ifndef AK_OPTIMIZED
		RefreshMonitoringMute();
#endif
		// the bus becomes active, must initialize its Volume status from now on, make sure it is updated.
		ResetControlBusVolume();

		return bRet;
	}

	void OnMixingBusStatusChanged()
	{
		if( m_bControBusVolumeUpdated )
			ResetControlBusVolume();
	}

	void ResetControlBusVolume()
	{
		m_bControBusVolumeUpdated = false;
		GetControlBusVolume(); // will set m_bControBusVolumeUpdated upon completion.
	}

	AkUInt8/*AkDuckingState*/	m_eDuckingState :DUCKING_STATE_NUM_STORAGE_BIT;
	AkUInt8						m_bHdrReleaseModeExponential	:1;
	AkUInt8						m_bHdrReleaseTimeDirty			:1;
	AkUInt8						m_bHdrGainComputerDirty			:1;
	AkUInt8						m_bMainOutputHierarchy			:1;

#if defined( AK_XBOX360 ) || defined( AK_PS3 )

public:
	static void MuteBackgroundMusic();
	static void UnmuteBackgroundMusic();

// For Wwise purpose, those two functions are exposed, 
// but defined only if connected to AK_XBOX360 or PS3

	void SetAsBackgroundMusicBus();
	void UnsetAsBackgroundMusicBus();

private:
	void BackgroundMusic_Mute();
	void BackgroundMusic_Unmute();

// AK_XBOX360 exclusive members

	AkUInt8 m_bIsBackgroundMusicBus		:1;
	AkUInt8 m_bIsBackgroundMusicMuted	:1;

	static CAkBus* m_pBackgroundMusicBus;
	static CAkLock m_BackgroundMusicLock;
#endif // AK_XBOX360 || AK_PS3
};

inline CAkBus* GetPrimaryMasterBus(){ return g_MasterBusCtx.GetBus(); }
inline CAkBus* GetSecondaryMasterBus(){ return g_SecondaryMasterBusCtx.GetBus(); }

#ifndef AK_OPTIMIZED
AkForceInline bool CheckBusMonitoringMute( CAkBus* in_pBus )
{
	return in_pBus && in_pBus->IsMonitoringMute();
}
#endif


#endif //_BUS_H_
