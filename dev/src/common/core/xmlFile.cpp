/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "xmlFile.h"
#include "feedback.h"
#include "dataError.h"
#include "../redSystem/error.h"

namespace rapidxml
{
	String error_file;

	void set_error_file( const String& new_error_file )
	{
		error_file = new_error_file;
	}

	void parse_error_handler( const char *what, void* where )
	{
		String text = where != nullptr ? String( static_cast< Char* >( where ) ) : String::EMPTY;
		if ( error_file.Empty() )
		{
			text = String::Printf( TXT("XML Parsing error: '%" ) RED_PRIWas TXT("' near '%" ) RED_PRIWs TXT("'"), what, text.LeftString( 100 ).AsChar() );
		}
		else
		{
			text = String::Printf( TXT("XML Parsing error in '%ls': '%" ) RED_PRIWas TXT("' near '%" ) RED_PRIWs TXT("'"), error_file.AsChar(), what, text.LeftString( 100 ).AsChar() );
		}
		ERR_CORE( TXT("ERROR: %s"), text.AsChar() );
		RED_WARNING( false, "XML Parser: %s", text.AsChar() );
		GFeedback->ShowError( text.AsChar() );
	}
}

IXMLFile::IXMLFile()
	: IFile( FF_Buffered | FF_FileBased | FF_XML )
	, m_doc( NULL )
	, m_topNodeStack( NULL )
	, m_currentChild( NULL )
	, m_xmlData( NULL )
	, m_lastUsedAttr( NULL )
{
}

IXMLFile::~IXMLFile()
{
}

void IXMLFile::Serialize( void*, size_t )
{
}

Uint64 IXMLFile::GetOffset() const
{
	return 0;
}

Uint64 IXMLFile::GetSize() const
{
	return 0;
}

void IXMLFile::Seek( Int64 )
{
}
