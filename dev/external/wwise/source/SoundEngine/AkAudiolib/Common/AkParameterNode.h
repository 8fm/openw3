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
// AkParameterNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PARAMETER_NODE_H_
#define _PARAMETER_NODE_H_

#include "AkKeyArray.h"
#include "AkModifiers.h"
#include "AkParameterNodeBase.h"
#include "AkPathManager.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkGen3DParams.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>

class CAkBus;
class CAkGen3DParams;
class CAkLayer;

struct Params3DSound;
struct AkPathVertex;
struct AkPathListItemOffset;

// class corresponding to node having parameters and states
//
// Author:  alessard
class CAkParameterNode : public CAkParameterNodeBase
{
	friend class CAkPBI;
	friend class CAkSIS;

public:
	// Constructors
    CAkParameterNode(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkParameterNode();

	AKRESULT Init(){ return CAkParameterNodeBase::Init(); }

	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams ) = 0;

	AKRESULT Play( AkPBIParams& in_rPBIParams )
	{
		AKRESULT eResult = HandleInitialDelay( in_rPBIParams );
		if( eResult == AK_PartialSuccess )
			return AK_Success;
		else if( eResult == AK_Success )
			return PlayInternal( in_rPBIParams );
		return eResult;
	}

	// Set the value of a property at the node level (float)
	virtual void SetAkProp( 
		AkPropID in_eProp, 
		AkReal32 in_fValue, 
		AkReal32 in_fMin, 
		AkReal32 in_fMax 
		);
	
	// Set the value of a property at the node level (int)
	virtual void SetAkProp( 
		AkPropID in_eProp, 
		AkInt32 in_iValue, 
		AkInt32 in_iMin, 
		AkInt32 in_iMax 
		);

	// Set a runtime property value (SIS)
	virtual void SetAkProp(
		AkPropID in_eProp, 
		CAkRegisteredObj * in_pGameObj,		// Game object associated to the action
		AkValueMeaning in_eValueMeaning,	// Target value meaning
		AkReal32 in_TargetValue = 0,		// Pitch target value
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs in_lTransitionTime = 0
		);

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

	//Unmute all per object elements
	virtual void UnmuteAllObj(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Notify the children that the associated object was unregistered
	virtual void Unregister(
		CAkRegisteredObj * in_pGameObj //Game object associated to the action
		);

	// Allocate 3D parameter blob if required.
	void Get3DParams(
		CAkGen3DParams*& out_rp3DParams,
		CAkRegisteredObj * in_GameObj,
		AkPannerType & out_ePannerType,
		AkPositionSourceType & out_ePosType,	
		BaseGenParams * io_pBasePosParams
		);

	virtual bool ParamOverriden( AkRTPC_ParameterID in_ParamID )
	{
		bool  bOverriden;
		switch( in_ParamID )
		{
		case RTPC_UserAuxSendVolume0:
		case RTPC_UserAuxSendVolume1:
		case RTPC_UserAuxSendVolume2:
		case RTPC_UserAuxSendVolume3:
			bOverriden = m_bOverrideUserAuxSends || !m_pParentNode;
			break;

		case RTPC_GameAuxSendVolume:
			bOverriden = m_bOverrideGameAuxSends || !m_pParentNode;
			break;

		case RTPC_OutputBusVolume:
		case RTPC_OutputBusLPF:
			bOverriden = m_pBusOutputNode != NULL;
			break;

		default:
			bOverriden = false;
		}
		return bOverriden;
	}

	virtual bool ParamMustNotify( AkRTPC_ParameterID in_ParamID )
	{
		bool  bMustNotify;
		switch( in_ParamID )
		{
		case RTPC_UserAuxSendVolume0:
		case RTPC_UserAuxSendVolume1:
		case RTPC_UserAuxSendVolume2:
		case RTPC_UserAuxSendVolume3:
			bMustNotify = m_bOverrideUserAuxSends || !m_pParentNode;
			break;

		case RTPC_GameAuxSendVolume:
			bMustNotify = m_bOverrideGameAuxSends || !m_pParentNode;
			break;

		case RTPC_OutputBusVolume:
		case RTPC_OutputBusLPF:
			bMustNotify = m_pBusOutputNode != NULL;
			break;

		default:
			bMustNotify = true;
		}
		return bMustNotify;
	}

	AKRESULT GetStatic3DParams( AkPositioningInfo& out_rPosInfo );

	void UpdateBaseParams(
		CAkRegisteredObj * in_GameObj,
		BaseGenParams * io_pBasePosParams,
		CAkGen3DParams * io_p3DParams
		);

	// Fill the parameters structures with the new parameters
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetAudioParameters(
		AkSoundParamsEx &out_Parameters,	// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		);

#ifdef AK_MOTION
	AKRESULT GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck = true);

	virtual AkVolumeValue GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr );
	virtual AkLPFType GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr );
