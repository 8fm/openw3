/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "xmlFileWriter.h"

using namespace rapidxml;

CXMLFileWriter::CXMLFileWriter( IFile& file )
: CXMLWriter()
, m_file( file )
{
    // UTF-16 Signature
    Uint8 signature[ 2 ] = { 0xFF, 0xFE };
    m_file.Serialize( signature, 2 );
}

void CXMLFileWriter::Flush()
{
    CXMLWriter::Flush();
	m_file.Serialize( (void *)m_data.AsChar(), m_data.GetLength() * sizeof( Char ) );
}
