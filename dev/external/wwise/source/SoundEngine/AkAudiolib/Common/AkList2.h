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
// AkList2.h
//
// Simpler list class, no stl, no new.
//
//////////////////////////////////////////////////////////////////////
#ifndef _LIST2_H_
#define _LIST2_H_

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkArray.h>

//====================================================================================================
// how to implement a FIFO :
//
// Adding items	  : eResult = FifoList.AddLast(ThisItem);
// Getting items  : eResult = FifoList.First(ThisItem);
// Deleting items : eResult = FifoList.RemoveFirst();
//====================================================================================================
// tells what to do when allocation de-allocation needs to be done
//====================================================================================================
enum ListAllocFlag
{
	AkFixedSize		= 0,	// list will not grow or shrink
	AkAllocAndFree	= 1,	// allocate and de-allocate as needed
	AkAllocAndKeep	= 2		// allocate and keep new ones
};
#define AK_NO_MAX_LIST_SIZE AK_UINT_MAX
//====================================================================================================
//====================================================================================================
template <class T, class ARG_T, ListAllocFlag TAllocFlag, class U_POOL = ArrayPoolDefault> class CAkList2
{
public:
    struct ListItem
	{
		ListItem*	pNextListItem;	// our next one
		T			Item;			// user's stuff
	};

	struct Iterator
	{
		ListItem* pItem;

		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = pItem->pNextListItem;
			return *this;
		}

		inline T& operator*()
		{
			AKASSERT( pItem );
			return pItem->Item;
		}

		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}
		
		/* Commented out since it is unused, uncomment it on demand
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pItem == in_rOp.pItem );
		}
		*/
		
	};

	// The IteratorEx iterator is intended for usage when a possible erase may occurs
	// when simply iterating trough a list, use the simple Iterator, it is faster and lighter.
	struct IteratorEx : public Iterator
	{
		ListItem* pPrevItem;

		IteratorEx& operator++()
		{
			AKASSERT( this->pItem );
			
			pPrevItem = this->pItem;
			this->pItem = this->pItem->pNextListItem;
			
			return *this;
		}
	};

	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		IteratorEx returnedIt;
		returnedIt.pItem = in_rIter.pItem->pNextListItem;
		returnedIt.pPrevItem = in_rIter.pPrevItem;

		RemoveItem( in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

    IteratorEx Insert( const IteratorEx& in_rIter,
                       ARG_T in_item )
	{
        IteratorEx returnedIt;
		if ( AddItem( in_item, in_rIter.pItem, in_rIter.pPrevItem ) == NULL )
        {            
            returnedIt.pPrevItem = in_rIter.pPrevItem;
            returnedIt.pItem = NULL;
            return returnedIt;
        }
        returnedIt = in_rIter;
        return returnedIt;
	}

	inline Iterator End() const
	{
		Iterator returnedIt;
		returnedIt.pItem = NULL;
		return returnedIt;
	}

	inline IteratorEx BeginEx()
	{
		IteratorEx returnedIt;
		
		returnedIt.pItem = m_pFirst;
		returnedIt.pPrevItem = NULL;
		
		return returnedIt;
	}

	inline Iterator Begin() const
	{
		Iterator returnedIt;
		
		returnedIt.pItem = m_pFirst;
		
		return returnedIt;
	}

	inline IteratorEx FindEx( ARG_T in_Item )
	{
		IteratorEx it = BeginEx();
		for ( ; it != End(); ++it )
		{
			if ( *it == in_Item )
				break;
		}

		return it;
	}

    // recompute iterator Ex according to its previous item
    inline void Actualize( IteratorEx & io_rIter )
	{
        if ( !io_rIter.pPrevItem )  // it's the first
            io_rIter.pPrevItem = m_pFirst;
        if ( io_rIter.pPrevItem )   // the list is not empty
            io_rIter.pItem = io_rIter.pPrevItem->pNextListItem;
        else
            io_rIter.pItem = NULL;
	}

//====================================================================================================
// inits default values
//====================================================================================================
	CAkList2()
		: m_ulMinNumListItems( 0 )
		, m_ulMaxNumListItems( 0 )
		, m_ulNumListItems( 0 )
	{
	}
//====================================================================================================
// copy constructor
//====================================================================================================
	CAkList2( const CAkList2<T, ARG_T, TAllocFlag> & in_other )
		: m_ulMinNumListItems( 0 )
		, m_ulMaxNumListItems( 0 )
		, m_ulNumListItems( 0 )
	{
		if ( Init( in_other.m_ulMinNumListItems, in_other.m_ulMaxNumListItems ) == AK_Success )
		{
			operator=( in_other );
		}
	}
//====================================================================================================
// copy operator
//====================================================================================================
	CAkList2<T, ARG_T, TAllocFlag> & operator=( const CAkList2<T, ARG_T, TAllocFlag> & in_other )
	{
		// trap 'invalid' assignation case
		AKASSERT( ( m_ulMaxNumListItems == 0 && in_other.m_ulMaxNumListItems == 0 ) ||
			( m_ulMaxNumListItems > 0 && in_other.m_ulMaxNumListItems > 0 ) );

		if ( m_ulMaxNumListItems > 0 && in_other.m_ulMaxNumListItems > 0 ) // if both lists are initialized, copy contents
		{
			// copy all items
			ListItem* pListItem = in_other.m_pFirst;
			while ( pListItem != NULL )
			{
				if ( !AddLast( pListItem->Item ) )
				{
					AKASSERT( !"Insufficient memory for list copy" );
					break; // No more space ? give up.
				}

				pListItem = pListItem->pNextListItem;
			}
		}

		return *this;
	}

//====================================================================================================
//====================================================================================================
	~CAkList2()
	{
		AKASSERT( m_ulMaxNumListItems == 0 ); // Need to call Term if initialized
	}
//====================================================================================================
//====================================================================================================
	bool IsInitialized() const
	{
		return m_ulMaxNumListItems > 0;
	}
//====================================================================================================
// gets a list ready for use.
// in_ulMinNumItems : the minimum number of items that the list will hold at creation.
// in_ulMaxNumItems : the maximum number of items that the list will hold at any time.
// in_AllocFlag     : how list grow and shrink should be managed.
//====================================================================================================
	AKRESULT Init(unsigned int in_ulMinNumItems, unsigned int in_ulMaxNumItems)
	{
		AKASSERT(m_ulMinNumListItems == 0 && m_ulMaxNumListItems == 0);

		AKRESULT eResult = AK_Success;

		// remember these three
		m_ulNumListItems = 0;
		m_ulMaxNumListItems = in_ulMaxNumItems;
		m_pFree = NULL;

		// do we need to allocate anything ?
		if( in_ulMinNumItems > 0 )
		{
			AKASSERT( (TAllocFlag != AkFixedSize) || (in_ulMinNumItems == in_ulMaxNumItems) );
			eResult = Reserve( in_ulMinNumItems );
		}
		else
		{
			m_pvMemStart = NULL;
		}

		m_pFirst = NULL;
		m_pLast = NULL;

		return eResult;
	}
//====================================================================================================
// cleans up before leaving
//====================================================================================================
	AKRESULT Term()
	{
		if ( m_ulMaxNumListItems > 0 )
		{
			// empty the list
			RemoveAll();

			// look for extra items to free
			if( ( TAllocFlag != AkFixedSize ) && ( m_pFree != NULL ) )
			{
				ListItem*	pListItem = m_pFree;
				do
				{
					// grab next one
					ListItem*	pNextListItem = pListItem->pNextListItem;
					// is this one an extra ?
					if( IsExtraItem( pListItem ) )
					{
						// release the memory
						pListItem->Item.~T();
						AkFree(U_POOL::Get(),pListItem);
					}
					// move to next one
					pListItem = pNextListItem;
				}
				while(pListItem != NULL);
			}
			// any fixed size memory ?
			if(m_pvMemStart != NULL)
			{
				// free the memory we used
				for ( unsigned int i = 0; i < m_ulMinNumListItems; ++i )
					m_pvMemStart[ i ].Item.~T();
				AkFree(U_POOL::Get(), m_pvMemStart);
			}

			m_ulMinNumListItems = 0;
			m_ulMaxNumListItems = 0;

            m_pFirst = NULL;
		    m_pLast = NULL;
		}

		return AK_Success;
	}
	// Pre-allocate items
	AKRESULT Reserve( unsigned int in_cItems )
	{
		AKASSERT( m_ulMinNumListItems == 0 );
		AKASSERT( in_cItems <= m_ulMaxNumListItems );

		if ( in_cItems )
		{
			m_pvMemStart = (ListItem *) AkAlloc( U_POOL::Get(), sizeof( ListItem ) * in_cItems );
			if ( !m_pvMemStart )
				return AK_InsufficientMemory;
			// build a clean free items list
			m_ulMinNumListItems = in_cItems;
			m_pFree = m_pvMemStart;
			ListItem * pListItem = m_pFree;
			for(unsigned int ItemCounter = 0 ; ItemCounter < m_ulMinNumListItems ; ++ItemCounter)
			{
				AkPlacementNew( &(pListItem->Item) ) T; 

				// init pointer to next one
				pListItem->pNextListItem = pListItem + 1;
				// move to next one
				++pListItem;
			}
			(pListItem - 1)->pNextListItem = NULL;
		}

		return AK_Success;
	}
//====================================================================================================
// Adds an item to the beginning of the list
// fast, faster if nothing gets allocated, only pointers are changed
//====================================================================================================
	T * AddFirst(ARG_T in_rItem)
	{
		EnsureInitialized();

		// have we got space for a new one ?
		if(CanAddOne())
		{
			// add it to the list
			T * pItem = DoAddFirst();
			*pItem = in_rItem;
			return pItem;
		}
		
		return NULL;
	}
	T * AddFirst()
	{
		EnsureInitialized();

		// have we got space for a new one ?
		if( CanAddOne() )
			return DoAddFirst();
		else
			return NULL;
	}
//====================================================================================================
// Adds an item to the end of the list
// fast, faster if nothing gets allocated, only pointers are changed
//====================================================================================================
	T * AddLast(ARG_T in_rItem)
	{
		EnsureInitialized();

		// have we got space for a new one ?
		if(CanAddOne())
		{
			// add it to the list
			T * pItem = DoAddLast();
			*pItem = in_rItem;
			return pItem;
		}

		return NULL;
	}
	T * AddLast()
	{
		EnsureInitialized();

		// have we got space for a new one ?
		if( CanAddOne() )
			return DoAddLast();
		else
			return NULL;
	}
//====================================================================================================
// Adds an item to some specified position
// slow, needs to go through the list to find the spot
//====================================================================================================
	T * AddAt(size_t Index, ARG_T in_rItem)
	{
		unsigned int ulIndex = static_cast<unsigned int>( Index );

		if ( ulIndex == 0 )
		{
			return AddFirst( in_rItem );
		}
		else if ( ulIndex == m_ulNumListItems )
		{
			return AddLast( in_rItem );
		}
		else if ( ulIndex < m_ulNumListItems && CanAddOne() )
		{
			EnsureInitialized();

			T * pItem = DoAddAt( ulIndex );
			*pItem = in_rItem;
			return pItem;
		}

		return NULL;
	}
	T * AddAt(size_t Index)
	{
		unsigned int ulIndex = static_cast<unsigned int>( Index );

		if ( ulIndex == 0 )
		{
			return AddFirst();
		}
		else if ( ulIndex == m_ulNumListItems )
		{
			return AddLast();
		}
		else if ( ulIndex < m_ulNumListItems && CanAddOne() )
		{
			EnsureInitialized();

			return DoAddAt( ulIndex );
		}

		return NULL;
	}
//====================================================================================================
// Removes an item from the list
// slow, needs to go through the list to find the item
//====================================================================================================
	AKRESULT Remove(ARG_T in_rItem)
	{
		EnsureInitialized();

		IteratorEx it = FindEx( in_rItem );
		if ( it != End() )
		{
			Erase( it );
			return AK_Success;
		}

		return AK_Fail;
	}
//====================================================================================================
// Removes first item in the list
// fast, faster if nothing gets de-allocated, only pointers are changed
//====================================================================================================
	AKRESULT RemoveFirst()
	{
		EnsureInitialized();

		if(m_pFirst != NULL)
		{
			ListItem*	pListItem = m_pFirst;
			ListItem*	pPrevListItem = NULL;
			// move it to the free list
			RemoveItem(pListItem,pPrevListItem);
			return AK_Success;
		}
		return AK_Fail;
	}
//====================================================================================================
// Removes all element contained in the list
// slow
//====================================================================================================
	inline void RemoveAll()
	{
		EnsureInitialized();

		while( RemoveFirst() == AK_Success ) {}
	}
//====================================================================================================
// Removes item number Index from the list
// slow, needs to go through the list to find the item
//====================================================================================================
	AKRESULT RemoveAt(size_t Index)
	{
		EnsureInitialized();

		unsigned int ulItemCounter = static_cast<unsigned int>(Index);
		if ( ulItemCounter >= m_ulNumListItems )
		{
			return AK_Fail;
		}

		// scan them all
		ListItem* pListItem = m_pFirst;
		ListItem* pPrevListItem = NULL;
		while( ulItemCounter-- > 0 )
		{
			// move to next one
			pPrevListItem = pListItem;
			pListItem = pListItem->pNextListItem;
		}
		// set position and leave
		RemoveItem(pListItem,pPrevListItem);
		return AK_Success;
	}
//====================================================================================================
// Get the first element of the list
// fast
//====================================================================================================
	inline T& First()
	{
		EnsureInitialized();

		AKASSERT(m_pFirst);
		return m_pFirst->Item;
	}
//====================================================================================================
// Get the last element of the list
// fast
//====================================================================================================
	inline T& Last()
	{
		EnsureInitialized();

		return m_pLast->Item;
	}
//====================================================================================================
// returns the position in the list of a given item
// slow, needs to go through the list to find the item
//====================================================================================================
	bool GetPosition(ARG_T in_rItem,unsigned int& out_ruiPosition)
	{
		EnsureInitialized();

		ListItem*	pListItem = m_pFirst;
		// scan them all
		for(unsigned int ItemCounter = 0 ; ItemCounter < m_ulNumListItems ; ++ItemCounter)
		{
			// is it the one we're looking for ?
			if(pListItem->Item == in_rItem)
			{
				// set position and leave
				out_ruiPosition = ItemCounter;
				return true;
			}
			// move to next one
			pListItem = pListItem->pNextListItem;
		}
		return false;
	}
//====================================================================================================
// Get the number of elements in the list
// fast
//====================================================================================================
	inline unsigned int Length() const
	{
		return m_ulNumListItems;
	}
//====================================================================================================
// Check if the list is empty
// fast
//====================================================================================================
	inline bool IsEmpty() const
	{
		return m_ulNumListItems == 0;
	}
//====================================================================================================
// Check if specified item exists
// slow, needs to go through the list to find the item
// it returns a pointer to the item so you don't have to fetch it again if you need it
//====================================================================================================
	T* Exists(ARG_T in_Item) const
	{
		EnsureInitialized();

		// start at first one
		ListItem*	pListItem = m_pFirst;
		while ( pListItem != NULL )
		{
			// is it this one ?
			if(pListItem->Item == in_Item)
			{
				// found
				return &pListItem->Item;
			}
			pListItem = pListItem->pNextListItem;
		}
		// not found
		return NULL;
	}
//====================================================================================================

//====================================================================================================
// Accesses an element of the list
// slow, needs to go through the list to find the item
//====================================================================================================
	T& operator[](size_t in_Index)
	{
		EnsureInitialized();

		// can we reach this one ?
		if(in_Index < m_ulNumListItems)
		{
			// is it the first one ?
			if(in_Index == 0)
			{
				return m_pFirst->Item;
			}
			// is it the last one
			else if(in_Index == m_ulNumListItems - 1)
			{
				return m_pLast->Item;
			}
			// nope, it's some weirdo in the middle of the list
			else
			{
				// pass all those we don't need
				ListItem*	pListItem = m_pFirst;
				for(unsigned int Counter = 0 ; Counter < in_Index ; ++Counter)
				{
					// move to next one
					pListItem = pListItem->pNextListItem;
				}
				// return the one we need
				return pListItem->Item;
			}
		}
		return m_pLast->Item;
	}

protected:
//====================================================================================================
// Moves a list item in the free list
//====================================================================================================
	inline void AddFreeOne(ListItem* pListItem)
	{
		// tweak the pointers to include it in the free list
		pListItem->pNextListItem = m_pFree;
		m_pFree = pListItem;
	}
//====================================================================================================
// Adds an item in the first place of the list
//====================================================================================================
	T * DoAddFirst()
	{
		// is it the first to be added  ?
		if(m_pFirst == NULL)
		{
			// this is our last one as well
			m_pLast = m_pFree;
		}
		// remember the one we filled in
		ListItem*	pPreviousFree = m_pFree;
		// move to the next free one
		m_pFree = m_pFree->pNextListItem;
		// put the new one before the first one
		pPreviousFree->pNextListItem = m_pFirst;
		// the new one becomes our new first one
		m_pFirst = pPreviousFree;

		// we have one more
		++m_ulNumListItems;

		return &(pPreviousFree->Item);
	}
//====================================================================================================
// Adds an item in the last place of the list
//====================================================================================================
	T * DoAddLast()
	{
		// is it the first to be added  ?
		if(m_pLast == NULL)
		{
			// this is our first one as well
			m_pFirst = m_pFree;
		}
		else
		{
			// current last one links to added one
			m_pLast->pNextListItem = m_pFree;
		}
		// new one becomes our last one
		m_pLast = m_pFree;
		// move to the next free spot
		m_pFree = m_pFree->pNextListItem;
		m_pLast->pNextListItem = NULL;
		// we have one more
		++m_ulNumListItems;

		return &(m_pLast->Item);
	}
//====================================================================================================
// Adds an item in the middle place of the list
//====================================================================================================
	T* DoAddAt( unsigned int in_ulIndex )
	{
		// look for the requested spot
		ListItem*	pListItem = m_pFirst;
		ListItem*	pPrevListItem = NULL;
		while(in_ulIndex-- > 0)
		{
			// move to next one
			pPrevListItem = pListItem;
			pListItem = pListItem->pNextListItem;
		}

		// insert new one
		pPrevListItem->pNextListItem = m_pFree;
		// move to the next free spot
		m_pFree = m_pFree->pNextListItem;
		pPrevListItem->pNextListItem->pNextListItem = pListItem;
		// we have one more
		++m_ulNumListItems;

		// done
		return &(pPrevListItem->pNextListItem->Item);
	}
//====================================================================================================
// Figures out if it is possible to add a new item to the list
//====================================================================================================
	bool CanAddOne()
	{
		if( m_pFree )
		{
			// we can add one
			return true;
		}
//----------------------------------------------------------------------------------------------------
// are we allowed to add one ?
//----------------------------------------------------------------------------------------------------
		else if((TAllocFlag != AkFixedSize)
			&& (m_ulNumListItems < m_ulMaxNumListItems)) // here m_ulNumListItems is equivalent to the number of allocated items
		{
			ListItem * pTemp = (ListItem *) AkAlloc( U_POOL::Get(), sizeof( ListItem ));
			if( pTemp )
			{
				AkPlacementNew( &(pTemp->Item) ) T; 

				// stuff it in the free list
				AddFreeOne( pTemp );
				// we can add one
				return true;
			}
		}
		// we can't add one
		return false;
	}
//====================================================================================================
// moves a given item to the free list
// frees it if needed
//====================================================================================================
	void RemoveItem(ListItem*	in_pListItem, ListItem* in_pPrevListItem)
	{
//----------------------------------------------------------------------------------------------------
// is it the first one ?
//----------------------------------------------------------------------------------------------------
		if(in_pListItem == m_pFirst)
		{
			// new first one is the next one
			m_pFirst = in_pListItem->pNextListItem;
		}
		else
		{
			// take it out of the used space
			in_pPrevListItem->pNextListItem = in_pListItem->pNextListItem;
		}
//----------------------------------------------------------------------------------------------------
// is it the last one ?
//----------------------------------------------------------------------------------------------------
		if(in_pListItem == m_pLast)
		{
			// new last one is the previous one
			m_pLast = in_pPrevListItem;
		}
//----------------------------------------------------------------------------------------------------
// is it an extra one we have to free ?
//----------------------------------------------------------------------------------------------------
		if((TAllocFlag == AkAllocAndFree)
			&& IsExtraItem( in_pListItem ) )
		{
			// free it
			in_pListItem->Item.~T();
			AkFree(U_POOL::Get(),in_pListItem);
		}
		else
		{
			// insert it in the free space
			AddFreeOne(in_pListItem);
		}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
		// we have one less
		--m_ulNumListItems;
	}

//====================================================================================================
// adds an item to the list if it can.
//====================================================================================================
	T* AddItem( ARG_T in_rItem, ListItem* in_pNextListItem, ListItem* in_pPrevListItem )
	{
		if(CanAddOne())
		{
            AKASSERT( m_pFree );
            // insert new one
            ListItem * pNewListItem = m_pFree;
		    // move to the next free spot
		    m_pFree = m_pFree->pNextListItem;
            pNewListItem->pNextListItem = in_pNextListItem;

            pNewListItem->Item = in_rItem;

            // Update head.
            if ( in_pPrevListItem == NULL )
                m_pFirst = pNewListItem;
            else
                in_pPrevListItem->pNextListItem = pNewListItem;

            // Update tail.
            // Note: will never occur since it is used with iteratorEx (have EndEx?).
            if ( in_pNextListItem == NULL )   
                m_pLast = pNewListItem;

            // we have one more
		    ++m_ulNumListItems;

		    // done
		    return &(pNewListItem->Item);
        }
        else
            return NULL;
	}

	bool IsExtraItem( ListItem * pListItem ) const
	{
		if ( TAllocFlag == AkFixedSize )
			return false;
		else // Item is extra if it's not contained in the original allocation buffer
			return ( pListItem < m_pvMemStart )
				|| ( pListItem >= ( m_pvMemStart + m_ulMinNumListItems ) );
	}

	void EnsureInitialized() const
	{
		AKASSERT( IsInitialized() );
	}


protected:
	ListItem*		m_pFirst;					// top of list
	ListItem*		m_pLast;					// bottom of list
	ListItem*		m_pFree;					// top of free ones list
	
	unsigned int	m_ulMinNumListItems;		// how many we can have at least
	unsigned int	m_ulMaxNumListItems;		// how many we can have at most
	unsigned int	m_ulNumListItems;			// how many we have
	ListItem*	    m_pvMemStart;				// start of allocated memory
};

#endif
