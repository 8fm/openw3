/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_WEAK_PTR_H_
#define _RED_MEMORY_WEAK_PTR_H_

#include "sharedPtr.h"

namespace red
{
	template< typename T, template< typename > class Ownership = DefaultOwnership >
	class TWeakPtr : public Ownership< T >
	{
	public:
		
		typedef Ownership< T > ParentType;

		TWeakPtr();
		TWeakPtr( const TWeakPtr & pointer );

		template< typename U >
		TWeakPtr( const TWeakPtr< U, Ownership > & pointer );

		template< typename U  >
		TWeakPtr( const TSharedPtr< U, Ownership> & pointer );
		
		~TWeakPtr();

		TWeakPtr & operator=( const TWeakPtr & pointer );

		template< typename U >
		TWeakPtr & operator=( const TWeakPtr< U, Ownership > & pointer );

		template< typename U >
		TWeakPtr & operator=( const TSharedPtr< U, Ownership > & pointer );

		TSharedPtr< T, Ownership > Lock() const;

		bool Expired() const;
		void Reset();
		void Swap( TWeakPtr & swapWith );
	};
}

#include "weakPtr.hpp"

#endif 
