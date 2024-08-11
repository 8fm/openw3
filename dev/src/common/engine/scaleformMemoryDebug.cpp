/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../redThreads/redThreadsThread.h"
#include "../core/configVar.h"
#include "scaleformMemoryDebug.h"

#ifdef USE_SCALEFORM

#ifdef SCALEFORM_MEMORY_STATS
namespace Config
{
	TConfigVar< Bool > cvScaleformDebugDumpIngoreSingleHitMemAllocs( "Scaleform/Debug", "DumpIgnoreSingleHitMemAllocs", false );
	TConfigVar< Bool > cvScaleformDebugDumpMemAllocs( "Scaleform/Debug", "DumpMemAllocs", false );
}
#endif

#ifdef SCALEFORM_MEMORY_STATS
namespace ScaleformDebugHelpers
{

CCallstackTable CallstackTable;

static Uint64 RuntimeHash( const Callstack& callstack )
{
	const Uint64 hash = Red::System::CalculateHash64( &callstack.frame[0], sizeof(Callstack::CallstackFrames) * callstack.numFrames );
	RED_FATAL_ASSERT( hash != 0, "Unviable zero hash");
	return hash;
}


static const AnsiChar* GetShorterScaleformString( const AnsiChar* str )
{
	const AnsiChar* ch = str;

	const size_t sfLen = Red::System::StringLengthCompileTime("Scaleform::");
	if ( Red::System::StringCompare( ch, "Scaleform::", sfLen) == 0 )
	{
		ch += sfLen;
	}

	const size_t gfxLen = Red::System::StringLengthCompileTime("GFx::");
	if ( Red::System::StringCompare( ch, "GFx::", gfxLen) == 0 )
	{
		ch += gfxLen;
	}

	return ch;
}

CCallstackTable::CCallstackTable()
	: m_totalHits(0)
{
	m_callstackMap.Reserve( TABLE_SIZE );
}

Bool CCallstackTable::AddAlloc( const Callstack& callstack, Uint64 size, Uint64 align )
{
	const Uint64 hash = RuntimeHash( callstack );

	CScopedLock lock( m_lock );

	CallstackInfo* pInfo = m_callstackMap.FindPtr( hash );
	if ( !pInfo )
	{
		CallstackInfo newInfo;
		InitInfo_NoLock( newInfo, callstack );
		m_callstackMap.Insert( hash, Move( newInfo ) );
		pInfo = m_callstackMap.FindPtr( hash );
		RED_FATAL_ASSERT( pInfo, "Broken hash?");
	}

	const Uint32 alignPow2 = (align == 0) ? 0 : static_cast<Uint32>(Red::System::BitUtils::Log2(align));
	const AllocInfo allocInfo = { (Uint32)size, alignPow2 };
	UpdateStats_NoLock( *pInfo, allocInfo );

	++pInfo->m_totalHits;
	++m_totalHits;

	return true;
}

void CCallstackTable::Dump()
{
	CScopedLock lock( m_lock );

	TDynArray< CallstackInfo > callstacks;
	m_callstackMap.GetValues( callstacks );

	const Bool ignoreSingleHit = Config::cvScaleformDebugDumpIngoreSingleHitMemAllocs.Get();

	Printf( "\n== callstacks=%u, total hits=%u==\n", m_callstackMap.Size(), m_totalHits);
	Printf( "\n> By total hits\n");

	Sort( callstacks.Begin(), callstacks.End(), [](const CallstackInfo& a, const CallstackInfo& b) { return a.m_totalHits > b.m_totalHits; } );
	for ( const auto& it : callstacks )
	{
		if ( !ignoreSingleHit || it.m_totalHits > 1 )
			Dump_NoLock( it );
	}

	Printf("\n> By alloc size\n");
	Sort( callstacks.Begin(), callstacks.End(), [](const CallstackInfo& a, const CallstackInfo& b ) 
	{
		AllocInfo maxA = { 0, 0 };
		AllocInfo maxB = { 0, 0 };

		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(a.m_maxAllocs); ++i )
		{
			if ( a.m_maxAllocs[i].IsValid() && a.m_maxAllocs[i] > maxA )
				maxA = a.m_maxAllocs[i];
			if ( b.m_maxAllocs[i].IsValid() && b.m_maxAllocs[i] > maxB )
				maxB = b.m_maxAllocs[i];
		}

		return maxA > maxB;
	});
	for ( const auto& it : callstacks )
	{
		if ( !ignoreSingleHit || it.m_totalHits > 1 )
			Dump_NoLock( it );
	}
}

