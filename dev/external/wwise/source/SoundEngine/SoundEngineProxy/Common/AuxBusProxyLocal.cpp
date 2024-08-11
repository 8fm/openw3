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

#include "AuxBusProxyLocal.h"
#include "AkAuxBus.h"
#include "AkAudioLib.h"
#include "AkCritical.h"


AuxBusProxyLocal::AuxBusProxyLocal()
{
}

AuxBusProxyLocal::~AuxBusProxyLocal()
{
}

void AuxBusProxyLocal::Init( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_BusNode );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkAuxBus::Create( in_id ) );
}

#endif
#endif // #ifndef AK_OPTIMIZED
