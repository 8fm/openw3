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
// AkPBI.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PBI_H_
#define _PBI_H_

#include "AkParameters.h"
#include "AkMutedMap.h"
#include "PrivateStructures.h"
#include "AkContinuationList.h"
#include "AkMonitorData.h"
#include "AkActionPlay.h"
#include "AkRTPC.h"
#include "AkCntrHistory.h"
#include "AkRegisteredObj.h"
#include "AkBusCtx.h"
#include "AkRegistryMgr.h"
#include "AkSource.h"
#include "AkTransportAware.h"
#include "AkGen3DParams.h"
#include "AkMonitor.h"
#include "ITransitionable.h"

#ifdef AK_MOTION
#include "AkFeedbackStructs.h"
#endif // AK_MOTION

class CAkURenderer;
class CAkSoundBase;
class CAkSource;
class CAkRegisteredObj;
class CAkSrcBase;
class CAkTransition;
class CAkGen3DParams;
class CAkPath;
class CAkAttenuation;
class CAkPBIAware;
class CAkLimiter;
class CAkVPLSrcNode;

struct NotifParams;

class CAkUsageSlot;

extern AkReal32 g_fVolumeThreshold;
extern AkReal32 g_fVolumeThresholdDB;

#ifndef AKPBI_CBXCLASS
	#if defined(AK_WII)||defined(AK_3DS)
		#define AKPBI_CBXCLASS CAkVPLSrcCbxNode
	#else
		#define AKPBI_CBXCLASS CAkVPLSrcCbxNodeBase
	#endif

	class AKPBI_CBXCLASS;
#endif

#define AK_INVALID_SEQUENCE_ID 0

#define CODECID_FROM_PLUGINID(x) ((x>>(4+12))&0xffff)

enum AkCtxState
{
	CtxStateStop		= 0,
	CtxStatePause		= 1,
	CtxStateResume		= 2,
	CtxStatePlay		= 3,
	CtxStateToDestroy	= 4
#define CTXSTATE_NUM_STORAGE_BIT 4
};

enum AkCtxDestroyReason
{
	CtxDestroyReasonFinished = 0,
	CtxDestroyReasonPlayFailed
};

enum AkCtxVirtualHandlingResult
{
	VirtualHandling_NotHandled,
	VirtualHandling_RequiresSeeking,
	VirtualHandling_ShouldStop
};

enum KickFrom
{
	KickFrom_Stopped,
	KickFrom_OverNodeLimit,
	KickFrom_OverGlobalLimit,
	KickFrom_OverMemoryLimit
};

struct AkPriorityStruct
{
	AkReal32 priority;
	AkUniqueID PBIID;
	
	AkForceInline bool operator==(const AkPriorityStruct& other)		
	{
		return priority == other.priority 
			&& PBIID == other.PBIID;
	}

	AkForceInline bool operator!=(const AkPriorityStruct& other)		
	{
		return priority != other.priority 
			|| PBIID != other.PBIID;
	}
};

struct AkPriorityStruct_INC : public AkPriorityStruct
{
	AkForceInline bool operator<(const AkPriorityStruct& other)		
	{
		if( priority == other.priority )
		{
			return PBIID < other.PBIID;//Norm: Olders by default
		}
		return priority > other.priority;
	}

	AkForceInline bool operator>(const AkPriorityStruct& other)		
	{
		if( priority == other.priority )
		{
			return PBIID > other.PBIID;//Norm: Olders by default
		}
		return priority < other.priority;
	}
};

struct AkPriorityStruct_DEC : public AkPriorityStruct
{
	AkForceInline bool operator<( const AkPriorityStruct& other)
	{
		if( priority == other.priority )
		{
			return PBIID > other.PBIID;//Norm: Olders by default
		}
		return priority > other.priority;
	}

	AkForceInline bool operator>( const AkPriorityStruct& other)
	{
		if( priority == other.priority )
		{
			return PBIID < other.PBIID;//Norm: Olders by default
		}
		return priority < other.priority;
	}
};

struct PriorityInfo
{
	PriorityInfo()
		:priority(AK_DEFAULT_PRIORITY)
		,distanceOffset(0.0f)
	{}

	AkReal32 priority;			// Priority 0-100.
	AkReal32 distanceOffset;	// Offset to priority at max radius if m_bPriorityApplyDistFactor.
};

struct PriorityInfoCurrent
{
	PriorityInfoCurrent()
	{
		currentPriority.priority = AK_DEFAULT_PRIORITY;
	}

	PriorityInfoCurrent( PriorityInfo in_PriorityInfo )
		:priorityInfo ( in_PriorityInfo )
	{
		currentPriority.priority = in_PriorityInfo.priority ;
	}

	AkForceInline AkReal32 GetCurrent() const { return currentPriority.priority; }
	AkReal32 BasePriority() const { return priorityInfo.priority; }
	AkReal32 DistanceOffset()const { return priorityInfo.distanceOffset; }
	void ResetBase( AkReal32 in_newBase )
	{
		priorityInfo.priority = in_newBase;
	}
	void Reset( const PriorityInfo& in_priorityInfo )
	{
		priorityInfo = in_priorityInfo;
	}

