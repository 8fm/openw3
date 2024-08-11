/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_UNIQUE_PTR_
#define _CORE_UNIQUE_PTR_

#include "defaultDeleter.h"

namespace Red
{
	template< typename PtrType, typename DeleterType = DefaultDeleter< PtrType > >
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
		
		PtrType * m_pointee;
		DeleterType m_destroyer;
	};

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator==( const TUniquePtr< LeftType, DeleterType> & leftPtr, const TUniquePtr< RightType, DeleterType> & rightPtr );

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator!=( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr );

	template< typename LeftType, typename RightType, typename DeleterType >
	bool operator<( const TUniquePtr< LeftType, DeleterType > & leftPtr, const TUniquePtr< RightType, DeleterType > & rightPtr );

}

#include "uniquePtr.inl"

#endif
