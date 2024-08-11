/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "pagedMemoryWriter.h"

//----------------------

CPagedMemoryBuffer::CPagedMemoryBuffer( const Uint32 pageSize /*=DEFAULT_PAGE_SIZE*/ )
	: m_pageSize( pageSize )
	, m_curPage( nullptr )
	, m_totalSize( 0 )
{
	RED_FATAL_ASSERT( pageSize != 0, "Invalid page size" );
}

CPagedMemoryBuffer::~CPagedMemoryBuffer()
{
	Clear();
}

void CPagedMemoryBuffer::Clear()
{
	for ( const auto& page : m_pages )
	{
		FreePage( page.m_base );
	}

	m_pages.ClearFast();
	m_totalSize = 0;
	m_curPage = nullptr;
}

const Uint32 CPagedMemoryBuffer::GetTotalSize() const
{
	return m_totalSize;
}

void CPagedMemoryBuffer::CopyData( void* outData, const Uint32 bufferSize ) const
{
	Uint32 totalSize = 0;
	for ( const auto& page : m_pages )
	{
		totalSize += page.GetDataSize();
	}
	RED_FATAL_ASSERT( totalSize == m_totalSize, "Inconsistent memory size" );
	RED_FATAL_ASSERT( totalSize <= bufferSize, "Output buffer is to small" );

	Uint8* writePtr = (Uint8*) outData;
	for ( const auto& page : m_pages )
	{
		RED_FATAL_ASSERT( page.m_base != nullptr, "Invalid page data" );

		Red::MemoryCopy( writePtr, page.m_base, page.GetDataSize() );
		writePtr += page.GetDataSize();
	}
}

Bool CPagedMemoryBuffer::SaveToFile( IFile* file ) const
{
	// reset error flags on file
	file->ClearError();

	// save content
	for ( const auto& page : m_pages )
	{
		RED_FATAL_ASSERT( page.m_base != nullptr, "Invalid page data" );
		file->Serialize( page.m_base, page.GetDataSize() );
	}

	// saved without errors ?
	return !file->HasErrors();
}

Bool CPagedMemoryBuffer::LoadFromFile( IFile* file )
{
	Clear();

	// reset error flags on file
	file->ClearError();

	// prepare pages
	Uint32 sizeToRead = (Uint32) file->GetSize();
	m_pages.Reserve( (sizeToRead + m_pageSize-1) / m_pageSize );

	// read file in pages
	Uint32 readPos = 0;
	while ( readPos < sizeToRead && !file->HasErrors() )
	{
		// allocate full page
		Page pageInfo;
		pageInfo.m_base = AllocPage( m_pageSize );
		pageInfo.m_end = pageInfo.m_base + m_pageSize;
		pageInfo.m_pos = pageInfo.m_base;
		m_pages.PushBack( pageInfo );

		// read data
		const Uint32 readInPage = Min< Uint32 >( m_pageSize, (sizeToRead-readPos) );
		file->Serialize( pageInfo.m_base, readInPage );

		// advance
		pageInfo.m_pos += readInPage;
		m_totalSize += readInPage;
	}

	// data read
	return !file->HasErrors();
}

void CPagedMemoryBuffer::Append( const void* data, const Uint32 dataSize )
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

	// accumulate size
	m_totalSize += dataSize;
}

