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
#ifndef PROXYCENTRAL_CONNECTED

//////////////////////////////////////////////////////////////////////
//
// LayerProxyLocal.cpp
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////


#include "LayerProxyLocal.h"

#include "AkLayer.h"
#include "AkAudioLib.h"
#include "AkRegistryMgr.h"
#include "AkCritical.h"


LayerProxyLocal::LayerProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Layer );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkLayer::Create( in_id ) );
}

void LayerProxyLocal::SetRTPC(
	AkRtpcID in_RTPC_ID,
	AkRTPC_ParameterID in_ParamID,
	AkUniqueID in_RTPCCurveID,
	AkCurveScaling in_eScaling,
	AkRTPCGraphPoint* in_pArrayConversion,
	AkUInt32 in_ulConversionArraySize
)
{
	CAkLayer* pLayer = static_cast<CAkLayer*>( GetIndexable() );
	if( pLayer )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pLayer->SetRTPC( in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
}

void LayerProxyLocal::UnsetRTPC(
	AkRTPC_ParameterID in_ParamID,
	AkUniqueID in_RTPCCurveID
)
{
	CAkLayer* pLayer = static_cast<CAkLayer*>( GetIndexable() );
	if( pLayer )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pLayer->UnsetRTPC( in_ParamID, in_RTPCCurveID );
	}
}

void LayerProxyLocal::SetChildAssoc(
	AkUniqueID in_ChildID,
	AkRTPCGraphPoint* in_pCrossfadingCurve,	// NULL if none
	AkUInt32 in_ulCrossfadingCurveSize		// 0 if none
)
{
	CAkLayer* pLayer = static_cast<CAkLayer*>( GetIndexable() );
	if ( pLayer )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pLayer->SetChildAssoc( in_ChildID, in_pCrossfadingCurve, in_ulCrossfadingCurveSize );
	}
}

void LayerProxyLocal::UnsetChildAssoc(
	AkUniqueID in_ChildID 
)
{
	CAkLayer* pLayer = static_cast<CAkLayer*>( GetIndexable() );
	if ( pLayer )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pLayer->UnsetChildAssoc( in_ChildID );
	}
}

void LayerProxyLocal::SetCrossfadingRTPC(
	AkRtpcID in_rtpcID
)
{
	CAkLayer* pLayer = static_cast<CAkLayer*>( GetIndexable() );
	if ( pLayer )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pLayer->SetCrossfadingRTPC( in_rtpcID );
	}
}

#endif
#endif // #ifndef AK_OPTIMIZED
