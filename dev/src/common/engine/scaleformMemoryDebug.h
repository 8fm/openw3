/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#ifndef RED_FINAL_BUILD
//# define SCALEFORM_MEMORY_STATS
#endif

#ifdef SCALEFORM_MEMORY_STATS


namespace Config
{
	extern TConfigVar< Bool > cvScaleformDebugDumpIngoreSingleHitMemAllocs;
	extern TConfigVar< Bool > cvScaleformDebugDumpMemAllocs;
}

namespace ScaleformDebugHelpers
{
typedef Red::System::Error::Callstack Callstack;

inline void Printf( const AnsiChar* fmt, ... )
{
	AnsiChar buf[512];
	va_list args;
	va_start( args, fmt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), fmt, args );
	va_end( args );

#if defined( RED_PLATFORM_WINPC )
	::OutputDebugStringA( buf );
#elif defined( RED_PLATFORM_DURANGO )
	::OutputDebugString( ANSI_TO_UNICODE( buf ) );
#else
	::fputws( ANSI_TO_UNICODE( buf ), stderr );
#endif
}


template< Uint32 N >
class CStringTable
{
public:
	CStringTable()
	{
		Red::System::MemoryZero( m_stringTable, sizeof(m_stringTable) );
		m_nextFree = &m_stringTable[0];

		const Uint32 AVG_SYMBOL_LEN = 32;
		m_stringMap.Reserve( Min<Uint32>(1, N / AVG_SYMBOL_LEN) );
	}

public:
	Bool HashString( const AnsiChar* str, Uint64& outHash )
	{
		outHash = Red::System::CalculateHash64( str );
		if ( m_stringMap.KeyExist(outHash) )
		{
			return true;
		}

		const AnsiChar* shorterString = GetShorterScaleformString( str );
		const size_t len = Red::System::StringLength( shorterString );

		const AnsiChar* strEntry = AllocString( shorterString, len );
		if ( !strEntry )
		{
			return false;
		}

		RED_VERIFY( m_stringMap.Insert( outHash, strEntry ) );

		return true;
	}

	Bool GetString( Uint64 hash, const AnsiChar*& outString ) const
	{
		return m_stringMap.Find( hash, outString );
	}

private:
	enum { NULL_SZ = 1 };

	const AnsiChar* AllocString( const AnsiChar* str, size_t len )
	{
		const ptrdiff_t used = m_nextFree - m_stringTable;
		if ( used + len + NULL_SZ > N )
		{
			return nullptr;
		}

		const AnsiChar* ret = m_nextFree;

		Red::System::MemoryCopy( m_nextFree, str, len );
		m_nextFree += len;
		*m_nextFree++ = '\0';

		return ret;
	}

private:
	typedef THashMap< Uint64, const AnsiChar* > StringMap;

private:
	StringMap	m_stringMap;
	AnsiChar	m_stringTable[N];
	AnsiChar*	m_nextFree;
};

class CCallstackTable
{
public:
	CCallstackTable();
	Bool AddAlloc( const Callstack& callstack, Uint64 size, Uint64 align );
	void Dump();

private:
	struct AllocInfo
	{
		Uint32 m_size:28;
		Uint32 m_alignPow2:4;

		Bool IsValid() const { const Uint32 uval = m_size | (m_alignPow2<<28); return uval != 0 && uval != 0xFFFFFFFF; }

		RED_FORCE_INLINE Bool operator<(const AllocInfo rhs) const
		{
			if ( m_alignPow2 == rhs.m_alignPow2 )
			{
				return m_size < rhs.m_size;
			}
			return m_alignPow2 < rhs.m_alignPow2;
		}

		RED_FORCE_INLINE Bool operator==(const AllocInfo rhs ) const { return m_alignPow2 == rhs.m_alignPow2 && m_size == rhs.m_size; }
		RED_FORCE_INLINE Bool operator!=(const AllocInfo rhs ) const { return !(*this == rhs); }
		RED_FORCE_INLINE Bool operator>(const AllocInfo rhs ) const { return !(*this < rhs) && *this != rhs; }
	};

	struct CallstackInfo
	{
		Uint64			m_frames[64];
		AllocInfo		m_maxAllocs[64];
		Uint32			m_totalHits;
		Uint32			m_numFrames;
	};

private:
	void UpdateStats_NoLock( CallstackInfo& callstackInfo, const AllocInfo allocInfo );

private:
	void Dump_NoLock( const CallstackInfo& callstackInfo );

private:
	void InitInfo_NoLock( CallstackInfo& outInfo, const Callstack& callstack );

private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	enum { TABLE_SIZE = 257 };
	enum { NUM_SYMBOL_CHARS = 1024 * 1024 };

private:
	mutable CMutex m_lock;
	typedef THashMap< Uint64, CallstackInfo, DefaultHashFunc<Uint64>, DefaultEqualFunc<Uint64>, MC_Debug > InfoMap;

	InfoMap m_callstackMap;
	CStringTable< NUM_SYMBOL_CHARS > m_stringTable;
	Uint64 m_totalHits;
};

extern CCallstackTable CallstackTable;

} // namespace ScaleformDebugHelpers
#endif // SCALEFORM_MEMORY_STATS


#endif // USE_SCALEFORM