#endif // AK_MOTION

	virtual AKRESULT PlayAndContinueAlternate( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteActionExceptParentCheck( ActionParamsExcept& in_rAction );

	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );
	void OverrideFXParent( bool in_bIsFXOverrideParent );

	virtual void GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj * in_GameObj = NULL
		);

	virtual void GetFXDataID(
		AkUInt32	in_uFXIndex,
		AkUInt32	in_uDataIndex,
		AkUInt32&	out_rDataID
		);

	bool GetBypassFX( 
		AkUInt32	in_uFXIndex,
		CAkRegisteredObj * in_pGameObj );

	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj );

	virtual void ResetFXBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

//////////////////////////////////////////////////////////////////////////////
//Positioning information setting
//////////////////////////////////////////////////////////////////////////////

	// WAL entry point for positioning type. 
	virtual void PosSetPositioningType( bool in_bOverride, bool in_bRTPC, AkPannerType in_ePanner, AkPositionSourceType in_ePosSource );
	AKRESULT PosSetConeUsage( bool in_bIsConeEnabled );
	
	void PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled );
	void PosSetAttenuationID( AkUniqueID in_AttenuationID );

	void PosSetIsPositionDynamic( bool in_bIsDynamic );
	void PosSetFollowOrientation( bool in_bFollow );

	AKRESULT PosSetPathMode( AkPathMode in_ePathMode );
	AKRESULT PosSetIsLooping( bool in_bIsLooping );
	AKRESULT PosSetTransition( AkTimeMs in_TransitionTime );

	AKRESULT PosSetPath(
		AkPathVertex*           in_pArrayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

#ifndef AK_OPTIMIZED
	void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		);	
#endif

	void PosSetPathRange(AkUInt32 in_ulPathIndex, AkReal32 in_fXRange, AkReal32 in_fYRange);

	virtual bool GetMaxRadius( AkReal32 & out_fRadius );
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

	inline void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior )
	{
		m_eBelowThresholdBehavior = in_eBelowThresholdBehavior;
	}
	inline void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior )
	{
		m_eVirtualQueueBehavior = in_eBehavior;
	}

	AKRESULT SetAuxBusSend( AkUniqueID in_AuxBusID, AkUInt32 in_ulIndex );
	void SetOverrideGameAuxSends( bool in_bOverride );
	void SetUseGameAuxSends( bool in_bUse );
	void SetOverrideUserAuxSends( bool in_bOverride );

	// Used to increment/decrement the playcount used for notifications and ducking
	virtual AKRESULT IncrementPlayCount( CounterParameters& io_params );

	virtual void DecrementPlayCount(
		CounterParameters& io_params
		);

	virtual void IncrementVirtualCount( 
		CounterParameters& io_params
		);

	virtual void DecrementVirtualCount( 
		CounterParameters& io_params
		);

	virtual void ApplyMaxNumInstances( AkUInt16 in_u16MaxNumInstance, CAkRegisteredObj* in_pGameObj = NULL, void* in_pExceptArray = NULL, bool in_bFromRTPC = false );

	bool IsOrIsChildOf( CAkParameterNodeBase * in_pNodeToTest, bool in_bIsBusChecked = false );

	// Returns true if the Context may jump to virtual, false otherwise.
	AkBelowThresholdBehavior GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior ) const;

	virtual bool Has3DParams();

	AKRESULT AssociateLayer( CAkLayer* in_pLayer );
	AKRESULT DissociateLayer( CAkLayer* in_pLayer );

	bool GetFxParentOverride(){ return m_bIsFXOverrideParent; }

	AKRESULT HandleInitialDelay( AkPBIParams& in_rPBIParams );
	AKRESULT DelayPlayback( AkReal32 in_fDelay, AkPBIParams& in_rPBIParams );

	// HDR
	void SetOverrideHdrEnvelope( bool in_bOverrideParent );
	void SetOverrideAnalysis( bool in_bOverrideParent );
	void SetNormalizeLoudness( bool in_bNormalizeLoudness );
	void SetEnableEnvelope( bool in_bEnableEnvelope );

