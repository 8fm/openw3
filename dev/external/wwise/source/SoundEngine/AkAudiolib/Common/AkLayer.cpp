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
// AkLayer.cpp
//
// Implementation of the CAkLayer class, which represents a Layer
// in a Layer Container (CAkLayerCntr)
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkLayer.h"	
#include "AkLayerCntr.h"
#include "AkAudioLibIndex.h"
#include "AkPBI.h"
#include "AkRTPCMgr.h"
#include "AkParameterNode.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

CAkLayer::CAkLayer( AkUniqueID in_ulID )
	: CAkIndexable( in_ulID )
	, m_pOwner( NULL )
	, m_crossfadingRTPCID( 0 )
{
}

CAkLayer::~CAkLayer(void)
{
	// Remove all RTPCs
	AKASSERT( g_pRTPCMgr );

	for ( AkUInt32 iBit = 0; !m_RTPCBitArray.IsEmpty(); ++iBit )
	{ 
		if ( m_RTPCBitArray.IsSet( iBit ) )
		{
			g_pRTPCMgr->UnSubscribeRTPC( this, iBit );
			m_RTPCBitArray.UnsetBit( iBit );
		}
	}

	if ( m_crossfadingRTPCID )
		g_pRTPCMgr->UnregisterLayer( this, m_crossfadingRTPCID );

	// Terminate all associations
	for( AssociatedChildMap::Iterator iter = m_assocs.Begin(); iter != m_assocs.End(); ++iter )
	{
		AKVERIFY( AK_Success == (*iter).item.Term( this ) );
	}

	m_assocs.Term();
}

CAkLayer* CAkLayer::Create( AkUniqueID in_ulID )
{
	CAkLayer* pAkLayer = AkNew( g_DefaultPoolId, CAkLayer( in_ulID ) );
	if ( pAkLayer )
	{
		if ( pAkLayer->Init() != AK_Success )
		{
			pAkLayer->Release();
			pAkLayer = NULL;
		}
	}

	return pAkLayer;
}

AKRESULT CAkLayer::Init()
{
	AddToIndex();

	return AK_Success;
}

void CAkLayer::SetRTPC(
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,		// NULL if none
		AkUInt32						in_ulConversionArraySize	// 0 if none
		)
{
	AKASSERT(g_pRTPCMgr);
	AKASSERT( in_ParamID < sizeof(AkUInt64)*8 );

	// Remember that there is RTPC on this param
	m_RTPCBitArray.SetBit( in_ParamID );

	g_pRTPCMgr->SubscribeRTPC( 
		this,
		in_RTPC_ID, 
		in_ParamID, 
		in_RTPCCurveID,
		in_eScaling,
		in_pArrayConversion, 
		in_ulConversionArraySize,
		NULL,								// Get notified for all Game Objects
		CAkRTPCMgr::SubscriberType_CAkLayer
	);
}

void CAkLayer::UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	AKASSERT(g_pRTPCMgr);
	AKASSERT( in_ParamID < sizeof(AkUInt64)*8 );

	bool bMoreCurvesRemaining = false;
	g_pRTPCMgr->UnSubscribeRTPC(
		this,
		in_ParamID,
		in_RTPCCurveID,
		&bMoreCurvesRemaining
	);

	// No more RTPC on this param?
	if ( ! bMoreCurvesRemaining )
		m_RTPCBitArray.UnsetBit( in_ParamID );

	RecalcNotification();
}

