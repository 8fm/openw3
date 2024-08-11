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
#ifndef AK_OPTIMIZED

//////////////////////////////////////////////////////////////////////
//
// LayerProxyConnected.cpp
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "LayerProxyConnected.h"
#include "AkLayer.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "ILayerProxy.h"

LayerProxyConnected::LayerProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Layer );
	if ( !pIndexable )
		pIndexable = CAkLayer::Create( in_id );

	SetIndexable( pIndexable );
}

void LayerProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkLayer * pLayer = static_cast<CAkLayer *>( GetIndexable() );

	switch( in_uMethodID )
	{
		case ILayerProxy::MethodSetRTPC:
			{
				LayerProxyCommandData::SetRTPC setRTPC;
				if( in_rSerializer.Get( setRTPC ) )
					pLayer->SetRTPC( setRTPC.m_RTPCID, setRTPC.m_paramID, setRTPC.m_RTPCCurveID, setRTPC.m_eScaling, setRTPC.m_pArrayConversion, setRTPC.m_ulConversionArraySize );

				break;
			}

		case ILayerProxy::MethodUnsetRTPC:
			{
				LayerProxyCommandData::UnsetRTPC unsetRTPC;
				if( in_rSerializer.Get( unsetRTPC ) )
					pLayer->UnsetRTPC( unsetRTPC.m_paramID, unsetRTPC.m_RTPCCurveID );

				break;
			}

		case ILayerProxy::MethodSetChildAssoc:
			{
				LayerProxyCommandData::SetChildAssoc setChildAssoc;
				if( in_rSerializer.Get( setChildAssoc ) )
					pLayer->SetChildAssoc( setChildAssoc.m_ChildID, setChildAssoc.m_pCrossfadingCurve, setChildAssoc.m_ulCrossfadingCurveSize );

				break;
			}

		case ILayerProxy::MethodUnsetChildAssoc:
			{
				LayerProxyCommandData::UnsetChildAssoc clearChildAssoc;
				if( in_rSerializer.Get( clearChildAssoc ) )
					pLayer->UnsetChildAssoc( clearChildAssoc.m_ChildID );

				break;
			}

		case ILayerProxy::MethodSetCrossfadingRTPC:
			{
				LayerProxyCommandData::SetCrossfadingRTPC setCrossfadingRTPC;
				if( in_rSerializer.Get( setCrossfadingRTPC ) )
					pLayer->SetCrossfadingRTPC( setCrossfadingRTPC.m_rtpcID );

				break;
			}

		default:
			AKASSERT( false );
	}
}

#endif // #ifndef AK_OPTIMIZED
