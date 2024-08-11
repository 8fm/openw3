/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/redSystem/stringWriter.h"

/// Helper class that can be used to replace values in text delimited by #
class IBasicTextBuilderAutotext;

/// Helper class that allows for ULTRA FAST text document construction
/// Does not reallocate big blocks on memory and is efficient when it comes to appending strings
class CBasicTextBuilder
{
public:
	 CBasicTextBuilder( const Uint32 pageSize = 65536 );
	~CBasicTextBuilder();

	// set text replacer
	void SetTextReplacer( const IBasicTextBuilderAutotext* textReplacer );

	// print into the document
	void Write( const AnsiChar* txt ); // faster
	void Writef( const AnsiChar* txt, ... );

	// get size of the data
	const Uint32 GetDataSize() const;

	// copy data to memory, output buffer should be big enough
	void CopyData( void* outputBuffer ) const;

	// save data to tile
	Bool SaveToFile( const String& absolutePath ) const;

	// save data to tile
	void SaveToFile( IFile* file ) const;

private:
	// page buffer
	class PagedBuffer
	{
	public:
		PagedBuffer( const Uint32 pageSize );
		~PagedBuffer();

		// get total size of data in the buffer
		const Uint32 GetTotalSize() const;;

		// copy out the data
		void CopyData( void* outData ) const;

		// save data to tile
		Bool SaveToFile( IFile* file ) const;

		// append data
		void Append( const void* data, const Uint32 dataSize );

	private:
		struct Page
		{
			Uint8*	m_base;
			Uint8*	m_end;
			Uint8*	m_pos;

			RED_INLINE const Uint32 GetDataSize() const
			{
				return (Uint32)( m_pos - m_base );
			}
		};

		Uint32				m_pageSize;
		TDynArray< Page >	m_pages;
		Page*				m_curPage;

		static Uint8* AllocPage( const Uint32 pageSize );
		static void FreePage( void* ptr );
	};

	// file output stream
	template< typename CH >
	class FileStreamWriter
	{
	public:
		FileStreamWriter( PagedBuffer& buffer )
			: m_outputBuffer( &buffer )
		{}

		bool Flush( const Bool forced, const CH* data, const Red::System::Uint32 count )
		{
			m_outputBuffer->Append( (void*)data, count * sizeof(CH) );
			return true;
		}

		static FileStreamWriter& GetInstance()
		{
			static FileStreamWriter theInstance;
			return theInstance;
		}

		void Close()
		{
		}

	private:
		PagedBuffer*	m_outputBuffer;
	};

	// local printer
	typedef AnsiChar TLocalBuffer[ 512 ];

	// internal text writer
	typedef Red::System::StackStringWriter< AnsiChar, 1024, FileStreamWriter< AnsiChar > > TWriter;
	typedef FileStreamWriter<AnsiChar> TStreamWriter;
	TStreamWriter	m_stream;
	mutable TWriter	m_writer;

	// text replacer
	const IBasicTextBuilderAutotext*	m_replacer;

	// paged buffer
	PagedBuffer		m_buffer;
};

/// Replacement lookup for text builder
/// Helper class that can be used to replace values in text delimited by #
class IBasicTextBuilderAutotext
{
public:
	virtual ~IBasicTextBuilderAutotext() {};

	// get value for given key, NULL if not found, assumes values are cached
	// key text is part of the text stream, hence the length
	virtual const AnsiChar* MatchKey( const AnsiChar* key, const Uint32 keyLength ) const = 0;
};