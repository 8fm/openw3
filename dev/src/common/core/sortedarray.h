/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "pair.h"
#include "dynarray.h"

template < typename T, 
		   typename CompareFunc = DefaultCompareFunc< T > >
class TSortedArray : public TDynArray<T>
{
	typedef TDynArray< T > tSuper;
public:
	struct SPred
	{
		template< class _Type1, class _Type2 >
		RED_INLINE Bool operator()( const _Type1& lhs, const _Type2& rhs ) const
		{
			return CompareFunc::Less( lhs, rhs );
		}
	};

public:
	typedef typename tSuper::iterator iterator;
	typedef typename tSuper::const_iterator const_iterator;

public:
	using tSuper::Begin;
	using tSuper::End;

public:
	using tSuper::m_size;

public:
	TSortedArray() {}
	RED_INLINE iterator Insert( const T& val )
	{	
		SPred predicate;
		typename TDynArray<T>::iterator it = LowerBound( this->Begin(), this->End(), val, predicate );
		Uint32 pos = PtrDiffToUint32( (void*)(it - this->Begin()) );
		TDynArray<T>::Insert( pos, val );
		return Begin()+pos;
	}

	RED_INLINE iterator Insert( T&& val )
	{	
		SPred predicate;
		typename TDynArray<T>::iterator it = LowerBound( this->Begin(), this->End(), val, predicate );
		Uint32 pos = PtrDiffToUint32( (void*)(it - this->Begin()) );
		TDynArray<T>::Insert( pos, Move( val ) );
		return Begin()+pos;
	}
	RED_INLINE iterator InsertUnique( const T& val )
	{	
		SPred predicate;
		typename TDynArray<T>::iterator it = LowerBound( this->Begin(), this->End(), val, predicate );
		Uint32 pos = PtrDiffToUint32( (void*)(it - this->Begin()) );
		if (  pos == m_size || predicate( val, (*this)[ pos ] ) )
		{
			TDynArray<T>::Insert( pos, val );
		}
		return Begin()+pos;
	}
	RED_INLINE iterator InsertUnique( T&& val )
	{	
		SPred predicate;
		typename TDynArray<T>::iterator it = LowerBound( this->Begin(), this->End(), val, predicate );
		Uint32 pos = PtrDiffToUint32( (void*)(it - this->Begin()) );
		if (  pos == m_size || predicate( val, (*this)[ pos ] ) )
		{
			TDynArray<T>::Insert( pos, Move( val ) );
		}
		return Begin()+pos;
	}

	template< class K >
	RED_INLINE typename TDynArray<T>::const_iterator Find( const K& val ) const
	{
		SPred predicate;
		typename TDynArray<T>::const_iterator lowerBound = LowerBound( this->Begin(), this->End(), val, predicate );
		if ( lowerBound != this->End() && !( predicate( val, *lowerBound ) ) )
		{
			return lowerBound;
		}
		else
		{
			return this->End();
		}
	}

	template< class K >
	//RED_INLINE iterator Find( const K& val )
	RED_INLINE typename TDynArray<T>::iterator Find( const K& val )
	{
		SPred predicate;
		typename TDynArray<T>::iterator lowerBound = LowerBound( this->Begin(), this->End(), val, predicate );
		if ( lowerBound != this->End() && !( predicate( val, *lowerBound ) ) )
		{
			return lowerBound;
		}
		else
		{
			return this->End();
		}
	}
	RED_INLINE Bool IsSorted() const
	{
		return ::IsSorted( Begin(), End(), SPred() );
	}
	RED_INLINE void Sort()
	{
		::Sort( Begin(), End(), SPred() );
	}
	RED_INLINE void InsertionSort()
	{
		::InsertionSort( Begin(), End(), SPred() );
	}
};

template < class TKey
	, class TElem , class TOrder = DefaultCompareFunc< TKey > >
class TArrayMap : public TSortedArray < TPair < TKey, TElem >, ComparePairFunc< TKey, TElem, TOrder > >
{
private:
	typedef TSortedArray < TPair < TKey, TElem >, ComparePairFunc< TKey, TElem, TOrder > > tSuper;

public:
	typedef TKey key_type;
	typedef TElem elem_type;
	typedef typename tSuper::value_type value_type;
	typedef typename tSuper::iterator iterator;
	typedef typename tSuper::const_iterator const_iterator;

public:
	using tSuper::Begin;
	using tSuper::End;
	using tSuper::m_size;

private:
	////////////////////////////////////////////////////////////////////////
	// Some implementation 
	struct SKeyPred
	{

		// MSVC: Resolved as "Won't fix" due to low resources, but recognized as a real bug.
		// http://connect.microsoft.com/VisualStudio/feedback/details/567070/bogus-c2535-error-when-overloading-method-name-with-dependent-typedef
		// Work around seems to be don't use the base class typedef.
		RED_INLINE Bool operator()( const TKey& lhs, const typename tSuper::value_type& rhs ) const
		{
			return TOrder::Less( lhs, rhs.m_first );
		}

		RED_INLINE Bool operator()( const typename tSuper::value_type& lhs, const TKey& rhs ) const
		{
			return TOrder::Less( lhs.m_first, rhs );
		}

		template< class _Type1, class _Type2 >
		RED_INLINE Bool operator()( const _Type1& lhs, const _Type2& rhs ) const
		{
			return TOrder::Less( lhs, rhs );
		}
	};

public:

	TArrayMap() : tSuper() {}
	
	RED_INLINE iterator Insert(const TKey& cKey, const TElem& cElem)
	{
		return tSuper::Insert(value_type(cKey,cElem));
	}
	// Its unclear when we try to use dyn array interface sometimes
	//TElem& operator[](const TKey& cKey)
	//{
	//	SKeyPred predicate;
	//	iterator itRet = UpperBound(Begin(),End(),cKey,predicate);
	//	if( itRet != End() )
	//	{
	//		if (cKey == (*itRet))
	//			return itRet->m_second;
	//	}

	//	return TSortedArray::tSuper::Insert( itRet,  value_type( cKey,TElem() ) )->m_second;
	//}
	RED_INLINE iterator Find(const TKey& cKey)
	{
		SKeyPred predicate;
		iterator itRet = LowerBound(Begin(),End(),cKey,predicate);
		if ( itRet != End() && !( predicate( cKey, *itRet ) ) )
			return itRet;
		else
			return End();
	}
	RED_INLINE const_iterator Find(const TKey& cKey) const
	{
		SKeyPred predicate;
		const_iterator itRet = LowerBound(Begin(),End(),cKey,predicate);
		if ( itRet != End() && !( predicate( cKey, *itRet ) ) )
			return itRet;
		else
			return End();
	}

	RED_INLINE TElem& GetRef( const TKey& key, const TElem& defaultValue = TElem() )
	{
		SKeyPred predicate;
		iterator it = LowerBound( Begin(), End(), key, predicate );
		Uint32 pos = PtrDiffToUint32( (void*)(it - Begin()) );
		if (  pos == m_size || predicate( key, *it ) )
		{
			TDynArray<value_type>::Insert( pos, value_type(key,defaultValue) );
		}
		return (Begin()+pos)->m_second;
	}

	RED_INLINE Bool Erase(const TKey& cKey)
	{
		SKeyPred predicate;
		iterator itRet = LowerBound(Begin(),End(),cKey,predicate);
		if ( itRet != End() && !( predicate( cKey, *itRet ) ) )
		{
			tSuper::Erase( itRet );
			return true;
		}
		return false;
	}
	RED_INLINE void Erase( iterator it )
	{
		tSuper::Erase( it );
	}
};


