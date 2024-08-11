/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "reportWriter.h"

//----

CHTMLReportWriter::CHTMLReportWriter( IFile* file )
	: m_outputFile( file )
	, m_stream( file )
	, m_writer( m_stream )
	, m_inTagHeader( false )
	, m_hadLineBreak( true )
{
}

CHTMLReportWriter::~CHTMLReportWriter()
{
	m_writer.Flush();
	m_stream.Close();
}

CHTMLReportWriter* CHTMLReportWriter::Create( const String& absolutePath )
{
	IFile* file = GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath /*| FOF_Buffered*/ ); // non buffered - we do it ourselves
	if ( !file )
		return nullptr;

	return new CHTMLReportWriter( file );
}

void CHTMLReportWriter::EnsureLineBreak()
{
	if ( !m_hadLineBreak )
	{
		m_hadLineBreak = true;
		m_writer.Append( "\n" );
	}
}

void CHTMLReportWriter::CloseTagHeader()
{
	if ( m_inTagHeader && !m_stack.Empty() )
	{
		m_writer.Append( ">");
		m_inTagHeader = false;
		m_hadLineBreak = false;
	}
}

void CHTMLReportWriter::Open( const AnsiChar* tag )
{
	CloseTagHeader();

	EnsureLineBreak();

	m_writer.Append( "<" );
	m_writer.Append( tag );

	m_inTagHeader = true;

	m_stack.PushBack( tag );
}

void CHTMLReportWriter::Openf( const AnsiChar* tag, ... )
{
	TLocalBuffer buf;
	va_list args;

	va_start( args, tag );
	Red::SNPrintF( buf, ARRAY_COUNT(buf), tag, args );
	va_end( args );

	Open( buf );
}

void CHTMLReportWriter::Close( const AnsiChar* tag )
{
	if ( m_stack.Empty() )
		return;

	if ( m_inTagHeader )
	{
		// just close current tag
		m_writer.Append( "/>");
		m_inTagHeader = false;
	}
	else
	{
		// full close
		const StringAnsi& tagName = m_stack.Back();
		m_writer.Append( "</" );
		m_writer.Append( tagName.AsChar() );
		m_writer.Append( ">" );
	}

	if ( tag != nullptr )
	{
		RED_ASSERT( m_stack.Back() == tag, TXT("HTML tag mismatch") );
	}

	m_stack.PopBack();

	m_hadLineBreak = false;
}

void CHTMLReportWriter::Attr( const AnsiChar* name, const AnsiChar* value )
{
	RED_ASSERT( !m_stack.Empty(), TXT("Need to be in a XML node to add the attributes") );
	RED_ASSERT( m_inTagHeader, TXT("Need to be in the XML tag header to add the attributes") );
	if ( !m_inTagHeader )
		return;

	m_writer.Append( " " );
	m_writer.Append( name );
	m_writer.Append( "=\"" );
	m_writer.Append( value );
	m_writer.Append( "\"" );

	m_hadLineBreak = false;
}

void CHTMLReportWriter::Attrf( const AnsiChar* name, const AnsiChar* value, ... )
{
	TLocalBuffer buf;
	va_list args;

	va_start( args, value );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), value, args );
	va_end( args );

	Attr( name, buf );
}

void CHTMLReportWriter::Write( const AnsiChar* txt )
{
	RED_ASSERT( !m_stack.Empty(), TXT("Need to be in a XML node to add the attributes") );

	CloseTagHeader();

	m_writer.Append( txt );

	m_hadLineBreak = false;
}

void CHTMLReportWriter::Writef( const AnsiChar* txt, ... )
{
	TLocalBuffer buf;
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	Write( buf );
}

//----

CHTMLNode::CHTMLNode( CHTMLReportWriter* writer, const AnsiChar* tag )
	: m_writer( writer )
	, m_tag( tag )
{
	m_writer->Open( tag );
}