void CCallstackTable::UpdateStats_NoLock( CallstackInfo& callstackInfo, const AllocInfo allocInfo )
{
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(callstackInfo.m_maxAllocs); ++i )
	{
		if ( allocInfo == callstackInfo.m_maxAllocs[i] )
		{
			break;
		}
		else if ( allocInfo > callstackInfo.m_maxAllocs[i] )
		{
			// Shift the stats down at the insertion point
			const Uint32 numToCopy = ARRAY_COUNT_U32(callstackInfo.m_maxAllocs) - 1 - i;
			if ( numToCopy > 0 )
			{
				Red::System::MemoryCopy( &callstackInfo.m_maxAllocs[i+1], &callstackInfo.m_maxAllocs[i], sizeof(AllocInfo)*numToCopy);
			}
			callstackInfo.m_maxAllocs[i] = allocInfo;
			break;
		}
	}
}

void CCallstackTable::Dump_NoLock( const CallstackInfo& callstackInfo )
{
	Printf( "hits=%u (%.2f%%)\n", callstackInfo.m_totalHits, (callstackInfo.m_totalHits * 100.f / m_totalHits ) );
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(callstackInfo.m_maxAllocs); ++i )
	{
		const AllocInfo& allocInfo = callstackInfo.m_maxAllocs[i];
		if ( allocInfo.IsValid() )
		{
			const Uint32 align = 1 << allocInfo.m_alignPow2;
			Printf( "size=%u, align=%u\n", allocInfo.m_size, align );
		}
	}

	for ( Uint32 i = 0; i < callstackInfo.m_numFrames; ++i )
	{
		const Uint64 frameHash = callstackInfo.m_frames[i];
		const AnsiChar* symbol = "<Unknown>";
		if (!m_stringTable.GetString( frameHash, symbol ))
		{
			RED_FATAL("Unknown string for hash");
		}
		Printf( "\t%hs\n", symbol );
	}
	Printf("\n");
}

void CCallstackTable::InitInfo_NoLock( CallstackInfo& outInfo, const Callstack& callstack )
{
	RED_FATAL_ASSERT( callstack.numFrames <= ARRAY_COUNT_U32(outInfo.m_frames), "Callstack has more frames than supported!");

	Red::System::MemoryZero( &outInfo, sizeof(CallstackInfo) );
	outInfo.m_numFrames = callstack.numFrames;
	for ( Uint32 i = 0; i < callstack.numFrames; ++i )
	{
		const Callstack::CallstackFrames& frame = callstack.frame[i];
		Uint64 strHash = 0;
		if ( !m_stringTable.HashString(UNICODE_TO_ANSI(frame.symbol), strHash) )
		{
			RED_FATAL("Failed to allocate string for stack frame symbol '%ls'", frame.symbol);
		}
		outInfo.m_frames[i] = strHash;
	}

	Red::System::MemorySet( outInfo.m_maxAllocs, 0x0, sizeof(outInfo.m_maxAllocs) );
	//Red::System::MemorySet( outInfo.m_minAllocs, 0xF, sizeof(outInfo.m_minAllocs) );
}

} // namespace ScaleformDebugHelpers
#endif // SCALEFORM_MEMORY_STATS

#endif // USE_SCALEFORM