AKRESULT CAkLayer::GetAudioParameters(
	CAkParameterNode*	in_pAssociatedChild,	// Associated child making the call
	AkSoundParams		&io_Parameters,			// Set of parameter to be filled
	AkUInt32				in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
	AkMutedMap&			io_rMutedMap,			// Map of muted elements
	CAkRegisteredObj*	in_GameObjPtr			// Game object associated to the query
)
{
	AKASSERT( in_pAssociatedChild );
	const AssociatedChildMap::Iterator itChild = m_assocs.FindEx( in_pAssociatedChild->ID() );

	// Children call GetAudioParameters() on their associated layers. Since that association
	// is made by this class, we know there should always be an assoc for a child that calls this function.
	AKASSERT( itChild != m_assocs.End() );

	if ( in_ulParamSelect & PT_Volume && m_RTPCBitArray.IsSet( RTPC_Volume ) )
	{
		io_Parameters.Volume += g_pRTPCMgr->GetRTPCConvertedValue( this, RTPC_Volume, in_GameObjPtr );
	}

	if ( in_ulParamSelect & PT_Pitch && m_RTPCBitArray.IsSet( RTPC_Pitch ) )
	{
		io_Parameters.Pitch += (AkPitchValue)g_pRTPCMgr->GetRTPCConvertedValue( this, RTPC_Pitch, in_GameObjPtr );
	}

	if ( in_ulParamSelect & PT_LPF && m_RTPCBitArray.IsSet( RTPC_LPF ) )
	{
		io_Parameters.LPF += g_pRTPCMgr->GetRTPCConvertedValue( this, RTPC_LPF, in_GameObjPtr );
	}

	// Crossfading

	if ( m_crossfadingRTPCID != 0 && itChild.pItem->item.m_fadeCurve.m_pArrayGraphPoints )
	{
		// Get the value of the Game Parameter for the specified Game Object. If it
		// doesn't exist (was never set), then use the default value.
		AkReal32 rtpcValue;
		bool bGameObjectSpecificValue = true;
		if ( ! g_pRTPCMgr->GetRTPCValue( m_crossfadingRTPCID, in_GameObjPtr, rtpcValue, bGameObjectSpecificValue ) )
		{
			bGameObjectSpecificValue = false;
			
			rtpcValue = g_pRTPCMgr->GetDefaultValue( m_crossfadingRTPCID );
		}

		// Compute the muting level for this child at the current position of the Game Parameter.
		AkReal32 fMuteLevel = itChild.pItem->item.m_fadeCurve.Convert( rtpcValue );
		
		if ( fMuteLevel != AK_UNMUTED_RATIO || bGameObjectSpecificValue )
		{
			// Set the value in the map. If it's AK_UNMUTED_RATIO, we still set it if
			// it's a game object-specific value, because we want the PBI
			// to know that it has a game object-specific muting value so it
			// doesn't replace it with a global value later.

			AkMutedMapItem item;
            item.m_bIsPersistent = false;
			item.m_bIsGlobal = ! bGameObjectSpecificValue;
			item.m_Identifier = this;

			// The map should be clean before calling this function!
			AKASSERT( ! io_rMutedMap.Exists( item ) );

			io_rMutedMap.Set( item, fMuteLevel );
		}
	}

	return AK_Success;
}

AKRESULT CAkLayer::SetParamComplexFromRTPCManager( 
	void * in_pToken,
	AkRTPC_ParameterID in_Param_id, 
	AkRtpcID in_RTPCid,
	AkReal32 in_value, 
	CAkRegisteredObj * in_GameObj,
	void* in_pGameObjExceptArray
)
{
	AKASSERT( m_RTPCBitArray.IsSet( in_Param_id ) );
	AKASSERT( in_Param_id == RTPC_Volume || in_Param_id == RTPC_Pitch || in_Param_id == RTPC_LPF );

	AkReal32 l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid );
	Notification( in_Param_id, in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );

	return AK_Success;
}

void CAkLayer::RecalcNotification()
{
	for( AssociatedChildMap::Iterator iter = this->m_assocs.Begin(); iter != this->m_assocs.End(); ++iter )
	{
		if ( (*iter).item.m_pChild && (*iter).item.m_pChild->IsPlaying() )
		{
			(*iter).item.m_pChild->RecalcNotification();
		}
	}
}

void CAkLayer::AddToIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxLayers.SetIDToPtr( this );
}

void CAkLayer::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxLayers.RemoveID( ID() );
}

AkUInt32 CAkLayer::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxLayers.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkLayer::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxLayers.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex(); 
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

void CAkLayer::Notification( AkRTPC_ParameterID in_eType, AkReal32 in_fValue, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = in_eType;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.fValue = in_fValue;
	l_Params.pExceptObjects = in_pExceptArray;
	ParamNotification( l_Params );
}

void CAkLayer::ParamNotification( NotifParams& in_rParams )
{
	// Since this is used only privately, we know the notification never comes
	// from a bus. If this ever changes, then look at what other bus-aware
	// classes do in that case.
	AKASSERT( ! in_rParams.bIsFromBus );

	for( AssociatedChildMap::Iterator iter = this->m_assocs.Begin(); iter != this->m_assocs.End(); ++iter )
	{
		if ( (*iter).item.m_pChild && (*iter).item.m_pChild->IsPlaying() )
		{
			(*iter).item.m_pChild->ParamNotification( in_rParams );
		}
	}
}

