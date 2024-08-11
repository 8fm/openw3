/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "math.h"
#include "basicURL.h"
#include "object.h"
#include "resource.h"
#include "diskFile.h"
#include "engineTime.h"
#include "version.h"
#include "debugPageHTMLDoc.h"
#include "basicDataBlob.h"
#include "httpResponseData.h"

//----------------

CDebugPageHTMLResponse::CDebugPageHTMLResponse( const CBasicURL& fullURL )
	: m_fullURL( fullURL )
{
}

class CHTTPResponseData* CDebugPageHTMLResponse::GetData()
{
	const Uint32 dataSize = m_text.GetDataSize();
	DataBlobPtr data( new CDataBlob( dataSize ) );
	m_text.CopyData( data->GetData() );
	return new CHTTPResponseData( "text/html", data );
}

//----------------

namespace Helper
{
	static CHTMLDocumentDesc PrepareDebugPageHTMLDesc( const StringAnsi& title )
	{
		CHTMLDocumentDesc ret;
		ret.m_styles.PushBack( "static/main.css" );
		ret.m_scripts.PushBack( "static/scripts/bucket.js" );
		ret.m_scripts.PushBack( "//code.jquery.com/jquery-1.11.0.min.js" );
		ret.m_title = title;
		return ret;
	}
};

CDebugPageHTMLDocument::CDebugPageHTMLDocument( CDebugPageHTMLResponse& responseContext, const StringAnsi& title )
	: CHTMLDocument( responseContext.m_text, responseContext.m_fullURL, Helper::PrepareDebugPageHTMLDesc( title ) )
	, m_startTime( (Double) EngineTime::GetNow() )
{
	// document div
	Write( "<div class=\"doc\">");

	// header div
	Open("div").Attr("class", "header").
		Open("div").Attr("class", "logo").
			Writef( "<a href=\"#<host>#index.html\"><img src=\"#<host>#static/logo.png\"></a>" ).
		Pop().
		Open("div").Attr("class", "title").
			Writef( "<p>%hs</p>", title.AsChar() ).
		Pop().
	Pop();

	// content div
	Write( "<div class=\"content\">");
}

CDebugPageHTMLDocument::~CDebugPageHTMLDocument()
{
	// end of content div
	Write( "</div>");

	// measure time it took to generate the page
	const Double timeTaken = ( (Double)EngineTime::GetNow() - m_startTime );

	// tail div
	Open("div").Attr("class", "footer").
		Writef("<p>Build %hs (CL %hs)</p>", APP_VERSION_BUILD, APP_LAST_P4_CHANGE).
		Writef("<p>Page generated in %1.3fms</p>", timeTaken * 1000.0);

	// end of the doc div
	Write( "</div>");
}

//----------------

CDebugPageHTMLInfoBlock::CDebugPageHTMLInfoBlock( CDebugPageHTMLDocument& doc, const AnsiChar* title, ... )
	: m_doc( doc )
	, m_hasInfoBlockOpened( false )
{
	AnsiChar buf[512];
	va_list args;

	va_start( args, title );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), title, args );
	va_end( args );

	m_doc << "<div class=\"info\">";

	m_doc << "<p class=\"infoheader\">";
	m_doc << buf;
	m_doc << "</p>";
}

CDebugPageHTMLInfoBlock::~CDebugPageHTMLInfoBlock()
{
	if ( m_hasInfoBlockOpened )
	{
		m_hasInfoBlockOpened = false;
		m_doc << "</p>";
	}

	m_doc << "</div>";
}

CDebugPageHTMLDocument& CDebugPageHTMLInfoBlock::Info( const AnsiChar* txt, ... )
{
	AnsiChar buf[512];
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
	va_end( args );

	if ( m_hasInfoBlockOpened )
	{
		m_hasInfoBlockOpened = false;
		m_doc << "</p>";
	}

	m_doc << "<p class=\"infodata\">";
	m_doc << buf;

	m_hasInfoBlockOpened = true;

	return m_doc;
}

//----------------

CDebugPageHTMLTable::CDebugPageHTMLTable( CDebugPageHTMLDocument& doc, const AnsiChar* tableName )
	: m_doc( doc )
	, m_tableName( tableName )
{
}

