/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "xmlFileReader.h"

using namespace rapidxml;

namespace rapidxml 
{
	extern void set_error_file( const String& new_error_file );
};

CXMLFileReader::CXMLFileReader( IFile &file )
: CXMLReader()
{
    Parse( file );
}

CXMLFileReader::CXMLFileReader( IFile *file )
{
	Parse( *file );
	delete file;
}

CXMLFileReader::~CXMLFileReader()
{
    if ( m_doc )
    {
        m_doc->clear();
        delete m_doc;
        m_doc = NULL;
    }

    if ( m_xmlData )
    {
        RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, m_xmlData );
        m_xmlData = NULL;
    }
}

static RED_INLINE void ByteSwapChar( Char* ch )
{
	Uint8* chBytes = (Uint8*)ch;
	Uint8 temp = chBytes[0];
	chBytes[0] = chBytes[1];
	chBytes[1] = temp;
}

#ifdef RED_ARCH_X64
static void* XMLAllocate( size_t size )
{
	return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, size );
}
#else
static void* XMLAllocate( Uint32 size )
{
	return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, size );
}
#endif
static void XMLDealocate( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, ptr );
}

void CXMLFileReader::Parse( IFile &file )
{
	// Get basic file info
	Uint32 fileSize = static_cast< Uint32 >( file.GetSize() );
	Uint16 signature;
	file.Serialize( &signature, 2 );

	// Check signature
	if ( signature != 0xFEFF && signature != 0xFFFE ) // TODO: ANSI version
	{
		rapidxml::parse_error_handler( "Wrong character encoding in XML file, UTF-16 expected.", nullptr );
		
		return;
	}

	// allocate xml string memory
	Uint32 charSize = Red::Math::NumericalUtils::Max<Uint32>( fileSize / sizeof(Char) - 1, 0 ); // size in Chars without signature
	m_xmlData = reinterpret_cast< Char* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary , sizeof( Char ) * (charSize + 1)) );
	file.Serialize( m_xmlData, charSize * sizeof(Char) );
	m_xmlData[charSize] = 0; // Append zero

	// Byteswap
	if ( signature == 0xFFFE )
	{
		for ( Uint32 i=0; i<charSize; ++i )
		{
			ByteSwapChar( &m_xmlData[i] );
		}
	}

	// read data into buffer
	m_data = m_xmlData;
	if ( !m_data.BeginsWith( TXT("<?xml version=") ) )
	{
		rapidxml::parse_error_handler( "Xml file reader cannot parse the given content. It's not an xml format ('<?xml version=' signature not found).", UNICODE_TO_ANSI( m_data.AsChar() ) );
		return;
	}

	// initializations
	m_doc = new XMLDoc;
	m_nodeStack.Clear();
	m_currentChild = NULL;	
	m_doc->set_allocator( &XMLAllocate, &XMLDealocate );
	rapidxml::set_error_file( file.GetFileNameForDebug() );
	if( m_doc->parse<0>( m_xmlData ) )
	{
		m_currentChild = m_doc->first_node();
	}
}
