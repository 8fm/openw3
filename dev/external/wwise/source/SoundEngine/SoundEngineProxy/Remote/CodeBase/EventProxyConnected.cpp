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

#include "EventProxyConnected.h"

#include "AkCritical.h"
#include "AkEvent.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "IEventProxy.h"

EventProxyConnected::EventProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Event );
	if ( !pIndexable )
		pIndexable = CAkEvent::Create( in_id );

	SetIndexable( pIndexable );

	//Must Clear the Actions in the Event to ensure it will totally match the content of Wwise
	CAkFunctionCritical SpaceSetAsCritical;
	if ( pIndexable )
		static_cast<CAkEvent *>( pIndexable )->Clear();
}

EventProxyConnected::~EventProxyConnected()
{
}

void EventProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkEvent * pEvent = static_cast<CAkEvent *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IEventProxy::MethodAdd:
		{
			EventProxyCommandData::Add add;
			if (in_rSerializer.Get( add ))
				pEvent->Add( add.m_param1 );

			break;
		}

	case IEventProxy::MethodRemove:
		{
			EventProxyCommandData::Remove remove;
			if( in_rSerializer.Get( remove ) )
				pEvent->Remove( remove.m_param1 );
			break;
		}

	case IEventProxy::MethodClear:
		{
			EventProxyCommandData::Clear clear;
			if( in_rSerializer.Get( clear ) )
				pEvent->Clear();
			break;
		}

	default:
		AKASSERT( false );
	}
}
#endif // #ifndef AK_OPTIMIZED