CDebugPageHTMLTable::~CDebugPageHTMLTable()
{
}

void CDebugPageHTMLTable::AddColumn( const AnsiChar* caption, const Float width/*=0.0f*/, const Bool sortable/*=false*/, const Bool isVisible/*=true*/ )
{
	Column* info = new ( m_columns ) Column;
	info->m_name = caption;
	info->m_sortable = sortable;
	info->m_visible = isVisible;
	info->m_widthRequested = width;
	info->m_width = 0; // calculated at the "Render"
}

void CDebugPageHTMLTable::AddRow( IRow* row )
{
	m_rows.PushBack( Red::TSharedPtr< IRow >(row) );
	m_rowsOrder.PushBack( m_rowsOrder.Size() );
}

//----------------------------

CDebugPageHTMLRowString::CDebugPageHTMLRowString( const StringAnsi& txt )
	: m_data( txt )
{}

Bool CDebugPageHTMLRowString::OnCompare( const IRowData* other ) const
{
	const auto* data = static_cast< const CDebugPageHTMLRowString* >( other );
	return m_data < data->m_data;
}

void CDebugPageHTMLRowString::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	doc.Write( m_data.AsChar() );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const AnsiChar* txt )
{
	return new CDebugPageHTMLRowString( txt );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const String& txt )
{
	return new CDebugPageHTMLRowString( UNICODE_TO_ANSI( txt.AsChar() ) );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const StringAnsi& txt )
{
	return new CDebugPageHTMLRowString( txt );
}

//----------------------------

CDebugPageHTMLRowInt::CDebugPageHTMLRowInt( const Int64 value )
	: m_data( value )
{
}

Bool CDebugPageHTMLRowInt::OnCompare( const IRowData* other ) const
{
	const auto* data = static_cast< const CDebugPageHTMLRowInt* >( other );
	return m_data < data->m_data;
}

void CDebugPageHTMLRowInt::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	doc.Writef( "%i", (Int64)m_data );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const Int64 value )
{
	return new CDebugPageHTMLRowInt( value );
}
//----------------------------

CDebugPageHTMLRowFloat::CDebugPageHTMLRowFloat( const Float value )
	: m_data( value )
{
}

Bool CDebugPageHTMLRowFloat::OnCompare( const IRowData* other ) const
{
	const auto* data = static_cast< const CDebugPageHTMLRowFloat* >( other );
	return m_data < data->m_data;
}

void CDebugPageHTMLRowFloat::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	doc.Writef( "%f", m_data );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const Float value )
{
	return new CDebugPageHTMLRowFloat( value );
}

//----------------------------

CDebugPageHTMLRowObject::CDebugPageHTMLRowObject( const class CObject* object )
	: m_object( object )
{
}

Bool CDebugPageHTMLRowObject::OnCompare( const IRowData* other ) const
{
	const auto* data = static_cast< const CDebugPageHTMLRowObject* >( other );
	return m_object < data->m_object;
}

void CDebugPageHTMLRowObject::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	doc.LinkObject( m_object );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const CObject* object )
{
	return new CDebugPageHTMLRowObject( object );
}

//----------------------------

CDebugPageHTMLRowFile::CDebugPageHTMLRowFile( const class CDiskFile* file )
{
	if ( file )
	{
		m_path = UNICODE_TO_ANSI( file->GetDepotPath().AsChar() );
	}
}

Bool CDebugPageHTMLRowFile::OnCompare( const IRowData* other ) const
{
	const auto* data = static_cast< const CDebugPageHTMLRowFile* >( other );
	return m_path < data->m_path;
}

void CDebugPageHTMLRowFile::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	doc.LinkFile( m_path );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const class CDiskFile* file )
{
	return new CDebugPageHTMLRowFile( file );
}

//----------------------------

CDebugPageHTMLRowGenericData::CDebugPageHTMLRowGenericData( const class IRTTIType* dataType, const void* data )
	: m_type( dataType )
	, m_data( data )
{
}

Bool CDebugPageHTMLRowGenericData::OnCompare( const IRowData* other ) const
{
	return (Uint64)this < (Uint64)other;
}

