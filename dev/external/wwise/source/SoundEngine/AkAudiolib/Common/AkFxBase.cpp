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

#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkEffectsMgr.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkLEngine.h"
#include "AkRTPCMgr.h"

CAkFxBase::CAkFxBase( AkUniqueID in_ulID )
	: CAkIndexable( in_ulID )
	, m_FXID( AK_INVALID_PLUGINID )
	, m_pParam( NULL )
{
}

CAkFxBase::~CAkFxBase()
{
	for ( RTPCSubsArray::Iterator iter = m_rtpcsubs.Begin(); iter != m_rtpcsubs.End(); ++iter )
	{
		(*iter).ConversionTable.Unset();
	}

	m_rtpcsubs.Term();

	m_media.Term();

	// Delete the existing parameter object.
	if( m_pParam )
	{
		m_pParam->Term( AkFXMemAlloc::GetUpper( ) );
	}
}

AKRESULT CAkFxBase::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_uDataSize )
{
	// We don't care about the shareset ID, just skip it
	SKIPBANKDATA( AkUInt32, in_pData, in_uDataSize );

	AkPluginID fxID = READBANKDATA( AkPluginID, in_pData, in_uDataSize );
	AkUInt32 uSize = READBANKDATA( AkUInt32, in_pData, in_uDataSize );

	AKRESULT eResult = AK_Success;

	if( fxID != AK_INVALID_PLUGINID )
		eResult = SetFX( fxID, in_pData, uSize );

	// Skip the plugin data blob

	in_pData += uSize;
	in_uDataSize -= uSize;

	// Read media

	if( eResult == AK_Success )
	{
		AkUInt32 uNumBankData = READBANKDATA( AkUInt8, in_pData, in_uDataSize );
		m_media.Reserve( uNumBankData );

		for( AkUInt32 i = 0; i < uNumBankData; ++i )
		{
			AkUInt32 index = READBANKDATA( AkUInt8, in_pData, in_uDataSize );
			AkUInt32 sourceId = READBANKDATA( AkUInt32, in_pData, in_uDataSize );
			m_media.Set( index, sourceId );
		}

		// Read RTPC

		AkUInt32 ulNumRTPC = READBANKDATA( AkUInt16, in_pData, in_uDataSize );

		for( AkUInt32 i = 0; i < ulNumRTPC; ++i )
		{
			AkRtpcID RTPCID = READBANKDATA( AkUInt32, in_pData, in_uDataSize );
			AkRTPC_ParameterID ParamID = (AkRTPC_ParameterID)READBANKDATA(AkUInt32, in_pData, in_uDataSize); // Volume, Pitch, LFE...
			const AkUniqueID rtpcCurveID = (AkUniqueID)READBANKDATA(AkUInt32, in_pData, in_uDataSize);
			AkCurveScaling eScaling = (AkCurveScaling) READBANKDATA(AkUInt8, in_pData, in_uDataSize);

			// ArraySize //i.e.:number of conversion points
			AkUInt32 ulSize = READBANKDATA(AkUInt16, in_pData, in_uDataSize);

			SetRTPC( RTPCID, ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)in_pData, ulSize, false );

			//Skipping the Conversion array
			in_pData += ( ulSize*sizeof(AkRTPCGraphPoint) );
			in_uDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
		}
	}

	return eResult;
}

void CAkFxBase::SetFX( 
	AkPluginID in_FXID,
	AK::IAkPluginParam * in_pParam
	)
{
	// Delete the existing parameter object.
	if( m_pParam )
	{
		m_pParam->Term( AkFXMemAlloc::GetUpper( ) );
		m_pParam = NULL;
	}

	m_FXID = in_FXID;
	m_pParam = in_pParam;
}

AKRESULT CAkFxBase::SetFX( 
	AkPluginID in_FXID,
	void * in_pParamBlock,
	AkUInt32 in_uParamBlockSize
	)
{
	// Create and init the parameter object for the specified effect.
	AK::IAkPluginParam * pParam = NULL;
	AKRESULT eResult = CAkEffectsMgr::AllocParams( AkFXMemAlloc::GetUpper(), in_FXID, pParam );

	AKASSERT( eResult != AK_Success || pParam != NULL );
	if( eResult != AK_Success || pParam == NULL )
	{
		// Yes success. We don't want a bank to fail loading because an FX is not registered or not working.
		// So we don't set the FX and return, as if there was never any FX.
		// The sound will play without FX.
		return AK_Success;
	}

	eResult = pParam->Init( AkFXMemAlloc::GetUpper(), in_pParamBlock, in_uParamBlockSize );
	if( eResult != AK_Success )
	{
		pParam->Term( AkFXMemAlloc::GetUpper( ) );
		return eResult;
	}

	SetFX( in_FXID, pParam );

	return AK_Success;
}

