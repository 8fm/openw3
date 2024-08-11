/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "htmlDocument.h"

#include "object.h"
#include "resource.h"
#include "diskFile.h"

//-----

class CHTMLDocumentTextReplacer : public IBasicTextBuilderAutotext
{
public:
	CHTMLDocumentTextReplacer( const TSortedMap< Uint32, StringAnsi >& keys )
		: m_keys( &keys )
	{}

	const AnsiChar* MatchKey( const AnsiChar* key, const Uint32 keyLength ) const
	{
		// compute text hash
		const Uint32 hash = Red::CalculateHash32( (const void*)key, keyLength );
		const auto* value = m_keys->FindPtr( hash );
		if ( value != nullptr )
			return value->AsChar();

		// not found
		return nullptr;
	}

private:
	const TSortedMap< Uint32, StringAnsi >*	m_keys;
};

//-----

CHTMLDocument::CHTMLDocument( CBasicTextBuilder& text, const CBasicURL& fullURL, const CHTMLDocumentDesc& desc )
	: m_url( fullURL )
	, m_textReplacer( new CHTMLDocumentTextReplacer( m_replaceMap ) )
	, m_text( &text )
	, m_tagId( 1 )
{
	m_text->SetTextReplacer( m_textReplacer );

	// URL without query (http://127.0.0.1:37001/dupa.html)
	AddReplacementKey( "url", fullURL.ToString() );

	// host path (http://127.0.0.1:37001/)
	AddReplacementKey( "host", fullURL.GetProtocol() + "://" + fullURL.GetAddress() + "/" );

	// host address (127.0.0.1:37001)
	AddReplacementKey( "addr", fullURL.GetAddress() );

	// initialize document header
	InitDoc( text, desc );
}

CHTMLDocument::~CHTMLDocument()
{
	FinishDoc( *m_text );

	if ( m_textReplacer != nullptr )
	{
		m_text->SetTextReplacer( nullptr );
		delete m_textReplacer;
	}
}

void CHTMLDocument::AddReplacementKey( const StringAnsi& key, const StringAnsi& value )
{
	const Uint32 hash = Red::CalculateHash32( (const void*) key.AsChar(), key.GetLength() );
	RED_FATAL_ASSERT( !m_replaceMap.KeyExist( hash ), "Duplicated key in the replacement map" );
	m_replaceMap.Insert( hash, value );
}

void CHTMLDocument::InitDoc( CBasicTextBuilder& text, const CHTMLDocumentDesc& desc )
{
	text.Write( "<!DOCTYPE HTML>" );
	text.Write( "<html>" );

	text.Write( "<head>" );
	text.Write( "<meta charset=\"UTF-8\">" );

	for ( Uint32 i=0; i<desc.m_styles.Size(); ++i )
	{
		text.Writef( "<link rel=\"stylesheet\" type=\"text/css\"  href=\"#<host>#%hs\">",
			desc.m_styles[i].AsChar() );
	}

	for ( Uint32 i=0; i<desc.m_scripts.Size(); ++i )
	{
		const auto& path = desc.m_scripts[i];

		// absolute vs relative path
		if ( path.BeginsWith("//") )
		{
			text.Writef( "<script type=\"text/javascript\" src=\"%hs\"></script>",
				desc.m_scripts[i].AsChar() );

		}
		else
		{
			text.Writef( "<script type=\"text/javascript\" src=\"#<host>#%hs\"></script>",
				desc.m_scripts[i].AsChar() );
		}
	}	

	text.Write( "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>" );
	text.Write( "<meta http-equiv=\"Content-Language\" content=\"en\"/>" );

	text.Write( "</head>" );
	text.Write( "<body>" );
}

void CHTMLDocument::FinishDoc( CBasicTextBuilder& text )
{
	text.Write( "</body>" );
	text.Write( "</html>" );
}

//-----

CHTMLTagHelper::CHTMLTagHelper( class CHTMLDocument* doc, const Uint32 tagId )
	: m_doc( doc )
{
	m_tagId.PushBack( tagId );
}

CHTMLTagHelper::CHTMLTagHelper( CHTMLTagHelper&& other )
{
	m_doc = other.m_doc;
	other.m_doc = nullptr;

	m_tagId = std::move( other.m_tagId );
	other.m_tagId.ClearFast();
}

CHTMLTagHelper::~CHTMLTagHelper()
{
	while ( !m_tagId.Empty() )
	{
		m_doc->CloseTag( m_tagId.Back() );
		m_tagId.PopBack();
	}
}

