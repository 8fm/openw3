/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonPrettyWriter.h"

/// Pretty JSON writer
template<class encoding, class allocator>
class CJSONFilePrettyWriter : public CJSONPrettyWriter<encoding, allocator>, public Red::System::NonCopyable
{
public:
	typedef typename encoding::Ch CharType;

protected:
	IFile& m_file;		// Output file

public:
	CJSONFilePrettyWriter( IFile& file ) : m_file( file )
	{
		// UTF-8 Signature
		if( sizeof( CharType ) == sizeof( Uint8 ) )
		{
			Uint8 signature[ 3 ] = { 0xEF, 0xBB, 0xBF };
			m_file.Serialize( signature, 3 );
		}
		// UTF-16 Signature
		else if( sizeof( CharType ) == sizeof( Uint16 ) )
		{
			Uint8 signature[ 2 ] = { 0xFF, 0xFE };
			m_file.Serialize( signature, 2 );
		}
	}

public:

	// Saves JSON content
	void Flush()
	{
		MemSize strLen = Red::System::StringLength( CJSONWriter< encoding, allocator >::GetContent() );
		m_file.Serialize( (void *)CJSONWriter< encoding, allocator >::GetContent(), strLen * sizeof( CharType ) );
	}

};

typedef CJSONFilePrettyWriter< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONFilePrettyWriterUTF8;
typedef CJSONFilePrettyWriter< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONFilePrettyWriterUTF16;