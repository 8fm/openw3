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
// AkFXContext.cpp
//
// Implementation of FX context interface for source, insert and bus FX.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkFXContext.h"
#include "AkBusCtx.h"
#include "AkMonitor.h"
#include "AkVPLMixBusNode.h"
#include "AkBankMgr.h"
#include "AkFxBase.h"
#include "AkRuntimeEnvironmentMgr.h"

AkDataReferenceArray::~AkDataReferenceArray()
{
	for( AkDataReferenceArray::Iterator iter = Begin(); iter != End(); ++iter )
	{
		AkDataReference& ref = (*iter).item;
		if( ref.pData && ref.uSourceID != AK_INVALID_SOURCE_ID )
		{
			g_pBankManager->ReleaseMedia( ref.uSourceID );
			if( ref.pUsageSlot )
				ref.pUsageSlot->Release( false );
		}
	}

	Term();
}

AkDataReference * AkDataReferenceArray::AcquireData( AkUInt32 in_uDataIdx, AkUInt32 in_uSourceID )
{
	AkDataReference * pDataReference = NULL;
	CAkUsageSlot* pReturnedUsageSlot = NULL;
	AkMediaInfo mediaInfo = g_pBankManager->GetMedia( in_uSourceID, pReturnedUsageSlot );

	if( mediaInfo.pInMemoryData )
	{
		pDataReference = Set( in_uDataIdx );
		if( pDataReference )
		{
			pDataReference->pData = mediaInfo.pInMemoryData;
			pDataReference->uSize = mediaInfo.uInMemoryDataSize;
			pDataReference->uSourceID = in_uSourceID;
			pDataReference->pUsageSlot = pReturnedUsageSlot;
		}
		else
		{
			//not enough memory, releasing the data now.
			g_pBankManager->ReleaseMedia( in_uSourceID );
			if( pReturnedUsageSlot )
			{
				pReturnedUsageSlot->Release( false );
			}
		}
	}

	return pDataReference;
}

//-----------------------------------------------------------------------------
// CAkEffectContextBase class.
//-----------------------------------------------------------------------------
CAkEffectContextBase::CAkEffectContextBase( AkUInt32 in_uFXIndex )
	: m_uFXIndex( in_uFXIndex )
{
}

CAkEffectContextBase::~CAkEffectContextBase()
{
}

AK::IAkStreamMgr * CAkEffectContextBase::GetStreamMgr( ) const
{
	return AK::IAkStreamMgr::Get();
}

static bool FindAlternateMedia( const CAkUsageSlot* in_pSlotToCheck, AkDataReference& io_rDataRef, AK::IAkPlugin* in_pCorrespondingFX )
{
	AKASSERT( in_pCorrespondingFX );
	if( in_pCorrespondingFX->SupportMediaRelocation() )
	{
		CAkUsageSlot* pReturnedUsageSlot = NULL;
		AkMediaInfo mediaInfo = g_pBankManager->GetMedia( io_rDataRef.uSourceID, pReturnedUsageSlot );

		if( mediaInfo.pInMemoryData )
		{
			if( in_pCorrespondingFX->RelocateMedia( mediaInfo.pInMemoryData, io_rDataRef.pData ) == AK_Success )
			{
				// Here Release old data and do swap in PBI to.
				if( io_rDataRef.pData && io_rDataRef.uSourceID != AK_INVALID_SOURCE_ID )
				{
					g_pBankManager->ReleaseMedia( io_rDataRef.uSourceID );
					if( io_rDataRef.pUsageSlot )
					{
						io_rDataRef.pUsageSlot->Release( false );
					}
				}
				
				io_rDataRef.pData = mediaInfo.pInMemoryData;
				io_rDataRef.pUsageSlot = pReturnedUsageSlot;

				return true;
			}
			else
			{
				//It is quite unexpected that a call to RelocateMedia would fail, but just in case we still need to release.
				if ( mediaInfo.pInMemoryData )
				{
					g_pBankManager->ReleaseMedia( io_rDataRef.uSourceID );
					if( pReturnedUsageSlot )
						pReturnedUsageSlot->Release( false );
				}
				
			}
		}
	}

	return false;
}

bool CAkEffectContextBase::IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot, AK::IAkPlugin* in_pCorrespondingFX ) 
{
	for( AkDataReferenceArray::Iterator iter = m_dataArray.Begin(); iter != m_dataArray.End(); ++iter )
	{
		AkDataReference& ref = (*iter).item;
		if( ref.pUsageSlot == in_pUsageSlot )
		{
			if( !FindAlternateMedia( in_pUsageSlot, ref, in_pCorrespondingFX ) )
				return true;
		}
	}

	return false;
}

