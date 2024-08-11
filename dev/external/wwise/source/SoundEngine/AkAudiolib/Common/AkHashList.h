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

#ifndef _AKHASHLIST_H
#define _AKHASHLIST_H

#include "AkKeyDef.h" // for MapStruct
#include <AK/Tools/Common/AkObject.h>

// NOTE: when using this template, a hashing function of the following form must be available: 
//
// unsigned int AkHash( T_KEY );

AkForceInline unsigned int AkHash( AkUIntPtr in_key ) { return (unsigned int) in_key; }

template < class T_KEY, class T_ITEM, AkUInt32 T_HASHSIZE > 
class AkHashList
{
public:
    struct Item
	{
		Item * pNextItem;               // our next one
		MapStruct<T_KEY, T_ITEM> Assoc;	// key-item association
	};

	struct Iterator
	{
		Item ** pTable;
		unsigned int uiTable;
		Item* pItem;

		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = pItem->pNextItem;
			
			while ( ( pItem == NULL ) && ( ++uiTable < T_HASHSIZE ) )
				pItem = pTable[ uiTable ];

			return *this;
		}

		inline MapStruct<T_KEY, T_ITEM>& operator*()
		{
			AKASSERT( pItem );
			return pItem->Assoc;
		}

		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}		
	};

	// The IteratorEx iterator is intended for usage when a possible erase may occurs
	// when simply iterating trough a list, use the simple Iterator, it is faster and lighter.
	struct IteratorEx : public Iterator
	{
		Item* pPrevItem;

		IteratorEx& operator++()
		{
			AKASSERT( this->pItem );
			
			pPrevItem = this->pItem;
			this->pItem = this->pItem->pNextItem;
			
			while ( ( this->pItem == NULL ) && ( ++this->uiTable < T_HASHSIZE ) )
			{
				pPrevItem = NULL;
				this->pItem = this->pTable[ this->uiTable ];
			}

			return *this;
		}
	};

	Iterator Begin()
	{
		Iterator returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = 0;
		returnedIt.pItem = m_table[ 0 ];
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
			returnedIt.pItem = m_table[ returnedIt.uiTable ];

		return returnedIt;
	}

	IteratorEx BeginEx()
	{
		IteratorEx returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = 0;
		returnedIt.pItem = m_table[ 0 ];
		returnedIt.pPrevItem = NULL;
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
			returnedIt.pItem = m_table[ returnedIt.uiTable ];

		return returnedIt;
	}

	inline Iterator End()
	{
		Iterator returnedIt;
		returnedIt.pItem = NULL;
		return returnedIt;
	}

	IteratorEx FindEx( T_KEY in_Key )
	{
		IteratorEx returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = AkHash( in_Key ) % T_HASHSIZE;
		returnedIt.pItem = m_table[ returnedIt.uiTable ];
		returnedIt.pPrevItem = NULL;

		while ( returnedIt.pItem != NULL )
		{
			if (returnedIt.pItem->Assoc.key == in_Key )
				break;

			returnedIt.pPrevItem = returnedIt.pItem;
			returnedIt.pItem = returnedIt.pItem->pNextItem;
		}

		return returnedIt;
	}

	AkHashList()
		: m_MemPoolId( AK_INVALID_POOL_ID )
		, m_uiSize( 0 )
	{
	}

	~AkHashList()
	{
		AKASSERT( m_MemPoolId == AK_INVALID_POOL_ID );
	}

	bool IsInitialized()
	{
		return m_MemPoolId != AK_INVALID_POOL_ID;
	}

	void Init( AkMemPoolId in_MemPoolId )
	{
		m_MemPoolId = in_MemPoolId;
		m_uiSize = 0;

		for ( unsigned int i = 0; i < T_HASHSIZE; ++i )
			m_table[ i ] = NULL;
	}

	void Term()
	{
		if ( m_MemPoolId != AK_INVALID_POOL_ID )
		{
			RemoveAll();

			m_MemPoolId = AK_INVALID_POOL_ID;
		}
	}

	void RemoveAll()
	{
		for ( unsigned int i = 0; i < T_HASHSIZE; ++i )
		{
			Item * pItem = m_table[ i ];
			while ( pItem != NULL )
			{
				Item * pNextItem = pItem->pNextItem;
				pItem->Assoc.item.~T_ITEM();
				AkFree( m_MemPoolId, pItem );
				pItem = pNextItem;
			}
			
			m_table[ i ] = NULL;
		}

		m_uiSize = 0;
	}

	T_ITEM * Exists( T_KEY in_Key )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		AkUIntPtr uiTable = AkHash( in_Key ) % T_HASHSIZE;
		return ExistsInList( in_Key, uiTable );
	}

	// Set using an externally preallocated Item -- Hash list takes ownership of the Item.
	T_ITEM * Set( Item * in_pItem )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		unsigned int uiTable = (unsigned int) AkHash( in_pItem->Assoc.key ) % T_HASHSIZE;

		AKASSERT( !ExistsInList( in_pItem->Assoc.key, uiTable ) ); // Item must not exist in list !

		// Add new entry

		in_pItem->pNextItem = m_table[ uiTable ];
		m_table[ uiTable ] = in_pItem;

		++m_uiSize;

		return &(in_pItem->Assoc.item);
	}

	T_ITEM * Set( T_KEY in_Key )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		AkUIntPtr uiTable = AkHash( in_Key ) % T_HASHSIZE;
		T_ITEM * pItem = ExistsInList( in_Key, uiTable );
		if ( pItem ) 
			return pItem;

		return CreateEntry( in_Key, uiTable );
	}

	T_ITEM * Set( T_KEY in_Key, bool& out_bWasAlreadyThere )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		unsigned int uiTable = AkHash( in_Key ) % T_HASHSIZE;
		T_ITEM * pItem = ExistsInList( in_Key, uiTable );
		if ( pItem )
		{
			out_bWasAlreadyThere = true;
			return pItem;
		}
		else
		{
			out_bWasAlreadyThere = false;
		}

		return CreateEntry( in_Key, uiTable );
	}

	void Unset( T_KEY in_Key )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		unsigned int uiTable = (unsigned int)(AkHash( in_Key ) % T_HASHSIZE);
		Item * pItem = m_table[ uiTable ];
		Item * pPrevItem = NULL;
		while ( pItem != NULL )
		{
			if ( pItem->Assoc.key == in_Key )
				break;

			pPrevItem = pItem;
			pItem = pItem->pNextItem;
		}

		if ( pItem )
			RemoveItem( uiTable, pItem, pPrevItem );
	}

	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		IteratorEx returnedIt;
		returnedIt.pTable = in_rIter.pTable;
		returnedIt.uiTable = in_rIter.uiTable;
		returnedIt.pItem = in_rIter.pItem->pNextItem;
		returnedIt.pPrevItem = in_rIter.pPrevItem;
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
		{
			returnedIt.pPrevItem = NULL;
			returnedIt.pItem = returnedIt.pTable[ returnedIt.uiTable ];
		}
		
		RemoveItem( in_rIter.uiTable, in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

	void RemoveItem( unsigned int in_uiTable, Item* in_pItem, Item* in_pPrevItem )
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		if( in_pPrevItem ) 
			in_pPrevItem->pNextItem = in_pItem->pNextItem;
		else
			m_table[ in_uiTable ] = in_pItem->pNextItem;

		in_pItem->Assoc.item.~T_ITEM();
		AkFree( m_MemPoolId, in_pItem );

		--m_uiSize;
	}

	unsigned int Length() const
	{
		return m_uiSize;
	}

    AkMemPoolId PoolId() { return m_MemPoolId; }

