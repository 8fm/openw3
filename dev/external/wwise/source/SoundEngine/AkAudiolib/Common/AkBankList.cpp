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
// AkBankList.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBankList.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkBankMgr.h"

CAkLock CAkBankList::m_BankListLock;

void CAkBankList::Init()
{
	m_ListLoadedBanks.Init();
}

void CAkBankList::Term()
{
	m_ListLoadedBanks.Term();
}

void CAkBankList::Set( AkBankKey in_BankKey, CAkUsageSlot* in_pSlot )
{
	AkAutoLock<CAkBankList> BankListGate( *this );

	CAkUsageSlot* pExistingSlot = m_ListLoadedBanks.Exists( in_BankKey );
	if ( pExistingSlot )
	{
		AKASSERT( pExistingSlot == in_pSlot ); // should never overwrite a different assignment here
		return;
	}

	in_pSlot->key = in_BankKey;
	m_ListLoadedBanks.Set( in_pSlot );
}

CAkUsageSlot * CAkBankList::Get( const AkBankKey in_BankKey )
{
	AkAutoLock<CAkBankList> BankListGate( *this );

	return m_ListLoadedBanks.Exists( in_BankKey );
}

void CAkBankList::Remove( AkBankKey in_BankKey )
{ 
	AkAutoLock<CAkBankList> BankListGate( *this );

	m_ListLoadedBanks.Unset( in_BankKey );
}
