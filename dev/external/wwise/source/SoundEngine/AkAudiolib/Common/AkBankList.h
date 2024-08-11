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
// AkBankList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _BANK_LIST_H_
#define _BANK_LIST_H_

#include <AK/Tools/Common/AkLock.h>
#include "AkKeyArray.h"
#include "AkHashList.h"

class CAkUsageSlot;

struct AkBankKey
{
	AkBankKey()
	{}
	AkBankKey(AkBankID in_bankID, const void* in_pInMemoryPtr)
		:bankID( in_bankID )
		,pInMemoryPtr( in_pInMemoryPtr )
	{}

	// members
	AkBankID bankID;
	const void* pInMemoryPtr;

	bool operator ==(const AkBankKey& in_Op) const
	{
		return ( (bankID == in_Op.bankID) && (pInMemoryPtr == in_Op.pInMemoryPtr) );
	}
};

inline AkUIntPtr AkHash( AkBankKey in_key ) { return (AkUIntPtr) in_key.bankID + (AkUIntPtr) in_key.pInMemoryPtr; }

class CAkBankList
{
public:

	// List type declaration
	typedef AkHashListBare<AkBankKey, CAkUsageSlot, AK_SMALL_HASH_SIZE> AkListLoadedBanks;

	// Should be called only when terminating the SoundBank Manager
	void Init();
	void Term();

	static inline void Lock() { m_BankListLock.Lock(); }
	static inline void Unlock() { m_BankListLock.Unlock(); }

	// Safe access to the list.
	void			Set( AkBankKey in_BankKey, CAkUsageSlot* in_pSlot );
	CAkUsageSlot *	Get( const AkBankKey in_BankKey );
	void			Remove( AkBankKey in_BankKey );

	// Must be used while all the threads stopped running.
	inline CAkBankList::AkListLoadedBanks& GetUNSAFEBankListRef() { return m_ListLoadedBanks; }

private:
	static CAkLock m_BankListLock;

	AkListLoadedBanks m_ListLoadedBanks;
};

#endif //_BANK_LIST_H_
