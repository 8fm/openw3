/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _SS_EVENT_NAVIGATION_HISTORY_H_
#define _SS_EVENT_NAVIGATION_HISTORY_H_

class CNavigationHistoryEvent : public wxEvent
{
public:
	CNavigationHistoryEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CNavigationHistoryEvent( const wxString& file, const wxString& snippet, Red::System::Int32 line );
	virtual ~CNavigationHistoryEvent();

	inline const wxString& GetFile() const { return m_file; }
	inline const wxString& GetSnippet() const { return m_snippet; }
	inline Red::System::Int32 GetLine() const { return m_line; }

private:
	virtual wxEvent* Clone() const override final { return new CNavigationHistoryEvent( m_file, m_snippet, m_line ); }

private:
	wxString m_file;
	wxString m_snippet;
	Red::System::Int32 m_line;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CNavigationHistoryEvent );
};

wxDECLARE_EVENT( ssEVT_NAVIGATION_HISTORY_EVENT, CNavigationHistoryEvent );

#endif // _SS_EVENT_NAVIGATION_HISTORY_H_