CHTMLNode::CHTMLNode( const CHTMLReportDocument& document, const AnsiChar* tag )
	: m_writer( *document )
	, m_tag( tag )
{
	m_writer->Open( tag );
}

CHTMLNode::CHTMLNode( const CHTMLNode& parent, const AnsiChar* tag )
	: m_writer( parent.m_writer )
	, m_tag( tag )
{
	m_writer->Open( tag );
}

CHTMLNode::~CHTMLNode()
{
	m_writer->Close( m_tag );
}

//----

CHTMLReportDocument::CHTMLReportDocument( const String& absolutePath, const AnsiChar* title )
{
	m_writer = CHTMLReportWriter::Create( absolutePath );
	if ( m_writer )
	{
		m_writer->Open( "html" );

		{
			CHTMLNode node( m_writer, "head" );
			{
				//CHTMLNode node( m_writer, "style" );
				//node << ""
			}
		}

		m_writer->Open( "body" );
	}
}

CHTMLReportDocument::~CHTMLReportDocument()
{
	if ( m_writer )
	{
		m_writer->Close();
		m_writer->Close();

		delete m_writer;
	}
}

//----

CHTMLTable::CHTMLTable( CHTMLReportWriter* writer )
	: m_writer( writer )
	, m_headerDone( false )
	, m_hasRow( false )
	, m_numCols( 0 )
	, m_curCol( 0 )
{
	m_writer->Open( "table" );
	Config();

	m_writer->Open( "tr" );
}

CHTMLTable::CHTMLTable( const CHTMLReportDocument& document )
	: m_writer( *document )
	, m_headerDone( false )
	, m_hasRow( false )
	, m_numCols( 0 )
	, m_curCol( 0 )
{
	m_writer->Open( "table" );
	Config();

	m_writer->Open( "tr" );
}

CHTMLTable::CHTMLTable( const CHTMLNode& parent )
	: m_writer( *parent )
	, m_headerDone( false )
	, m_hasRow( false )
	, m_numCols( 0 )
	, m_curCol( 0 )
{
	m_writer->Open( "table" );
	Config();

	m_writer->Open( "tr" );
}

CHTMLTable::~CHTMLTable()
{
	EndHeader();

	if ( m_hasRow )
	{
		m_writer->Close( "tr" );
	}

	m_writer->Close( "table" ); // table
}

void CHTMLTable::EndHeader()
{
	if ( !m_headerDone )
	{
		m_writer->Close( "tr" );
		m_headerDone = true;
	}
}

void CHTMLTable::Config( const Uint32 border /*= 1*/, const Uint32 cellSpacing/*=0*/, const Uint32 cellPadding/*=0*/ )
{
	m_writer->Attrf( "border", "%d", border );
	m_writer->Attrf( "cellpadding", "%d", cellPadding );
	m_writer->Attrf( "cellspacing", "%d", cellSpacing );
}

void CHTMLTable::Column( const AnsiChar* name, const Uint32 width/*=0*/ )
{
	if ( !m_headerDone )
	{
		m_writer->Open( "th" );
		m_writer->Attrf( "width", "%d", width );
		m_writer->Write( "<center><b>" );
		m_writer->Write( name );
		m_writer->Write( "</center></b>" );
		m_writer->Close( "th" );

		m_numCols += 1;
	}
}

void CHTMLTable::Row()
{
	EndHeader();

	if ( m_hasRow )
	{
		m_writer->Close( "tr" ); // tr
	}

	m_hasRow = true;
	m_writer->Open( "tr" );
	m_curCol = 0;
}

void CHTMLTable::Cell( const char* txt )
{
	if ( m_hasRow )
	{
		if ( m_curCol < m_numCols )
		{
			m_writer->Open( "td" );
			m_writer->Write( txt );
			m_writer->Close( "td" );

			m_curCol += 1;
		}
	}
}

void CHTMLTable::Cellf( const char* txt, ... )
{
	TLocalBuffer buf;
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	Cell( buf );
}
