
#include "build.h"
#include "htmlLog.h"

CEdHtmlLog::CEdHtmlLog( wxWindow* parent )
	: wxHtmlWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO )
{

}

void CEdHtmlLog::Log( const String& msg )
{
	m_source += msg.AsChar();
	m_source += wxT("<br>");

	RefreshLogHtml();
}

void CEdHtmlLog::Log( const wxString& msg )
{
	m_source += msg;
	m_source += wxT("<br>");

	RefreshLogHtml();
}

void CEdHtmlLog::ClearLog()
{
	m_source.Clear();

	RefreshLogHtml();
}

void CEdHtmlLog::RefreshLogHtml()
{
	SetPage( m_source );
}