protected:
	T_ITEM * ExistsInList( T_KEY in_Key, AkUIntPtr in_uiTable )
	{
		Item * pItem = m_table[ in_uiTable ];
		while ( pItem != NULL )
		{
			if ( pItem->Assoc.key == in_Key )
				return &(pItem->Assoc.item ); // found

			pItem = pItem->pNextItem;
		}

		return NULL; // not found
	}

	T_ITEM * CreateEntry( T_KEY in_Key, AkUIntPtr in_uiTable )
	{
		Item * pNewItem = (Item *) AkAlloc( m_MemPoolId, sizeof( Item ) );
		if ( pNewItem == NULL )
			return NULL;

		pNewItem->pNextItem = m_table[ in_uiTable ];
		pNewItem->Assoc.key = in_Key;

		AkPlacementNew( &(pNewItem->Assoc.item) ) T_ITEM; 

		m_table[ in_uiTable ] = pNewItem;

		++m_uiSize;

		return &(pNewItem->Assoc.item);
	}

	AkMemPoolId  m_MemPoolId;
	Item *       m_table[ T_HASHSIZE ]; // table of single-link lists
	unsigned int m_uiSize;
};

// this one lets you define the structure
// only requirement is that T_MAPSTRUCT must have members pNextItem and key.
// client is responsible for allocation/deallocation of T_MAPSTRUCTS.
template <class T_KEY, class T_MAPSTRUCT>
struct AkDefaultHashListBarePolicy
{
	static const T_KEY& Key(const T_MAPSTRUCT* in_pItem) {return in_pItem->key;}
};