bool CAkLayer::IsPlaying() const
{
	for( AssociatedChildMap::Iterator iter = this->m_assocs.Begin(); iter != this->m_assocs.End(); ++iter )
	{
		if ( (*iter).item.m_pChild && (*iter).item.m_pChild->IsPlaying() )
		{
			// Just one playing associated object is enough
			return true;
		}
	}

	return false;
}

AKRESULT CAkLayer::SetInitialRTPC( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// Read count
	const AkUInt32 ulNumRTPC = READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );

	for(AkUInt32 i = 0; i < ulNumRTPC; ++i)
	{
		// Read RTPCID
		const AkRtpcID l_RTPCID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read ParameterID (Volume, Pitch, LFE...)
		const AkRTPC_ParameterID l_ParamID = (AkRTPC_ParameterID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read Curve ID
		const AkUniqueID rtpcCurveID = (AkUniqueID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read curve scaling type (None, dB...)
		AkCurveScaling eScaling = (AkCurveScaling) READBANKDATA(AkUInt8, io_rpData, io_rulDataSize);
		// ArraySize (number of points in the RTPC curve)
		AkUInt32 ulSize = READBANKDATA(AkUInt16, io_rpData, io_rulDataSize);

		// Read and apply the RTPC
		SetRTPC( l_RTPCID, l_ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)io_rpData, ulSize );

		// Skip the Conversion array that was processed by SetRTPC() above
		io_rpData += ( ulSize*sizeof(AkRTPCGraphPoint) );
		io_rulDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
	}

	return AK_Success;
}

AKRESULT CAkLayer::SetInitialValues( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	//
	// Read ID
	//

	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	//
	// Read RTPCs
	//

	AKRESULT eResult = SetInitialRTPC( io_rpData, io_rulDataSize );

	if ( eResult == AK_Success )
	{
		//
		// Read Crossfading Info
		//

		eResult = SetCrossfadingRTPC( READBANKDATA(AkUInt32, io_rpData, io_rulDataSize) );
		if ( eResult == AK_Success )
		{
			//
			// Read Assocs
			//
	
			const AkUInt32 ulNumAssoc = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
	
			if ( ulNumAssoc )
			{
				eResult = m_assocs.Reserve( ulNumAssoc );
				if ( eResult == AK_Success )
				{
					for( AkUInt32 i = 0; i < ulNumAssoc; ++i )
					{
						// Read the associated child's ID
						const AkUInt32 ulAssociatedChildID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
	
						// Read the crossfading curve
						AkUInt32 ulCurveSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
	
						// Create the assoc
						eResult = SetChildAssoc( ulAssociatedChildID, (AkRTPCGraphPoint*)io_rpData, ulCurveSize );
	
						if ( eResult != AK_Success )
						{
							// Stop loading as soon as something doesn't work
							break;
						}
	
						// Skip the Conversion array that was processed by SetChildAssoc() above
						io_rpData += ( ulCurveSize * sizeof(AkRTPCGraphPoint) );
						io_rulDataSize -= ( ulCurveSize * sizeof(AkRTPCGraphPoint) );
					}
				}
			}
		}
	}

	return eResult;
}

AKRESULT CAkLayer::CanAssociateChild( CAkParameterNodeBase * in_pAudioNode )
{
	AKRESULT eResult = AK_Fail;

	AKASSERT( in_pAudioNode );

	if ( m_pOwner && in_pAudioNode->Parent() )
	{
		// We have an Owner and the object has a parent: It must be the same object
		if ( in_pAudioNode->Parent() == m_pOwner )
			eResult = AK_Success;
	}
	else
	{
		// Either we don't yet have an owner or the object doesn't yet
		// have a parent. That's Ok, we'll try again later when we are
		// assigned an owner or when the object becomes a child of the
		// container (in the meantime, we will not connect to the object)
		eResult = AK_PartialSuccess;
	}

	return eResult;
}

