/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/************************************************************************/
/* Skipable binary block of data										*/
/************************************************************************/
#define SKIP_BUFFER_SIZE 256

class CFileSkipableBlock : public Red::System::NonCopyable
{
private:
	IFile&		m_file;
	Uint64		m_archiveOffset;
	Uint32		m_skipOffset; // Serialized to file - do not make size_t/uintptr_t etc
	Uint8		m_skipBuffer[ SKIP_BUFFER_SIZE ];

public:
	RED_INLINE CFileSkipableBlock( IFile& file )
		: m_file( file )
		, m_archiveOffset( file.GetOffset() )
		, m_skipOffset( 0 )
	{
		if( file.IsSkipBlockDataInline() )
		{
			// Serialize skip offset
			file << m_skipOffset;
		}
		else if( file.IsReader() )
		{
			RED_FATAL_ASSERT( file.QuerySkippableBlockCache(), "No block cache implemented for this file" );
			m_skipOffset = file.QuerySkippableBlockCache()->QuerySkippableBlockOffset( static_cast< Uint32 >( file.GetOffset() ) );
		}
	}

	RED_INLINE ~CFileSkipableBlock()
	{
		Uint64 diskPastOffset = m_file.GetOffset();
		m_skipOffset = static_cast< Uint32 >( diskPastOffset - m_archiveOffset );

		// Saving, save skip offset
		if( m_file.IsSkipBlockDataInline() )
		{
			if ( m_file.IsWriter() )
			{
				// Serialize skip offset
				m_file.Seek( m_archiveOffset );
				m_file << m_skipOffset;
				m_file.Seek( diskPastOffset );
			}
		}
		else if( m_file.IsWriter() )
		{
			RED_FATAL_ASSERT( m_file.QuerySkippableBlockCache(), "No block cache implemented for this file" );
			m_file.QuerySkippableBlockCache()->RegisterSkippableBlockOffset( static_cast< Uint32 >( m_archiveOffset ), m_skipOffset );
		}
	}

	// Skip this data block
	RED_INLINE void Skip()
	{
		if ( m_file.IsReader() )
		{
			// Calculate jump position
			const Uint64 skipPos = m_archiveOffset + m_skipOffset;
			const Uint64 howMuchLeftToSkip = skipPos - m_file.GetOffset();

			if ( howMuchLeftToSkip > SKIP_BUFFER_SIZE || m_file.IsBuffered() )
			{
				m_file.Seek( skipPos );
			}
			else
			{
				m_file.Serialize( m_skipBuffer, static_cast<size_t>( howMuchLeftToSkip ) );
			}
		}
	}

	// Get the offset of the block end
	RED_INLINE Uint64 GetEndOffset() const
	{
		return m_skipOffset + m_archiveOffset;
	}

	// Do we have any data stored ?
	RED_INLINE Bool HasData() const
	{
		return m_skipOffset > sizeof(m_skipOffset);
	}
};