void CDebugPageHTMLRowGenericData::OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
{
	CDebugPageHTMLPropertyDump data(doc);
	data.DumpTypedData( m_type, m_data );
}

CDebugPageHTMLTable::IRowData* CDebugPageHTMLTable::CreateRowData( const class IRTTIType* dataType, const void* data )
{
	return new CDebugPageHTMLRowGenericData( dataType, data );
}

//----------------------------

class CDebugPageHTMLTableAutoRow : public CDebugPageHTMLTable::IRow
{
public:
	CDebugPageHTMLTableAutoRow() {};
	virtual ~CDebugPageHTMLTableAutoRow() { m_data.ClearPtr(); }

	void AddData( CDebugPageHTMLTable::IRowData* data )
	{
		m_data.PushBack( data ? data : new CDebugPageHTMLRowString("") );
	}

	virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
	{
		// valid data case
		if ( columnID >= 0 && columnID <= m_data.SizeInt() )
		{
			const CDebugPageHTMLTableAutoRow* otherRow = static_cast< const CDebugPageHTMLTableAutoRow* >( other );
			if ( columnID <= otherRow->m_data.SizeInt() )
			{
				return m_data[ columnID-1 ]->OnCompare( otherRow->m_data[ columnID-1 ] );
			}
		}

		// default case
		return (Uint64)this < (Uint64)other;
	}

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
	{
		if ( columnID >= 1 && columnID <= m_data.SizeInt() )
		{
			m_data[ columnID-1 ]->OnGenerateHTML( doc );
		}
	}


private:
	TDynArray< CDebugPageHTMLTable::IRowData* >		m_data;
};

void CDebugPageHTMLTable::AddRow( IRowData* a, IRowData* b /*= nullptr*/, IRowData* c /*= nullptr*/, IRowData* d /*= nullptr*/, IRowData* e /*= nullptr*/  )
{
	CDebugPageHTMLTableAutoRow* newRow = new CDebugPageHTMLTableAutoRow();
	if ( a ) newRow->AddData( a );
	if ( b ) newRow->AddData( b );
	if ( c ) newRow->AddData( c );
	if ( d ) newRow->AddData( d );
	if ( e ) newRow->AddData( e );
	AddRow( newRow );
}

//----------------------------

void CDebugPageHTMLTable::SetupColumnWidths( const Uint32 tableWidth )
{
	// calculate leftover size after columns with constant size are handled
	Int32 dynamicWidth = tableWidth;
	Float dynamicSum = 0.0f;
	for ( Column& info : m_columns )
	{
		if ( info.m_visible )
		{
			if ( info.m_widthRequested > 1.0f )
			{
				dynamicWidth -= (Int32) info.m_widthRequested;
				info.m_width = (Int32) info.m_widthRequested;
			}
			else
			{
				dynamicSum += info.m_widthRequested;
			}
		}
	}

	// distribute the left overs proportionally
	if ( dynamicWidth > 0 )
	{
		for ( Column& info : m_columns )
		{
			if ( info.m_visible )
			{
				if ( info.m_widthRequested <= 1.0f )
				{
					const Float frac = (dynamicSum > 0.0f) ? (info.m_widthRequested / dynamicSum) : 1.0f;
					info.m_width = (Int32)( dynamicWidth * frac );
				}
			}
		}
	}
}

