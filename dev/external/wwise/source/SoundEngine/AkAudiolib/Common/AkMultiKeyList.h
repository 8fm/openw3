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
// AkMultiKeyList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MULTI_KEYLIST_H_
#define _MULTI_KEYLIST_H_

#include "AkKeyList.h"

template <class T_KEY, class T_ITEM, ListAllocFlag TAllocFlag> 
class CAkMultiKeyList : public CAkKeyList< T_KEY, T_ITEM, TAllocFlag>
{
public:
//====================================================================================================
// In development
//====================================================================================================
	AKRESULT Insert( T_KEY in_Key, T_ITEM& in_rItem )
	{
		this->EnsureInitialized();

		// have we got space for a new one ?
		if( this->CanAddOne() )
		{
			// add it in the next free spot
			this->m_pFree->Item.key = in_Key;
			this->m_pFree->Item.item = in_rItem;

			if( this->m_pFirst == NULL )
			{
				this->m_pLast = this->m_pFree;

				// move to the next free one
				this->m_pFree = this->m_pFree->pNextListItem;
				// put the new one before the first one
				this->m_pLast->pNextListItem = NULL;
				// the new one becomes our new first one
				this->m_pFirst = this->m_pLast;
			}
			else
			{
				// look for the requested spot
				typename CAkMultiKeyList<T_KEY, T_ITEM, TAllocFlag>::ListItem*	pListItem = this->m_pFirst;
				typename CAkMultiKeyList<T_KEY, T_ITEM, TAllocFlag>::ListItem*	pPrevListItem = NULL;

				while( ( pListItem != NULL ) && ( pListItem->Item.key <= in_Key ) )
				{
					// move to next one
					pPrevListItem = pListItem;
					pListItem = pListItem->pNextListItem;
				}
				
				typename CAkMultiKeyList<T_KEY, T_ITEM, TAllocFlag>::ListItem* pNewITem = this->m_pFree;

				if( !pListItem )
				{
					this->m_pLast = this->m_pFree;
				}

				if( pPrevListItem )
				{
					pPrevListItem->pNextListItem = pNewITem;
				}
				else
				{
					this->m_pFirst = pNewITem;
				}

				this->m_pFree = this->m_pFree->pNextListItem;
				pNewITem->pNextListItem = pListItem;
			}

			++this->m_ulNumListItems;

			return AK_Success;
		}
		return AK_Fail;
	}

	AKRESULT Remove( T_KEY in_Key, T_ITEM& in_rItem )
	{
		this->EnsureInitialized();

		typename CAkMultiKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx it = this->BeginEx();
		for ( ; it != this->End(); ++it )
		{
			if ( (*it).key == in_Key && (*it).item == in_rItem )
			{
				this->Erase( it );
				return AK_Success;
			}
		}

		return AK_Fail;
	}
};

#endif //_MULTI_KEYLIST_H_
