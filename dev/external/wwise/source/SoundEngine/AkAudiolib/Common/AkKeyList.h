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
// AkKeyList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _KEYLIST_H_
#define _KEYLIST_H_

#include "AkList2.h"
#include "AkKeyDef.h"

// The Key list is simply a list that may be referenced using a key
// NOTE : 
template <class T_KEY, class T_ITEM, ListAllocFlag TAllocFlag, class U_POOL = ArrayPoolDefault> 
class CAkKeyList : public CAkList2< MapStruct<T_KEY, T_ITEM>, const MapStruct<T_KEY, T_ITEM>&, TAllocFlag, U_POOL >
{
public:
//====================================================================================================
// Return NULL if the Key does not exisis
// Return T_ITEM* otherwise
//====================================================================================================
	T_ITEM* Exists(T_KEY in_Key)
	{
        // This assert was added for WG-15837, if you get to this assert
        // This could lead to the explanation of a problem not totally solved. 
        this->EnsureInitialized();

		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag,U_POOL>::ListItem* pCurrent = this->m_pFirst;
		while(pCurrent != NULL)
		{
			if(pCurrent->Item.key == in_Key)
			{
				return &(pCurrent->Item.item);
			}
			pCurrent = pCurrent->pNextListItem;
		}
		return NULL;
	}

public:
//====================================================================================================
// Sets the item referenced by the specified key and item
// Return NULL if the item could not be added.
//====================================================================================================
	T_ITEM * Set(T_KEY in_Key, T_ITEM in_Item)
	{
		T_ITEM* pSearchedItem = Exists(in_Key);
		if(pSearchedItem)
		{
			*pSearchedItem = in_Item;
		}
		else
		{
			//pSearchedItem = AddKeylistItem(in_Key, in_Item);
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pStruct->item = in_Item;
				pSearchedItem = &(pStruct->item);
			}
		}
		return pSearchedItem;
	}

	T_ITEM * Set(T_KEY in_Key)
	{
		T_ITEM* pSearchedItem = Exists(in_Key);
		if(!pSearchedItem)
		{
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pSearchedItem = &(pStruct->item);
			}
		}
		return pSearchedItem;
	}

	// NOTE: The real definition should be 
	// typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx FindEx(T_KEY in_Key)
	// Typenaming the base class is a workaround for bug MTWX33123 in the new Freescale CodeWarrior.
	typename CAkList2< MapStruct<T_KEY, T_ITEM>, const MapStruct<T_KEY, T_ITEM>&, TAllocFlag >::IteratorEx FindEx(T_KEY in_Key)
	{
		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx it = this->BeginEx();
		for ( ; it != this->End(); ++it )
		{
			if ( (*it).key == in_Key )
			{
				break;
			}
		}
		return it;
	}
//====================================================================================================
//	Remove the item referenced by the specified key
//====================================================================================================

	void Unset(T_KEY in_Key)
	{
		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag,U_POOL>::IteratorEx it = this->BeginEx();
		for ( ; it != this->End(); ++it )
		{
			if ( (*it).key == in_Key )
			{
				this->Erase( it );
				break;
			}
		}
	}
};

#endif //_KEYLIST_H_
