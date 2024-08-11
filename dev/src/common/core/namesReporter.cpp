/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "namesReporter.h"
#include "names.h"
#include "fileSkipableBlock.h"

void CNamesRemapper::Reset()
{
	m_counter.SetValue( 0 );
	Red::MemoryZero( m_toLocal, sizeof( Uint32 ) * MAX_NUM_GLOBAL_NAMES );
	Red::MemoryZero( m_toGlobal, sizeof( Uint32 ) * MAX_NUM_LOCAL_NAMES );
}

void CNamesRemapper::Save( IFile* writer )
{
	CFileSkipableBlock fileSkipBlock( *writer );
	{
		Uint32 magic = NC_MAGIC_START;
		*writer << magic;

		magic = 0;
		Uint32 numEntries = m_counter.GetValue();
		*writer << numEntries; 
		*writer << magic; // legacy format (skip offset)

		CNamesPool& pool = SNamesPool::GetInstance();
		for ( Uint32 local = 1; local <= numEntries; ++local )
		{
			const Uint32 global = ToGlobal( local );

			Uint8 len;
			size_t length;
			const AnsiChar* buf = pool.FindTextAnsi( CNamesPool::TIndex( global ) );
			length = Red::StringLength( buf );
			ASSERT( length < 256 );
			len = Uint8( Clamp< size_t > ( length, 0, 255 ) );

			if ( global == 0 )
				len = 0; // hack for "None"

			*writer << len;
			writer->Serialize( ( void* ) buf, len * sizeof( AnsiChar ) );
		}

		Uint32 offset = 0;	// legacy format stuff
		*writer << offset;

		magic = NC_MAGIC_END;
		*writer << magic;
	}
}

Bool CNamesRemapper::LoadData( IFile* reader, Uint32& loadedSkipOffset )
{
	Uint32 magic;
	*reader << magic;

	if ( magic != NC_MAGIC_START )
	{
		RED_LOG( CNamesRemapper, TXT("Wrong file magic.") );
		return false;
	}

	Uint32 numEntries;
	*reader << numEntries;
	*reader << loadedSkipOffset;

	CNamesPool& pool = SNamesPool::GetInstance();
	Uint8 len;
	AnsiChar buf[ 256 ];
	for ( Uint32 i = 0; i < numEntries; ++i )
	{
		*reader << len;
		reader->Serialize( buf, len );
		buf[ len ] = 0;

		ToLocal( pool.AddDynamicPoolName( ANSI_TO_UNICODE( buf ) ) );
	}

	Uint32 offset;
	*reader	<< offset;
	*reader << magic;

	if( magic != NC_MAGIC_END )
	{
		RED_LOG( CNamesRemapper, TXT( "Wrong file magic at end" ) );
		return false;
	}

	return true;
}


void CNamesRemapper::Load( IFile* reader )
{
	Uint32 loadedSkipOffset = 0;
	if( reader->GetVersion() < VER_SKIPABLE_BLOCKS_NOINLINE_SUPPORT )		// Legacy data format
	{
		LoadData( reader, loadedSkipOffset );
	}
	else
	{
		CFileSkipableBlock fileSkipBlock( *reader );
		if( !LoadData( reader, loadedSkipOffset ) )
		{
			fileSkipBlock.Skip();
		}
	}
}

Uint32 CNamesRemapper::ToLocal( Uint32 global )
{
	Uint32 local = m_toLocal[ global ].GetValue();
	if ( local )
	{
		// MOST of the cases
		return local; 
	}

	// not mapped yet (rarely)
	return NewMapping( global );
}

Uint32 CNamesRemapper::NewMapping( Uint32 global )
{
	// first lock...
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_assignLock );

	// ...then re-check
	Uint32 local = m_toLocal[ global ].GetValue();
	if ( local )
	{
		return local;
	}

	local = m_counter.Increment();

	#ifdef RED_LOGGING_ENABLED
		if ( global >= MAX_NUM_GLOBAL_NAMES )
		{
			RED_LOG( Save, TXT("Mapping name with global index too big: %ld, (AsChar='%ls')"), global, CName( global ).AsChar() );
		}

		if ( local >= MAX_NUM_LOCAL_NAMES )
		{
			RED_LOG( Save, TXT("Mapping name with local index too big: %ld, (AsChar='%ls')"), local, CName( global ).AsChar() );
		}
	#endif

	RED_FATAL_ASSERT( local < MAX_NUM_LOCAL_NAMES, "Increase CNamesRemapper::MAX_NUM_LOCAL_NAMES now." );
	RED_FATAL_ASSERT( global < MAX_NUM_GLOBAL_NAMES, "Increase CNamesRemapper::MAX_NUM_GLOBAL_NAMES now." );
	RED_FATAL_ASSERT( m_toGlobal[ local ].GetValue() == 0, "Mapping already mapped name. This means broken code and needs to be debugged." );

	if ( global == 0 || local == 0 || global >= MAX_NUM_GLOBAL_NAMES || local >= MAX_NUM_LOCAL_NAMES )
		return 0;

	m_toLocal[ global ].SetValue( local );
	m_toGlobal[ local ].SetValue( global );
	return local;
}

Uint32 CNamesRemapper::ToGlobal( Uint32 local )
{
	if ( local >= MAX_NUM_LOCAL_NAMES )
		return 0;

	Uint32 global = m_toGlobal[ local ].GetValue();
	RED_ASSERT( global, TXT("That name should be mapped and it's not.") );
	return global; 
}