	void SetCurrent( AkReal32 in_priority )
	{
		currentPriority.priority = in_priority; 
	}

	void SetPBIID( AkUniqueID in_PBIID )
	{
		currentPriority.PBIID = in_PBIID;
	}

	AkForceInline AkUniqueID GetPBIID()
	{
		return currentPriority.PBIID;
	}

	AkForceInline AkPriorityStruct& GetPriorityKey()
	{
		return currentPriority;
	}

private:
	AkPriorityStruct currentPriority;
	PriorityInfo priorityInfo;
};

struct Prev2DParams
{
	BaseGenParams	prev2DParams;
	AkReal32		prevVolume;		// Collapsed volume from actor hierarchy, "voice volume" on bus hierarchy and all fades.
	AkReal32		prevDryLevel;
#ifdef AK_MOTION
	AkReal32		prevMotionVol;
#endif

	Prev2DParams()
	{
		// Make sure the initial value is not a valid one, forcing the volume to compute 
		// the volume the first time without having to add an additionnal flag and an  another if in normal loop.
		// m_fPAN_X_2D range from -100 to 100. No need to init all values.
		Invalidate();
	}

	void Invalidate()
	{
		prev2DParams.Invalidate();
	}
};

struct AkPBIParams
{
	enum ePBIType
	{
		PBI,
		ContinuousPBI,
		DynamicSequencePBI
	};

	AkPBIParams()
		: bSkipDelay(false)
	{
		//Default values.
#ifdef AK_MOTION
		bTargetFeedback = false;	//Not a feedback PBI
#endif // AK_MOTION
		playHistory.Init();
	}
	
	AkPBIParams( PlayHistory& in_rPlayHist )
		: bSkipDelay(false)
	{
		//Default values.
#ifdef AK_MOTION
		bTargetFeedback = false;	//Not a feedback PBI
#endif // AK_MOTION
		playHistory = in_rPlayHist;
	}

	ePBIType			eType;
	CAkPBIAware*		pInstigator;
	CAkRegisteredObj*	pGameObj;
	TransParams*		pTransitionParameters;
	UserParams			userParams;
	PlayHistory			playHistory;
	AkPlaybackState		ePlaybackState;
	AkUInt32			uFrameOffset;

	// continuous specific member
	ContParams*			pContinuousParams;
	AkUniqueID			sequenceID;
#ifdef AK_MOTION
	bool				bTargetFeedback;
#endif // AK_MOTION
	bool				bIsFirst;
	bool				bSkipDelay;
};

// class corresponding to a Playback instance
//
// Author:  alessard
class CAkPBI : public CAkTransportAware,
			   public ITransitionable
{
public:
	CAkPBI * pNextItem; // For CAkURenderer::m_listCtxs
	CAkPBI * pNextLightItem; // For sound's PBI List

public:

    // Constructor
	CAkPBI( CAkSoundBase*				in_pSound,			// Pointer to the sound.
			CAkSource*					in_pSource,
			CAkRegisteredObj *			in_pGameObj,		// Game object and channel association.
			UserParams&					in_UserParams,		// User Parameters.
			PlayHistory&				in_rPlayHistory,	// History stuff.
			AkUniqueID					in_SeqID,			// Sample accurate seq id.
			const PriorityInfoCurrent&	in_rPriority,
#ifdef AK_MOTION
			bool						in_bTargetFeedback,	// Do we send the data to the feedback pipeline?
#endif // AK_MOTION
			AkUInt32					in_uSourceOffset,
			CAkLimiter*					in_pAMLimiter,
			CAkLimiter*					in_pBusLimiter
	);

    //Destructor
	virtual ~CAkPBI();

	virtual AKRESULT Init( AkPathInfo* in_pPathInfo );
	virtual void Term( bool in_bFailedToInit);

	virtual void TransUpdateValue(
		AkIntPtr in_eTarget,	// Transition target type
		AkReal32 in_fValue,				// New Value
		bool in_bIsTerminated			// Is it the end of the transition
		);

	AKRESULT SetParam(
			AkPluginParamID in_paramID,         ///< Plug-in parameter ID
			const void *	in_pParam,          ///< Parameter value pointer
			AkUInt32		in_uParamSize		///< Parameter size
			);

	virtual void SetEstimatedLength( AkReal32 in_fEstimatedLength );

	// Play the PBI
	void _InitPlay();

	AKRESULT _Play( TransParams & in_transParams, bool in_bPaused, bool in_bForceIgnoreSync = false );

	// Stop the PBI (the PBI is then destroyed)
	//
	virtual void _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false, bool in_bHasNotStarted = false );

	// Stop the PBI (the PBI is then destroyed)
	//
	void _Stop( 
		const TransParams & in_transParams,	// Fade parameters
		bool in_bUseMinTransTime			// If true, ensures that a short fade is applied if TransitionTime is 0. 
											// If false, the caller needs to ensure that the lower engine will stop the PBI using another method... (stop offset anyone?)
		);

