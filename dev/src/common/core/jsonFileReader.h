/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonReader.h"

/// Simple JSON reader
template<class encoding, class allocator>
class CJSONFileReader : public CJSONReader<encoding, allocator>
{
public:
	typedef typename encoding::Ch CharType;

public:

	// this constructor will not delete the file
	CJSONFileReader( IFile &file ): m_jsonData(NULL)
	{
		 Parse( file );
	}

	// This constructor will delete the file
	CJSONFileReader( IFile *file ): m_jsonData(NULL)
	{
		Parse( *file );
		delete file;
	}

	virtual ~CJSONFileReader()
	{
		if ( m_jsonData )
		{
			allocator::PoolAllocatorInstance::GetInstance().Free( m_jsonData );
			m_jsonData = NULL;
		}
	}

private:
	void Parse( IFile &file )
	{
		// Get basic file info
		Uint32 fileSize = static_cast< Uint32 >( file.GetSize() );

		// UTF-8 Signature
		Uint8 signature[3];
		if( sizeof( CharType ) == sizeof( Uint8 ) )
		{
			file.Serialize( signature, 3 );
			fileSize -= 3;
			if(    signature[0] != 0xEF 
				|| signature[1] != 0xBB 
				|| signature[2] != 0xBF )
			{
				file.Seek( 0 ); // no signature
			}
		}
		// UTF-16 Signature
		else if( sizeof( CharType ) == sizeof( Uint16 ) )
		{
			file.Serialize( signature, 2 );
			fileSize -= 2;
			if( !(( signature[0] == 0xFF && signature[1] == 0xFE ) || ( signature[0] == 0xFE && signature[1] == 0xFF )) )
			{
				file.Seek( 0 ); // no signature
			}
		}

		// allocate JSON string memory
		Uint32 charSize = Red::Math::NumericalUtils::Max<Uint32>( fileSize / sizeof(CharType), 0 ); // size in Chars without signature
				
		m_jsonData = reinterpret_cast< CharType* >( allocator::PoolAllocatorInstance::GetInstance().Malloc( sizeof( CharType ) * (charSize + 1)) );
		file.Serialize( m_jsonData, charSize * sizeof(CharType) );
		m_jsonData[charSize] = 0; // Append zero

		// Byteswap
		if( sizeof( CharType ) == sizeof( Uint16 ) )
		{
			if ( *((Uint16*)(signature)) == 0xFFFE )
			{
				for ( Uint32 i=0; i<charSize; ++i )
				{
					ByteSwapChar16( &m_jsonData[i] );
				}
			}
		}

		CJSONReader<encoding, allocator>::Read( m_jsonData );
	}

	void ByteSwapChar16( CharType* jsonData )
	{
		Uint8* chBytes = (Uint8*)jsonData;
		Uint8 temp = chBytes[0];
		chBytes[0] = chBytes[1];
		chBytes[1] = temp;
	}
	
private:
	CharType* m_jsonData;
};

typedef CJSONFileReader< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONFileReaderUTF8;
typedef CJSONFileReader< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONFileReaderUTF16;
