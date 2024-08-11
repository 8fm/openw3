/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SCOPED_PTR_H_
#define _RED_MEMORY_SCOPED_PTR_H_

#include "uniquePtrStorage.h"

namespace red
{
	template< typename PtrType, typename DeleterType = memory::DefaultUniquePtrDestructor >
	class TScopedPtr
	{
	public:

		explicit TScopedPtr( PtrType * object = nullptr );
		explicit TScopedPtr( PtrType * object, DeleterType destroyer );
		~TScopedPtr();

		PtrType * Get() const;

		void Reset( PtrType * object = nullptr );

		void Swap( TScopedPtr< PtrType, DeleterType > & swapWith );

		PtrType * operator->() const;
		PtrType & operator*() const;
	
		struct BoolConversion{ int valid; };
		typedef int BoolConversion::*bool_operator;

		operator bool_operator() const;
		bool operator !() const;
		
	private:

		TScopedPtr( const TScopedPtr & );
		TScopedPtr & operator=( const TScopedPtr & );

		// The idea is that if the Destructor provided is empty, the size of UniquePtr is still the size of a single pointer.
		typedef typename memory::UniquePtrStorage_Resolver< PtrType, DeleterType >::Type StorageType;
		StorageType m_storage;
	};
}

#include "scopedPtr.hpp"

#endif
