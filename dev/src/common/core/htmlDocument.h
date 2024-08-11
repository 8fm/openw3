/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sortedMap.h"
#include "staticarray.h"
#include "basicURL.h"
#include "basicTextBuilder.h"

/// HTML tag monitor - will close the tag that was opened when this object is destroyed
class CHTMLTagHelper : public Red::NonCopyable
{
public:
	CHTMLTagHelper( class CHTMLDocument* doc, const Uint32 tagId );
	CHTMLTagHelper( CHTMLTagHelper&& other );
	~CHTMLTagHelper();

	// define attribute in the tag
	CHTMLTagHelper& Attr( const AnsiChar* name, const AnsiChar* value ); // faster
	CHTMLTagHelper& Attrf( const AnsiChar* name, const AnsiChar* value, ... );

	// define conditional attribute
	CHTMLTagHelper& CondAttr( const Bool condition, const AnsiChar* name, const AnsiChar* value ); // faster
	CHTMLTagHelper& CondAttrf( const Bool condition, const AnsiChar* name, const AnsiChar* value, ... );

	// write into the tag
	CHTMLTagHelper& Write( const AnsiChar* txt ); // faster
	CHTMLTagHelper& Writef( const AnsiChar* txt, ... );

	// open sub tag
	CHTMLTagHelper& Open( const AnsiChar* tagName );

	// close current tag and go back to parent tag
	CHTMLTagHelper& Pop();

	// get document directly
	RED_INLINE CHTMLDocument& Doc() const { return *m_doc; }

private:
	class CHTMLDocument*	m_doc;
	TDynArray< Uint32 >		m_tagId;
};

/// HTML document desc
class CHTMLDocumentDesc
{
public:
	StringAnsi						m_title;
	TDynArray< StringAnsi >			m_scripts;
	TDynArray< StringAnsi >			m_styles;
};

/// HTML document builder - helper class
class CHTMLDocument
{
public:
	CHTMLDocument( CBasicTextBuilder& text, const CBasicURL& fullURL, const CHTMLDocumentDesc& desc );
	~CHTMLDocument();

	// Get full URL of the document being generated
	RED_INLINE const CBasicURL& GetURL() const { return m_url; }

	// Add automatic replacement text (%xxx% -> custom value)
	void AddReplacementKey( const StringAnsi& key, const StringAnsi& value );

	// Write text - note - be careful with manual tags in here
	CHTMLDocument& Write( const AnsiChar* txt );
	CHTMLDocument& Writef( const AnsiChar* txt, ... );

	// Open new HTML tag
	CHTMLTagHelper Open( const AnsiChar* tagName );

	// Link helper
	CHTMLTagHelper Link( const AnsiChar* href, ... );

	// Specific links
	CHTMLDocument& LinkObject( const CObject* object, const StringAnsi& customName="" );
	CHTMLDocument& LinkFile( const StringAnsi& path );
	CHTMLDocument& LinkScript( const AnsiChar* style, const AnsiChar* func, ... );

	// Image
	CHTMLDocument& Image( const AnsiChar* imageClassName );

	// Direct writing operator
	CHTMLDocument& operator <<( const AnsiChar* txt );
	CHTMLDocument& operator <<( const StringAnsi& txt );

private:
	struct TagStackElement
	{
		StringAnsi		m_tag;
		Uint32			m_id;
	};
	typedef TStaticArray< TagStackElement, 128 >		TTagStack;

	// tag list
	Uint32				m_tagId;
	TTagStack			m_tagStack;
	Bool				m_inTagHeader;	

	// automatic replacement map
	TSortedMap< Uint32, StringAnsi >	m_replaceMap;

	// writer
	IBasicTextBuilderAutotext*	m_textReplacer;
	CBasicTextBuilder*			m_text;

	// generation context
	CBasicURL				m_url;
	CHTMLDocumentDesc		m_desc;

	// initialize document
	static void InitDoc( CBasicTextBuilder& text, const CHTMLDocumentDesc& desc );
	static void FinishDoc( CBasicTextBuilder& text );

	// end current tag argument value
	void AttrTag( const Uint32 tagId, const AnsiChar* name, const AnsiChar* value );
	void CloseTag( const Uint32 tagId );
	void CloseTagHeader();

	friend class CHTMLTagHelper;
};

/// HTML table wrapper
class CHTMLTable
{
public:
	CHTMLTable( CHTMLDocument& doc );
	~CHTMLTable();

	void AddColumn( const AnsiChar* name, const Uint32 width=0 );
	void StartRow();
	void StartCell();

private:
	CHTMLDocument*	m_writer;
	Uint32			m_numCols;
	Uint32			m_curCol;
	Bool			m_headerDone;
	Bool			m_hasRow;

	// local printer
	typedef AnsiChar TLocalBuffer[ 512 ];

	void EndHeader();
	void Config( const Uint32 border = 1, const Uint32 cellSpacing=0, const Uint32 cellPadding=0 );
};