#ifndef AK_OPTIMIZED
	virtual void _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		);
#endif

	//Pause the PBI
	//
	virtual void _Pause( bool in_bIsFromTransition = false );

	//Pause the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	void _Pause( TransParams & in_transParams );

	//Resume the PBI
	//
	virtual void _Resume();

	//Resume the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	void _Resume( TransParams & in_transParams, bool in_bIsMasterResume );

	virtual void PlayToEnd( CAkParameterNodeBase * in_pNode );

	//Seeking
	virtual void SeekTimeAbsolute( AkTimeMs in_iPosition, bool in_bSnapToMarker );
	virtual void SeekPercent( AkReal32 in_fPercent, bool in_bSnapToMarker );

	// Gives the information about the associated game object
	//
	// Return - CAkRegisteredObj * - The game object and Custom ID
	inline CAkRegisteredObj * GetGameObjectPtr() const { return m_pGameObj; }
	inline AkUInt32 GetNumGameObjectPositions() const { return GetGameObjectPtr()->GetPosition().GetNumPosition(); }
	inline SoundEngine::MultiPositionType GetGameObjectMultiPositionType() const { return GetGameObjectPtr()->GetPosition().GetMultiPositionType(); }
	inline AkReal32 GetGameObjectScaling() const { return GetGameObjectPtr()->GetScalingFactor(); }
	AKRESULT GetGameObjectPosition(
		AkUInt32 in_uIndex,
		AkSoundPosition & out_position
		) const;
	inline AkUInt32 GetListenerMask() const { return GetGameObjectPtr()->GetPosition().GetListenerMask(); }
	AKRESULT GetListenerData(
		AkUInt32 in_uListenerMask,
		AkListener & out_listener
		) const;

	void SetCbx( AKPBI_CBXCLASS * in_pCbx ) { m_pCbx = in_pCbx; }
	AKPBI_CBXCLASS * GetCbx() const { return m_pCbx; }

	void ParamNotification( NotifParams& in_rParams );

	// Notify the PBI that the Mute changed
	void MuteNotification(
		AkReal32 in_fMuteRatio, //New mute ratio
		AkMutedMapItem& in_rMutedItem, //Element where mute changed
		bool in_bPrioritizeGameObjectSpecificItems = false
		);

	// Notify the PBI that a 3d param changed
	void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// New muting level
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		);

	// Notify the PBI that the PBI must recalculate the parameters
	void RecalcNotification();

#ifdef AK_MOTION
	void InvalidateFeedbackParameters();
#endif // AK_MOTION

	void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

#ifndef AK_OPTIMIZED
	void InvalidatePaths();
#endif

	//Calculate the Muted/Faded effective volume
	void CalculateMutedEffectiveVolume();
	void CalculateEffectiveLPF();

	// Get Sound.
	CAkSoundBase*	GetSound() const { return m_pSound; }

	// get the current play stop transition
	CAkTransition* GetPlayStopTransition();

	// get the current pause resume transition
	CAkTransition*	GetPauseResumeTransition();

    // direct access to Mute Map
    AKRESULT        SetMuteMapEntry( 
        AkMutedMapItem & in_key,
        AkReal32 in_fFadeRatio
        );
  
	virtual void	SetPauseStateForContinuous(bool in_bIsPaused);

	// Prepare the Sample accurate next sound if possible and available.
	virtual void	PrepareSampleAccurateTransition();

	// 3D sound parameters.
	AkForceInline bool	Is3DSound() const { return m_p3DSound != NULL; }
	CAkGen3DParams *	Get3DSound() { return m_p3DSound; }

	const BaseGenParams& GetBasePosParams(){ return m_BasePosParams; }
	Prev2DParams& GetPrevPosParams(){ return m_Prev2DParams; }

	AKRESULT			SubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation );
	void				UnsubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation );

	inline void			GetDataPtr( AkUInt8 *& out_pBuffer, AkUInt32 & out_uDataSize ) const
	{ 
#ifndef AK_OPTIMIZED
		if ( AK_EXPECT_FALSE( !m_pDataPtr ) )
			MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_MediaNotLoaded, this );
