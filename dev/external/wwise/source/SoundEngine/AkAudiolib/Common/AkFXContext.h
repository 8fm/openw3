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
// AkFXContext.h
//
// Implementation of FX context interface for source, insert and bus FX.
// These class essentially package up information into a more unified interface for the
// FX API.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FXCONTEXT_H_
#define _AK_FXCONTEXT_H_

#include "AkPBI.h"
#include <AK/Tools/Common/AkObject.h>
#if defined AK_WII_FAMILY_HW
#include "AkVoicePlaybackCtrl.h"
#endif

class CAkUsageSlot;

struct AkDataReference
{
	AkUInt8* pData;
	AkUInt32 uSize;
	AkUInt32 uSourceID;
	CAkUsageSlot* pUsageSlot;
};

class AkDataReferenceArray
	: public CAkKeyArray<AkUInt32, AkDataReference>
{
public:
	~AkDataReferenceArray();

	AkDataReference * AcquireData( AkUInt32 in_uDataIdx, AkUInt32 in_uSourceID );
};

//-----------------------------------------------------------------------------
// CAkEffectContextBase class.
//-----------------------------------------------------------------------------
class CAkEffectContextBase
	: public AK::IAkEffectPluginContext
{

public:
	CAkEffectContextBase( AkUInt32 in_uFXIndex );
	virtual ~CAkEffectContextBase();

	// Retrieve streaming manager interface if desired
	virtual AK::IAkStreamMgr * GetStreamMgr() const;

	bool IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot, AK::IAkPlugin* in_pCorrespondingFX );
	bool IsUsingThisSlot( const AkUInt8* in_pData );

#if (defined AK_CPU_X86 || defined AK_CPU_X86_64) && !(defined AK_IOS)
	// Return an interface to query processor specific features 
	virtual IAkProcessorFeatures * GetProcessorFeatures();
#endif

protected:

	AkUInt32 m_uFXIndex;
	AkDataReferenceArray m_dataArray;
};

#if !defined (AK_WII_FAMILY_HW) && !defined(AK_3DS)
// No insert FX on the Wii
//-----------------------------------------------------------------------------
// CAkInsertFXContext class.
//-----------------------------------------------------------------------------
class CAkInsertFXContext 
	: public CAkEffectContextBase
{
public:
	CAkInsertFXContext( CAkPBI * in_pCtx, AkUInt32 in_uFXIndex );
	virtual ~CAkInsertFXContext();

	inline void SetPBI( CAkPBI * in_pCtx ) { m_pContext = in_pCtx; }

	// Determine if our effect is Send Mode or Insert
	bool IsSendModeEffect() const;
	AkUInt16 GetMaxBufferLength( ) const
	{ 
		// Note: This should be EQUAL to the allocated buffer size that the inserted effect node sees (pitch node output)
		return LE_MAX_FRAMES_PER_BUFFER;
	}

	virtual AKRESULT PostMonitorData(
		void *		in_pData,
		AkUInt32	in_uDataSize
		);

	virtual bool	 CanPostMonitorData();

	virtual void GetPluginMedia( 
		AkUInt32 in_dataIndex,		///< Index of the data to be returned.
		AkUInt8* &out_rpData,		///< Pointer to the data.
		AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
		);

	virtual AkPlayingID GetPlayingID() const { return m_pContext->GetPlayingID(); }

	virtual AkGameObjectID GetGameObjectID() const { return m_pContext->GetGameObjectPtr()->ID(); }

	virtual void GetPositioningType(AkPannerType &out_ePanner, AkPositionSourceType &out_ePosSource) const { out_ePanner = m_pContext->GetPannerType(); out_ePosSource = m_pContext->GetPositionSourceType();}

	virtual AkUInt32 GetNumEmitterListenerPairs() const { return m_pContext->GetNumEmitterListenerPairs(); }

	virtual AKRESULT GetEmitterListenerPair(
		AkUInt32 in_uIndex,
		AkEmitterListenerPair & out_emitterListenerPair
		) const { return m_pContext->GetEmitterListenerPair( in_uIndex, out_emitterListenerPair ); }

	virtual AkUInt32 GetNumGameObjectPositions() const { return m_pContext->GetNumGameObjectPositions(); }
	
	virtual SoundEngine::MultiPositionType GetGameObjectMultiPositionType() const { return m_pContext->GetGameObjectMultiPositionType(); }

	virtual AkReal32 GetGameObjectScaling() const { return m_pContext->GetGameObjectScaling(); }

	virtual AKRESULT GetGameObjectPosition(
		AkUInt32 in_uIndex,
		AkSoundPosition & out_position
		) const { return m_pContext->GetGameObjectPosition( in_uIndex, out_position ); }

	virtual AkUInt32 GetListenerMask() const { return m_pContext->GetListenerMask(); }

	virtual AKRESULT GetListenerData(
		AkUInt32 in_uListenerMask,
		AkListener & out_listener
		) const { return m_pContext->GetListenerData( in_uListenerMask, out_listener ); }

protected:
	CAkPBI * m_pContext;
};
#endif
//-----------------------------------------------------------------------------
// CAkBusFXContext class.
//-----------------------------------------------------------------------------
class CAkBusFX;

