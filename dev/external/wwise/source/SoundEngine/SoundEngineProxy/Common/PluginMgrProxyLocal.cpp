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


#include "PluginMgrProxyLocal.h"

#include "AkAudioLib.h"

#ifdef AK_WIN
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif

PluginMgrProxyLocal::PluginMgrProxyLocal()
{
}

PluginMgrProxyLocal::~PluginMgrProxyLocal()
{
}

AKRESULT PluginMgrProxyLocal::RegisterPlugin( 
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID, 
	AkUInt32 in_ulPluginID,  
	AkCreatePluginCallback in_pCreateFunc,
	AkCreateParamCallback in_pCreateParamFunc 
	) const
{
    if ( !AK::SoundEngine::IsInitialized() )
    {
        AKASSERT( !"Sound engine not initialized" );
        return AK_Fail;
    }

	return AK::SoundEngine::RegisterPlugin( in_eType, in_ulCompanyID, in_ulPluginID, in_pCreateFunc, in_pCreateParamFunc );
}
#endif // #ifndef AK_OPTIMIZED