bool CAkEffectContextBase::IsUsingThisSlot( const AkUInt8* in_pData ) 
{
	for( AkDataReferenceArray::Iterator iter = m_dataArray.Begin(); iter != m_dataArray.End(); ++iter )
	{
		AkDataReference& ref = (*iter).item;
		if( ref.pData == in_pData )
			return true;
	}

	return false;
}
	
#if (defined AK_CPU_X86 || defined AK_CPU_X86_64) && !(defined AK_IOS)
IAkProcessorFeatures * CAkEffectContextBase::GetProcessorFeatures()
{
	return AkRuntimeEnvironmentMgr::Instance();
}
#endif

#if !defined(AK_WII_FAMILY_HW) && !defined(AK_3DS)
//-----------------------------------------------------------------------------
// CAkInsertFXContext class.
//-----------------------------------------------------------------------------
CAkInsertFXContext::CAkInsertFXContext( CAkPBI * in_pCtx, AkUInt32 in_uFXIndex ) 
	: CAkEffectContextBase( in_uFXIndex )
	, m_pContext( in_pCtx )
{
}

CAkInsertFXContext::~CAkInsertFXContext( )
{
}

bool CAkInsertFXContext::IsSendModeEffect( ) const
{
	return false;	// Always insert FX mode
}

AKRESULT CAkInsertFXContext::PostMonitorData(
	void *		/*in_pData*/,
	AkUInt32	/*in_uDataSize*/
	)
{
	// Push data not supported on insert effects, simply because we have no way of representing 
	// a single instance of an effect in the UI.
	return AK_Fail;
}

bool CAkInsertFXContext::CanPostMonitorData()
{
	// Push data not supported on insert effects, simply because we have no way of representing 
	// a single instance of an effect in the UI.
	return false;
}

void CAkInsertFXContext::GetPluginMedia( 
		AkUInt32 in_dataIndex,		///< Index of the data to be returned.
		AkUInt8* &out_rpData,		///< Pointer to the data (refcounted, must be released by the plugin).
		AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
		)
{
	// Search for already existing data pointers.
	AkDataReference* pDataReference = m_dataArray.Exists( in_dataIndex );
	if( !pDataReference )
	{
		// Get the source ID
		AkUInt32 l_dataSourceID = AK_INVALID_SOURCE_ID;
		m_pContext->GetFXDataID( m_uFXIndex, in_dataIndex, l_dataSourceID );

		// Get the pointers
		if( l_dataSourceID != AK_INVALID_SOURCE_ID )
			pDataReference = m_dataArray.AcquireData( in_dataIndex, l_dataSourceID );
	}

	if( pDataReference )
	{
		// Setting what we found.
		out_rDataSize = pDataReference->uSize;
		out_rpData = pDataReference->pData;
	}
	else
	{
		// Clean the I/Os variables, there is no data available.
		out_rpData = NULL; 
		out_rDataSize = 0;
	}
}
#endif

//-----------------------------------------------------------------------------
// CAkBusFXContext class.
//-----------------------------------------------------------------------------
CAkBusFXContext::CAkBusFXContext( 
		CAkBusFX * in_pBusFX, 
		AkUInt32 in_uFXIndex, 
		const AK::CAkBusCtx& in_rBusContext
#if defined(AK_3DS)
		,nn::snd::CTR::AuxBusId in_AuxBusID
#endif
		) 
	: CAkEffectContextBase( in_uFXIndex )
	, m_pBusFX( in_pBusFX )
	, m_BuxCtx( in_rBusContext )
#if defined(AK_3DS)
	, m_AuxBusID( in_AuxBusID )
#endif
{
	AKASSERT( m_pBusFX );
}

CAkBusFXContext::~CAkBusFXContext( )
{
}

bool CAkBusFXContext::IsSendModeEffect() const
{
	return m_BuxCtx.IsAuxBus();
}

AKRESULT CAkBusFXContext::PostMonitorData(
	void *		in_pData,
	AkUInt32	in_uDataSize
	)
{
#ifndef AK_OPTIMIZED
	AkPluginID	pluginID;
	AkUInt32	uFXIndex;
	m_pBusFX->FindFXByContext( this, pluginID, uFXIndex );
	MONITOR_PLUGINSENDDATA( in_pData, in_uDataSize, m_BuxCtx.ID(), pluginID, uFXIndex );
	return AK_Success;
#else
	return AK_Fail;
#endif
}

bool CAkBusFXContext::CanPostMonitorData()
{
#ifndef AK_OPTIMIZED
	return AkMonitor::IsMonitoring();
#else
	return false;
#endif
}

