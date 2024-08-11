/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef _CORE_SCOPED_PTR_
#define _CORE_SCOPED_PTR_

#include "defaultDeleter.h"

namespace Red
{
	
	template< typename PtrType, typename DeleterType = DefaultDeleter< PtrType > >
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

		PtrType * m_pointee;
		DeleterType m_destroyer;
	};
}

#include "scopedPtr.inl"

#endif
