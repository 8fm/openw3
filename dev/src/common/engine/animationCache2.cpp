
#include "build.h"
#include "animationCache2.h"

Bool AnimationsCache::Initialize( const String& absoluteFileName )
{
	CTimeCounter timer;

	m_file = NULL;
	m_animOffset = 0;
	m_entries.Clear();

	if ( GFileManager->GetFileSize( absoluteFileName ) == 0 )
	{
		ERR_ENGINE( TXT("Couldn't find anim.cache file! Animation cache can't be initialized.") );
		return false;
	}

	// Clear old data
	m_entries.Clear();

	// Open file
	m_file = GFileManager->CreateFileReader( absoluteFileName, FOF_AbsolutePath | FOF_Buffered );
	if ( !m_file )
	{
		WARN_ENGINE( TXT("Unable to load anim cache from '%ls'"), absoluteFileName.AsChar() );
		return false;
	}

	// Check magic value - header
	Uint32 magic = 0;
	*m_file << magic;

	if ( magic == ANIM_CACHE_MAGIC_HEADER_BS )
	{
		// Restart file, this time with byte swapping enabled
		m_file->Seek( 0 );
		m_file->SetByteSwapping( true );

		// Read the magic once more
		*m_file << magic;
	}

	if ( magic != ANIM_CACHE_MAGIC_HEADER )
	{
		WARN_ENGINE( TXT("Unable to load anim cache from '%ls' - file type is not animation cache"), absoluteFileName.AsChar() );
		return false;
	}

	// Load number of entries
	Uint32 numEntries = 0;
	*m_file << numEntries;

	// Resize entries array
	m_entries.Resize( numEntries );

	// Load entries offset
	Uint32 entriesOffset = 0;
	*m_file << entriesOffset;

	// Cache anim offset
	m_animOffset = static_cast< Uint32 >( m_file->GetOffset() );

	// Go to entries place
	m_file->Seek( entriesOffset );

	// Check magic value - separator
	*m_file << magic;
	if ( magic != ANIM_CACHE_MAGIC_SEP )
	{
		WARN_ENGINE( TXT("Anim cache is corrupted") );
		return false;
	}

	// Load entires
	for ( Uint32 i=0; i<numEntries; ++i )
	{
		AnimCacheEntry& entry = m_entries[ i ];
		entry.OnSerialize( *m_file );
	}

	// Data sizes
	const Uint32 dataSize = static_cast< Uint32 >( m_file->GetSize() );
	Uint32 animDataSize = dataSize - m_animOffset - sizeof( Uint32 ); // - footer

	// Check magic value - footer
	*m_file << magic;
	if ( magic != ANIM_CACHE_MAGIC_FOOTER )
	{
		WARN_ENGINE( TXT("Anim cache is corrupted") );
		return false;
	}

	// Stats
	Float inv = 1.f / ( 1024.0f * 1024.f );
	LOG_ENGINE( TXT("Loaded %i entries, file size %1.2f MB, anim entires ( %1.2f MB ), anim data ( %1.2f MB ) in anim cache in %1.2fs "), 
		m_entries.Size(), dataSize * inv, (entriesOffset-m_animOffset) * inv, animDataSize * inv, timer.GetTimePeriod() );

	return true;
}

void AnimationsCache::Destroy()
{
	// Close file
	delete m_file;
	m_file = NULL;

	// Reset offset
	m_animOffset = 0;

	// Delete all entries
	m_entries.Clear();
}

IFileLatentLoadingToken* AnimationsCache::CreateLoadingToken( Int32 animationId )
{
	// Open only valid entries
	if ( animationId >= 0 && animationId < (Int32)m_entries.Size() )
	{
		const AnimCacheEntry& entry = m_entries[ animationId ];

		// Get offset
		const Uint32 offset = entry.m_offset;

		// Return loading token
		return new CAnimCacheLoadingToken( m_file, offset );
	}

	// Entry not found
	return NULL;
}
