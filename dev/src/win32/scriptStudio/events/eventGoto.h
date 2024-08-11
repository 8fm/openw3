/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __EVENT_GOTO_H__
#define __EVENT_GOTO_H__

class COpenFileEvent : public wxEvent
{
protected:
	COpenFileEvent( wxEventType commandType, const wxString& file, Red::System::Bool generateHistory );

public:
	COpenFileEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	COpenFileEvent( const wxString& file, Red::System::Bool generateHistory = false );
	virtual ~COpenFileEvent();

	inline const wxString& GetFile() const { return m_file; }
	inline Red::System::Bool GenerateHistory() const { return m_generateHistory; }

protected:
	virtual wxEvent* Clone() const override { return new COpenFileEvent( m_file, m_generateHistory ); }

private:
	wxString m_file;
	Red::System::Bool m_generateHistory;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( COpenFileEvent );
};

wxDECLARE_EVENT( ssEVT_OPEN_FILE_EVENT, COpenFileEvent );

//////////////////////////////////////////////////////////////////////////

class CGotoEvent : public COpenFileEvent
{
public:
	explicit CGotoEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	explicit CGotoEvent( const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory = false );
	explicit CGotoEvent( wxEventType commandType, const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory = false );
	virtual ~CGotoEvent();

	inline Red::System::Int32 GetLine() const { return m_line; }

private:
	virtual wxEvent* Clone() const override { return new CGotoEvent( GetFile(), m_line, GenerateHistory() ); }

private:
	Red::System::Int32 m_line;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CGotoEvent );
};

wxDECLARE_EVENT( ssEVT_GOTO_EVENT, CGotoEvent );

#endif // __EVENT_GOTO_H__