class CAkBusFXContext 
	: public CAkEffectContextBase						
{
public:
	CAkBusFXContext( 
		CAkBusFX * in_pBusFX, 
		AkUInt32 in_uFXIndex, 
		const AK::CAkBusCtx& in_rBusContext
#if defined(AK_3DS)
		,nn::snd::CTR::AuxBusId in_AuxBusID
#endif
);
	virtual ~CAkBusFXContext();

	virtual bool IsSendModeEffect() const;
	virtual AkUInt16 GetMaxBufferLength( ) const
	{ 
#if defined AK_WII_FAMILY_HW
		return AX_IN_SAMPLES_PER_FRAME;
#else
		// Note: This should be EQUAL to the allocated buffer size that the inserted effect node sees (pitch node output)
		return LE_MAX_FRAMES_PER_BUFFER;
#endif
	}

	virtual AKRESULT PostMonitorData(
		void *		in_pData,
		AkUInt32	in_uDataSize
		);

	virtual bool	 CanPostMonitorData();

	virtual void GetPluginMedia( 
		AkUInt32 in_dataIndex,		///< Index of the data to be returned.
		AkUInt8* &out_rpData,		///< Pointer to the data (refcounted, must be released by the plugin).
		AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
		);

	virtual AkPlayingID GetPlayingID() const { return AK_INVALID_PLAYING_ID; }

	virtual AkGameObjectID GetGameObjectID() const { return AK_INVALID_GAME_OBJECT; }

	virtual void GetPositioningType(AkPannerType &out_ePanner, AkPositionSourceType &out_ePosSource) const { out_ePanner = Ak2D; out_ePosSource = AkUserDef;}

	virtual AkUInt32 GetNumEmitterListenerPairs() const { return 0; }

	virtual AKRESULT GetEmitterListenerPair(
		AkUInt32 /*in_uIndex*/,
		AkEmitterListenerPair & /*out_emitterListenerPair*/
		) const { return AK_Fail; }

	virtual AkUInt32 GetNumGameObjectPositions() const { return 0; }
	
	virtual SoundEngine::MultiPositionType GetGameObjectMultiPositionType() const { return AK::SoundEngine::MultiPositionType_MultiSources; }

	virtual AkReal32 GetGameObjectScaling() const { return 1.f; }

	virtual AKRESULT GetGameObjectPosition(
		AkUInt32 /*in_uIndex*/,
		AkSoundPosition & /*out_position*/
		) const { return AK_Fail; }

	virtual AkUInt32 GetListenerMask() const { return 0; }

	virtual AKRESULT GetListenerData(
		AkUInt32 /*in_uListenerMask*/,
		AkListener & /*out_listener*/
		) const { return AK_Fail; }

#if defined(AK_3DS)
	AkForceInline nn::snd::CTR::AuxBusId GetAuxBusId() { return m_AuxBusID; }
#endif

	CAkBus* GetBus(){ return m_BuxCtx.GetBus(); }

protected:
	CAkBusFX * m_pBusFX;
	AK::CAkBusCtx m_BuxCtx;
#if defined(AK_3DS)
	nn::snd::CTR::AuxBusId m_AuxBusID;
#endif
};

