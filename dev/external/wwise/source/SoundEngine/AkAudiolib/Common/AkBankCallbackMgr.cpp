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
// AkBankCallbackMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkBankCallbackMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkBankMgr.h"

CAkBankCallbackMgr::CAkBankCallbackMgr()
{
	m_CallbackActiveEvent.Init();
}

CAkBankCallbackMgr::~CAkBankCallbackMgr()
{
	m_ListCookies.Term();
}

AKRESULT CAkBankCallbackMgr::AddCookie( void* in_cookie )
{
	AkAutoLock<CAkLock> gate(m_csLock);
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		++(pItem->m_cookieCount);
	}
	else
	{
		if( !m_ListCookies.Set( in_cookie ) )
			return AK_InsufficientMemory;
	}
	return AK_Success;
}

void CAkBankCallbackMgr::RemoveOneCookie( void* in_cookie )
{
	m_csLock.Lock();
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		if( pItem->m_cookieCount <=1 )
		{
			m_ListCookies.Unset( in_cookie );
		}
		else
		{
			--(pItem->m_cookieCount);
		}
	}

	m_csLock.Unlock();
	if (CAkBankMgr::GetThreadID() != AKPLATFORM::CurrentThread())
		m_CallbackActiveEvent.Wait();	//Wait until any callback are done in other threads.
}

void CAkBankCallbackMgr::DoCallback( 
		AkBankCallbackFunc	in_pfnBankCallback,
		AkBankID			in_bankID,
		const void *		in_pInMemoryPtr,
		AKRESULT			in_eLoadResult,
		AkMemPoolId			in_memPoolId,
		void *				in_pCookie
		)
{
	if( in_pfnBankCallback )
	{
		m_csLock.Lock();
		BankCallbackItem* pItem = m_ListCookies.Exists( in_pCookie );
		if( pItem )
		{
			bool bNeedToSkip = pItem->m_toSkipCount != 0;

			if( pItem->m_cookieCount <=1 )
				m_ListCookies.Unset( in_pCookie );
			else
			{
				--(pItem->m_cookieCount);
				if( bNeedToSkip )
					--(pItem->m_toSkipCount);
			}

			m_CallbackActiveEvent.Reset();
			m_csLock.Unlock();

			if( !bNeedToSkip )
			{
				in_pfnBankCallback( in_bankID, in_pInMemoryPtr, in_eLoadResult, in_memPoolId, in_pCookie );
			}
			m_CallbackActiveEvent.Signal();
		}
		else
			m_csLock.Unlock();
	}
}

void CAkBankCallbackMgr::CancelCookie( void* in_cookie )
{
	m_csLock.Lock();
	BankCallbackItem* pItem = m_ListCookies.Exists( in_cookie );
	if( pItem )
	{
		// set the count over the number of callback that will have to be skipped.
		pItem->m_toSkipCount = pItem->m_cookieCount;		
	}

	m_csLock.Unlock();
	if (CAkBankMgr::GetThreadID() != AKPLATFORM::CurrentThread())
		m_CallbackActiveEvent.Wait();	//Wait until any callback are done in other threads.
}
