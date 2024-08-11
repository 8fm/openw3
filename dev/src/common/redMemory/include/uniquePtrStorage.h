/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_PTR_STORAGE_H_
#define _RED_MEMORY_UNIQUE_PTR_STORAGE_H_

namespace red
{
namespace memory
{
	class DefaultUniquePtrDestructor;

	template< typename T, typename DestructorFunctor >
	class UniquePtrStorage_EmptyDestructor;
	
	template< typename T, typename DestructorFunctor >
	class UniquePtrStorage_AggregateDestructor;

	template< typename T, typename DestructorFunctor >
	struct UniquePtrStorage_Resolver
	{
		typedef typename std::conditional< 
			std::is_empty< DestructorFunctor >::value,
			UniquePtrStorage_EmptyDestructor< T, DestructorFunctor >,
			UniquePtrStorage_AggregateDestructor< T, DestructorFunctor >
		>::type Type;
	};

	class DefaultUniquePtrDestructor
	{
	public:

		template< typename T >
		void operator()( T * ptr ) const;
	};

	template< typename T, typename DestructorFunctor >
	class UniquePtrStorage_EmptyDestructor : private DestructorFunctor
	{
	public:

		UniquePtrStorage_EmptyDestructor();
		UniquePtrStorage_EmptyDestructor( T * ptr );
		UniquePtrStorage_EmptyDestructor( T * ptr, const DestructorFunctor & functor );
		UniquePtrStorage_EmptyDestructor( T * ptr, DestructorFunctor && functor );
		UniquePtrStorage_EmptyDestructor( UniquePtrStorage_EmptyDestructor && storage );
		~UniquePtrStorage_EmptyDestructor();

		T * Get() const;
		T * Release();
		void Swap( UniquePtrStorage_EmptyDestructor & storage );
		DestructorFunctor & GetDestructor();
		const DestructorFunctor & GetDestructor() const;

	private:

		T * m_pointer;
	};

	template< typename T, typename DestructorFunctor >
	class UniquePtrStorage_AggregateDestructor
	{
	public:
		UniquePtrStorage_AggregateDestructor();
		UniquePtrStorage_AggregateDestructor( T * ptr );
		UniquePtrStorage_AggregateDestructor( T * ptr, const DestructorFunctor & functor );
		UniquePtrStorage_AggregateDestructor( T * ptr, DestructorFunctor && functor );
		UniquePtrStorage_AggregateDestructor( UniquePtrStorage_AggregateDestructor && storage );
		~UniquePtrStorage_AggregateDestructor();

		T * Get() const;
		T * Release();
		void Swap( UniquePtrStorage_AggregateDestructor & storage );
		DestructorFunctor & GetDestructor();
		const DestructorFunctor & GetDestructor() const;

	private:

		T * m_pointer;
		DestructorFunctor m_destructor;
	};
}
}

#include "uniquePtrStorage.hpp"

#endif
