/**
* Copyright � 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ATOMIC_SHARED_PTR_H_
#define _RED_MEMORY_ATOMIC_SHARED_PTR_H_

#include "sharedPtr.h"
#include "../../redSystem/redThreadsAtomic.h"

namespace red
{
	template< typename T >
	class AtomicRefCountingOwnership;

	template< typename T >
	class TAtomicWeakPtr;

	template< typename T >
	class TAtomicSharedPtr : public TSharedPtr< T, red::AtomicRefCountingOwnership >
	{
	public:
		typedef TSharedPtr< T, red::AtomicRefCountingOwnership > ParentType;

		TAtomicSharedPtr();
		TAtomicSharedPtr( const TAtomicSharedPtr & copyFrom );
		TAtomicSharedPtr( TAtomicSharedPtr && rvalue );

		template< typename U >
		explicit TAtomicSharedPtr( U * pointer );	

		template< typename U >
		TAtomicSharedPtr( const TAtomicSharedPtr< U > & copyFrom );

		template< typename U >
		TAtomicSharedPtr( TAtomicSharedPtr< U > && rvalue );

		template< typename U >
		TAtomicSharedPtr( const TAtomicWeakPtr< U > & copyFrom );

		template< typename U >
		TAtomicSharedPtr( TUniquePtr< U > && rvalue );

		TAtomicSharedPtr & operator=( const TAtomicSharedPtr & copyFrom );
		TAtomicSharedPtr & operator=( TAtomicSharedPtr && rvalue );

		template< typename U >
		TAtomicSharedPtr & operator=( const TAtomicSharedPtr< U >  & copyFrom );
	
		template< typename U >
		TAtomicSharedPtr & operator=( TAtomicSharedPtr< U >  && rvalue );

		template< typename U >
		TAtomicSharedPtr & operator=( TUniquePtr< U > && rvalue );
	};

	struct AtomicRefCount
	{
		typedef atomic::TAtomic32 RefCountType;

		AtomicRefCount( RefCountType s, RefCountType w )
			:	strong(s),
				weak(w)
		{}

		RefCountType strong;
		RefCountType weak;
	};

	template< typename T >  
	class AtomicRefCountingOwnership
	{
	public: 

		typedef T * PtrType;
		typedef AtomicRefCount::RefCountType RefCountType;
		
		RefCountType GetRefCount() const;
		RefCountType GetWeakRefCount() const;

	protected:

		AtomicRefCountingOwnership();
		AtomicRefCountingOwnership( PtrType );

		AtomicRefCountingOwnership( const AtomicRefCountingOwnership & copyFrom );
		AtomicRefCountingOwnership( AtomicRefCountingOwnership && rvalue );

		template< typename U >
		AtomicRefCountingOwnership( const AtomicRefCountingOwnership< U > & copyFrom );

		template< typename U >
		AtomicRefCountingOwnership( AtomicRefCountingOwnership< U > && rvalue );

		PtrType Get() const;

		void Release();
		void ReleaseWeak();
		void AddRef();
		void AddRefWeak();

		void UpgradeFromWeakToStrong( AtomicRefCountingOwnership & upgradeFrom );

		void Swap( AtomicRefCountingOwnership & swapWith );

	private:

		void AssignRValue( AtomicRefCountingOwnership && rvalue );

		AtomicRefCount * m_refCount;
		PtrType m_pointee;
	};
}

#include "atomicSharedPtr.hpp"

#endif
