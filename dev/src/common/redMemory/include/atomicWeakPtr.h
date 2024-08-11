/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ATOMIC_WEAK_PTR_H_
#define _RED_MEMORY_ATOMIC_WEAK_PTR_H_

#include "weakPtr.h"
#include "atomicSharedPtr.h"

namespace red
{
	template< typename T >
	class AtomicRefCountingOwnership;

	template< typename T >
	class TAtomicWeakPtr : public TWeakPtr< T, red::AtomicRefCountingOwnership >
	{
	public:
		typedef TWeakPtr< T, red::AtomicRefCountingOwnership > ParentType;

		TAtomicWeakPtr();
		TAtomicWeakPtr( const TAtomicWeakPtr & pointer );

		template< typename U >
		TAtomicWeakPtr( const TAtomicWeakPtr< U > & pointer );

		template< typename U  >
		TAtomicWeakPtr( const TAtomicSharedPtr< U> & pointer );

		TAtomicWeakPtr & operator=( const TAtomicWeakPtr & pointer );

		template< typename U >
		TAtomicWeakPtr & operator=( const TAtomicWeakPtr< U > & pointer );

		template< typename U >
		TAtomicWeakPtr & operator=( const TAtomicSharedPtr< U > & pointer );

		TAtomicSharedPtr< T > Lock() const;
	};
}

#include "atomicWeakPtr.hpp"


#endif