protected:

#ifndef AK_OPTIMIZED
			void InvalidateAllPaths();
			virtual void InvalidatePaths();
#endif

	virtual AKRESULT SetInitialParams( AkUInt8*& pData, AkUInt32& ulDataSize );
	
	virtual AKRESULT SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAuxParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	void Get3DCloneForObject( CAkGen3DParams*& in_rp3DParams, AkPositionSourceType & out_ePosType );
	AkForceInline AkPannerType GetPannerType() { return (AkPannerType)m_ePannerType; }

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj );

	AkPathState* GetPathState();

	AKRESULT Enable3DPosParams();
	void DisablePosParams();

	template<class T_VALUE>
	AkForceInline void ApplyRange( AkPropID in_ePropID, T_VALUE & io_value )
	{
#ifdef AKPROP_TYPECHECK
		AKASSERT( typeid( T_VALUE ) == *g_AkPropTypeInfo[ in_ePropID ] );
#endif
		RANGED_MODIFIERS<T_VALUE> * pRange = (RANGED_MODIFIERS<T_VALUE> *) m_ranges.FindProp( in_ePropID );
		if ( pRange )
			io_value += RandomizerModifier::GetMod( *pRange );
	}

private:
	void FreePathInfo();

// members
private:

	typedef CAkKeyArray<CAkRegisteredObj *, CAkSIS*> AkMapSIS;
	AkMapSIS* m_pMapSIS;		// Map of specific parameters associated to an object

	CAkGen3DParamsEx* m_p3DParameters;

	AkPropBundle< RANGED_MODIFIERS<AkPropValue> > m_ranges;

	typedef AkArray<CAkLayer*, CAkLayer*, ArrayPoolDefault, LIST_POOL_BLOCK_SIZE/sizeof(CAkLayer*)> LayerList;
	LayerList* m_pAssociatedLayers;

	//AK_NUM_AUX_SEND_PER_OBJ

	struct AuxChunk
	{
		AuxChunk();

		AkUniqueID aAux[ AK_NUM_AUX_SEND_PER_OBJ ];
	};

	AuxChunk * m_pAuxChunk;

	AkUInt8	m_eVirtualQueueBehavior :VIRTUAL_QUEUE_BEHAVIOR_NUM_STORAGE_BIT;
	AkUInt8	m_eBelowThresholdBehavior:BELOW_THRESHOLD_BEHAVIOR_NUM_STORAGE_BIT;
	AkUInt8 m_ePannerType			:PANNER_NUM_STORAGE_BITS;	//AkPannerType
	AkUInt8 m_ePosSourceType		:POSSOURCE_NUM_STORAGE_BITS;	//AkPositionSourceType
	AkUInt8	m_bOverrideGameAuxSends	:1;
	AkUInt8	m_bUseGameAuxSends		:1;
	AkUInt8	m_bOverrideUserAuxSends	:1;
	AkUInt8	m_bOverrideHdrEnvelope	:1;
	AkUInt8	m_bOverrideAnalysis		:1;	// Analysis used for auto-normalization
	AkUInt8	m_bNormalizeLoudness	:1;
	AkUInt8	m_bEnableEnvelope		:1;
};

#endif