void CDebugPageHTMLTable::Render( const Uint32 tableWidth, const AnsiChar* styleClass, const class CBasicURL& url )
{
	// TODO: toggle visibility of columns based on the URL settings

	// get sorting info
	Int32 sortByColumn = 0;
	const StringAnsi sortKey = m_tableName + "_sort";
	url.GetKey( sortKey, sortByColumn );

	// calculate column layout
	SetupColumnWidths( tableWidth );

	// create table layout
	m_doc.Writef( "<table class=\"%hs\">", styleClass );

	// sort items
	if ( sortByColumn != 0 )
	{
		::Sort( m_rowsOrder.Begin(), m_rowsOrder.End(), 
			[this, sortByColumn]( const Int32 a, const Int32 b) 
			{
				auto rowA = this->m_rows[a].Get();
				auto rowB = this->m_rows[b].Get();
				if ( sortByColumn > 0 )
					return rowA->OnCompare( rowB, sortByColumn );
				else
					return rowB->OnCompare( rowA, -sortByColumn );
			}
		);
	}

	// header
	{
		m_doc.Write( "<tr>" );

		// column info
		for ( Int32 columnIndex = 1; columnIndex<=m_columns.SizeInt(); ++columnIndex )
		{
			const auto& info = m_columns[ columnIndex-1 ]; // watch out, indexing goes from 1

			if ( info.m_visible )
			{
				m_doc.Writef( "<th width=\"%d\">", info.m_width );

				if ( info.m_sortable )
				{
					// preserve the URL, override JUST the sorting order for given table
					CBasicURL newURL( url );
					if ( abs(sortByColumn) == columnIndex )
					{
						newURL.SetKey( sortKey, -sortByColumn ); // invert sorting order
					}
					else
					{
						newURL.SetKey( sortKey, columnIndex ); // ascending sorting by given column
					}

					// sortable column
					m_doc.Writef( "<a href=\"%hs\">%hs</a>", 
						newURL.ToString().AsChar(),
						info.m_name.AsChar() );
				}
				else
				{
					// unsortable column
					m_doc.Write( info.m_name.AsChar() );
				}

				m_doc.Write( "</th>" );
			}
		}

		m_doc.Write( "</tr>" );
	}

	// items
	{

		for ( Uint32 i=0; i<m_rowsOrder.Size(); ++i )
		{
			auto row = m_rows[ m_rowsOrder[i] ].Get();

			m_doc.Write( "<tr>" );

			// render the data into columns
			for ( Int32 columnIndex = 1; columnIndex<=m_columns.SizeInt(); ++columnIndex )
			{
				const auto& info = m_columns[ columnIndex-1 ]; // watch out, indexing goes from 1
				if ( info.m_visible )
				{
					m_doc.Write( "<td>" );
					row->OnGenerateHTML( m_doc, columnIndex );
					m_doc.Write( "</td>" );
				}
			}

			m_doc.Write( "</tr>" );
		}
	}

	m_doc.Write( "</table>" );
}

//----------------

CDebugPageHTMLPropertyDump::CDebugPageHTMLPropertyDump( CDebugPageHTMLDocument& doc )
	: m_doc( doc )
{
}

void CDebugPageHTMLPropertyDump::DumpProperty( const CProperty* prop, const void* data )
{
	m_doc.Write("<p>");
	m_doc.Writef("%hs = ", prop->GetName().AsAnsiChar() );

	DumpTypedData( prop->GetType(), data );

	m_doc.Write("</p>");
}