void CPagedMemoryBuffer::Write( const Uint32 offset, const void* data, const Uint32 dataSize )
{
	// copy existing data into the buffers
	Uint32 pageBase = 0;
	for ( const Page& page : m_pages )
	{
		const Uint32 pageSize = (Uint32) (page.m_pos - page.m_base);
		const Uint32 pageEnd = pageBase + pageSize;

		// do we need to update something in this block 
		if ( (offset < pageEnd) && ((offset + dataSize) > pageBase) )
		{
			const Uint32 offsetClamped = Max< Uint32 >( offset, pageBase ); // offset clamped to current page
			const Uint32 dataToWriteInPage = Min< Uint32 >( dataSize, pageEnd - offsetClamped ); // how much can we write

			const Uint32 offsetInBlock = offsetClamped - offset; // offset in source data buffer
			const Uint32 offsetInPage = offsetClamped - pageBase; // offset in page data buffer

			Red::MemoryCopy( page.m_base + offsetInPage, (const Uint8*)data + offsetInBlock, dataToWriteInPage );
		}

		// advance to next page
		pageBase += pageSize;
	}

	// append additional data at the end
	if ( offset + dataSize > m_totalSize )
	{
		const Uint32 additonalSize = (offset + dataSize) - m_totalSize;
		const Uint32 additionalDataOffset = (dataSize - additonalSize);
		Append( (const Uint8*)data +  additionalDataOffset, additonalSize );
	}
}

void CPagedMemoryBuffer::Read( const Uint32 offset, void* data, const Uint32 dataSize, Uint32& outSizeRead ) const
{
	// reset size
	outSizeRead = 0;

	// copy existing data into the buffers
	Uint32 pageBase = 0;
	for ( const Page& page : m_pages )
	{
		RED_FATAL_ASSERT( page.m_base != nullptr, "Invalid page data" );

		const Uint32 pageSize = (Uint32) (page.m_pos - page.m_base);
		const Uint32 pageEnd = pageBase + pageSize;

		// do we need to update something in this block 
		if ( (offset < pageEnd) && ((offset + dataSize) > pageBase) )
		{
			const Uint32 offsetClamped = Max< Uint32 >( offset, pageBase ); // offset clamped to current page
			const Uint32 dataToReadFromPage = Min< Uint32 >( dataSize, pageEnd - offsetClamped ); // how much can we write

			const Uint32 offsetInBlock = offsetClamped - offset; // offset in source data buffer
			const Uint32 offsetInPage = offsetClamped - pageBase; // offset in page data buffer

			Red::MemoryCopy( (Uint8*)data + offsetInBlock, page.m_base + offsetInPage, dataToReadFromPage );
			outSizeRead += dataToReadFromPage;
		}

		// advance to next page
		pageBase += pageSize;
	}
}

Uint8* CPagedMemoryBuffer::AllocPage( const Uint32 pageSize )
{
	auto mem = (Uint8*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, pageSize );
	RED_FATAL_ASSERT( mem != nullptr, "Failed to allocate memory for paged buffer" );
	return mem;
}

void CPagedMemoryBuffer::FreePage( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr );
}

//----------------------

CPagedMemoryWriter::CPagedMemoryWriter( CPagedMemoryBuffer& buffer )
	: IFile( FF_Writer | FF_MemoryBased )
	, m_buffer( &buffer )
	, m_offset( 0 )
{
}

CPagedMemoryWriter::~CPagedMemoryWriter()
{
}

void CPagedMemoryWriter::Serialize( void* buffer, size_t size )
{
	m_buffer->Write( m_offset, buffer, (Uint32)size );
}

Uint64 CPagedMemoryWriter::GetOffset() const
{
	return m_offset;
}

Uint64 CPagedMemoryWriter::GetSize() const
{
	return m_buffer->GetTotalSize();
}

void CPagedMemoryWriter::Seek( Int64 offset )
{
	m_offset = (Uint32) offset;
}

//----------------------

CPagedMemoryReader::CPagedMemoryReader( const CPagedMemoryBuffer& buffer )
	: IFile( FF_Reader | FF_MemoryBased )
	, m_buffer( &buffer )
	, m_offset( 0 )
{
}

CPagedMemoryReader::~CPagedMemoryReader()
{
}

void CPagedMemoryReader::Serialize( void* buffer, size_t size )
{
}

Uint64 CPagedMemoryReader::GetOffset() const
{
	return m_offset;
}

Uint64 CPagedMemoryReader::GetSize() const
{
	return m_buffer->GetTotalSize();
}

void CPagedMemoryReader::Seek( Int64 offset )
{
	m_offset = (Uint32) offset;
}

//----------------------
