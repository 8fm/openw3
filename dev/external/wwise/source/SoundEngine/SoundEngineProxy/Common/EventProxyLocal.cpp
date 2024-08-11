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

#include "EventProxyLocal.h"

#include "AkAudioLib.h"
#include "AkCritical.h"
#include "AkEvent.h"

EventProxyLocal::EventProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Event );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkEvent::Create( in_id ) );
}

EventProxyLocal::~EventProxyLocal()
{
}

void EventProxyLocal::Add( AkUniqueID in_actionID )
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->Add( in_actionID );
	}
}

void EventProxyLocal::Remove( AkUniqueID in_actionID )
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->Remove( in_actionID );
	}
}

void EventProxyLocal::Clear()
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->Clear();
	}
}

#endif // #ifndef PROXYCENTRAL_CONNECTED
#endif // #ifndef AK_OPTIMIZED