struct _SetFXParamStruct
{
	AkPluginParamID uParamID;
	void * pParamBlock;
	AkUInt32 uParamBlockSize;
};

static void _SetFXParamFunc(
	AK::IAkPluginParam * in_pParam,
	CAkRegisteredObj * /*in_pGameObj*/,
	void * in_pCookie 
	)
{
	_SetFXParamStruct * pStruct = (_SetFXParamStruct *) in_pCookie;
	in_pParam->SetParam( pStruct->uParamID, pStruct->pParamBlock, pStruct->uParamBlockSize );
}

void CAkFxBase::SetFXParam(
	AkPluginParamID in_uParamID,	// ID of the param to modify, will be done by the plug-in itself
	void * in_pParamBlock,			// Pointer to the Param block
	AkUInt32 in_uParamBlockSize		// BLOB size
	)
{
	AKASSERT( in_pParamBlock );
	if ( m_pParam && in_pParamBlock )
	{
		m_pParam->SetParam( 
			in_uParamID,
			in_pParamBlock,
			in_uParamBlockSize );

		_SetFXParamStruct s;
		s.uParamID = in_uParamID;
		s.pParamBlock = in_pParamBlock;
		s.uParamBlockSize = in_uParamBlockSize;
		CAkLEngine::ForAllPluginParam( this, _SetFXParamFunc, &s );
	}
}

struct _SetRTPCStruct
{
	AkRtpcID			RTPC_ID;
	AkRTPC_ParameterID	ParamID;
	AkUniqueID			RTPCCurveID;
	AkCurveScaling		eScaling;
	AkRTPCGraphPoint*	pArrayConversion;
	AkUInt32			ulConversionArraySize;
};

static void _SetRTPCFunc(
	AK::IAkPluginParam * in_pParam,
	CAkRegisteredObj * in_pGameObj,
	void * in_pCookie 
	)
{
	_SetRTPCStruct * pStruct = (_SetRTPCStruct *) in_pCookie;
	g_pRTPCMgr->SubscribeRTPC( in_pParam, pStruct->RTPC_ID, pStruct->ParamID, pStruct->RTPCCurveID, pStruct->eScaling, pStruct->pArrayConversion, pStruct->ulConversionArraySize, in_pGameObj, CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber );
}

void CAkFxBase::SetRTPC( 
	AkRtpcID			in_RTPC_ID,
	AkRTPC_ParameterID	in_ParamID,
	AkUniqueID			in_RTPCCurveID,
	AkCurveScaling		in_eScaling,
	AkRTPCGraphPoint*	in_pArrayConversion/* = NULL*/,
	AkUInt32			in_ulConversionArraySize/* = 0*/,
	bool				in_bNotify
	)
{
	UnsetRTPC( in_ParamID, in_RTPCCurveID, in_bNotify );

	RTPCSubs * pSubs = m_rtpcsubs.AddLast();
	if ( pSubs )
	{
		pSubs->RTPCCurveID = in_RTPCCurveID;
		pSubs->RTPCID = in_RTPC_ID;
		pSubs->ParamID = in_ParamID;

		if( in_pArrayConversion && in_ulConversionArraySize )
		{
			pSubs->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );
		}

		if ( in_bNotify )
		{
			_SetRTPCStruct s;
			s.RTPC_ID = in_RTPC_ID;
			s.ParamID = in_ParamID;
			s.RTPCCurveID = in_RTPCCurveID;
			s.eScaling = in_eScaling;
			s.pArrayConversion = in_pArrayConversion;
			s.ulConversionArraySize = in_ulConversionArraySize;
			CAkLEngine::ForAllPluginParam( this, _SetRTPCFunc, &s );
		}
	}
}

struct _UnsetRTPCStruct
{
	AkRTPC_ParameterID	ParamID;
	AkUniqueID			RTPCCurveID;
};