#endif
		out_pBuffer = m_pDataPtr; 
		out_uDataSize = m_uDataSize; 
	}

	bool IsAuxRoutable();
	bool HasUserDefineAux();
	AkForceInline bool IsGameDefinedAuxEnabled(){return m_EffectiveParams.bGameDefinedAuxEnabled;}

	// IAkAudioCtx interface implementation.
	void				Destroy( AkCtxDestroyReason in_eReason );

	void				Play( AkReal32 in_fDuration );
	void				Stop();
	void				Pause();
	void				Resume();
	void				NotifAddedAsSA();

	bool                WasPaused() { return m_bWasPaused; }
	bool				WasStopped() { return m_bWasStopped; }
	
	void				ProcessContextNotif( AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason = CtxDestroyReasonFinished, AkReal32 in_fEstimatedLength = 0.0f );

	AkUniqueID			GetSoundID() const;
	AkUniqueID			GetSequenceID() const { return m_SeqID; }
	CAkBus*				GetOutputBusPtr();
	AkUniqueID			GetBusOutputProfilingID();
	void				UpdatePriority( AkReal32 in_NewPriority );
	AkForceInline AkReal32 GetPriorityFloat() 
	{
		AKASSERT( m_PriorityInfoCurrent.GetCurrent() >= AK_MIN_PRIORITY && m_PriorityInfoCurrent.GetCurrent() <= AK_MAX_PRIORITY );
		return m_PriorityInfoCurrent.GetCurrent(); 
	}
	AkPriority			GetPriority() const
	{ 
		AKASSERT( m_PriorityInfoCurrent.GetCurrent() >= AK_MIN_PRIORITY && m_PriorityInfoCurrent.GetCurrent() <= AK_MAX_PRIORITY );
		return (AkPriority)m_PriorityInfoCurrent.GetCurrent();
	}
	bool				IsPrefetched() const { return m_pSource->IsZeroLatency(); }
	AkUInt16			GetLooping() const { return m_LoopCount; }
	
	AkReal32			GetLoopStartOffsetSeconds() const;
	AkReal32			GetLoopEndOffsetSeconds() const; //Seconds from the end of the file
	
	AkUInt32			GetLoopStartOffsetFrames() const;
	AkUInt32			GetLoopEndOffsetFrames() const; //Frames from the end of the file
	
	AkReal32			GetLoopCrossfadeSeconds() const;
	void				LoopCrossfadeCurveShape( AkCurveInterpolation& out_eCrossfadeUpType,  
													AkCurveInterpolation& out_eCrossfadeDownType) const;

	void				GetTrimSeconds(AkReal32& out_fBeginTrim, AkReal32& out_fEndTrim) const;

	void GetSourceFade(	AkReal32& out_fBeginFadeOffsetSec, AkCurveInterpolation& out_eBeginFadeCurveType, 
					AkReal32& out_fEndFadeOffsetSec, AkCurveInterpolation& out_eEndFadeCurveType  );

	const AkAudioFormat& GetMediaFormat() const { return m_sMediaFormat; }
	void				SetMediaFormat(const AkAudioFormat& in_rFormat ) 
	{ 
		m_sMediaFormat = in_rFormat; 
#ifdef AK_MOTION
		InvalidateFeedbackParameters();	//The memory allocated for the feedback params is not the right size.  Recompute.
#endif // AK_MOTION
	}
	AkBelowThresholdBehavior GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior );

	AkForceInline void CalcEffectiveParams()
	{
		if( !m_bAreParametersValid )
			RefreshParameters();
	}
	AkForceInline const AkSoundParams & GetEffectiveParams() { AKASSERT( m_bAreParametersValid ); return m_EffectiveParams; }

	// Compute collapsed voice volume: actor hierarchy, "voice" bus hierarchy, fades.
	AkForceInline AkReal32 ComputeCollapsedVoiceVolume()
	{
		const AkSoundParams & params = GetEffectiveParams();
		return AkMath::dBToLin( params.Volume + GetVoiceVolumedB() ) * params.fFadeRatio;
	}

#ifndef AK_OPTIMIZED
	AkForceInline void GetEffectiveParamsForMonitoring( AkSoundParamsEx & out_params ) 
	{ 
		CalcEffectiveParams();

		out_params = m_EffectiveParams;
		out_params.fOutputBusVolume = GetOutputBusVolumeValuedB();
		if( IsAuxRoutable() )
		{
			out_params.fOutputBusVolume += AkMath::FastLinTodB( GetDryLevelValue() );
		}
	} 
#endif

	// Compute volume data rays for 3D positioning. All emitter-listener pairs (rays) are computed
	// and cached on game object if applicable. 
	// Returns the number of rays.
	AkUInt32			ComputeVolumeData3D( AkPositionSourceType in_eType, AkVolumeDataArray & out_arVolumeData );
	bool				IsMultiPositionTypeMultiSources();
	void				VirtualPositionUpdate();
	
	AkForceInline AkPannerType GetPannerType() const { return (AkPannerType)m_ePannerType; } 
	AkForceInline AkPositionSourceType	GetPositionSourceType() const { return (AkPositionSourceType)m_ePosSourceType; } 
	AkForceInline bool HasPositioningTypeChanged() const { return m_bPosTypeChanged; } 
	AkForceInline void ResetPositioningTypeChanged() { m_bPosTypeChanged = false; } 
	AkForceInline AkReal32 GetDivergenceCenter() { return m_BasePosParams.m_fCenterPCT * 0.01f; }

	// Emitter-listener pairs (game object position(s) vs listener in spherical coordinates).
	//
	// Get number of pairs of the associated game object. 
	// Note: Returns the real number of pairs, as if the sound was full-featured 3D.
	inline AkUInt32 GetNumEmitterListenerPairs() { return GetGameObjectPtr()->GetNumEmitterListenerPairs(); }

	// Get the ith emitter-listener pair. The returned pair is fully computed (distance and angles).
	AKRESULT GetEmitterListenerPair( 
		AkUInt32 in_uIndex,
		AkEmitterListenerPair & out_emitterListenerPair
		);

    void				GetAuxSendsValues( AkAuxSendValueEx* AK_RESTRICT io_paEnvVal );

