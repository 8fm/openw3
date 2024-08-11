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


#ifdef AK_MOTION

#include "FeedbackBusProxyLocal.h"
#include "AkFeedbackBus.h"
#include "AkAudioLib.h"
#include "AkCritical.h"


FeedbackBusProxyLocal::FeedbackBusProxyLocal()
{
}

FeedbackBusProxyLocal::~FeedbackBusProxyLocal()
{
}

void FeedbackBusProxyLocal::Init( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_BusNode );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkFeedbackBus::Create( in_id ) );
}

AkUniqueID FeedbackBusProxyLocal::GetID() const
{
	if (GetIndexable() == NULL)
		return 0;

	return BusProxyLocal::GetID();
}
#endif
#endif
#endif // #ifndef AK_OPTIMIZED