static void _UnsetRTPCFunc(
	AK::IAkPluginParam * in_pParam,
	CAkRegisteredObj * /*in_pGameObj*/,
	void * in_pCookie 
	)
{
	_UnsetRTPCStruct * pStruct = (_UnsetRTPCStruct *) in_pCookie;
	g_pRTPCMgr->UnSubscribeRTPC( in_pParam, pStruct->ParamID, pStruct->RTPCCurveID );
}

void CAkFxBase::UnsetRTPC( 
	AkRTPC_ParameterID in_ParamID,
	AkUniqueID in_RTPCCurveID,
	bool in_bNotify
	)
{
	bool bRemoved = false;

	RTPCSubsArray::Iterator iter = m_rtpcsubs.Begin();
	while( iter != m_rtpcsubs.End() )
	{
		if( (*iter).ParamID == in_ParamID && (*iter).RTPCCurveID == in_RTPCCurveID )
		{
			(*iter).ConversionTable.Unset();
			iter = m_rtpcsubs.Erase( iter );
			bRemoved = true;
		}
		else
		{
			++iter;
		}
	}

	if ( bRemoved && in_bNotify )
	{
		_UnsetRTPCStruct s;
		s.ParamID = in_ParamID;
		s.RTPCCurveID = in_RTPCCurveID;
		CAkLEngine::ForAllPluginParam( this, _UnsetRTPCFunc, &s );
	}
}

void CAkFxBase::SetMediaID( AkUInt32 in_uIdx, AkUniqueID in_mediaID )
{
	m_media.Set( in_uIdx, in_mediaID );
}

void CAkFxBase::SubscribeRTPC( AK::IAkRTPCSubscriber* in_pSubscriber, CAkRegisteredObj * in_pGameObj )
{
	for( RTPCSubsArray::Iterator iter = m_rtpcsubs.Begin(); iter != m_rtpcsubs.End(); ++iter )
	{
		RTPCSubs& l_rFXRTPC = *iter;

		g_pRTPCMgr->SubscribeRTPC( 
			in_pSubscriber,
			l_rFXRTPC.RTPCID,
			l_rFXRTPC.ParamID,
			l_rFXRTPC.RTPCCurveID,
			l_rFXRTPC.ConversionTable.m_eScaling,
			l_rFXRTPC.ConversionTable.m_pArrayGraphPoints,
			l_rFXRTPC.ConversionTable.m_ulArraySize,
			in_pGameObj,
			CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
			);
	}
}

void CAkFxBase::UnsubscribeRTPC( AK::IAkRTPCSubscriber* in_pSubscriber )
{
	for( RTPCSubsArray::Iterator iter = m_rtpcsubs.Begin(); iter != m_rtpcsubs.End(); ++iter )
	{
		RTPCSubs& l_rFXRTPC = *iter;

		g_pRTPCMgr->UnSubscribeRTPC( 
			in_pSubscriber,
			l_rFXRTPC.ParamID
			);
	}
}

CAkFxShareSet* CAkFxShareSet::Create( AkUniqueID in_ulID )
{
	CAkFxShareSet* pFx = AkNew( g_DefaultPoolId, CAkFxShareSet( in_ulID ) );
	if( pFx )
		g_pIndex->m_idxFxShareSets.SetIDToPtr( pFx );	
	return pFx;
}

AkUInt32 CAkFxShareSet::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxFxShareSets.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkFxShareSet::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxFxShareSets.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		g_pIndex->m_idxFxShareSets.RemoveID( ID() );
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

CAkFxShareSet::CAkFxShareSet( AkUniqueID in_ulID )
	: CAkFxBase( in_ulID )
{
}

CAkFxCustom* CAkFxCustom::Create( AkUniqueID in_ulID )
{
	CAkFxCustom* pFx = AkNew( g_DefaultPoolId, CAkFxCustom( in_ulID ) );
	if( pFx )
		g_pIndex->m_idxFxCustom.SetIDToPtr( pFx );	
	return pFx;
}

AkUInt32 CAkFxCustom::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxFxCustom.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkFxCustom::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxFxCustom.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
		g_pIndex->m_idxFxCustom.RemoveID( ID() );
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

CAkFxCustom::CAkFxCustom( AkUniqueID in_ulID )
	: CAkFxBase( in_ulID )
{
}