#ifdef AK_MOTION
// Feedback devices support
	inline AkFeedbackParams* GetFeedbackParameters()
	{
		if ( !m_bFeedbackParametersValid )
			ValidateFeedbackParameters();
		return m_pFeedbackInfo;
	}
	AkForceInline bool IsForFeedbackPipeline(){ return m_bTargetIsFeedback; }
#else // AK_MOTION
	AkForceInline bool IsForFeedbackPipeline(){ return false; }
#endif // AK_MOTION

	AkForceInline AkReal32 GetDryLevelValue(){ return m_pGameObj->GetDryLevelValue(); }
	AkForceInline AkReal32 GetOutputBusOutputLPF(){ return m_EffectiveParams.fOutputBusLPF; }
	AkForceInline AkReal32 GetObstructionValue( AkUInt32 in_uListener ){ return m_pGameObj->GetObjectObstructionValue( in_uListener ); }
	AkForceInline AkReal32 GetOcclusionValue( AkUInt32 in_uListener ){ return m_pGameObj->GetObjectOcclusionValue( in_uListener ); }
	
	AkForceInline AkSrcTypeInfo *	GetSrcTypeInfo() const { return m_pSource->GetSrcTypeInfo(); }

	AkReal32 GetVoiceVolumedB();
	AkForceInline AkReal32 GetVoiceVolume(){ return AkMath::dBToLin( GetVoiceVolumedB() ); }
	AkReal32 GetOutputBusVolumeValuedB();
	AkForceInline AkReal32 GetOutputBusVolumeValue(){ return AkMath::dBToLin( GetOutputBusVolumeValuedB() ); }

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	AkForceInline bool GetBypassAllFX() { return m_bBypassAllFX; }
#endif
	AkForceInline CAkSource* GetSource() const { return m_pSource; }

	AkForceInline AkPathInfo* GetPathInfo() { return &m_PathInfo; }

	AkForceInline CAkBus* GetControlBus() { return m_pControlBus; }

	AkForceInline bool	WasKicked(){ return m_bWasKicked; }
	void				Kick( KickFrom in_eIsForMemoryThreshold );

	AkForceInline AkPlayingID GetPlayingID() const { return m_UserParams.PlayingID(); };

	virtual AkUInt32	GetStopOffset() const;		// Returns stop offset value.
	virtual AkUInt32	GetAndClearStopOffset();	// Returns stop offset value effectively used for audio frame truncation, and clears it.
	virtual AkCtxVirtualHandlingResult	NotifyVirtualOff( AkVirtualQueueBehavior in_eBehavior );

	bool IsUsingThisSlot( const CAkUsageSlot* in_pSlotToCheck );
	bool IsUsingThisSlot( const AkUInt8* in_pData );

	bool FindAlternateMedia( const CAkUsageSlot* in_pSlotToCheck );

	// Seeking / source offset management
	inline bool RequiresSourceSeek() const { return m_bSeekDirty; }
	inline bool IsSeekRelativeToDuration() const { return m_bSeekRelativeToDuration; }
	
	// Returns seek position in samples (at the source's sample rate).
	// NOTE: IsSeekRelativeToDuration() must be false to use this method.
	inline AkUInt32 GetSeekPosition( bool & out_bSnapToMarker )
	{
		AKASSERT( !IsSeekRelativeToDuration() );		
		out_bSnapToMarker = m_bSnapSeekToMarker;
		// Note: Force using 64 bit calculation to avoid uint32 overflow.
#ifdef AK_MOTION
		AkUInt32 uNativeSampleRate = IsForFeedbackPipeline() ? AK_FEEDBACK_SAMPLE_RATE : AK_CORE_SAMPLERATE;
#else
		AKASSERT(!IsForFeedbackPipeline());
		AkUInt32 uNativeSampleRate = AK_CORE_SAMPLERATE;
#endif
		return (AkUInt32)( (AkUInt64)m_uSeekPosition * GetMediaFormat().uSampleRate / uNativeSampleRate );
	}

	// Returns seek position in samples (at the source's sample rate). 
	// It is computed using the seeking percentage, so the actual source duration must be given.
	// NOTE: IsSeekRelativeToDuration() must be true to use this method.
	inline AkUInt32 GetSeekPosition( AkReal32 in_fSourceDuration, bool & out_bSnapToMarker )
	{
		AKASSERT( IsSeekRelativeToDuration() );
		out_bSnapToMarker = m_bSnapSeekToMarker;
		return (AkUInt32)( m_fSeekPercent * in_fSourceDuration * GetMediaFormat().uSampleRate / 1000.f );
	}

	// IMPORTANT: SetSourceOffsetRemainder() is intended to be used by the lower engine ONLY. Every time a source uses the 
	// source offset, it has to clear or push the error value to the PBI using this method (the pitch node
	// handles the error). It has the effect of clearing the dirty flag. 
	// On the other hand, behavioral engines (PBI and derived classes) must set the source offset using
	// SetNewSeekXX() (which sets the dirty flag).
	inline void SetSourceOffsetRemainder( AkUInt32 in_uSourceOffset )
	{ 
		m_uSeekPosition = in_uSourceOffset; 
		m_bSeekDirty = false; 
		m_bSeekRelativeToDuration = false; 
		m_bSnapSeekToMarker	= false;
	}
	inline AkUInt32 GetSourceOffsetRemainder() 
	{
		// Ignore seeking error if a new seek is pending.
		if ( !m_bSeekDirty )
			return m_uSeekPosition;
		return 0;
	}
	
	// Get current frame offset.
	AkForceInline AkInt32 GetFrameOffset() { return m_iFrameOffset; }
	// Get frame offset before it was consumed. Call this when you need to know the frame offset value before
	// it was consumed in CAkVPLSrcCbxNode::StartRun().
	AkForceInline AkInt32 GetFrameOffsetBeforeFrame() 
	{
#ifdef AK_MOTION
		if ( !IsForFeedbackPipeline() )
			return m_iFrameOffset + AK_NUM_VOICE_REFILL_FRAMES;
		else
			return m_iFrameOffset + AK_FEEDBACK_MAX_FRAMES_PER_BUFFER; 
#else
		return m_iFrameOffset + AK_NUM_VOICE_REFILL_FRAMES;
#endif
	}
	AkForceInline void SetFrameOffset( AkInt32 in_iFrameOffset )
    { 
        m_iFrameOffset = in_iFrameOffset;
    }
    AkForceInline void ConsumeFrameOffset( AkInt32 in_iSamplesConsumed ) 
    { 
		// Stop consuming frame offset once it drops below zero (late) to prevent underflow.
		if ( AK_EXPECT_FALSE( m_iFrameOffset >= 0 ) )
	        m_iFrameOffset -= in_iSamplesConsumed; 
    }

	AkForceInline bool RequiresPreBuffering() const { return m_bRequiresPreBuffering; }

	void GetFXDataID( AkUInt32 in_uFXIndex, AkUInt32 in_uDataIndex, AkUInt32& out_rDataID );

	void UpdateFx(
		AkUInt32	   	in_uFXIndex
		);

