/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Tag helper
class CHTMLTag
{
public:
	wxString*	m_wxString;
	wxString	m_wxTag;

	String*		m_string;
	String		m_tag;

public:
	CHTMLTag( wxString& str, const wxChar* tag )
		: m_wxString( &str )
		, m_wxTag( tag )
	{
		*m_wxString += wxT( "<");
		*m_wxString += m_wxTag;
		*m_wxString += wxT( ">");
	}

	CHTMLTag( String& str, const String& tag )
		: m_string( &str )
		, m_tag( tag )
		, m_wxString( NULL )
	{
		*m_string += TXT( "<");
		*m_string += m_tag;
		*m_string += TXT( ">");
	}


	~CHTMLTag()
	{
		if ( m_wxString )
		{
			*m_wxString += wxT( "</");
			*m_wxString += m_wxTag;
			*m_wxString += wxT( ">");
		}
		else
		{
			*m_string += TXT( "</");
			*m_string += m_tag;
			*m_string += TXT( ">");
		}
	}

};
