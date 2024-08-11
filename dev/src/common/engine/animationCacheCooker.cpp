
#include "build.h"
#include "animationCacheCooker.h"
#include "../core/dependencyMapper.h"

AnimationCacheCooker* GAnimationCacheCooker = NULL;

AnimationCacheCooker::AnimationCacheCooker( const String& absoluteFileName, ECookingPlatform platform )
	: m_file( NULL )
	, m_animStartOffset( 0 )
	, m_headerDataOffset( 0 )
{
	if ( GFileManager->GetFileSize( absoluteFileName ) == 0 || !LoadFromFile( absoluteFileName ) )
	{
		m_file = GFileManager->CreateFileWriter( absoluteFileName, FOF_AbsolutePath );
		if ( !m_file )
		{
			WARN_ENGINE( TXT("Unable to create anim cache file '%ls'"), absoluteFileName.AsChar() );
			return;
		}	

		CreateFileHeader( platform );
	}
	else
	{
		ASSERT( m_file );
	}
}

AnimationCacheCooker::~AnimationCacheCooker()
{
	m_entriesList.Clear();
	m_entriesMap.Clear();

	CloseFile();
}

void AnimationCacheCooker::CreateFileHeader( ECookingPlatform platform )
{
	// Set cooking flags
	if ( platform != PLATFORM_None )
	{
		m_file->SetCooker( true );
	}

	// Save header
	Uint32 magic = ANIM_CACHE_MAGIC_HEADER;
	*m_file << magic;

	// Save entires num and entires offset
	m_headerDataOffset = static_cast< Uint32 >( m_file->GetOffset() );
	Uint32 temp = 0;
	*m_file << temp;
	*m_file << temp;

	// Save offset
	m_animStartOffset = static_cast< Uint32 >( m_file->GetOffset() );
}

void AnimationCacheCooker::CloseFile()
{
	if ( m_file )
	{
		delete m_file;
		m_file = NULL;
	}
}

void AnimationCacheCooker::AddAnimation( Uint32& index, Uint32& animSize, CSkeletalAnimation* animation, Uint64 hash )
{
	Uint32 offset;
	SaveAnimation( animation, offset, animSize );

	index = static_cast< Uint32 >( m_entriesList.Grow( 1 ) );
	AnimCacheEntry& entry = m_entriesList[ index ];
	entry.m_hash = hash;
	entry.m_offset = offset;
	entry.m_size = animSize;

	m_entriesMap.Insert( hash, index );
}

Bool AnimationCacheCooker::GetAnimationData( Uint64 hash, Uint32& index, Uint32& animSize ) const
{
	Uint32 animIndex;

	if ( m_entriesMap.Find( hash, animIndex ) )
	{
		if ( animIndex < m_entriesList.Size() )
		{
			const AnimCacheEntry& entry = m_entriesList[ animIndex ];

			index = animIndex;
			animSize = entry.m_size;

			return true;
		}
		else
		{
			ASSERT( animIndex < m_entriesList.Size() );
		}
	}

	return false;
}

void AnimationCacheCooker::SaveAnimation( CSkeletalAnimation* animation, Uint32& offset, Uint32& size )
{
	ASSERT( m_file );

	const Uint32 start = static_cast< Uint32 >( m_file->GetOffset() );

#ifdef USE_HAVOK_ANIMATION
	// save the Havok data buffer
	AnimBuffer* buff = const_cast<AnimBuffer*>(animation->GetAnimBuffer());
	ASSERT( buff );
	*m_file << *buff;
#else
	// new way requires serializing the whole data

#endif

	const Uint32 end = static_cast< Uint32 >( m_file->GetOffset() );

	offset = start;
	size = end - start;
}

Uint32 AnimationCacheCooker::SaveEntries()
{
	ASSERT( m_file );

	Uint32 start = static_cast< Uint32 >( m_file->GetOffset() );

	for ( Uint32 i=0; i<m_entriesList.Size(); ++i )
	{
		AnimCacheEntry& entry = m_entriesList[ i ];
		entry.OnSerialize( *m_file );
	}

	Uint32 end = static_cast< Uint32 >( m_file->GetOffset() );

	return end - start;
}