#ifndef AK_OPTIMIZED

	void UpdateAttenuationInfo();

#endif //AK_OPTIMIZED

#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	inline bool NeedPriorityUpdate()
	{
		bool returnedvalue = m_bDoPriority;
		m_bDoPriority = false;
		return returnedvalue;
	}
#endif

	// Used for queries.
	AkForceInline AkReal32 GetMaxDistance(){ return m_fMaxDistance; }
	AkForceInline void SetMaxDistance( AkReal32 in_MaxDistance ){ m_fMaxDistance = in_MaxDistance; }

#ifndef AK_OPTIMIZED
	// Notify the monitor with the specified reason
	virtual void Monitor(
		AkMonitorData::NotificationReason in_Reason,			// Reason for the notification
		bool in_bUpdateCount = true
		);

	// Returns true if should be silent because of authoring.
	inline bool IsMonitoringMute() { return m_bIsMonitoringMute; }

	// Walk through hierarchy to determine status of monitoring mute.
	void RefreshMonitoringMute();
#else
	inline void Monitor(
		AkMonitorData::NotificationReason,
		bool /*in_bUpdateCount*/ = true
		)
	{/*No implementation in Optimized version*/}
#endif

	bool IsInitiallyUnderThreshold( AkVolumeDataArray & out_arVolumeData );
	
	void Virtualize();
	void Devirtualize( bool in_bAllowKick = true );
	inline bool IsVirtualOrForcedVirtual(){ return (m_bIsForcedToVirtualize || m_bIsVirtual); }
	inline bool IsVirtual(){ return m_bIsVirtual; }
	inline bool IsForcedVirtualized(){ return m_bIsForcedToVirtualize; }
	inline bool WasForcedVirtualized(){ return m_bWasForcedToVirtualize; }

	inline bool NeedsFadeIn() { return m_bNeedsFadeIn; }

	inline bool IsHDR() { return m_bIsHDR; }
	inline bool IsLoudnessNormalizationEnabled() 
	{ 
		CalcEffectiveParams();
		return GetEffectiveParams().normalization.bNormalizeLoudness;
	}
	
	void ForceVirtualize( KickFrom in_eReason );
	void ForceVirtualize()// Called by the Wii, watch out when playing with this code.
	{
		m_bIsForcedToVirtualize = true;
	}

	void ForceDevirtualize()
	{
		m_bWasForcedToVirtualize = m_bIsForcedToVirtualize;
		m_bIsForcedToVirtualize = false; 
	}

	AkForceInline AkPriorityStruct& GetPriorityKey(){ return m_PriorityInfoCurrent.GetPriorityKey(); }
	AkForceInline AkUniqueID GetPBIID(){ return m_PriorityInfoCurrent.GetPBIID(); }

