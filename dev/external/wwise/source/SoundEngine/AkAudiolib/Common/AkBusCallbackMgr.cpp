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

//////////////////////////////////////////////////////////////////////
//
// AkBusCallbackMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkBusCallbackMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkLEngine.h"

CAkBusCallbackMgr::CAkBusCallbackMgr()
{
}

CAkBusCallbackMgr::~CAkBusCallbackMgr()
{
	m_ListCallbacks.Term();
}

AKRESULT CAkBusCallbackMgr::SetCallback( AkUniqueID in_busID, AkBusCallbackFunc in_pfnCallback )
{
	{
		AkAutoLock<CAkLock> gate(m_csLock);
		if ( in_pfnCallback )
		{
			AkBusCallbackFunc* pItem = m_ListCallbacks.Exists( in_busID );
			if( !pItem )
			{
				pItem = m_ListCallbacks.Set( in_busID );
				if( !pItem )
					return AK_InsufficientMemory;
			}
			*pItem = in_pfnCallback;
		}
		else
		{
			m_ListCallbacks.Unset( in_busID );
		}
	}
	CAkLEngine::EnableVolumeCallback( in_busID, in_pfnCallback != NULL );

	return AK_Success;
}

bool CAkBusCallbackMgr::DoCallback( AkSpeakerVolumeMatrixBusCallbackInfo& in_rCallbackInfo )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	AkBusCallbackFunc* pItem = m_ListCallbacks.Exists( in_rCallbackInfo.busID );
	if( pItem )
	{
		(*pItem)(&in_rCallbackInfo);
		return true;
	}
	return false;
}

bool CAkBusCallbackMgr::IsCallbackEnabled( AkUniqueID in_busID )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	AkBusCallbackFunc* pItem = m_ListCallbacks.Exists( in_busID );
	return pItem?true:false;
}
