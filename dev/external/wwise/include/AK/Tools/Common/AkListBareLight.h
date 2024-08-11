//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkListBareLight.h

#ifndef _AKLISTBARELIGHT_H
#define _AKLISTBARELIGHT_H

// this one lets you define the structure
// only requirement is that T must have member pNextLightItem,
// or use the template getter.
// client is responsible for allocation/deallocation of T.

// WATCH OUT !
// - remember that removeall/term can't delete the elements for you.
// - be sure to destroy elements AFTER removing them from the list, as remove will
// access members of the element.
// WARNING : Each AkListBareLight item can be used in only one AkListBareLight at 
//           once since the member pNextLightItem cannot be re-used.

/// Next item name policy.
template <class T> struct AkListBareLightNextItem
{
	/// Default policy.
	static AkForceInline T *& Get( T * in_pItem ) 
	{
		return in_pItem->pNextLightItem;
	}
};

/// Implementation of List Bare.
template <class T, class U_NEXTITEM = AkListBareLightNextItem< T > > class AkListBareLight
{
public:
	/// Iterator.
	struct Iterator
	{
		T* pItem;	///< Next item.

		/// Operator ++.
		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = U_NEXTITEM::Get( pItem );
			return *this;
		}

		/// Operator *.
		inline T * operator*()
		{
			AKASSERT( pItem );
			return pItem;
		}

		/// Operator !=.
		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}
	};

	/// The IteratorEx iterator is intended for usage when a possible erase may occurs
	/// when simply iterating trough a list, use the simple Iterator, it is faster and lighter.
	struct IteratorEx : public Iterator
	{
		T* pPrevItem;	///< Previous item.

		/// Operator ++.
		IteratorEx& operator++()
		{
			AKASSERT( this->pItem );
			
			pPrevItem = this->pItem;
			this->pItem = U_NEXTITEM::Get( this->pItem );
			
			return *this;
		}
	};

	/// Erase item.
	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		IteratorEx returnedIt;
		returnedIt.pItem = U_NEXTITEM::Get( in_rIter.pItem );
		returnedIt.pPrevItem = in_rIter.pPrevItem;

		RemoveItem( in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

	/// Insert item.
    IteratorEx Insert( const IteratorEx& in_rIter,
                       T * in_pItem )
	{
		AddItem( in_pItem, in_rIter.pItem, in_rIter.pPrevItem );
        return in_rIter;
	}

	/// End condition.
	inline Iterator End()
	{
		Iterator returnedIt;
		returnedIt.pItem = NULL;
		return returnedIt;
	}

	/// Get IteratorEx at beginning.
	inline IteratorEx BeginEx()
	{
		IteratorEx returnedIt;
		
		returnedIt.pItem = m_pFirst;
		returnedIt.pPrevItem = NULL;
		
		return returnedIt;
	}

	/// Get Iterator at beginning.
	inline Iterator Begin()
	{
		Iterator returnedIt;
		
		returnedIt.pItem = m_pFirst;
		
		return returnedIt;
	}

	/// Get Iterator from item.
	inline IteratorEx FindEx( T *  in_pItem )
	{
		IteratorEx it = BeginEx();
		for ( ; it != End(); ++it )
		{
			if ( it.pItem == in_pItem )
				break;
		}

		return it;
	}

	/// Constructor.
	AkForceInline AkListBareLight()
	{
		m_pFirst = NULL;
	}

	// Copy constructors.
	AkForceInline AkListBareLight( AkListBareLight & in_listToCopy )
	{
		m_pFirst = in_listToCopy.m_pFirst;
	}

	AkForceInline AkListBareLight( T * in_pHeadToCopy )
	{
		m_pFirst = in_pHeadToCopy;
	}

	/// Destructor.
	AkForceInline ~AkListBareLight()
	{
	}

	/// Terminate.
	void Term()
	{
		RemoveAll();
	}

	/// Add element at the beginning of list.
	void AddFirst( T * in_pItem )
	{
		if ( m_pFirst == NULL )
		{
			m_pFirst = in_pItem;
			U_NEXTITEM::Get( in_pItem ) = NULL;
		}
		else
		{
			U_NEXTITEM::Get( in_pItem ) = m_pFirst;
			m_pFirst = in_pItem;
		}
	}

	/// Remove an element.
	AKRESULT Remove( T * in_pItem )
	{
		IteratorEx it = FindEx( in_pItem );
		if ( it != End() )
		{
			Erase( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	/// Remove the first element.
	AKRESULT RemoveFirst()
	{
		if( m_pFirst == NULL )
			return AK_Fail;

		if ( U_NEXTITEM::Get( m_pFirst ) == NULL )
		{
			m_pFirst = NULL;
		}
		else
		{
			m_pFirst = U_NEXTITEM::Get( m_pFirst );
		}

		return AK_Success;
	}

	/// Remove all elements.
	AkForceInline void RemoveAll()
	{
		// Items being externally managed, all we need to do here is clear our members.
		m_pFirst = NULL;
	}

	/// Get first element.
	AkForceInline T * First()
	{
		return m_pFirst;
	}

	/// Empty condition.
	AkForceInline bool IsEmpty() const
	{
		return ( m_pFirst == NULL );
	}

	/// Remove an element.
	void RemoveItem( T * in_pItem, T * in_pPrevItem )
	{
		// Is it the first one ?

		if( in_pItem == m_pFirst )
		{
			// new first one is the next one
			m_pFirst = U_NEXTITEM::Get( in_pItem );
		}
		else
		{
			// take it out of the used space
			U_NEXTITEM::Get( in_pPrevItem ) = U_NEXTITEM::Get( in_pItem );
		}
	}

	/// Add an element.
	void AddItem( T * in_pItem, T * in_pNextItem, T * in_pPrevItem )
	{
		U_NEXTITEM::Get( in_pItem ) = in_pNextItem;

		if ( in_pPrevItem == NULL )
            m_pFirst = in_pItem;
        else
            U_NEXTITEM::Get( in_pPrevItem ) = in_pItem;

	}

protected:
	T *				m_pFirst;					///< top of list
};

#endif // _AKLISTBARELIGHT_H
