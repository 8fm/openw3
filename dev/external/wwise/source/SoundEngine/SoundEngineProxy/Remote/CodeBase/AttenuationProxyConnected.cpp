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

#include "AttenuationProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "AkAttenuationMgr.h"
#include "IAttenuationProxy.h"

AttenuationProxyConnected::AttenuationProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Attenuation );
	if ( !pIndexable )
		pIndexable = CAkAttenuation::Create( in_id );

	SetIndexable( pIndexable );
}

AttenuationProxyConnected::~AttenuationProxyConnected()
{
}

void AttenuationProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkAttenuation * pAttenuation = static_cast<CAkAttenuation *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IAttenuationProxy::MethodSetAttenuationParams:
		{
			AttenuationProxyCommandData::SetAttenuationParams setAttenuationParams;
			if (in_rSerializer.Get( setAttenuationParams ))
				pAttenuation->SetAttenuationParams( setAttenuationParams.m_Params );
			break;
		}

	default:
		AKASSERT( false );
	}
}
#endif // #ifndef AK_OPTIMIZED
