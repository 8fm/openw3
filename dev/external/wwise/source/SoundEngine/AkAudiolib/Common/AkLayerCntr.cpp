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
// AkLayerCntr.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkLayerCntr.h"
#include "AkLayer.h"
#include "AkPBI.h"

CAkLayerCntr::CAkLayerCntr( AkUniqueID in_ulID )
	: CAkMultiPlayNode( in_ulID )
{
}

CAkLayerCntr::~CAkLayerCntr()
{
	// Cleanup: Release our Layers
	for ( LayerList::Iterator itLayer = m_layers.Begin(), itLayerEnd = m_layers.End();
		  itLayer != itLayerEnd;
		  ++itLayer )
	{
		(*itLayer)->SetOwner( NULL );
		(*itLayer)->Release();
	}

	m_layers.Term();

	CAkMultiPlayNode::Term();
}

AKRESULT CAkLayerCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = AK_Success;

	// Read ID
	{
		// We don't care about the ID, just skip it
		const AkUInt32 ulID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);
		AKASSERT( ID() == ulID );
	}

	// Read basic parameters
	eResult = SetNodeBaseParams(in_pData, in_ulDataSize, false);

	if( eResult == AK_Success )
	{
		eResult = SetChildren( in_pData, in_ulDataSize );
	}

	if(eResult == AK_Success)
	{
		// Process Layers list
		const AkUInt32 ulNumLayers = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

		for( AkUInt32 i = 0; i < ulNumLayers; ++i )
		{
			// Peek the Layer ID
			AkUniqueID ulLayerID = *(AkUniqueID*)(in_pData);

			// Create the Layer object
			CAkLayer* pLayer = CAkLayer::Create( ulLayerID );

			if( ! pLayer )
			{
				eResult = AK_Fail;
			}
			else
			{
				// Read Layer's stuff from the bank
				pLayer->SetOwner( this );
				eResult = pLayer->SetInitialValues( in_pData, in_ulDataSize );

				if( eResult != AK_Success )
				{
					pLayer->Release();
				}
				else
				{
					// Add the layer to our list
					if ( ! m_layers.AddLast( pLayer ) )
					{
						eResult = AK_Fail;
						pLayer->Release();
					}
				}
			}

			if( eResult != AK_Success )
			{
				break;
			}
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkLayerCntr::AddChild(WwiseObjectIDext in_ulID)
{
	AKRESULT eResult = __base::AddChild( in_ulID );

	if ( eResult == AK_Success )
	{
		// Maybe some layers referred to that child before it existed. Let's
		// tell them about it so they can connect to the child.
		for ( LayerList::Iterator itLayer = m_layers.Begin(), itLayerEnd = m_layers.End();
			  itLayer != itLayerEnd;
			  ++itLayer )
		{
			(*itLayer)->UpdateChildPtr( in_ulID.id );
		}
	}

	return eResult;
}

void CAkLayerCntr::RemoveChild( CAkParameterNodeBase* in_pChild )
{
	// Maybe some layers referred to that child. Let's tell it's about to go
	for ( LayerList::Iterator itLayer = m_layers.Begin(), itLayerEnd = m_layers.End();
			itLayer != itLayerEnd;
			++itLayer )
	{
		(*itLayer)->UnsetChildAssoc( in_pChild->ID() );
	}

	return __base::RemoveChild( in_pChild );
}

CAkLayerCntr* CAkLayerCntr::Create( AkUniqueID in_ulID )
{
	CAkLayerCntr* pAkLayerCntr = AkNew( g_DefaultPoolId, CAkLayerCntr( in_ulID ) );
	if( pAkLayerCntr )
	{
		if( pAkLayerCntr->Init() != AK_Success )
		{
			pAkLayerCntr->Release();
			pAkLayerCntr = NULL;
		}
	}
	return pAkLayerCntr;
}

AkNodeCategory CAkLayerCntr::NodeCategory()
{
	return AkNodeCategory_LayerCntr;
}

AKRESULT CAkLayerCntr::PlayInternal( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Success;

    bool l_bIsContinuousPlay = in_rPBIParams.eType != AkPBIParams::PBI;

	// We must use this backup since there may be multiple playback, and this field will be overriden if multiple children are played.
	SafeContinuationList safeContList( in_rPBIParams, this );
	AkUInt32 l_ulNumSoundsStarted = 0;

	// Simply play each child
	AkUInt32 numPlayable = this->m_mapChildId.Length();
	for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
	{
		CAkParameterNodeBase* pNode = (*iter);
		AkPBIParams params = in_rPBIParams; // ensure that each play is truly independent

		if( !l_bIsContinuousPlay )
		{
			eResult = static_cast<CAkParameterNode*>(pNode)->Play( params );
		}
		else
		{
			AkContParamsAndPath continuousParams( params.pContinuousParams );

			// Cross fade on blend containers should be possible only when there is only one sound in the actual object, 
			// otherwise, we do the normal behavior
			if( numPlayable == 1 )
			{
                ContGetList( params.pContinuousParams->spContList, continuousParams.Get().spContList );
				eResult = AK_Success;
			}
			else
			{
				params.sequenceID = AK_INVALID_SEQUENCE_ID;//WG-16875 - Must prevent multi playback sounds from being sample accurate, they will default as using the normal transition system.

				continuousParams.Get().spContList.Attach( CAkContinuationList::Create() );
				
				if( continuousParams.Get().spContList )
					eResult = AddMultiplayItem( continuousParams, params, safeContList );
				else
					eResult = AK_InsufficientMemory;
			}

			if( eResult == AK_Success )
			{
                params.pContinuousParams = &continuousParams.Get();
				eResult = static_cast<CAkParameterNode*>(pNode)->Play( params );
			}
		}

		if( eResult == AK_Success )
		{
			++l_ulNumSoundsStarted;
		}
	}
	if( l_ulNumSoundsStarted >= 1 )
	{
		// Fixing WG-15390
		// we ignore errors on purpose here, if at least one children did play something, we call it a success.
		eResult = AK_Success;
	}

	if( l_bIsContinuousPlay )
	{
		if( l_ulNumSoundsStarted == 0 )
		{
			eResult = PlayAndContinueAlternateMultiPlay( in_rPBIParams );
		}
		else
		{
			if( safeContList.Get() )
			{
				eResult = ContUnrefList( safeContList.Get() );
			}
		}
	}

	return eResult;
}

AKRESULT CAkLayerCntr::AddLayer(
	AkUniqueID in_LayerID
)
{
	// Get the Layer object
	CAkLayer* pLayer = g_pIndex->m_idxLayers.GetPtrAndAddRef( in_LayerID );
	if ( !pLayer )
		return AK_IDNotFound;

	// When the object has been loaded from a bank, and is then
	// packaged by the WAL, it's possible that AddLayer is called
	// twice for the same layer, so let's make sure it's a new one
	if ( m_layers.FindEx( pLayer ) != m_layers.End() )
	{
		pLayer->Release();
		return AK_Success;
	}

	// Add the layer to our list
	if ( ! m_layers.AddLast( pLayer ) )
	{
		pLayer->Release();
		return AK_Fail;
	}

	pLayer->SetOwner( this );
	// keeping addref'd pLayer

	return AK_Success;
}

AKRESULT CAkLayerCntr::RemoveLayer(
	AkUniqueID in_LayerID 
)
{
	// Get the Layer object
	CAkLayer* pLayer = g_pIndex->m_idxLayers.GetPtrAndAddRef( in_LayerID );
	if ( !pLayer )
		return AK_IDNotFound;

	// Remove it from the list
	AKRESULT eResult = m_layers.Remove( pLayer );

	if ( eResult == AK_Success )
	{
		pLayer->SetOwner( NULL );
		pLayer->Release();
	}

	pLayer->Release();

	return eResult;
}
