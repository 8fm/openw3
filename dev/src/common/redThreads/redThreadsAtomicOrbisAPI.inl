/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_ATOMIC_ORBISAPI_INL
#define RED_THREADS_ATOMIC_ORBISAPI_INL
#pragma once

#include <sce_atomic.h>

namespace Red { namespace Threads { namespace OrbisAPI {

	/*!
		Special cased to return the same value as Windows InterlockedIncrement/Decrement.
		sceAtomicIncrement/Decrement() return the initial value with __sync_fetch_and_add,
		whereas InterlockdIncrement/Decrement() return the new value.
	*/
	static inline int32_t redThreadsAtomicIncrement32(volatile int32_t* ptr) { return __sync_add_and_fetch(ptr, 1);	}
	static inline int32_t redThreadsAtomicDecrement32(volatile int32_t* ptr) { return __sync_add_and_fetch(ptr, -1); }

	struct SAtomicOps32
	{
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef int32_t TAtomic32 __attribute__((aligned (4) ));

		inline static TAtomic32		Increment( TAtomic32 volatile* addend ) { return redThreadsAtomicIncrement32( addend ); }
		inline static TAtomic32		Decrement( TAtomic32 volatile* addend ) { return redThreadsAtomicDecrement32( addend ); }
		inline static TAtomic32		Exchange( TAtomic32 volatile* target, TAtomic32 value ) { return ::sceAtomicExchange32( target, value ); }
		inline static TAtomic32		CompareExchange( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand ) { return ::sceAtomicCompareAndSwap32( destination, comparand, exchange ); }
		inline static TAtomic32		ExchangeAdd( TAtomic32 volatile* addend, TAtomic32 value ) { return ::sceAtomicAdd32( addend, value ); }
		inline static TAtomic32		Or( TAtomic32 volatile* destination, TAtomic32 value ) { return ::sceAtomicOr32( destination, value ); }
		inline static TAtomic32		And( TAtomic32 volatile* destination, TAtomic32 value ) { return ::sceAtomicAnd32( destination, value ); }
		inline static TAtomic32		FetchValue( TAtomic32 volatile* destination) { return *(volatile TAtomic32*)(destination); }
	};

	/*!
		Special cased to return the same value as Windows InterlockedIncrement/Decrement.
		sceAtomicIncrement/Decrement() return the initial value with __sync_fetch_and_add,
		whereas InterlockdIncrement/Decrement() return the new value.
	*/
	static inline int64_t redThreadsAtomicIncrement64(volatile int64_t* ptr) { return __sync_add_and_fetch(ptr, 1); }
	static inline int64_t redThreadsAtomicDecrement64(volatile int64_t* ptr) { return __sync_add_and_fetch(ptr, -1); }

	struct SAtomicOps64
	{
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef int64_t TAtomic64 __attribute__((aligned (8) ));

		inline static TAtomic64		Increment( TAtomic64 volatile* addend ) { return redThreadsAtomicIncrement64( addend ); }
		inline static TAtomic64		Decrement( TAtomic64 volatile* addend ) { return redThreadsAtomicDecrement64( addend ); }
		inline static TAtomic64		Exchange( TAtomic64 volatile* target, TAtomic64 value ) { return ::sceAtomicExchange64( target, value ); }
		inline static TAtomic64		CompareExchange( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand ) { return ::sceAtomicCompareAndSwap64( destination, comparand, exchange ); }
		inline static TAtomic64		ExchangeAdd( TAtomic64 volatile* addend, TAtomic64 value ) { return ::sceAtomicAdd64( addend, value ); }
		inline static TAtomic64		Or( TAtomic64 volatile* destination, TAtomic64 value ) { return ::sceAtomicOr64( destination, value ); }
		inline static TAtomic64		And( TAtomic64 volatile* destination, TAtomic64 value ) { return ::sceAtomicAnd64( destination, value ); }		
		inline static TAtomic64		FetchValue( TAtomic64 volatile* destination ) { return *(volatile TAtomic64*)(destination); }
	};

	struct SAtomicOpsPtr
	{
		typedef void* TAtomicPtr __attribute__((aligned (8) ));

		inline static TAtomicPtr Exchange( TAtomicPtr volatile* target, TAtomicPtr value )
		{ 
			const SAtomicOps64::TAtomic64 retval =
			SAtomicOps64::Exchange
				(
					reinterpret_cast< SAtomicOps64::TAtomic64 volatile * >( target ),
					reinterpret_cast< SAtomicOps64::TAtomic64 >( value )
				);

			return reinterpret_cast< TAtomicPtr >( retval );
		}

		inline static TAtomicPtr CompareExchange( TAtomicPtr volatile* destination, TAtomicPtr exchange, TAtomicPtr comparand )
		{
			const SAtomicOps64::TAtomic64 retval =
			SAtomicOps64::CompareExchange
				( 
					reinterpret_cast< SAtomicOps64::TAtomic64 volatile * >( destination ),
					reinterpret_cast< SAtomicOps64::TAtomic64 >( exchange ),
					reinterpret_cast< SAtomicOps64::TAtomic64 >( comparand )
				);
			return reinterpret_cast< TAtomicPtr >( retval );
		}

		inline static TAtomicPtr FetchValue( TAtomicPtr volatile* destination )
		{
			return *(volatile TAtomicPtr*)(destination);
		}

	};

	template <size_t Size>
	class CAtomicIntBase
	{
	};

	template <>
	class CAtomicIntBase<4U>
	{
	protected:
		typedef SAtomicOps32::TAtomic32 TAtomic;
		typedef SAtomicOps32 SAtomicOps;
	};

	template <>
	class CAtomicIntBase<8U>
	{
	protected:
		typedef SAtomicOps64::TAtomic64 TAtomic;
		typedef SAtomicOps64 SAtomicOps;
	};

	class CAtomicPtrBase
	{
	protected:
		typedef SAtomicOpsPtr::TAtomicPtr TAtomicPtr;
		typedef SAtomicOpsPtr SAtomicOps;
	};

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

#endif // RED_THREADS_ATOMIC_ORBISAPI_INL