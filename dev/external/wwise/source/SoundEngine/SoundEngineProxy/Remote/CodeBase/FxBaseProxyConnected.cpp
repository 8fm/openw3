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

#include "FxBaseProxyConnected.h"
#include "AkFxBase.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

FxBaseProxyConnected::FxBaseProxyConnected( AkUniqueID in_id, bool in_bShareSet )
{
	CAkIndexable * pIndexable;

	if ( in_bShareSet )
	{
		pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_FxShareSet );
		if ( !pIndexable )
			pIndexable = CAkFxShareSet::Create( in_id );
	}
	else
	{
		pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_FxCustom );
		if ( !pIndexable )
			pIndexable = CAkFxCustom::Create( in_id );
	}

	SetIndexable( pIndexable );
}

FxBaseProxyConnected::~FxBaseProxyConnected()
{
}

void FxBaseProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkFxBase * pFX = static_cast<CAkFxBase *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IFxBaseProxy::MethodSetFX:
		{
			FxBaseProxyCommandData::SetFX setfx;
			if (in_rSerializer.Get( setfx ))
				pFX->SetFX( setfx.m_param1, NULL, 0 );
			break;
		}

	case IFxBaseProxy::MethodSetFXParam:
		{
			FxBaseProxyCommandData::SetFXParam setfxparam;
			if (in_rSerializer.Get( setfxparam ))
				pFX->SetFXParam( setfxparam.m_param2, setfxparam.m_param1.pBlob, setfxparam.m_param1.uBlobSize );
			break;
		}

	case IFxBaseProxy::MethodSetRTPC:
		{
			ParameterableProxyCommandData::SetRTPC setRTPC;
			if (in_rSerializer.Get( setRTPC ))
				pFX->SetRTPC( setRTPC.m_RTPCID, setRTPC.m_paramID, setRTPC.m_RTPCCurveID, setRTPC.m_eScaling, setRTPC.m_pArrayConversion, setRTPC.m_ulConversionArraySize );
			break;
		}

	case IFxBaseProxy::MethodUnsetRTPC:
		{
			FxBaseProxyCommandData::UnsetRTPC unsetrtpc;
			if (in_rSerializer.Get( unsetrtpc ))
				pFX->UnsetRTPC( (AkRTPC_ParameterID) unsetrtpc.m_param1, unsetrtpc.m_param2 );
			break;
		}

	case IFxBaseProxy::MethodSetMediaID:
		{
			// Not implemented
			break;
		}

	default:
		AKASSERT( false );
	}
}
#endif // #ifndef AK_OPTIMIZED
