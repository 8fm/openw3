
#pragma once

class CEdHtmlLog : public wxHtmlWindow
{
	wxString m_source;

public:
	CEdHtmlLog( wxWindow* parent );

	void Log( const wxString& mgs );
	void Log( const String& mgs );

	void ClearLog();

private:
	void RefreshLogHtml();
};
