/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// General integral template
//////////////////////////////////////////////////////////////////////////

template< typename T > inline CAtomic< T >::CAtomic( T value /*=T()*/ )
	: m_target( value )
{
}

template< typename T > inline T CAtomic< T >::Increment()
{
	return SAtomicOps::Increment( &m_target );
}

template< typename T > inline T CAtomic< T >::Decrement()
{
	return SAtomicOps::Decrement( &m_target );
}

template< typename T > inline T	CAtomic< T >::Or( T value )
{
	return SAtomicOps::Or( &m_target, value );
}

template< typename T > inline T CAtomic< T >::And( T value )
{
	return SAtomicOps::And( &m_target, value );
}

template< typename T > inline T CAtomic< T >::Exchange( T value )
{
	return static_cast< T >( SAtomicOps::Exchange( &m_target, static_cast< TAtomic >( value ) ) );
}

template< typename T > inline T CAtomic< T >::CompareExchange( T exchange, T comparand )
{
	return static_cast< T >( SAtomicOps::CompareExchange( &m_target, static_cast< TAtomic >( exchange ),
																	 static_cast< TAtomic >( comparand ) ) );
}

template< typename T > inline T CAtomic< T >::ExchangeAdd( T value )
{
	return SAtomicOps::ExchangeAdd( &m_target, value );
}

template< typename T > inline void CAtomic< T >::SetValue( T value )
{
	(void)SAtomicOps::Exchange( &m_target, static_cast< TAtomic >( value ) );
}

template< typename T > inline T CAtomic< T >::GetValue() const
{
	return static_cast< T >( SAtomicOps::FetchValue( &m_target ) );
}

//////////////////////////////////////////////////////////////////////////
// Boolean template specialization
//////////////////////////////////////////////////////////////////////////

inline CAtomic< Bool >::CAtomic( Bool value /*= false */ )
	: m_target( value )
{
}

inline Bool	CAtomic< Bool >::Exchange( Bool value )
{
	return SAtomicOps::Exchange( &m_target, value ? TAtomic(1) : TAtomic(0) ) != TAtomic(0);
}

inline Bool CAtomic< Bool >::CompareExchange( Bool exchange, Bool comparand )
{
	return SAtomicOps::CompareExchange( &m_target, exchange ? TAtomic(1) : TAtomic(0), comparand ? TAtomic(1) : TAtomic(0) ) != TAtomic(0);
}

inline void CAtomic< Bool >::SetValue( Bool value )
{
	(void)SAtomicOps::Exchange( &m_target, value ? TAtomic(1) : TAtomic(0) );
}

inline Bool CAtomic< Bool >::GetValue() const
{
	return SAtomicOps::FetchValue( &m_target) != TAtomic(0);
}

//////////////////////////////////////////////////////////////////////////
// Pointer template specialization
//////////////////////////////////////////////////////////////////////////

template< typename T > inline CAtomic< T* >::CAtomic( T* value /*=nullptr*/ )
	: m_target( const_cast< void* >( static_cast< const void* >( value ) ) )
{
}

// Because of strict-aliasing. Could go through a union, but that wouldn't work
// well with type qualifiers on template argument types.
template < typename TFrom, typename TTo> inline TTo redthr_alias_cast__( TFrom from )
{
	// Simple types
	TTo to = {0};
		
	static_assert( sizeof(to) == sizeof(from), "Invalid alias cast" );
	Red::System::MemoryCopy( &to, &from, sizeof from );
	return to;
}

template < typename TFrom, typename TTo> inline TTo redthr_ptr_cast__( TFrom from )
{
	// Simple types
	const MemUint addr = reinterpret_cast< MemUint >( from );
	TTo to = reinterpret_cast< TTo >( addr );

	static_assert( sizeof(to) == sizeof(from), "Invalid ptr cast" );

	return to;
}

template< typename T > inline T* CAtomic< T* >::Exchange( T* value )
{
	const TAtomicPtr result = SAtomicOps::Exchange( &m_target, redthr_ptr_cast__< T*, TAtomicPtr >(value) );
	return redthr_ptr_cast__< TAtomicPtr, T* >(result);
}

template< typename T > inline T* CAtomic< T* >::CompareExchange( T* exchange, T* comparand )
{
	const TAtomicPtr result = SAtomicOps::CompareExchange( &m_target, redthr_ptr_cast__< T*, TAtomicPtr >(exchange),
																   redthr_ptr_cast__< T*, TAtomicPtr >(comparand) );
	return redthr_ptr_cast__< TAtomicPtr, T* >(result);
}

template< typename T > inline void CAtomic< T* >::SetValue( T* value )
{
	(void)SAtomicOps::Exchange( &m_target, redthr_ptr_cast__< T*, TAtomicPtr >( value ) );
}

template< typename T > inline T* CAtomic< T* >::GetValue() const
{
	TAtomicPtr result = SAtomicOps::FetchValue( &m_target);
	return redthr_ptr_cast__< TAtomicPtr, T* >( result );
}

} } // namespace Red { namespace Threads {