//-----------------------------------------------------------------------------
// CAkSourceFXContext class.
//-----------------------------------------------------------------------------
class CAkSourceFXContext : public AK::IAkSourcePluginContext
							
{
public:
	CAkSourceFXContext( CAkPBI * );
	virtual ~CAkSourceFXContext();

	// Number of loops set through the context, nothing to do
	virtual AkUInt16 GetNumLoops() const;

	// Retrieve streaming manager interface if desired
	virtual AK::IAkStreamMgr * GetStreamMgr() const;

	bool IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot, AK::IAkPlugin* in_pCorrespondingFX );
	bool IsUsingThisSlot( const AkUInt8* in_pData );

	virtual AkUInt16 GetMaxBufferLength( ) const
	{ 
#if defined AK_WII_FAMILY_HW
		return MAX_SAMPLE_FRAMES_PER_AX_FRAME;
#else
		// Note: This should be EQUAL to the allocated buffer size that the inserted effect node sees (pitch node output)
		return LE_MAX_FRAMES_PER_BUFFER;
#endif
	}

	virtual void GetPluginMedia( 
		AkUInt32 in_dataIndex,		///< Index of the data to be returned.
		AkUInt8* &out_rpData,		///< Pointer to the data.
		AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
		);

	virtual AKRESULT PostMonitorData(
		void *		in_pData,
		AkUInt32	in_uDataSize
		);

	virtual bool	 CanPostMonitorData();

	virtual AkPlayingID GetPlayingID() const { return m_pContext->GetPlayingID(); }

	virtual AkGameObjectID GetGameObjectID() const { return m_pContext->GetGameObjectPtr()->ID(); }

	virtual void GetPositioningType(AkPannerType &out_ePanner, AkPositionSourceType &out_ePosSource) const { out_ePanner = m_pContext->GetPannerType(); out_ePosSource = m_pContext->GetPositionSourceType();}

	virtual AkUInt32 GetNumEmitterListenerPairs() const { return m_pContext->GetNumEmitterListenerPairs(); }

	virtual AKRESULT GetEmitterListenerPair(
		AkUInt32 in_uIndex,
		AkEmitterListenerPair & out_emitterListenerPair
		) const { return m_pContext->GetEmitterListenerPair( in_uIndex, out_emitterListenerPair ); }

	virtual AkUInt32 GetNumGameObjectPositions() const { return m_pContext->GetNumGameObjectPositions(); }
	
	virtual SoundEngine::MultiPositionType GetGameObjectMultiPositionType() const { return m_pContext->GetGameObjectMultiPositionType(); }

	virtual AkReal32 GetGameObjectScaling() const { return m_pContext->GetGameObjectScaling(); }

	virtual AKRESULT GetGameObjectPosition(
		AkUInt32 in_uIndex,
		AkSoundPosition & out_position
		) const { return m_pContext->GetGameObjectPosition( in_uIndex, out_position ); }

	virtual AkUInt32 GetListenerMask() const { return m_pContext->GetListenerMask(); }

	virtual AKRESULT GetListenerData(
		AkUInt32 in_uListenerMask,
		AkListener & out_listener
		) const { return m_pContext->GetListenerData( in_uListenerMask, out_listener ); }

#if (defined AK_CPU_X86 || defined AK_CPU_X86_64) && !(defined AK_IOS)
	// Return an interface to query processor specific features 
	virtual IAkProcessorFeatures * GetProcessorFeatures();
#endif

protected:
	CAkPBI * m_pContext;
	AkDataReferenceArray m_dataArray;
};

#endif // _AK_FXCONTEXT_H_