template < class T_KEY, class T_MAPSTRUCT, unsigned int T_HASHSIZE, class KEY_POLICY = AkDefaultHashListBarePolicy<T_KEY, T_MAPSTRUCT> > 
class AkHashListBare
{
public:
	struct Iterator
	{
		T_MAPSTRUCT ** pTable;
		unsigned int uiTable;
		T_MAPSTRUCT* pItem;

		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = pItem->pNextItem;
			
			while ( ( pItem == NULL ) && ( ++uiTable < T_HASHSIZE ) )
				pItem = pTable[ uiTable ];

			return *this;
		}

		inline T_MAPSTRUCT * operator*()
		{
			AKASSERT( pItem );
			return pItem;
		}

		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}		
	};

	// The IteratorEx iterator is intended for usage when a possible erase may occurs
	// when simply iterating trough a list, use the simple Iterator, it is faster and lighter.
	struct IteratorEx : public Iterator
	{
		T_MAPSTRUCT* pPrevItem;

		IteratorEx& operator++()
		{
			AKASSERT( this->pItem );
			
			pPrevItem = this->pItem;
			this->pItem = this->pItem->pNextItem;
			
			while ( ( this->pItem == NULL ) && ( ++this->uiTable < T_HASHSIZE ) )
			{
				pPrevItem = NULL;
				this->pItem = this->pTable[ this->uiTable ];
			}

			return *this;
		}
	};

	Iterator Begin()
	{
		Iterator returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = 0;
		returnedIt.pItem = m_table[ 0 ];
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
			returnedIt.pItem = m_table[ returnedIt.uiTable ];

		return returnedIt;
	}

	IteratorEx BeginEx()
	{
		IteratorEx returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = 0;
		returnedIt.pItem = m_table[ 0 ];
		returnedIt.pPrevItem = NULL;
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
			returnedIt.pItem = m_table[ returnedIt.uiTable ];

		return returnedIt;
	}

	inline Iterator End()
	{
		Iterator returnedIt;
		returnedIt.pItem = NULL;
		return returnedIt;
	}

	IteratorEx FindEx( T_KEY in_Key )
	{
		IteratorEx returnedIt;

		returnedIt.pTable = m_table;
		returnedIt.uiTable = AkHash( in_Key ) % T_HASHSIZE;
		returnedIt.pItem = m_table[ returnedIt.uiTable ];
		returnedIt.pPrevItem = NULL;

		while ( returnedIt.pItem != NULL )
		{
			if (KEY_POLICY::Key(returnedIt.pItem) == in_Key )
				break;

			returnedIt.pPrevItem = returnedIt.pItem;
			returnedIt.pItem = returnedIt.pItem->pNextItem;
		}

		return returnedIt;
	}

	AkHashListBare()
		: m_uiSize( 0 )
	{
	}

	~AkHashListBare()
	{
	}

	void Init()
	{
		m_uiSize = 0;

		for ( unsigned int i = 0; i < T_HASHSIZE; ++i )
			m_table[ i ] = NULL;
	}

	void Term()
	{
		AKASSERT( m_uiSize == 0 );
	}