#if defined(AK_VITA_HW)
	AkForceInline bool IsHardware() { return m_bHardwareVoice; }
	void SetHardware();
#endif

protected:
	enum PBIInitialState
	{
		PBI_InitState_Playing,
		PBI_InitState_Paused,
		PBI_InitState_Stopped
#define PBIINITSTATE_NUM_STORAGE_BIT 3
	};

	// Overridable methods.
	virtual void RefreshParameters();

	// Internal use only
	// Notify the monitor with the specified reason
	virtual void MonitorFade(
		AkMonitorData::NotificationReason in_Reason,			// Reason for the notification
		AkTimeMs in_FadeTime
		);

	void CreateTransition(
		bool in_bIsPlayStopTransition,		// true if it is a PlayStop transition, false if it is PauseResume
		AkIntPtr in_transitionTarget,		// Transition target type
		TransParams in_transParams,         // Transition parameters.
		bool in_bIsFadingTransition			// is the transition fading out(pause of stop)
		);

	// Prepare PBI for stopping with minimum transition time. Note: also used in PBI subclasses.
	void StopWithMinTransTime();

	void DecrementPlayCount();

    void RemoveAllVolatileMuteItems();


	// Seeking, from PBI and derived classes only (behavioral engine).
	inline void SetNewSeekPosition( AkUInt32 in_uSeekPosition, bool in_bSnapSeekToMarker ){ m_uSeekPosition = in_uSeekPosition; m_bSeekRelativeToDuration = false; SetNewSeekFlags( in_bSnapSeekToMarker ); }
	inline void SetNewSeekPercent( AkReal32 in_fSeekPercent, bool in_bSnapSeekToMarker ){ m_fSeekPercent = in_fSeekPercent; m_bSeekRelativeToDuration = true; SetNewSeekFlags( in_bSnapSeekToMarker ); }

	// 3D game defined positioning-dependent priority offset.
	void ComputePriorityOffset( AkReal32 in_fMinDistance, Gen3DParams * in_p3DParams );

#ifdef AK_MOTION
	void ValidateFeedbackParameters();
#endif

	AKRESULT Init3DPath(AkPathInfo* in_pPathInfo);

// Static member
	
	static AkUniqueID	ms_PBIIDGenerator;
	static AkUniqueID GetNewPBIID(){ return ms_PBIIDGenerator++; }

// Members
	CAkUsageSlot*		m_pUsageSlot;

	AkMutedMap			m_mapMutedNodes;
	AkPBIModValues		m_Ranges;

	CAkGen3DParams *	m_p3DSound;					// 3D parameters.
    
	UserParams			m_UserParams;				// User Parameters.

	PlaybackTransition	m_PBTrans;
	
	AkUniqueID			m_SeqID;					// Sample accurate seq id.
	CAkSoundBase*		m_pSound;					// Parent SoundNode
	CAkSource*			m_pSource;					// Associated Source
	CAkRegisteredObj*	m_pGameObj;					// CAkRegisteredObj to use to Desactivate itself once the associated game object were unregistered.
	AKPBI_CBXCLASS*   m_pCbx;						// Combiner pointer	

	AkAudioFormat		m_sMediaFormat;				// Audio format of the source.
	AkSoundParamsEx		m_EffectiveParams;			// Includes mutes and fades

	AkVolumeValue		m_Volume;					// Effective volume (ignores Fades and mute).
	AkLPFType			m_LPF;						// Effective LPF (ignores LPF automation).
	AkLPFType			m_LPFAutomationOffset;		// LPF Automation. NOTE: currently it is the only non-RTPC'd offset to LPF. Convert to a "mute map" if required (update in CalculateEffectiveLPF).

	AkReal32			m_fPlayStopFadeRatio;
	AkReal32			m_fPauseResumeFadeRatio;
	
	CAkCntrHist			m_CntrHistArray;

	// Seek position (expressed in absolute time or percentage, according to m_bSeekRelativeToDuration).
	union
	{
		AkUInt32			m_uSeekPosition;		// Seek offset in Native samples.
		AkReal32			m_fSeekPercent;			// Seek offset expressed in percentage of duration ([0,1]).
	};

	AkInt16				m_LoopCount;

