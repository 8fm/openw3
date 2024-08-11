/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINERS_QUEUE_H
#define RED_CONTAINERS_QUEUE_H

namespace Red { namespace Containers {

	template< class ArrayType >
	class Queue
	{
	public:
		Queue();
		Queue( const Queue& other );
		Queue( Queue&& other );
		~Queue();
		Queue& operator=( const Queue& other );
		Queue& operator=( Queue&& other );

		// Accessors
		Red::System::Uint32 Size();
		Red::System::Uint32 Capacity();
		Red::System::Bool Empty();

		// Buffer manipulation
		void Reserve( Red::System::Uint32 elementCount );
		void Clear();

		// Element type
		typedef typename ArrayType::value_type value_type;

		// Element manipulation
		void Push( const value_type& value );
		void Pop();

		// Element accessors
		const value_type& Front() const;
		value_type& Front();
		const value_type& Back() const;
		value_type& Back();

		// Comparison ops
		Red::System::Bool operator==( const Queue& other );
		Red::System::Bool operator!=( const Queue& other );

	private:
		ArrayType m_array;
		Red::System::Uint32	m_numKeys;
		Red::System::Uint32	m_backIndex;
		Red::System::Uint32	m_frontIndex;
	};

} }

#include "queue.inl"

#endif