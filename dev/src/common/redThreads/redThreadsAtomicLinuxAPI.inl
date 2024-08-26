/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_ATOMIC_LINUXAPI_H
#define RED_THREADS_ATOMIC_LINUXAPI_H
#pragma once

#define RED_THREADS_ATOMIC_MEMORDER __ATOMIC_RELAXED

namespace Red { namespace Threads { namespace LinuxAPI {

	struct SAtomicOps32
	{
		typedef int32_t TAtomic32 __attribute__( ( aligned( 4 ) ) );

		// Conform to Win32 InterlockedIncrement by returning the new value
		inline static TAtomic32		Increment( TAtomic32 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic32		Decrement( TAtomic32 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

		// Conform to Win32 Interlocked* by returning the old value
		inline static TAtomic32		Exchange( TAtomic32 volatile* target, TAtomic32 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic32		CompareExchange( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand )
		{
			__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
		}
		inline static TAtomic32		ExchangeAdd( TAtomic32 volatile* addend, TAtomic32 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic32		Or( TAtomic32 volatile* destination, TAtomic32 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic32		And( TAtomic32 volatile* destination, TAtomic32 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic32		FetchValue( TAtomic32 volatile* destination ) { return *( volatile TAtomic32* )( destination ); }
	};

#ifdef RED_ARCH_X64
	struct SAtomicOps64
	{
		typedef int64_t TAtomic64 __attribute__( ( aligned( 8 ) ) );

		// Conform to Win32 InterlockedIncrement by returning the new value
		inline static TAtomic64		Increment( TAtomic64 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic64		Decrement( TAtomic64 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

		// Conform to Win32 Interlocked* by returning the old value
		inline static TAtomic64		Exchange( TAtomic64 volatile* target, TAtomic64 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic64		CompareExchange( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand )
		{
			__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
		}
		inline static TAtomic64		ExchangeAdd( TAtomic64 volatile* addend, TAtomic64 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic64		Or( TAtomic64 volatile* destination, TAtomic64 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic64		And( TAtomic64 volatile* destination, TAtomic64 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
		inline static TAtomic64		FetchValue( TAtomic64 volatile* destination ) { return *( volatile TAtomic64* )( destination ); }
	};
#endif

	struct SAtomicOpsPtr
	{
		typedef void* TAtomicPtr __attribute__( ( aligned( 8 ) ) );

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
			return *( volatile TAtomicPtr* )( destination );
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

#ifdef RED_ARCH_X64
	template <>
	class CAtomicIntBase<8U>
	{
	protected:
		typedef SAtomicOps64::TAtomic64 TAtomic;
		typedef SAtomicOps64 SAtomicOps;
	};
#endif

	class CAtomicPtrBase
	{
	protected:
		typedef SAtomicOpsPtr::TAtomicPtr TAtomicPtr;
		typedef SAtomicOpsPtr SAtomicOps;
	};

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

#endif // RED_THREADS_ATOMIC_LINUXAPI_H
