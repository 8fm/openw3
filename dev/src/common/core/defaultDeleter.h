/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFAULT_DELETER_H_
#define _CORE_DEFAULT_DELETER_H_

#include "../redSystem/error.h"

namespace Red
{
	template< typename Type >
	class DefaultDeleter
	{
	public:

		DefaultDeleter()
		{}
		
		template< typename U >
		DefaultDeleter( const DefaultDeleter< U > & ) 
		{}

		void operator()(Type * ptr) const
		{
			static_assert( sizeof( Type ) > 0, "Cannot delete pointer to incomplete type. Did you forgot the include?" );
			static_assert( !std::is_polymorphic< Type >::value || std::has_virtual_destructor< Type >::value, "Virtual dtor is missing." );
			delete ptr;
		}
	};
}

#endif