AKRESULT CAkLayer::SetChildAssoc(
	AkUniqueID in_ChildID,
	AkRTPCGraphPoint* in_pCrossfadingCurve,
	AkUInt32 in_ulCrossfadingCurveSize
)
{
	AKRESULT eResult = AK_Fail;

	CAssociatedChildData* pAssoc = NULL;

	// Get/Create the assoc
	AssociatedChildMap::Iterator itFind = m_assocs.FindEx( in_ChildID );
	if ( itFind != m_assocs.End() )
	{
		// The assoc already exists, we will just update it
		pAssoc = &((*itFind).item);
		eResult = AK_Success;
	}
	else
	{
		// Create a new Assoc
		pAssoc = m_assocs.Set( in_ChildID );
		if ( pAssoc )
		{
			// Initialize the Assoc
			eResult = pAssoc->Init( this, in_ChildID );
			if ( eResult != AK_Success )
			{
				// Forget it
				m_assocs.Unset( in_ChildID );
				pAssoc = NULL;
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}

	if ( eResult == AK_Success )
	{
		// Set the new curve (this will first clear the existing curve if any)
		if ( in_ulCrossfadingCurveSize > 0 )
			eResult = pAssoc->m_fadeCurve.Set( in_pCrossfadingCurve, in_ulCrossfadingCurveSize, AkCurveScaling_None );
		else
			pAssoc->m_fadeCurve.Unset();

		// If this failed, then we just don't have a crossfading curve for that child, but
		// we still have the assoc (so RTPCs etc will work)

		// Notify even if Set() failed, since any existing curve was lost
		if ( pAssoc->m_pChild )
			pAssoc->m_pChild->RecalcNotification();
	}

	return eResult;
}

AKRESULT CAkLayer::UnsetChildAssoc(
	AkUniqueID in_ChildID 
)
{
	AKRESULT eResult = AK_InvalidInstanceID;

	AssociatedChildMap::Iterator itFind = m_assocs.FindEx( in_ChildID );
	if ( itFind != m_assocs.End() )
	{
		eResult = (*itFind).item.Term( this );
		m_assocs.Erase( itFind );
	}

	return eResult;
}

void CAkLayer::SetOwner( CAkLayerCntr* in_pOwner )
{
	AKASSERT( ! m_pOwner || ! in_pOwner );

	if ( m_pOwner )
	{
		// Disconnect all assocs
		for( AssociatedChildMap::Iterator iter = m_assocs.Begin(); iter != m_assocs.End(); ++iter )
		{
			(*iter).item.ClearChildPtr( this );
		}
	}

	m_pOwner = in_pOwner;

	if ( m_pOwner )
	{
		// Try to connect all assocs
		for( AssociatedChildMap::Iterator iter = m_assocs.Begin(); iter != m_assocs.End(); ++iter )
		{
			(*iter).item.UpdateChildPtr( this );
		}
	}
}

void CAkLayer::UpdateChildPtr( AkUniqueID in_ChildID )
{
	AssociatedChildMap::Iterator itFind = m_assocs.FindEx( in_ChildID );
	if ( itFind != m_assocs.End() )
	{
		(*itFind).item.UpdateChildPtr( this );
	}
}

void CAkLayer::ClearChildPtr( AkUniqueID in_ChildID )
{
	AssociatedChildMap::Iterator itFind = m_assocs.FindEx( in_ChildID );
	if ( itFind != m_assocs.End() )
	{
		(*itFind).item.ClearChildPtr( this );
	}
}

AKRESULT CAkLayer::SetCrossfadingRTPC( AkRtpcID in_rtpcID )
{
	AKRESULT eResult = AK_Success;

	if ( m_crossfadingRTPCID != in_rtpcID )
	{
		if ( m_crossfadingRTPCID )
			g_pRTPCMgr->UnregisterLayer( this, m_crossfadingRTPCID );

		m_crossfadingRTPCID = in_rtpcID;

		if ( m_crossfadingRTPCID )
		{
			eResult = g_pRTPCMgr->RegisterLayer( this, m_crossfadingRTPCID );

			if ( eResult != AK_Success )
				m_crossfadingRTPCID = 0;
		}

		RecalcNotification();
	}

	return eResult;
}

struct _LayerMuteStruct
{
	AkMutedMapItem item;
	CAkConversionTable<AkRTPCGraphPoint, AkReal32> * pCurve;
	AkReal32 fValue;
	AkReal32 fMute;
	bool bMuteCalculated;
};

static void _LayerMuteFunc(
  	CAkPBI * in_pPBI,
	CAkRegisteredObj *,
	void * in_pCookie 
	)
{
	_LayerMuteStruct * pMuteStruct = (_LayerMuteStruct *) in_pCookie;

	if ( !pMuteStruct->bMuteCalculated )
	{
		pMuteStruct->fMute = pMuteStruct->pCurve->Convert( pMuteStruct->fValue );
		pMuteStruct->bMuteCalculated = true;
	}
	in_pPBI->MuteNotification( pMuteStruct->fMute, pMuteStruct->item, true );
}

void CAkLayer::OnRTPCChanged( CAkRegisteredObj * in_GameObjPtr, AkReal32 in_fValue )
{
	// We register for notifications only when we have a m_crossfadingRTPCID, so
	// this should never be called unless we do have a valid m_crossfadingRTPCID.
	AKASSERT( m_crossfadingRTPCID && g_pRTPCMgr );

	if ( ! m_assocs.IsEmpty() )
	{
		_LayerMuteStruct muteStruct;
        muteStruct.item.m_bIsPersistent = false;
		muteStruct.item.m_bIsGlobal = ( in_GameObjPtr == NULL );
		muteStruct.item.m_Identifier = this;
		muteStruct.fValue = in_fValue;

		// Update the muting level for all our associated objects, based on the new
		// value of the Game Parameter.
		for( AssociatedChildMap::Iterator iter = m_assocs.Begin(); iter != m_assocs.End(); ++iter )
		{
			if ( iter.pItem->item.m_pChild && iter.pItem->item.m_pChild->IsPlaying() )
			{
				muteStruct.bMuteCalculated = false;
				muteStruct.pCurve = &iter.pItem->item.m_fadeCurve;

				iter.pItem->item.m_pChild->ForAllPBI(
					_LayerMuteFunc,
					in_GameObjPtr,
					&muteStruct );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CAkLayer::CAssociatedChildData
//////////////////////////////////////////////////////////////////////

CAkLayer::CAssociatedChildData::CAssociatedChildData()
	: m_ulChildID( 0 )
	, m_pChild( NULL )
{
}

CAkLayer::CAssociatedChildData::~CAssociatedChildData()
{
}

AKRESULT CAkLayer::CAssociatedChildData::Init( CAkLayer* in_pLayer, AkUniqueID in_ulAssociatedChildID )
{
	AKASSERT( ! m_pChild && ! m_ulChildID );

	// Remember the object's ID
	m_ulChildID = in_ulAssociatedChildID;

	// Try to get a pointer to the object. It's Ok if it doesn't exist yet,
	// maybe UpdateChildPtr() will be called again layer. Note that
	// UpdateChildPtr() doesn't fail if the object doesn't exist because
	// it's not an error.
	return UpdateChildPtr( in_pLayer );
}

AKRESULT CAkLayer::CAssociatedChildData::Term( CAkLayer* in_pLayer )
{
	AKRESULT eResult = ClearChildPtr( in_pLayer );

	// Forget the object
	m_pChild = NULL;
	m_ulChildID = 0;

	m_fadeCurve.Unset();

	return eResult;
}

AKRESULT CAkLayer::CAssociatedChildData::UpdateChildPtr( CAkLayer* in_pLayer )
{
	AKASSERT( in_pLayer && m_ulChildID );

	if ( m_pChild )
	{
		// Nothing to do
		AKASSERT( m_pChild->ID() == m_ulChildID );
		return AK_Success;
	}

	// We start with AK_Success because we don't want to fail if
	// the object doesn't exist -- that's a normal case (for example if
	// loading Soundbanks that contain only some of the children of
	// a container).
	AKRESULT eResult = AK_Success;

	CAkParameterNodeBase* pAudioNode = g_pIndex->GetNodePtrAndAddRef( m_ulChildID, AkNodeType_Default );
	if ( pAudioNode )
	{
		eResult = in_pLayer->CanAssociateChild( pAudioNode );
		if ( eResult == AK_Success )
		{
			// We can safely cast since all associated objects are children
			// of a Layer Container, and they are all CAkParameterNodes. The
			// only exception would be if there's an ID mismatch and pAudioNode is
			// not the expected object, but this might happen only if Soundbanks
			// are really messed up.
			m_pChild = static_cast<CAkParameterNode*>( pAudioNode );

			eResult = m_pChild->AssociateLayer( in_pLayer );
			if ( eResult != AK_Success )
			{
				m_pChild = NULL;
			}
		}
		else if ( eResult == AK_PartialSuccess )
		{
			// The child might be valid, but the layer doesn't have an owner yet
			// so we can't really tell. We'll just wait until the Layer has an
			// owner and we'll try again.
			eResult = AK_Success;
		}
		pAudioNode->Release();
	}

	return eResult;
}

AKRESULT CAkLayer::CAssociatedChildData::ClearChildPtr( CAkLayer* in_pLayer )
{
	AKRESULT eResult = AK_Success;

	if ( m_pChild )
	{
		eResult = m_pChild->DissociateLayer( in_pLayer );
		m_pChild = NULL;
	}

	return eResult;
}