void AnimationCacheCooker::Save()
{
	ASSERT( m_file );

	Uint32 currentFileOffset = static_cast< Uint32 >( m_file->GetOffset() );

	// Animation data size
	Uint32 animDataSize = currentFileOffset - m_animStartOffset;

	// All animation were saved. Entries will be saved now.
	Uint32 entriesOffset = currentFileOffset;

	// Save magic value - separator
	Uint32 magicSep = ANIM_CACHE_MAGIC_SEP;
	*m_file << magicSep;

	Uint32 toContinue = static_cast< Uint32 >( m_file->GetOffset() );

	// Go to header data
	m_file->Seek( m_headerDataOffset );

	// Save entires num and entires offset
	Uint32 entriesNum = m_entriesList.Size();

	*m_file << entriesNum;
	*m_file << entriesOffset;

	// Next, save entires
	m_file->Seek( toContinue );
	Uint32 entriesDataSize = SaveEntries();

	// Footer
	Uint32 magicNum = ANIM_CACHE_MAGIC_FOOTER;
	*m_file << magicNum;

	// Stats
	const Uint32 dataSize = static_cast< Uint32 >( m_file->GetSize() );
	Float inv = 1.f / ( 1024.0f * 1024.f );
	LOG_ENGINE( TXT("Saved %i entries, file size %1.2f MB, anim entires ( %1.2f MB ), anim data ( %1.2f MB ) in anim cache"), 
		entriesNum, dataSize * inv, entriesDataSize * inv, animDataSize * inv );

	// Return to proper place
	m_file->Seek( currentFileOffset );
}

Bool AnimationCacheCooker::LoadFromFile( const String& absoluteFileName )
{
	// Try to load existing file
	IFile* file = GFileManager->CreateFileReader( absoluteFileName, FOF_AbsolutePath | FOF_Buffered );
	if ( !file )
	{
		return false;
	}

	// Check magic value - header
	Uint32 magic = 0;
	*file << magic;

	if ( magic == ANIM_CACHE_MAGIC_HEADER_BS )
	{
		// Restart file, this time with byte swapping enabled
		file->Seek( 0 );
		file->SetByteSwapping( true );

		// Read the magic once more
		*file << magic;
	}

	if ( magic != ANIM_CACHE_MAGIC_HEADER )
	{
		WARN_ENGINE( TXT("Anim cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Save header offset
	m_headerDataOffset = static_cast< Uint32 >( file->GetOffset() );

	// Load number of entries
	Uint32 numEntries = 0;
	*file << numEntries;

	// Resize entries array
	m_entriesList.Resize( numEntries );

	// Load entries offset
	Uint32 entriesOffset = 0;
	*file << entriesOffset;

	// Cache anim offset
	m_animStartOffset = static_cast< Uint32 >( file->GetOffset() );

	// Go to entries place
	file->Seek( entriesOffset );

	// Check magic value - separator
	*file << magic;
	if ( magic != ANIM_CACHE_MAGIC_SEP )
	{
		WARN_ENGINE( TXT("Anim cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Load entires
	for ( Uint32 i=0; i<numEntries; ++i )
	{
		AnimCacheEntry& entry = m_entriesList[ i ];

		entry.OnSerialize( *file );

		m_entriesMap.Insert( entry.m_hash, i );
	}

	// Check magic value - footer
	*file << magic;
	if ( magic != ANIM_CACHE_MAGIC_FOOTER )
	{
		WARN_ENGINE( TXT("Anim cache cooker file is corrupted") );
		delete file;
		return false;
	}

	// Delete file reader
	delete file;
	file = NULL;

	// Create file writer
	m_file = GFileManager->CreateFileWriter( absoluteFileName, FOF_AbsolutePath | FOF_Append );
	if ( !m_file )
	{
		ASSERT( !m_file );
		return false;
	}

	// Seek to last animation place
	m_file->Seek( entriesOffset );

	return true;
}

Bool AnimationCacheCooker::CheckData() const
{
	// Check offset
	if ( m_animStartOffset == 0 || m_animStartOffset > m_file->GetSize() )
	{
		return false;
	}

	// Check all entries
	Uint32 prevOffset = m_animStartOffset;
	Uint32 prevSize = 0;
	for ( Uint32 i=0; i<m_entriesList.Size(); ++i )
	{
		const AnimCacheEntry& entry = m_entriesList[ i ];
		if ( !entry.IsValid() )
		{
			ERR_ENGINE( TXT("AnimationCacheCooker::CheckData - 1") );
			return false;
		}
		if ( entry.m_offset > m_file->GetSize() || entry.m_size > m_file->GetSize() )
		{
			ERR_ENGINE( TXT("AnimationCacheCooker::CheckData - 2") );
			return false;
		}

		if ( entry.m_offset != prevOffset + prevSize )
		{
			ERR_ENGINE( TXT("AnimationCacheCooker::CheckData - 3") );
			return false;
		}

		if ( !m_entriesMap.KeyExist( entry.m_hash ) )
		{
			ERR_ENGINE( TXT("AnimationCacheCooker::CheckData - 4") );
			return false;
		}

		prevOffset = entry.m_offset;
		prevSize = entry.m_size;
	}

	// Load all animations
	/*for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		const AnimCacheEntry& entry = m_entriesList[ i ];

		const Uint32 offset = entry.m_offset;

		CAnimCacheFileWrapper wrapper( m_file, offset );

		Uint32 start = m_file->GetOffset();

		AnimBuffer buff;
		wrapper << buff;

		Uint32 end = m_file->GetOffset();

		if ( end - start != entry.m_size )
		{
			return false;
		}

		buff.Clear();
	}*/

	return true;
}
