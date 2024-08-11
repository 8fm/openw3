/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_PTR_H_
#define _RED_MEMORY_UNIQUE_PTR_H_

#include "uniquePtrStorage.h"

namespace red
{
	template< typename PtrType, typename DeleterType = memory::DefaultUniquePtrDestructor >
	class TUniquePtr
	{
	public:
	
		TUniquePtr();
		explicit TUniquePtr( PtrType * pointer );
		TUniquePtr( PtrType * pointer, const DeleterType & destroyer );
		TUniquePtr( PtrType * pointer, DeleterType && destroyer );
		TUniquePtr( TUniquePtr && moveFrom ); 

		template< typename U, typename V > 
		TUniquePtr( TUniquePtr< U, V > && moveFrom );
		~TUniquePtr();
		
		PtrType * Get() const;

		PtrType & operator*() const;
		PtrType * operator->() const;
	
		PtrType * Release();
	
		void Reset( PtrType * pointer = nullptr );
	
		void Swap( TUniquePtr & swapWith );
		
		TUniquePtr& operator=( TUniquePtr&& moveFrom );

		template<typename U, typename V > 
		TUniquePtr& operator=( TUniquePtr< U, V > && moveFrom );

		struct BoolConversion{ int valid; };
		typedef int BoolConversion::*bool_operator;

		operator bool_operator () const;
		bool operator!() const;

		DeleterType & GetDeleter();
		const DeleterType & GetDeleter() const;

	private:

		TUniquePtr( const TUniquePtr& );
		TUniquePtr& operator=( const TUniquePtr& );
	
		// The idea is that if the Destructor provided is empty, the size of UniquePtr is still the size of a single pointer.
		typedef typename memory::UniquePtrStorage_Resolver< PtrType, DeleterType >::Type StorageType;
		StorageType m_storage;
	};

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator==( const TUniquePtr< LeftType, DeleterType> & leftPtr, const TUniquePtr< RightType, DeleterType> & rightPtr );

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator!=( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr );

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator<( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr );
}

#include "uniquePtr.hpp"

#endif