CHTMLTagHelper& CHTMLTagHelper::Attr( const AnsiChar* name, const AnsiChar* value )
{
	m_doc->AttrTag( m_tagId.Back(), name, value );
	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::Attrf( const AnsiChar* name, const AnsiChar* txt, ... )
{
	AnsiChar buf[512];
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	m_doc->AttrTag( m_tagId.Back(), name, buf );
	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::CondAttr( const Bool condition, const AnsiChar* name, const AnsiChar* value )
{
	if ( condition )
	{
		m_doc->AttrTag( m_tagId.Back(), name, value );
	}
	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::CondAttrf( const Bool condition, const AnsiChar* name, const AnsiChar* value, ... )
{
	if ( condition )
	{
		AnsiChar buf[512];
		va_list args;

		va_start( args, value );
		Red::VSNPrintF( buf, ARRAY_COUNT(buf), value, args );
		va_end( args );

		m_doc->AttrTag( m_tagId.Back(), name, buf );
	}

	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::Write( const AnsiChar* txt )
{
	m_doc->Write( txt );
	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::Writef( const AnsiChar* txt, ... )
{
	AnsiChar buf[512];
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	m_doc->Write( buf );
	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::Open( const AnsiChar* tagName )
{
	CHTMLTagHelper doc( m_doc->Open( tagName ) );

	// transfer to local list
	m_tagId.PushBack( doc.m_tagId );
	doc.m_tagId.ClearFast();

	return *this;
}

CHTMLTagHelper& CHTMLTagHelper::Pop()
{
	if ( m_tagId.Size() > 1 )
	{
		m_doc->CloseTag( m_tagId.Back() );
		m_tagId.PopBack();
	}

	return *this;
}

//-----

CHTMLTagHelper CHTMLDocument::Open( const AnsiChar* tagName )
{
	CloseTagHeader();

	m_text->Write( "<" );
	m_text->Write( tagName );
	m_inTagHeader = true;

	TagStackElement elem;
	elem.m_id = m_tagId++;
	elem.m_tag = tagName;
	m_tagStack.PushBack( elem );

	return CHTMLTagHelper( this, elem.m_id );
}

CHTMLTagHelper CHTMLDocument::Link( const AnsiChar* href, ... )
{
	CloseTagHeader();

	m_text->Write( "<a" );
	m_inTagHeader = true;

	TagStackElement elem;
	elem.m_id = m_tagId++;
	elem.m_tag = "a";
	m_tagStack.PushBack( elem );

	AnsiChar buf[512];
	{
		va_list args;

		va_start( args, href );
		Red::VSNPrintF( buf, ARRAY_COUNT(buf), href, args );
		va_end( args );

		m_text->Write( " href=\"#<host>#" );
		m_text->Write( (buf[0] == '/') ? buf+1 : buf+0 ); // skip the inital separator
		m_text->Write( "\"" );
	}

	return CHTMLTagHelper( this, elem.m_id );
}

CHTMLDocument& CHTMLDocument::LinkObject( const CObject* object, const StringAnsi& customName/*=""*/ )
{
	if ( object )
	{
		if ( customName.Empty() )
		{
			if ( object->IsA< CResource >() && ((const CResource*)object)->GetFile() )
			{
				const String depotPath = ((const CResource*)object)->GetFile()->GetDepotPath();
				Link( "/object/?id=%d", object->GetObjectIndex() ).Writef( "0x%016llX (%hs)", (Uint64)object, UNICODE_TO_ANSI(depotPath.AsChar()) );
			}
			else
			{
				Link( "/object/?id=%d", object->GetObjectIndex() ).Writef( "0x%016llX (%hs)", (Uint64)object, object->GetClass()->GetName().AsAnsiChar() );
			}
		}
		else
		{
			Link( "/object/?id=%d", object->GetObjectIndex() ).Write( customName.AsChar() );
		}
	}
	else
	{
		Write( "NULL" );
	}

	return *this;
}

CHTMLDocument& CHTMLDocument::LinkFile( const StringAnsi& path )
{
	if ( !path.Empty() )
	{
		Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
	}
	else
	{
		Write( "NULL" );
	}

	return *this;
}

CHTMLDocument& CHTMLDocument::LinkScript( const AnsiChar* style, const AnsiChar* func, ... )
{
	AnsiChar buf[512];
	va_list args;

	va_start( args, func );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), func, args );
	va_end( args );

	Link( "#<url>#" ).Attrf( "onclick", "%hs; return false;", buf ).Attr( "class", style );
	return *this;
}

void CHTMLDocument::AttrTag( const Uint32 tagId, const AnsiChar* name, const AnsiChar* value )
{
	if ( m_tagStack.Empty() || m_tagStack.Back().m_id != tagId )
		return;

	if ( !m_inTagHeader )
		return;

	m_text->Write( " " );
	m_text->Write( name );
	m_text->Write( "=\"" );
	m_text->Write( value );
	m_text->Write( "\"" );
}

void CHTMLDocument::CloseTag( const Uint32 tagId )
{
	RED_ASSERT( !m_tagStack.Empty(), TXT("Trying to close tag on empty tag stack") );
	RED_ASSERT( m_tagStack.Back().m_id == tagId, TXT("Trying to close invalid tag from the tag stack") );

	if ( m_tagStack.Empty() || m_tagStack.Back().m_id != tagId )
		return;

	if ( m_inTagHeader )
	{
		// just close current tag
		m_text->Write( "/>");
		m_inTagHeader = false;
	}
	else
	{
		// full close
		const StringAnsi& tagName = m_tagStack.Back().m_tag;
		m_text->Write( "</" );
		m_text->Write( tagName.AsChar() );
		m_text->Write( ">" );
	}

	m_tagStack.PopBack();
}

CHTMLDocument& CHTMLDocument::Write( const AnsiChar* txt )
{
	CloseTagHeader();
	m_text->Write( txt );
	return *this;
}

CHTMLDocument& CHTMLDocument::Writef( const AnsiChar* txt, ... )
{
	CloseTagHeader();

	AnsiChar buf[512];
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	m_text->Write( buf );
	return *this;
}

void CHTMLDocument::CloseTagHeader()
{
	if ( m_inTagHeader && !m_tagStack.Empty() )
	{
		m_text->Write( ">" );
		m_inTagHeader = false;
	}
}

CHTMLDocument& CHTMLDocument::operator <<( const AnsiChar* txt )
{
	m_text->Write( txt );
	return *this;
}

CHTMLDocument& CHTMLDocument::operator <<( const StringAnsi& txt )
{
	m_text->Write( txt.AsChar() );
	return *this;
}
