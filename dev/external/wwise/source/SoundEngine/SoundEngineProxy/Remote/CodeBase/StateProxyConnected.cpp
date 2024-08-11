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

#include "StateProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

StateProxyConnected::StateProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_CustomState );
	if ( !pIndexable )
		pIndexable = CAkState::Create( in_id );

	SetIndexable( pIndexable );
}

StateProxyConnected::~StateProxyConnected()
{
}

void StateProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkState * pState = static_cast<CAkState *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IStateProxy::MethodSetAkProp:
		{
			StateProxyCommandData::SetAkProp params;
			if( in_rSerializer.Get( params ) )
					pState->SetAkProp( (AkPropID) params.m_param1, params.m_param2 );
			break;
		}

	default:
		AKASSERT( false );
	}
}
#endif // #ifndef AK_OPTIMIZED
