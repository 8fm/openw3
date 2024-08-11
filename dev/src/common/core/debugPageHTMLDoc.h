/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sharedPtr.h"
#include "htmlDocument.h"

/// HTML document wrapper for debug pages - using this template provides best visual results and makes everything consistent
class CDebugPageHTMLResponse
{
public:
	CDebugPageHTMLResponse( const CBasicURL& fullURL );

	// Get response data - a HTML document
	class CHTTPResponseData* GetData();

public:
	CBasicURL				m_fullURL;
	CBasicTextBuilder		m_text;
};

/// HTML document wrapper for debug pages - using this template provides best visual results and makes everything consistent
class CDebugPageHTMLDocument : public CHTMLDocument
{
public:
	CDebugPageHTMLDocument( CDebugPageHTMLResponse& responseContext, const StringAnsi& title );
	~CDebugPageHTMLDocument();

private:
	Double			m_startTime;
};

/// Information block - helper class
class CDebugPageHTMLInfoBlock : public Red::NonCopyable
{
public:
	CDebugPageHTMLInfoBlock( CDebugPageHTMLDocument& doc, const AnsiChar* title, ... );
	~CDebugPageHTMLInfoBlock();

	CDebugPageHTMLDocument& Info( const AnsiChar* txt, ... );

private:
	CDebugPageHTMLDocument&		m_doc;
	Bool						m_hasInfoBlockOpened;
};

/// HTML table helper - with sorting
class CDebugPageHTMLTable : public Red::NonCopyable
{
public:
	CDebugPageHTMLTable( CDebugPageHTMLDocument& doc, const AnsiChar* tableName );
	~CDebugPageHTMLTable();

	// row of source data
	class IRow
	{
	public:
		virtual ~IRow() {};
		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) = 0;
		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columntID ) = 0;
	};

	// row data
	class IRowData
	{
	public:
		virtual ~IRowData() {}; 
		virtual Bool OnCompare( const IRowData* other ) const = 0;
		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const = 0;
	};

	// create row data
	static IRowData* CreateRowData( const AnsiChar* txt );
	static IRowData* CreateRowData( const StringAnsi& txt );
	static IRowData* CreateRowData( const String& txt );
	static IRowData* CreateRowData( const Float value );
	static IRowData* CreateRowData( const Int64 value );
	static IRowData* CreateRowData( const CObject* object );
	static IRowData* CreateRowData( const class CDiskFile* file );
	static IRowData* CreateRowData( const class IRTTIType* dataType, const void* data );

	// data
	void AddColumn( const AnsiChar* caption, const Float width=0.0f, const Bool sortable=false, const Bool isVisible=true );
	void AddRow( IRow* row );
	void AddRow( IRowData* a, IRowData* b = nullptr, IRowData* c = nullptr, IRowData* d = nullptr, IRowData* e = nullptr );

	// render the table to the final document
	void Render( const Uint32 tableWidth, const AnsiChar* styleClass, const class CBasicURL& url ); // helper - will extract table sorting information from the URL

private:
	CDebugPageHTMLDocument&						m_doc;
	StringAnsi									m_tableName;

	struct Column
	{
		Bool			m_visible;				// is this column visible ?
		StringAnsi		m_name;
		Bool			m_sortable;
		Float			m_widthRequested;		// 0.0-1.0 - weighted fraction of the size, >1.0 - absolute pixel width
		Uint32			m_width;
	};

	TDynArray< Red::TSharedPtr< IRow > >		m_rows;
	TDynArray< Int32 >							m_rowsOrder;
	TDynArray< Column >							m_columns;

	void SetupColumnWidths( const Uint32 tableWidth );
};

/// HTML helper to print property value
class CDebugPageHTMLPropertyDump : public Red::NonCopyable
{
public:
	CDebugPageHTMLPropertyDump( CDebugPageHTMLDocument& doc );

	void DumpProperty( const CProperty* prop, const void* data );
	void DumpTypedData( const IRTTIType* type, const void* data );

private:
	CDebugPageHTMLDocument&						m_doc;
};

/// HTML helper for bigger lists
class CDebugPageHTMLBigList : public Red::NonCopyable
{
public:
	CDebugPageHTMLBigList( CDebugPageHTMLDocument& doc, const AnsiChar* listHeader );
	~CDebugPageHTMLBigList();

	void StartElement();

private:
	CDebugPageHTMLDocument&						m_doc;
	Bool										m_hasElement;
};

/// HTML helper for AJAX forms (yup, I'm sorry)
/// The form is serialized and sent as POST request to given target URL
class CDebugPageHTMLFormHelperAJAX : public Red::NonCopyable
{
public:
	CDebugPageHTMLFormHelperAJAX( CDebugPageHTMLDocument& doc, const StringAnsi& id, const StringAnsi& target = StringAnsi::EMPTY );
	~CDebugPageHTMLFormHelperAJAX();

	// controls creation
	CDebugPageHTMLDocument& InputBox( const StringAnsi& id, const StringAnsi& caption, const StringAnsi& initialText, const Int32 numChars = -1 );
	CDebugPageHTMLDocument& Button( const StringAnsi& id, const StringAnsi& caption, const Int32 width=-1, const Int32 height=-1 );
	CDebugPageHTMLDocument& SubmitButton( const StringAnsi& caption, const Int32 width=-1, const Int32 height=-1 );

private:
	CDebugPageHTMLDocument&				m_doc;
	StringAnsi							m_id;
};


// Expandable panel with +/- icons to expand/collapse it
class CDebugPageHTMLExpandablePanel : public Red::NonCopyable
{
public:
	CDebugPageHTMLExpandablePanel( CDebugPageHTMLDocument& doc, const Bool isExpaneded, const AnsiChar* title, ... );
	~CDebugPageHTMLExpandablePanel();

private:
	CDebugPageHTMLDocument&				m_doc;
};

class CDebugPageHTMLRowString : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowString( const StringAnsi& txt );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

protected:
	StringAnsi		m_data;
};

class CDebugPageHTMLRowInt : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowInt( const Int64 value );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

private:
	Int64		m_data;
};

class CDebugPageHTMLRowFloat : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowFloat( const Float value );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

private:
	Float		m_data;
};

class CDebugPageHTMLRowObject : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowObject( const class CObject* object );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

private:
	const class CObject*	m_object;
};

class CDebugPageHTMLRowFile : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowFile( const class CDiskFile* file );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

private:
	StringAnsi				m_path;
};

class CDebugPageHTMLRowGenericData : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugPageHTMLRowGenericData( const class IRTTIType* dataType, const void* data );
	virtual Bool OnCompare( const IRowData* other ) const;
	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const; 

private:
	const class IRTTIType*	m_type;
	const void*				m_data;
};