void CDebugPageHTMLPropertyDump::DumpTypedData( const IRTTIType* type, const void* data )
{
	if ( type->GetType() == RT_Fundamental || type->GetType() == RT_Simple || type->GetType() == RT_Enum || type->GetType() == RT_BitField )
	{
		String value;
		type->ToString( data, value );
		m_doc.Write( UNICODE_TO_ANSI( value.AsChar() ) );
	}
	else if ( type->GetType() == RT_Class )
	{
		const CClass* propClass = static_cast< const CClass* >( type );

		// COLOR HACK :)
		if ( propClass == ClassID< Color >() )
		{
			const Color* color = (const Color*) data;
			m_doc.Writef("<div class=\"colorbox\" style=\"background-color: #%02X%02X%02X\" title=\"(Red=%d, Green=%d, Blue=%d, Alpha=%d)\"></div>",
				color->R, color->G, color->B,
				color->R, color->G, color->B, color->A );
		}

		// normal structure
		else
		{
			m_doc.Write( UNICODE_TO_ANSI( propClass->GetName().AsChar() ) );

			const auto& props = propClass->GetCachedProperties();
			if ( !props.Empty() )
			{
				m_doc.Write("<table class=\"prop\">");

				for ( const CProperty* classProp : props )
				{
					m_doc.Write("<tr>");
					m_doc.Write("<td>");

					const void* propData = classProp->GetOffsetPtr( data );
					DumpProperty( classProp, propData );

					m_doc.Write("</td>");
					m_doc.Write("</tr>");
				}

				m_doc.Write("</table>");
			}
		}
	}
	else if ( type->GetType() == RT_Handle || type->GetType() == RT_Pointer )
	{
		const CRTTIPointerType* pointerType = static_cast< const CRTTIPointerType* >( type );
		const CClass* pointedClass = pointerType->GetPointedType();
		const CPointer pointedObject = pointerType->GetPointer( data );

		if ( pointedObject.IsNull() )
		{
			m_doc.Write( "NULL" );
		}
		else if ( pointedClass->IsA< CResource >() && ((const CResource*)pointedObject.GetObjectPtr() )->GetFile() )
		{
			const CResource* res = static_cast< const CResource* >( pointedObject.GetObjectPtr() );
			const StringAnsi path( UNICODE_TO_ANSI( res->GetFile()->GetDepotPath().AsChar() ) );
			m_doc.Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
		}
		else if ( pointerType->GetPointedType()->IsA< CObject >() )
		{
			const CObject* obj = pointedObject.GetObjectPtr();
			if ( obj->HasFlag( OF_Inlined ) )
			{
				// inlined object, dump properties
				CDebugPageHTMLExpandablePanel panel( m_doc, true, "Inlined %hs", UNICODE_TO_ANSI( obj->GetClass()->GetName().AsChar() ) );

				// show object properties
				const auto& props = obj->GetClass()->GetCachedProperties();
				if ( !props.Empty() )
				{
					m_doc.Write("<table class=\"prop\">");

					for ( const CProperty* classProp : props )
					{
						m_doc.Write("<tr>");
						m_doc.Write("<td>");

						const void* propData = classProp->GetOffsetPtr( obj );
						DumpProperty( classProp, propData );

						m_doc.Write("</td>");
						m_doc.Write("</tr>");
					}

					m_doc.Write("</table>");
				}
			}
			else
			{
				// external object - link to it
				m_doc.Link( "/object/?id=%d", obj->GetObjectIndex() ).
					Writef( "%d (0x%016llx) %hs", obj->GetObjectIndex(), (Uint64)obj, obj->GetClass()->GetName().AsAnsiChar() );
			}
		}
		else
		{
			m_doc.Link( "/memory/?start=%lld&size=%d", (Uint64)pointedObject.GetPointer(), pointedClass->GetSize() );
		}
	}
	else if ( type->GetType() == RT_SoftHandle )
	{
		const BaseSoftHandle& softHandle = *(const BaseSoftHandle*) data;
		const StringAnsi path( UNICODE_TO_ANSI( softHandle.GetPath().AsChar() ) );
		m_doc.Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
	}
	else if ( type->GetType() == RT_Array || type->GetType() == RT_NativeArray || type->GetType() == RT_StaticArray )
	{
		const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( type );
		const Uint32 count = arrayType->ArrayGetArraySize( data );
		if ( count == 0)
		{
			m_doc.Write( "Empty array" );
		}
		else
		{
			const Bool isVisible = (count < 10);
			CDebugPageHTMLExpandablePanel panel( m_doc, isVisible, "Array of %d elements", count );

			m_doc.Write("<table class=\"prop\">");

			for ( Uint32 i=0; i<count; ++i )
			{
				m_doc.Write("<tr>");
				m_doc.Write("<td>");

				const void* propData = arrayType->ArrayGetArrayElement( data, i );
				DumpTypedData( arrayType->ArrayGetInnerType(), propData );
			}

			m_doc.Write("</td>");
			m_doc.Write("</tr>");
			m_doc.Write("</table>");
		}
	}
	else
	{
		m_doc.Write( "unknown" );
	}
}

//----------------

CDebugPageHTMLBigList::CDebugPageHTMLBigList( CDebugPageHTMLDocument& doc, const AnsiChar* listHeader )
	: m_doc( doc )
	, m_hasElement( false )
{
	m_doc.Write( "<table class=\"list\">" );

	if ( listHeader && listHeader[0] )
	{
		m_doc.Write( "<tr><th>");
		m_doc.Write( listHeader );
		m_doc.Write( "</th></tr>");
	}
}