// enums
	AkUInt8/*PBIInitialState*/	m_eInitialState			:PBIINITSTATE_NUM_STORAGE_BIT;
	AkUInt8/*AkCtxState*/		m_State					:CTXSTATE_NUM_STORAGE_BIT;
	AkUInt8/*AkPannerType*/		m_ePannerType			:PANNER_NUM_STORAGE_BITS; 
	AkUInt8/*AkPositionSourceType*/	m_ePosSourceType	:POSSOURCE_NUM_STORAGE_BITS; 
	AkUInt8						m_bPosTypeChanged		:1;	//Position type was changed.  Used for RTPC-able 2d-3d switch.  Recompute even if other parameters didn't change.
	AkUInt8						m_eCachedVirtualQueueBehavior	:VIRTUAL_QUEUE_BEHAVIOR_NUM_STORAGE_BIT;
	AkUInt8						m_eCachedBelowThresholdBehavior :BELOW_THRESHOLD_BEHAVIOR_NUM_STORAGE_BIT;
	AkUInt8						m_bVirtualBehaviorCached		:1;

// bools stored on one bit.
	AkUInt8				m_bAreParametersValid	:1;
    AkUInt8				m_bGameObjPositionCached:1;	// used when not dynamic.

	AkUInt8				m_bGetAudioParamsCalled	:1;

	AkUInt8				m_bNeedNotifyEndReached :1;
	AkUInt8				m_bIsNotifyEndReachedContinuous :1;

	AkUInt8				m_bTerminatedByStop		:1;
	AkUInt8				m_bPlayFailed			:1;
	
	//This flag is used to avoid sending commands that would be invalid anyway after sending a stop.
	AkUInt8				m_bWasStopped			:1;
	AkUInt8				m_bWasPreStopped		:1;
	AkUInt8				m_bWasPaused			:1;

	AkUInt8				m_bInitPlayWasCalled	:1;

	AkUInt8				m_bWasKicked			:1;
	AkUInt8				m_eWasKickedForMemory	:3;
	AkUInt8				m_bWasPlayCountDecremented :1;

	AkUInt8				m_bRequiresPreBuffering :1;	// True for PBIs that should not starve on start up (e.g. music).

#if defined AK_WII_FAMILY_HW || defined(AK_3DS)
	AkUInt8				m_bDoPriority			:1;		// Only required on platforms that have to update the hardware priority
#endif // AK_WII

#ifdef AK_MOTION
	AkUInt8				m_bFeedbackParametersValid :1;
	AkUInt8				m_bTargetIsFeedback		:1;
#endif // AK_MOTION

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
	AkUInt8             m_bBypassAllFX			:1;
#endif // AK_WII
	AkUInt8             m_bSeekDirty				:1;
	AkUInt8             m_bSeekRelativeToDuration	:1;	// When true, seeking is expressed as a percentage of the whole duration, and m_fSeekPercent must be used
	AkUInt8             m_bSnapSeekToMarker			:1;

	AkUInt8				m_bIsForcedToVirtualize		:1;
	AkUInt8				m_bWasForcedToVirtualize	:1;
	AkUInt8				m_bIsVirtual				:1;
	AkUInt8				m_bNeedsFadeIn				:1;
	AkUInt8				m_bIsHDR					:1;

#ifndef AK_OPTIMIZED
	AkUInt8				m_bIsMonitoringMute			:1;	// Should not be heard when muted
#endif

#if defined(AK_VITA_HW)
	AkUInt8				m_bHardwareVoice :1;
#endif

	PriorityInfoCurrent m_PriorityInfoCurrent;

	AkUInt32			m_ulPauseCount;

	AkInt32			    m_iFrameOffset;

	AkUInt8 *			m_pDataPtr;
	AkUInt32			m_uDataSize;
	AkReal32			m_fMaxDistance; // used for the query of the same name.

	// Back up of the first position when the position of the sound is not dynamic
	AkPositionKeeper 	m_cachedGameObjectPosition;

	BaseGenParams		m_BasePosParams;
	Prev2DParams		m_Prev2DParams;

#ifdef AK_MOTION
	// Feedback information.  This is an optional structure which should be present
	// only if there is information to be kept.  This means either: 
	// a) the object is connected to a feedback bus
	// b) the user set a feedback volume (even if not connected directly, children could be affected)
	AkFeedbackParams* m_pFeedbackInfo;
#endif // AK_MOTION

private:

	inline void SetNewSeekFlags( bool in_bSnapSeekToMarker )
	{ 
		m_bSnapSeekToMarker = in_bSnapSeekToMarker; 
		m_bSeekDirty = true; 
	}

	void PausePath(
		bool in_bPause
		);

	AkReal32 Scale3DUserDefRTPCValue( AkReal32 in_fValue );

	AkPathInfo			m_PathInfo;			// our path if any

	CAkLimiter* m_pAMLimiter;
	CAkLimiter* m_pBusLimiter;

	CAkBus* m_pControlBus;

#if defined (_DEBUG)
	void DebugCheckLimiters();
#endif

};

#endif
