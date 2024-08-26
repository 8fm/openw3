/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_ATOMIC_LINUXAPI_H
#define RED_THREADS_ATOMIC_LINUXAPI_H
#pragma once

#define RED_THREADS_ATOMIC_MEMORDER __ATOMIC_RELAXED

namespace red
{
	namespace LinuxAPI
	{

		struct AtomicOps8
		{
			typedef char TAtomic8;

			// Conform to Win32 InterlockedIncrement by returning the new value
			RED_FORCE_INLINE static TAtomic8		Increment( TAtomic8 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic8		Decrement( TAtomic8 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

			// Conform to Win32 Interlocked* by returning the old value
			RED_FORCE_INLINE static TAtomic8		Exchange( TAtomic8 volatile* target, TAtomic8 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic8		CompareExchange( TAtomic8 volatile* destination, TAtomic8 exchange, TAtomic8 comparand )
			{
				__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
			}
			RED_FORCE_INLINE static TAtomic8		ExchangeAdd( TAtomic8 volatile* addend, TAtomic8 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic8		Or( TAtomic8 volatile* destination, TAtomic8 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic8		And( TAtomic8 volatile* destination, TAtomic8 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic8		FetchValue( TAtomic8 volatile* destination ) { return *( volatile TAtomic8* )( destination ); }
		};

		struct AtomicOps16
		{
			typedef short TAtomic16 __attribute__( ( aligned( 2 ) ) );

			// Conform to Win32 InterlockedIncrement by returning the new value
			RED_FORCE_INLINE static TAtomic16		Increment( TAtomic16 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic16		Decrement( TAtomic16 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

			// Conform to Win32 Interlocked* by returning the old value
			RED_FORCE_INLINE static TAtomic16		Exchange( TAtomic16 volatile* target, TAtomic16 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic16		CompareExchange( TAtomic16 volatile* destination, TAtomic16 exchange, TAtomic16 comparand )
			{
				__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
			}
			RED_FORCE_INLINE static TAtomic16		ExchangeAdd( TAtomic16 volatile* addend, TAtomic16 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic16		Or( TAtomic16 volatile* destination, TAtomic16 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic16		And( TAtomic16 volatile* destination, TAtomic16 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic16		FetchValue( TAtomic16 volatile* destination ) { return *( volatile TAtomic16* )( destination ); }
		};

		struct AtomicOps32
		{
			typedef int32_t TAtomic32 __attribute__( ( aligned( 4 ) ) );

			// Conform to Win32 InterlockedIncrement by returning the new value
			RED_FORCE_INLINE static TAtomic32		Increment( TAtomic32 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic32		Decrement( TAtomic32 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

			// Conform to Win32 Interlocked* by returning the old value
			RED_FORCE_INLINE static TAtomic32		Exchange( TAtomic32 volatile* target, TAtomic32 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic32		CompareExchange( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand )
			{
				__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
			}
			RED_FORCE_INLINE static TAtomic32		ExchangeAdd( TAtomic32 volatile* addend, TAtomic32 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic32		Or( TAtomic32 volatile* destination, TAtomic32 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic32		And( TAtomic32 volatile* destination, TAtomic32 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic32		FetchValue( TAtomic32 volatile* destination ) { return *( volatile TAtomic32* )( destination ); }
		};

#ifdef RED_ARCH_X64
		struct AtomicOps64
		{
			typedef int64_t TAtomic64 __attribute__( ( aligned( 8 ) ) );

			// Conform to Win32 InterlockedIncrement by returning the new value
			RED_FORCE_INLINE static TAtomic64		Increment( TAtomic64 volatile* addend ) { return __atomic_add_fetch( addend, 1, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic64		Decrement( TAtomic64 volatile* addend ) { return __atomic_add_fetch( addend, -1, RED_THREADS_ATOMIC_MEMORDER ); }

			// Conform to Win32 Interlocked* by returning the old value
			RED_FORCE_INLINE static TAtomic64		Exchange( TAtomic64 volatile* target, TAtomic64 value ) { return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic64		CompareExchange( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand )
			{
				__atomic_compare_exchange_n( destination, &comparand, exchange, false, RED_THREADS_ATOMIC_MEMORDER, RED_THREADS_ATOMIC_MEMORDER ); return comparand;
			}
			RED_FORCE_INLINE static TAtomic64		ExchangeAdd( TAtomic64 volatile* addend, TAtomic64 value ) { return __atomic_fetch_add( addend, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic64		Or( TAtomic64 volatile* destination, TAtomic64 value ) { return __atomic_fetch_or( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic64		And( TAtomic64 volatile* destination, TAtomic64 value ) { return __atomic_fetch_and( destination, value, RED_THREADS_ATOMIC_MEMORDER ); }
			RED_FORCE_INLINE static TAtomic64		FetchValue( TAtomic64 volatile* destination ) { return *( volatile TAtomic64* )( destination ); }
		};
#endif

		struct AtomicOpsPtr
		{
			typedef void* TAtomicPtr __attribute__( ( aligned( 8 ) ) );

			RED_FORCE_INLINE static TAtomicPtr Exchange( TAtomicPtr volatile* target, TAtomicPtr value )
			{
				const AtomicOps64::TAtomic64 retval =
					AtomicOps64::Exchange
					(
						reinterpret_cast< AtomicOps64::TAtomic64 volatile * >( target ),
						reinterpret_cast< AtomicOps64::TAtomic64 >( value )
					);

				return reinterpret_cast< TAtomicPtr >( retval );
			}

			RED_FORCE_INLINE static TAtomicPtr CompareExchange( TAtomicPtr volatile* destination, TAtomicPtr exchange, TAtomicPtr comparand )
			{
				const AtomicOps64::TAtomic64 retval =
					AtomicOps64::CompareExchange
					(
						reinterpret_cast< AtomicOps64::TAtomic64 volatile * >( destination ),
						reinterpret_cast< AtomicOps64::TAtomic64 >( exchange ),
						reinterpret_cast< AtomicOps64::TAtomic64 >( comparand )
					);
				return reinterpret_cast< TAtomicPtr >( retval );
			}

			RED_FORCE_INLINE static TAtomicPtr FetchValue( TAtomicPtr volatile* destination )
			{
				return *( volatile TAtomicPtr* )( destination );
			}
		};

		template <size_t Size>
		class AtomicIntBase
		{
		};

		template <>
		class AtomicIntBase<1U>
		{
		protected:
			typedef AtomicOps8::TAtomic8 TAtomic;
			typedef AtomicOps8 AtomicOps;
		};

		template <>
		class AtomicIntBase<2U>
		{
		protected:
			typedef AtomicOps16::TAtomic16 TAtomic;
			typedef AtomicOps16 AtomicOps;
		};

		template <>
		class AtomicIntBase<4U>
		{
		protected:
			typedef AtomicOps32::TAtomic32 TAtomic;
			typedef AtomicOps32 AtomicOps;
		};

		template <>
		class AtomicIntBase<8U>
		{
		protected:
			typedef AtomicOps64::TAtomic64 TAtomic;
			typedef AtomicOps64 AtomicOps;
		};

		class AtomicPtrBase
		{
		protected:
			typedef AtomicOpsPtr::TAtomicPtr TAtomicPtr;
			typedef AtomicOpsPtr AtomicOps;
		};

	}
} // namespace red { namespace LinuxAPI {

#endif // RED_THREADS_ATOMIC_LINUXAPI_H
