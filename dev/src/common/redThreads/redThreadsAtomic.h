/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_ATOMIC_H
#define RED_THREADS_ATOMIC_H
#pragma once

#include "redThreadsPlatform.h"

#if defined ( RED_THREADS_PLATFORM_WINDOWS_API )
#	include "redThreadsAtomicWinAPI.inl"
#elif defined ( RED_PLATFORM_ORBIS )
#	include "redThreadsAtomicOrbisAPI.inl"
#else
#	error Platform not supported
#endif

//////////////////////////////////////////////////////////////////////////
//
namespace Red { namespace Threads {

	//////////////////////////////////////////////////////////////////////////
	// Atomic operations with full memory barriers
	//////////////////////////////////////////////////////////////////////////
	namespace AtomicOps
	{
		typedef OSAPI::SAtomicOps32::TAtomic32 TAtomic32;

		inline TAtomic32		Increment32( TAtomic32 volatile* addend ) { return OSAPI::SAtomicOps32::Increment( addend ); }
		inline TAtomic32		Decrement32( TAtomic32 volatile* addend ) { return OSAPI::SAtomicOps32::Decrement( addend ); }
		inline TAtomic32		Exchange32( TAtomic32 volatile* target, TAtomic32 value ) { return OSAPI::SAtomicOps32::Exchange( target, value ); }  
		inline TAtomic32		CompareExchange32( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand ) { return OSAPI::SAtomicOps32::CompareExchange( destination, exchange, comparand ); }
		inline TAtomic32		ExchangeAdd32( TAtomic32 volatile* addend, TAtomic32 value )  { return OSAPI::SAtomicOps32::ExchangeAdd( addend, value ); }
		inline TAtomic32		Or32( TAtomic32 volatile* destination, TAtomic32 value ) { return OSAPI::SAtomicOps32::Or( destination, value ); }
		inline TAtomic32		And32( TAtomic32 volatile* destination, TAtomic32 value ) { return OSAPI::SAtomicOps32::And( destination, value ); }
		inline TAtomic32		FetchValue32( TAtomic32 volatile* destination ) {  return OSAPI::SAtomicOps32::FetchValue( destination ); }

#ifdef RED_ARCH_X64
		typedef OSAPI::SAtomicOps64::TAtomic64 TAtomic64;

		inline TAtomic64		Increment64( TAtomic64 volatile* addend ) { return OSAPI::SAtomicOps64::Increment( addend ); }
		inline TAtomic64		Decrement64( TAtomic64 volatile* addend ) { return OSAPI::SAtomicOps64::Decrement( addend ); }
		inline TAtomic64		Exchange64( TAtomic64 volatile* target, TAtomic64 value ) { return OSAPI::SAtomicOps64::Exchange( target, value ); }
		inline TAtomic64		CompareExchange64( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand ) { return OSAPI::SAtomicOps64::CompareExchange( destination, exchange, comparand ); }
		inline TAtomic64		ExchangeAdd64( TAtomic64 volatile* addend, TAtomic64 value ) { return OSAPI::SAtomicOps64::ExchangeAdd( addend, value ); }
		inline TAtomic64		Or64( TAtomic64 volatile* destination, TAtomic64 value ) { return OSAPI::SAtomicOps64::Or( destination, value ); }
		inline TAtomic64		And64( TAtomic64 volatile* destination, TAtomic64 value ) { return OSAPI::SAtomicOps64::And( destination, value ); }
		inline TAtomic64		FetchValue64( TAtomic64 volatile* destination ) { return OSAPI::SAtomicOps64::FetchValue( destination ); }
#endif
	
		typedef OSAPI::SAtomicOpsPtr::TAtomicPtr TAtomicPtr;

		inline TAtomicPtr		ExchangePtr( TAtomicPtr volatile* target, TAtomicPtr value ) { return OSAPI::SAtomicOpsPtr::Exchange( target, value ); }
		inline TAtomicPtr		CompareExchangePtr( TAtomicPtr volatile* destination, TAtomicPtr exchange, TAtomicPtr comparand ) { return OSAPI::SAtomicOpsPtr::CompareExchange( destination, exchange, comparand ); }
	}

} } // namespace Red { namespace Threads {

namespace Red { namespace Threads {

	//////////////////////////////////////////////////////////////////////////
	// Generic interlocked operation type wrapper. Only 32 and 64 bit integral types supported.
	template< typename T >
	class CAtomic : protected OSAPI::CAtomicIntBase< sizeof(T) >
	{	
		REDTHR_NOCOPY_CLASS( CAtomic );

	private:
		typedef OSAPI::CAtomicIntBase< sizeof(T) > Base;
		typedef typename Base::TAtomic TAtomic;
		typedef typename Base::SAtomicOps SAtomicOps;

	private:
		mutable TAtomic	m_target;

	public:
		CAtomic( T value = T() );

		T			Increment();
		T			Decrement();
		T			Or( T value );
		T			And( T value );
		T			Exchange( T value );
		T			CompareExchange( T exchange, T comparand );
		T			ExchangeAdd( T value );
		void		SetValue( T value );
		T			GetValue() const;
	};

	//////////////////////////////////////////////////////////////////////////
	// Generic pointer interlocked operation type wrapper.
	template< typename T >
	class CAtomic< T* > : protected OSAPI::CAtomicPtrBase
	{
		REDTHR_NOCOPY_CLASS( CAtomic );

	private:
		typedef OSAPI::CAtomicPtrBase Base;
		typedef Base::TAtomicPtr TAtomicPtr;
		typedef Base::SAtomicOps SAtomicOps;

	private:
		mutable TAtomicPtr	m_target;

	public:
		CAtomic( T* value = nullptr );

		//T* operator->() const;

		T*			Exchange( T* value );
		T*			CompareExchange( T* exchange, T* comparand );
		void		SetValue( T* value );
		T*			GetValue() const;
	};

	//////////////////////////////////////////////////////////////////////////
	// Bool interlocked operation type wrapper
	template<>
	class CAtomic< Bool > : protected OSAPI::CAtomicIntBase< sizeof(Int32) >
	{
		REDTHR_NOCOPY_CLASS( CAtomic );

	private:
		typedef OSAPI::CAtomicIntBase< sizeof(Int32) > Base;
		typedef Base::TAtomic TAtomic;
		typedef Base::SAtomicOps SAtomicOps;

	private:
		mutable TAtomic	m_target;

	public:
						CAtomic( Bool value = false );

		Bool			Exchange( Bool value );
		Bool			CompareExchange( Bool exchange, Bool comparand );
		void			SetValue( Bool value );
		Bool			GetValue() const;
	};

} } // namespace Red { namespace Threads

#include "redThreadsAtomic.inl"

#endif // RED_THREADS_ATOMIC_H