void CAkBusFXContext::GetPluginMedia( 
		AkUInt32 in_dataIndex,		///< Index of the data to be returned.
		AkUInt8* &out_rpData,		///< Pointer to the data (refcounted, must be released by the plugin).
		AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
		)
{
	// Search for already existing data pointers.
	AkDataReference* pDataReference = m_dataArray.Exists( in_dataIndex );
	if( !pDataReference )
	{
		// Get the source ID
		AkUInt32 l_dataSourceID = AK_INVALID_SOURCE_ID;
		m_BuxCtx.GetFXDataID( m_uFXIndex, in_dataIndex, l_dataSourceID );

		// Get the pointers
		if( l_dataSourceID != AK_INVALID_SOURCE_ID )
			pDataReference = m_dataArray.AcquireData( in_dataIndex, l_dataSourceID );
	}

	if( pDataReference )
	{
		// Setting what we found.
		out_rDataSize = pDataReference->uSize;
		out_rpData = pDataReference->pData;
	}
	else
	{
		// Clean the I/Os variables, there is no data available.
		out_rpData = NULL; 
		out_rDataSize = 0;
	}
}

//-----------------------------------------------------------------------------
// CAkSourceFXContext class.
//-----------------------------------------------------------------------------
CAkSourceFXContext::CAkSourceFXContext( CAkPBI * in_pCtx ) 
	: m_pContext( in_pCtx )
{
}

CAkSourceFXContext::~CAkSourceFXContext( )
{
}

// Number of loops set through context

AkUInt16 CAkSourceFXContext::GetNumLoops( ) const
{
	AKASSERT( m_pContext != NULL );
	return m_pContext->GetLooping( );
}

AK::IAkStreamMgr * CAkSourceFXContext::GetStreamMgr( ) const
{
	return AK::IAkStreamMgr::Get();
}

bool CAkSourceFXContext::IsUsingThisSlot( const CAkUsageSlot* in_pUsageSlot, AK::IAkPlugin* in_pCorrespondingFX ) 
{
	for( AkDataReferenceArray::Iterator iter = m_dataArray.Begin(); iter != m_dataArray.End(); ++iter )
	{
		AkDataReference& ref = (*iter).item;
		if( ref.pUsageSlot == in_pUsageSlot )
		{
			if( !FindAlternateMedia( in_pUsageSlot, ref, in_pCorrespondingFX ) )
				return true;
		}
	}

	return false;
}

bool CAkSourceFXContext::IsUsingThisSlot( const AkUInt8* in_pData ) 
{
	for( AkDataReferenceArray::Iterator iter = m_dataArray.Begin(); iter != m_dataArray.End(); ++iter )
	{
		AkDataReference& ref = (*iter).item;
		if( ref.pData == in_pData )
			return true;
	}

	return false;
}

void CAkSourceFXContext::GetPluginMedia( 
	AkUInt32 in_dataIndex,		///< Index of the data to be returned.
	AkUInt8* &out_rpData,		///< Pointer to the data.
	AkUInt32 &out_rDataSize		///< size of the data returned in bytes.
	)
{
	// Search for already existing data pointers.
	AkDataReference* pDataReference = m_dataArray.Exists( in_dataIndex );
	if( !pDataReference )
	{
		// Get the source ID
		AkUInt32 l_dataSourceID = AK_INVALID_SOURCE_ID;
		{
			AkSrcTypeInfo * pSrcType = m_pContext->GetSrcTypeInfo();

			// Source plug-in are always custom for now
			CAkFxBase * pFx = g_pIndex->m_idxFxCustom.GetPtrAndAddRef( pSrcType->mediaInfo.sourceID );
			if ( pFx )
			{
				l_dataSourceID = pFx->GetMediaID( in_dataIndex );
				pFx->Release();
			}
		}

		// Get the pointers
		if( l_dataSourceID != AK_INVALID_SOURCE_ID )
			pDataReference = m_dataArray.AcquireData( in_dataIndex, l_dataSourceID );
	}

	if( pDataReference )
	{
		// Setting what we found.
		out_rDataSize = pDataReference->uSize;
		out_rpData = pDataReference->pData;
	}
	else
	{
		// Clean the I/Os variables, there is no data available.
		out_rpData = NULL; 
		out_rDataSize = 0;
	}
}

AKRESULT CAkSourceFXContext::PostMonitorData(
	void *		in_pData,
	AkUInt32	in_uDataSize
	)
{
	AKASSERT( !"Not implemented yet" );
	return AK_NotImplemented;
}

bool CAkSourceFXContext::CanPostMonitorData()
{
	// Not implemented yet.
	return false;
}

#if (defined AK_CPU_X86 || defined AK_CPU_X86_64) && !(defined AK_IOS)
IAkProcessorFeatures * CAkSourceFXContext::GetProcessorFeatures()
{
	return AkRuntimeEnvironmentMgr::Instance();
}
#endif
