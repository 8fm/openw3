/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

CClipboardData::CClipboardData( const String& formatName )
	: m_format( formatName.AsChar() )
	, m_isCopy( false )
	, m_next( NULL )
{
}

CClipboardData::CClipboardData( const String& formatName, const TDynArray< Uint8 >& data, Bool isCopy )
	: m_format( formatName.AsChar() )
	, m_data( data )
	, m_isCopy( isCopy )
	, m_next( NULL )
{
}

CClipboardData::~CClipboardData()
{
}

void CClipboardData::GetAllFormats( wxDataFormat *formats, Direction dir ) const
{
	formats[0] = m_format;
}

bool CClipboardData::GetDataHere( const wxDataFormat &format, void* buf ) const
{
	if ( format == m_format )
	{
		Red::System::MemoryCopy( buf, &m_isCopy, sizeof( Bool ) );
		Red::System::MemoryCopy( (Uint8*) buf + sizeof( Bool ), m_data.Data(), m_data.Size() );
		Red::System::MemoryCopy( (Uint8*) buf + sizeof( Bool ) + m_data.Size(), &m_next, sizeof( m_next ) );
		return true;
	}

	// Format not supported
	return false;
}

size_t CClipboardData::GetDataSize( const wxDataFormat &format) const
{
	if ( format == m_format )
	{
		return m_data.Size() + sizeof( Bool ) + sizeof( m_next );
	}

	// Format not supported
	return 0;
}

size_t CClipboardData::GetFormatCount( Direction dir ) const
{
	return 1;
}

wxDataFormat CClipboardData::GetPreferredFormat( Direction dir ) const
{
	return m_format;
}

bool CClipboardData::SetData(const wxDataFormat &format, size_t len, const void* buf )
{
	// Note: len is the total buffer size, NOT the size of m_data!
	if ( format == m_format )
	{
		size_t dataLength = len - (sizeof(Bool) + sizeof( m_next ));
		m_data.Resize( dataLength );
		Red::System::MemoryCopy( &m_isCopy, buf, sizeof(Bool) );
		Red::System::MemoryCopy( m_data.Data(), (Uint8*) buf + sizeof( Bool ), dataLength );
		Red::System::MemoryCopy( &m_next, (Uint8*) buf + sizeof( Bool ) + m_data.Size(), sizeof( m_next ) );
		return true;
	}

	// Format not supported
	return false;
}
