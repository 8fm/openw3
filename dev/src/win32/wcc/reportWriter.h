/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/redSystem/stringWriter.h"

/// Direct HTML writer, uses buffered string output
class CHTMLReportWriter
{
public:
	~CHTMLReportWriter();
	static CHTMLReportWriter* Create( const String& absolutePath );

	// open/close the tag
	void Open( const AnsiChar* tag ); // faster
	void Openf( const AnsiChar* tag, ... );
	void Close( const AnsiChar* tag = nullptr );

	// add attribute (to currently opened tag)
	void Attr( const AnsiChar* name, const AnsiChar* value ); // faster
	void Attrf( const AnsiChar* name, const AnsiChar* value, ... );

	// print into the document
	void Write( const AnsiChar* txt ); // faster
	void Writef( const AnsiChar* txt, ... );

	// fast node
	void Node( const AnsiChar* tag, const AnsiChar* value );
	void Nodef( const AnsiChar* tag, const AnsiChar* value, ... );

	// stream writing
	RED_INLINE void operator<<( const AnsiChar* txt ) { Write( txt ); }

private:
	CHTMLReportWriter( IFile* file );

	// file output stream
	template< typename CH >
	class FileStreamWriter
	{
	public:
		FileStreamWriter( IFile* outputFile )
			: m_outputFile( outputFile )
		{}

		bool Flush( const Bool forced, const CH* data, const Red::System::Uint32 count )
		{
			if ( m_outputFile )
				m_outputFile->Serialize( (void*)data, count * sizeof(CH) );
			return true;
		}

		static FileStreamWriter& GetInstance()
		{
			static FileStreamWriter theInstance;
			return theInstance;
		}

		void Close()
		{
			delete m_outputFile;
			m_outputFile = nullptr;
		}

	private:
		IFile*		m_outputFile;
	};

	// local printer
	typedef AnsiChar TLocalBuffer[ 512 ];

	// internal text writer
	typedef Red::System::StackStringWriter< AnsiChar, 1024, FileStreamWriter< AnsiChar > > TWriter;
	typedef FileStreamWriter<AnsiChar> TStreamWriter;
	TStreamWriter	m_stream;
	TWriter			m_writer;

	// tag list
	typedef TStaticArray< StringAnsi, 128 >		TTagStack;
	TTagStack		m_stack;
	Bool			m_inTagHeader;	
	Bool			m_hadLineBreak;

	// output stream
	IFile*			m_outputFile;

	// end current tag argument value
	void CloseTagHeader();

	// ensure line break
	void EnsureLineBreak();
};

/// Generic HTML document
class CHTMLReportDocument
{
public:
	CHTMLReportDocument( const String& absolutePath, const AnsiChar* title );
	~CHTMLReportDocument();

	// is it valid ?
	RED_INLINE operator Bool() const { return m_writer != nullptr; }

	// stream writing
	RED_INLINE void operator<<( const AnsiChar* txt ) { m_writer->Write( txt ); }

	// get the writer
	RED_INLINE CHTMLReportWriter* operator*() const { return m_writer; }

private:
	CHTMLReportWriter*		m_writer;
};

/// Direct HTML node
class CHTMLNode
{
public:
	CHTMLNode( CHTMLReportWriter* writer, const AnsiChar* tag );
	CHTMLNode( const CHTMLReportDocument& document, const AnsiChar* tag );
	CHTMLNode( const CHTMLNode& parent, const AnsiChar* tag );
	~CHTMLNode();

	// get the writer
	RED_INLINE CHTMLReportWriter* operator*() const { return m_writer; }

	// get the writer
	RED_INLINE CHTMLReportWriter* operator->() const { return m_writer; }

	// stream writing
	RED_INLINE CHTMLNode& operator<<( const AnsiChar* txt ) { m_writer->Write( txt ); return *this; }

private:
	CHTMLReportWriter*		m_writer;
	const AnsiChar*			m_tag;
};

/// HTML table wrapper
class CHTMLTable
{
public:
	CHTMLTable( CHTMLReportWriter* writer );
	CHTMLTable( const CHTMLReportDocument& document);
	CHTMLTable( const CHTMLNode& parent );
	~CHTMLTable();

	void Column( const AnsiChar* name, const Uint32 width=0 );
	void Row();
	void Cell( const char* txt );
	void Cellf( const char* txt, ... );

private:
	CHTMLReportWriter*	m_writer;
	Uint32		m_numCols;
	Uint32		m_curCol;
	Bool		m_headerDone;
	Bool		m_hasRow;

	// local printer
	typedef AnsiChar TLocalBuffer[ 512 ];

	void EndHeader();
	void Config( const Uint32 border = 1, const Uint32 cellSpacing=0, const Uint32 cellPadding=0 );
};