CDebugPageHTMLBigList::~CDebugPageHTMLBigList()
{
	if ( m_hasElement )
	{
		m_doc.Write( "</td></tr>");
	}

	m_doc.Write( "</table>");
}

void CDebugPageHTMLBigList::StartElement()
{
	if ( m_hasElement )
	{
		m_doc.Write( "</td></tr>");
	}

	m_doc.Write( "<tr><td>");
	m_hasElement = true;
}

//----------------

CDebugPageHTMLFormHelperAJAX::CDebugPageHTMLFormHelperAJAX( CDebugPageHTMLDocument& doc, const StringAnsi& id, const StringAnsi& target )
	: m_doc( doc )
	, m_id( id )
{
	const AnsiChar* targetURL = target.Empty() ? "#<url>#" : target.AsChar();
	m_doc.Writef("<form id=\"%hs\" action=\"#<url>#\" method=\"post\">", id.AsChar(), targetURL );
}

CDebugPageHTMLFormHelperAJAX::~CDebugPageHTMLFormHelperAJAX()
{
	m_doc.Writef("</form>" );

	// AJAX bullshit connector
	m_doc.Writef("<script type=\"text/javascript\">connectform('#%hs');</script>", m_id.AsChar() );
}

CDebugPageHTMLDocument& CDebugPageHTMLFormHelperAJAX::InputBox( const StringAnsi& id, const StringAnsi& caption, const StringAnsi& initialText, const Int32 numChars /*= -1*/ )
{
	m_doc.Write("<div class=\"field\">");

	if ( !caption.Empty() )
	{
		m_doc.Open("label").Attr("for",id.AsChar()).Write( caption.AsChar() );
	}

	m_doc.Open("input").
		Attr("id",id.AsChar()).
		Attr("name",id.AsChar()).
		Attr( "value", initialText.AsChar() ).
		CondAttrf( numChars > 0, "size", "%d", numChars );

	m_doc.Write("</div>");

	return m_doc;
}

CDebugPageHTMLDocument& CDebugPageHTMLFormHelperAJAX::Button( const StringAnsi& id, const StringAnsi& caption, const Int32 width/*=-1*/, const Int32 height/*=-1*/ )
{
	m_doc.Open("div").
		Attr("class","field").
		Open("button").
		Attr("type","button").
		Write( caption.AsChar() ).
		CondAttrf( width > 0, "width", "%d", width ).
		CondAttrf( height > 0, "height", "%d", height );

	return m_doc;
}

CDebugPageHTMLDocument& CDebugPageHTMLFormHelperAJAX::SubmitButton( const StringAnsi& caption, const Int32 width/*=-1*/, const Int32 height/*=-1*/ )
{
	m_doc.Open("div").
		Attr("class","field").
		Open("button").
		Attr("type","submit").
		Write( caption.AsChar() ).
		CondAttrf( width > 0, "width", "%d", width ).
		CondAttrf( height > 0, "height", "%d", height );

	return m_doc;
}

//----------------

CDebugPageHTMLExpandablePanel::CDebugPageHTMLExpandablePanel( CDebugPageHTMLDocument& doc, const Bool isExpaneded, const AnsiChar* title, ... )
	: m_doc( doc )
{
	// allocate ID
	static Uint32 globalPanelId = 1;
	const Uint32 id = globalPanelId++;

	// title
	AnsiChar buf[512];
	va_list args;

	va_start( args, title );
	Red::VSNPrintF( buf, ARRAY_COUNT(buf), title, args );
	va_end( args );

	// create +/i expand link
	m_doc.Open("a").
		Attr("href","#hide").
		Attr("class",isExpaneded ? "hide" : "show").
		Attrf("id","vis%d", id).
		Attrf("onclick","togglevisibility('vis%d')", id).
		Write("+");

	// title
	m_doc.Write( buf );

	// the panel that will be toggled
	m_doc.Writef("<div class=\"vis_%s\" id=\"vis%d_panel\">", isExpaneded ? "on" : "off", id );
}

CDebugPageHTMLExpandablePanel::~CDebugPageHTMLExpandablePanel()
{
	m_doc.Write( "</div>" );
}

//----------------
