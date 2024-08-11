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
// AkBankCallbackMgr.h
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <AK/Tools/Common/AkLock.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include "AkKeyArray.h"
#include "AkManualEvent.h"

class CAkBankCallbackMgr
{
private:
	struct BankCallbackItem
	{
		BankCallbackItem()
			:m_cookieCount(1)
			,m_toSkipCount(0)
		{}
		AkUInt32 m_cookieCount;
		AkUInt32 m_toSkipCount;// required to prevent the weird situation where a cookie would be re-used while still being referenced in the system.
	};

public:

	CAkBankCallbackMgr();
	~CAkBankCallbackMgr();

public:
	// Either increment the cookie ref count or add the entry with an initial count of 1.
	AKRESULT AddCookie( void* in_cookie );

	// Called if the cookie was added succesfully but for some reason the callback will not occur.
	void RemoveOneCookie( void* in_cookie );

	// Safe way to actually do the callback
	// Same prototype than the callback itself, with the exception that it may actually not do the callback if the
	// event was cancelled
	void DoCallback(
		AkBankCallbackFunc	in_pfnBankCallback,
		AkBankID			in_bankID,
		const void *		in_pInMemoryPtr,
		AKRESULT			in_eLoadResult,
		AkMemPoolId			in_memPoolId,
		void *				in_pCookie
		);

	void CancelCookie( void* in_cookie );

private:
	typedef CAkKeyArray<void *, BankCallbackItem> AkListCookies;
	AkListCookies m_ListCookies;

	CAkLock m_csLock;
	AkManualEvent m_CallbackActiveEvent;
};
