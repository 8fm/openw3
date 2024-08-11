/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_INTEGRATION_STUB_H_
#define _RED_MEMORY_INTEGRATION_STUB_H_

#include "../../redThreads/redThreadsAtomic.h"
#include "../../redSystem/utility.h"
#include "../../redThreads/redThreadsThread.h"
#include "../../redThreads/readWriteSpinLock.h"
#include "../../redMath/numericalutils.h"
#include "../../redSystem/bitUtils.h"
#include "../../redSystem/unitTestMode.h"
#include "../../redSystem/log.h"

namespace red
{
	namespace Error = Red::System::Error;
	namespace Math = Red::Math;
	namespace BitUtils = Red::System::BitUtils;

	typedef bool				Bool;

	typedef char				Int8;
	typedef unsigned char		Uint8;

	typedef std::int16_t		Int16;
	typedef std::uint16_t		Uint16;

	typedef std::int32_t		Int32;
	typedef std::uint32_t		Uint32;

	typedef std::int64_t		Int64;
	typedef std::uint64_t		Uint64;

	typedef float				Float;
	typedef double				Double;

	typedef wchar_t				UniChar;
	typedef char				AnsiChar;

	typedef size_t				MemSize;
	typedef ptrdiff_t			MemDiff;
	typedef intptr_t			MemInt;
	typedef	uintptr_t			MemUint;

	typedef Red::System::NonCopyable NonCopyable;

#if defined( UNICODE )

	typedef UniChar				Char;
#	define TXT(s)				L##s

#else
	// Prevent mix and matching across projects.
#error Should be using Unicode but not defined
	typedef AnsiChar			Char;
#	define TXT(s)				s

#endif

	typedef Red::Threads::CSpinLock CSpinLock;
	typedef Red::Threads::CRWSpinLock CRWSpinLock; 
	typedef Red::Threads::CMutex CMutex; 

	
	template< typename TAcquireRelease >
	class CScopedLock
	{	
	public:
		CScopedLock( TAcquireRelease& syncObject )
			: m_syncObjectRef( syncObject )
		{
			m_syncObjectRef.Acquire();
		}

		~CScopedLock()
		{
			m_syncObjectRef.Release();
		}

	private:

		TAcquireRelease& m_syncObjectRef;
	};

	template< typename TAcquireReleaseShared >
	class CScopedSharedLock
	{	
	public:
		CScopedSharedLock( TAcquireReleaseShared& syncObject )
			: m_syncObjectRef( syncObject )
		{
			m_syncObjectRef.AcquireShared();
		}
		
		~CScopedSharedLock()
		{
			m_syncObjectRef.ReleaseShared();
		}
	
	private:
		TAcquireReleaseShared& m_syncObjectRef;

	};

	template< typename T >
	struct ScopedFlag : public NonCopyable
	{
	private:
		T& m_flag;
		T  m_finalValue;

	public:
		ScopedFlag( T& flag, T finalValue )
			:	m_flag( flag ),
			m_finalValue( finalValue )
		{}

		~ScopedFlag() { m_flag = m_finalValue; }
	};

	inline void Memcpy( void* __restrict dest, const void* __restrict source, size_t size ) { Red::System::MemoryCopy( dest, source, size ); }
	inline void Memzero( void* buffer, size_t size ) { Red::System::MemoryZero( buffer, size ); }
	inline void Memset( void* buffer, Int32 value, size_t size ) { Red::System::MemorySet( buffer, value, size ); }

	inline size_t Strlen( const AnsiChar* str )												{ return std::strlen( str ); }
	inline size_t Strlen( const UniChar* str )												{ return ::wcslen( str ); }
	inline size_t Strlen( const AnsiChar* str, size_t maxBufferSize )							{ return ::strnlen_s( str, maxBufferSize ); }
	inline size_t Strlen( const UniChar* str, size_t maxBufferSize )							{ return ::wcsnlen_s( str, maxBufferSize ); }

	inline bool UnitTestMode() { return Red::System::UnitTestMode(); }

	namespace Log
	{
		typedef Red::Log::Manager Manager;
	}

	namespace atomic
	{
		typedef Red::Threads::AtomicOps::TAtomic32 TAtomic32;
		typedef Red::Threads::AtomicOps::TAtomic64 TAtomic64;
		typedef Red::Threads::AtomicOps::TAtomicPtr TAtomicPtr;

		inline TAtomic32		Increment32( TAtomic32 volatile* addend ) { return Red::Threads::AtomicOps::Increment32( addend ); }
		inline TAtomic32		Decrement32( TAtomic32 volatile* addend ) { return Red::Threads::AtomicOps::Decrement32( addend ); }
		inline TAtomic32		Exchange32( TAtomic32 volatile* target, TAtomic32 value ) { return Red::Threads::AtomicOps::Exchange32( target, value ); }  
		inline TAtomic32		CompareExchange32( TAtomic32 volatile* destination, TAtomic32 exchange, TAtomic32 comparand ) { return Red::Threads::AtomicOps::CompareExchange32( destination, exchange, comparand ); }
		inline TAtomic32		ExchangeAdd32( TAtomic32 volatile* addend, TAtomic32 value )  { return Red::Threads::AtomicOps::ExchangeAdd32( addend, value ); }
		inline TAtomic32		Or32( TAtomic32 volatile* destination, TAtomic32 value ) { return Red::Threads::AtomicOps::Or32( destination, value ); }
		inline TAtomic32		And32( TAtomic32 volatile* destination, TAtomic32 value ) { return Red::Threads::AtomicOps::And32( destination, value ); }
		inline TAtomic32		FetchValue32( TAtomic32 volatile* destination ) {  return Red::Threads::AtomicOps::FetchValue32( destination ); }


		inline TAtomic64		Increment64( TAtomic64 volatile* addend ) { return Red::Threads::AtomicOps::Increment64( addend ); }
		inline TAtomic64		Decrement64( TAtomic64 volatile* addend ) { return Red::Threads::AtomicOps::Decrement64( addend ); }
		inline TAtomic64		Exchange64( TAtomic64 volatile* target, TAtomic64 value ) { return Red::Threads::AtomicOps::Exchange64( target, value ); }
		inline TAtomic64		CompareExchange64( TAtomic64 volatile* destination, TAtomic64 exchange, TAtomic64 comparand ) { return Red::Threads::AtomicOps::CompareExchange64( destination, exchange, comparand ); }
		inline TAtomic64		ExchangeAdd64( TAtomic64 volatile* addend, TAtomic64 value ) { return Red::Threads::AtomicOps::ExchangeAdd64( addend, value ); }
		inline TAtomic64		Or64( TAtomic64 volatile* destination, TAtomic64 value ) { return Red::Threads::AtomicOps::Or64( destination, value ); }
		inline TAtomic64		And64( TAtomic64 volatile* destination, TAtomic64 value ) { return Red::Threads::AtomicOps::And64( destination, value ); }
		inline TAtomic64		FetchValue64( TAtomic64 volatile* destination ) { return Red::Threads::AtomicOps::FetchValue64( destination ); }

		
		inline TAtomicPtr		ExchangePtr( TAtomicPtr volatile* target, TAtomicPtr value ) { return Red::Threads::AtomicOps::ExchangePtr( target, value ); }
		inline TAtomicPtr		CompareExchangePtr( TAtomicPtr volatile* destination, TAtomicPtr exchange, TAtomicPtr comparand ) { return Red::Threads::AtomicOps::CompareExchangePtr( destination, exchange, comparand ); }
	}
}

#endif
