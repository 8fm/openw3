/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fileSys.h"
#include "scopedPtr.h"
#include "basicTextBuilder.h"

//------

CBasicTextBuilder::PagedBuffer::PagedBuffer( const Uint32 pageSize )
	: m_pageSize( pageSize )
	, m_curPage( nullptr )
{
}

CBasicTextBuilder::PagedBuffer::~PagedBuffer()
{
}

const Uint32 CBasicTextBuilder::PagedBuffer::GetTotalSize() const
{
	Uint32 totalSize = 0;
	for ( const auto& page : m_pages )
	{
		totalSize += page.GetDataSize();
	}
	return totalSize;
}

void CBasicTextBuilder::PagedBuffer::CopyData( void* outData ) const
{
	Uint8* writePtr = (Uint8*) outData;
	for ( const auto& page : m_pages )
	{
		Red::MemoryCopy( writePtr, page.m_base, page.GetDataSize() );
		writePtr += page.GetDataSize();
	}
}

Bool CBasicTextBuilder::PagedBuffer::SaveToFile( IFile* file ) const
{
	for ( const auto& page : m_pages )
	{
		file->Serialize( page.m_base, page.GetDataSize() );
	}
	return true;
}

void CBasicTextBuilder::PagedBuffer::Append( const void* data, const Uint32 dataSize )
{
	// will we fit into current page ?
	if ( !m_curPage || (m_curPage->m_pos + dataSize > m_curPage->m_end) )
	{
		// add new page
		Page newPage;
		newPage.m_base = AllocPage( m_pageSize );
		newPage.m_pos = newPage.m_base;
		newPage.m_end = newPage.m_base + m_pageSize;
		m_pages.PushBack( newPage );

		// get pointer to the new page
		m_curPage = &m_pages.Back();
	}

	// append data in page
	RED_FATAL_ASSERT( m_curPage != nullptr, "Invalid writer state" );
	RED_FATAL_ASSERT( m_curPage->m_pos + dataSize <= m_curPage->m_end, "Invalid writer state" );
	Red::MemoryCopy( m_curPage->m_pos, data, dataSize );
	m_curPage->m_pos += dataSize;
}

Uint8* CBasicTextBuilder::PagedBuffer::AllocPage( const Uint32 pageSize )
{
	return (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, pageSize );
}

void CBasicTextBuilder::PagedBuffer::FreePage( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr );
}

//------

CBasicTextBuilder::CBasicTextBuilder( const Uint32 pageSize /*= 65536*/ )
	: m_buffer( pageSize )
	, m_stream( m_buffer )
	, m_writer( m_stream )
	, m_replacer( nullptr )
{
}

CBasicTextBuilder::~CBasicTextBuilder()
{

}

void CBasicTextBuilder::SetTextReplacer( const IBasicTextBuilderAutotext* textReplacer )
{
	m_replacer = textReplacer;
}

void CBasicTextBuilder::Write( const AnsiChar* txt )
{
	// no dynamic text replacer
	if ( !m_replacer )
	{
		m_writer.Append( txt );
	}

	// process text
	const AnsiChar* cur = txt;
	while ( *cur )
	{
		// replacement delimiter
		if ( cur[0] == '#' && cur[1] == '<' )
		{
			// skip initial marker
			cur += 2;

			// look up the end symbol
			const AnsiChar* keyStart = cur;
			const AnsiChar* keyEnd = cur;
			while ( *cur )
			{
				if ( cur[0] == '>' && cur[1] == '#' )
				{
					keyEnd = cur;
					cur += 2; // skip the marker
					break;
				}
				++cur;
			}

			// match key in the replacer
			const Uint32 keyLength = (Uint32)(keyEnd - keyStart);
			const AnsiChar* replacedText = m_replacer->MatchKey( keyStart, keyLength );
			if ( replacedText != nullptr )
			{
				// insert the replaced text only
				m_writer.Append( replacedText );
			}
			else
			{
				// insert key
				m_writer.Append( "(Missing key: " );
				m_writer.Append( StringAnsi( keyStart, keyLength ).AsChar() );
				m_writer.Append( ")");
			}
		}
		else
		{
			// normal part of the text
			// TODO: optimize - we can print as much as till next #
			m_writer.Append( cur, 1 );
			cur += 1;
		}
	}
}

void CBasicTextBuilder::Writef( const AnsiChar* txt, ... )
{
	TLocalBuffer buf;
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	Write( buf );
}

const Uint32 CBasicTextBuilder::GetDataSize() const
{
	m_writer.Flush();
	return m_buffer.GetTotalSize();
}

void CBasicTextBuilder::CopyData( void* outputBuffer ) const
{
	m_writer.Flush();
	m_buffer.CopyData( outputBuffer );
}

Bool CBasicTextBuilder::SaveToFile( const String& absolutePath ) const
{
	m_writer.Flush();
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath ) );
	if ( file )
	{
		m_buffer.SaveToFile( file.Get() );
		return true;
	}

	return false;
}

void CBasicTextBuilder::SaveToFile( IFile* file ) const
{
	m_writer.Flush();
	m_buffer.SaveToFile( file );
}

//------