/*
	void RemoveAll()
	{
		AKASSERT( m_MemPoolId != AK_INVALID_POOL_ID );

		for ( unsigned int i = 0; i < T_HASHSIZE; ++i )
		{
			T_MAPSTRUCT * pItem = m_table[ i ];
			while ( pItem != NULL )
			{
				T_MAPSTRUCT * pNextItem = pItem->pNextItem;
				pItem->~T_MAPSTRUCT();
				AkFree( m_MemPoolId, pItem );
				pItem = pNextItem;
			}
			
			m_table[ i ] = NULL;
		}

		m_uiSize = 0;
	}
*/
	T_MAPSTRUCT * Exists( T_KEY in_Key )
	{
		unsigned int uiTable = (unsigned int) AkHash( in_Key ) % T_HASHSIZE;
		return ExistsInList( in_Key, uiTable );
	}

	// Set using an externally preallocated T_MAPSTRUCT -- Hash list takes ownership of the T_MAPSTRUCT.
	void Set( T_MAPSTRUCT * in_pItem )
	{
		unsigned int uiTable = (unsigned int) AkHash( KEY_POLICY::Key(in_pItem) ) % T_HASHSIZE;
		AKASSERT( !ExistsInList( KEY_POLICY::Key(in_pItem), uiTable ) ); // T_MAPSTRUCT must not exist in list !

		// Add new entry

		in_pItem->pNextItem = m_table[ uiTable ];
		m_table[ uiTable ] = in_pItem;

		++m_uiSize;
	}

	T_MAPSTRUCT * Unset( const T_KEY &in_Key )
	{
		unsigned int uiTable = (unsigned int) AkHash( in_Key ) % T_HASHSIZE;
		T_MAPSTRUCT * pItem = m_table[ uiTable ];
		T_MAPSTRUCT * pPrevItem = NULL;
		while ( pItem != NULL )
		{
			if ( KEY_POLICY::Key(pItem) == in_Key )
				break;

			pPrevItem = pItem;
			pItem = pItem->pNextItem;
		}

		if ( pItem )
			RemoveItem( uiTable, pItem, pPrevItem );

		return pItem;
	}

	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		IteratorEx returnedIt;
		returnedIt.pTable = in_rIter.pTable;
		returnedIt.uiTable = in_rIter.uiTable;
		returnedIt.pItem = in_rIter.pItem->pNextItem;
		returnedIt.pPrevItem = in_rIter.pPrevItem;
		
		while ( ( returnedIt.pItem == NULL ) && ( ++returnedIt.uiTable < T_HASHSIZE ) )
		{
			returnedIt.pPrevItem = NULL;
			returnedIt.pItem = returnedIt.pTable[ returnedIt.uiTable ];
		}
		
		RemoveItem( in_rIter.uiTable, in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

	unsigned int Length() const
	{
		return m_uiSize;
	}

protected:
	void RemoveItem( unsigned int in_uiTable, T_MAPSTRUCT* in_pItem, T_MAPSTRUCT* in_pPrevItem )
	{
		if( in_pPrevItem ) 
			in_pPrevItem->pNextItem = in_pItem->pNextItem;
		else
			m_table[ in_uiTable ] = in_pItem->pNextItem;

		--m_uiSize;
	}

	T_MAPSTRUCT * ExistsInList( T_KEY in_Key, unsigned int in_uiTable )
	{
		T_MAPSTRUCT * pItem = m_table[ in_uiTable ];
		while ( pItem != NULL )
		{
			if ( KEY_POLICY::Key(pItem) == in_Key )
				return pItem; // found

			pItem = pItem->pNextItem;
		}

		return NULL; // not found
	}

	T_MAPSTRUCT * m_table[ T_HASHSIZE ]; // table of single-link lists
	unsigned int m_uiSize;
};

#endif // _AKHASHLIST_H
