/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_ATOMIC_WINAPI_H
#define RED_THREADS_ATOMIC_WINAPI_H
#pragma once

#include <intrin.h>

#pragma intrinsic( _InterlockedIncrement )
#pragma intrinsic( _InterlockedDecrement )
#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedExchangeAdd )
#pragma intrinsic( _InterlockedOr )
#pragma intrinsic( _InterlockedAnd )

#ifdef RED_ARCH_X64
#pragma intrinsic( _InterlockedIncrement64 )
#pragma intrinsic( _InterlockedDecrement64 )
#pragma intrinsic( _InterlockedExchange64 )
#pragma intrinsic( _InterlockedCompareExchange64 )
#pragma intrinsic( _InterlockedExchangeAdd64 )

#pragma intrinsic( _InterlockedExchangePointer )
#pragma intrinsic( _InterlockedCompareExchangePointer )
#pragma intrinsic( _InterlockedOr64 )
#pragma intrinsic( _InterlockedAnd64 )

#define REDTHR_InterlockedExchangePointer _InterlockedExchangePointer
#define REDTHR_InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#else

#define REDTHR_InterlockedExchangePointer InterlockedExchangePointer
#define REDTHR_InterlockedCompareExchangePointer InterlockedCompareExchangePointer

#endif

namespace Red { namespace Threads { namespace WinAPI {

	struct SAtomicOps32
	{
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef __declspec(align(4)) LONG TAtomic32;

		inline static TAtomic32		Increment( TAtomic32 volatile* addend ) { return ::_InterlockedIncrement( addend ); }
		inline static TAtomic32		Decrement( TAtomic32 volatile* addend ) { return ::_InterlockedDecrement( addend ); }
		inline static TAtomic32		Exchange( TAtomic32 volatile* target, TAtomic32 value ) { return ::_InterlockedExchange( target, value ); }  
		inline static TAtomic32		CompareExchange( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand ) { return ::_InterlockedCompareExchange( destination, exchange, comparand ); }
		inline static TAtomic32		ExchangeAdd( TAtomic32 volatile* addend, TAtomic32 value )  { return ::_InterlockedExchangeAdd( addend, value ); }
		inline static TAtomic32		Or( TAtomic32 volatile* destination, TAtomic32 value ) { return ::_InterlockedOr( destination, value ); }
		inline static TAtomic32		And( TAtomic32 volatile* destination, TAtomic32 value ) { return ::_InterlockedAnd( destination, value ); }
		inline static TAtomic32		FetchValue( TAtomic32 volatile* destination) { return *(volatile TAtomic32*)(destination); }
	};

#ifdef RED_ARCH_X64
	struct SAtomicOps64
	{
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef __declspec(align(8)) LONG64 TAtomic64;

		inline static TAtomic64		Increment( TAtomic64 volatile* addend ) { return ::_InterlockedIncrement64( addend ); }
		inline static TAtomic64		Decrement( TAtomic64 volatile* addend ) { return ::_InterlockedDecrement64( addend ); }
		inline static TAtomic64		Exchange( TAtomic64 volatile* target, TAtomic64 value ) { return ::_InterlockedExchange64( target, value ); }
		inline static TAtomic64		CompareExchange( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand ) { return ::_InterlockedCompareExchange64( destination, exchange, comparand ); }
		inline static TAtomic64		ExchangeAdd( TAtomic64 volatile* addend, TAtomic64 value ) { return ::_InterlockedExchangeAdd64( addend, value ); }
		inline static TAtomic64		Or( TAtomic64 volatile* destination, TAtomic64 value ) { return ::_InterlockedOr64( destination, value ); }
		inline static TAtomic64		And( TAtomic64 volatile* destination, TAtomic64 value ) { return ::_InterlockedAnd64( destination, value ); }
		inline static TAtomic64		FetchValue( TAtomic64 volatile* destination ) { return *(volatile TAtomic64*)(destination); }
	};
#endif

	struct SAtomicOpsPtr
	{
#ifdef RED_ARCH_X64
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef __declspec(align(8)) void* TAtomicPtr;
#else
		//FIXME: redSystem align macro... alignas isn't a good match for MSVC __declspec(align).
		typedef __declspec(align(4)) void* TAtomicPtr;
#endif

		inline static TAtomicPtr Exchange( TAtomicPtr volatile* target, TAtomicPtr value ) { return REDTHR_InterlockedExchangePointer( target, value ); }
		inline static TAtomicPtr CompareExchange( TAtomicPtr volatile* destination, TAtomicPtr exchange, TAtomicPtr comparand ) { return REDTHR_InterlockedCompareExchangePointer( destination, exchange, comparand ); }
		inline static TAtomicPtr FetchValue( TAtomicPtr volatile* destination ) { return *(volatile TAtomicPtr*)(destination); }
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

} } } // namespace Red { namespace Threads { namespace WinAPI {

#endif // RED_THREADS_ATOMIC_WINAPI_H
