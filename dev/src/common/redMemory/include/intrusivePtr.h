/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_INTRUSIVE_PTR_H_
#define _RED_MEMORY_INTRUSIVE_PTR_H_

#include "sharedPtr.h"

namespace red
{
	template< typename T >
	class IntrusiveOwnership;

	template< typename T >
	class TIntrusivePtr : public TSharedPtr< T, red::IntrusiveOwnership >
	{
	public:

		typedef TSharedPtr< T, red::IntrusiveOwnership > ParentType;

		TIntrusivePtr();
		TIntrusivePtr( const TIntrusivePtr & copyFrom );
		TIntrusivePtr( TIntrusivePtr && rvalue );

		template< typename U >
		explicit TIntrusivePtr( U * pointer );	

		template< typename U >
		TIntrusivePtr( const TIntrusivePtr< U > & copyFrom );

		template< typename U >
		TIntrusivePtr( TIntrusivePtr< U > && rvalue );

		template< typename U >
		TIntrusivePtr( TUniquePtr< U > && rvalue );
	
		TIntrusivePtr & operator=( const TIntrusivePtr & copyFrom );
		TIntrusivePtr & operator=( TIntrusivePtr && rvalue );

		template< typename U >
		TIntrusivePtr & operator=( const TIntrusivePtr< U >  & copyFrom );

		template< typename U >
		TIntrusivePtr & operator=( TIntrusivePtr< U > && rvalue );
	
		template< typename U >
		TIntrusivePtr & operator=( TUniquePtr< U > && rvalue );
	};

	template< typename T >
	class IntrusiveOwnership
	{
	protected:

		typedef T * PtrType;

		IntrusiveOwnership();
		IntrusiveOwnership( PtrType pointer );

		IntrusiveOwnership( const IntrusiveOwnership & copyFrom );
		IntrusiveOwnership( IntrusiveOwnership && rvalue  );

		template< typename U >
		IntrusiveOwnership( const IntrusiveOwnership< U > & copyFrom );

		template< typename U >
		IntrusiveOwnership( IntrusiveOwnership< U > && rvalue );

		PtrType Get() const;

		void Release();
		void AddRef();

		void Swap( IntrusiveOwnership & swapWith );
	
	private:
	
		void AssignRValue( IntrusiveOwnership && rvalue );

		PtrType m_pointee;
	};


}

#include "intrusivePtr.hpp"

#endif
