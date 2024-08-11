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


#include "StateProxyLocal.h"

#include "AkState.h"
#include "AkAudioLib.h"
#include "AkCritical.h"


StateProxyLocal::StateProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_CustomState );
	SetIndexable( pIndexable != NULL ? pIndexable : CAkState::Create( in_id ) );
}

StateProxyLocal::~StateProxyLocal()
{
}

void StateProxyLocal::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetAkProp( in_eProp, in_fValue );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
