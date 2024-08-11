/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __DEBUGGER_TAB_BASE_H__
#define __DEBUGGER_TAB_BASE_H__

// Base window class that will allow all windows deriving from this one to detach and move from window to window
class CSSFloatingTab : public wxPanel
{
	wxDECLARE_CLASS( CSSFloatingTab );

public:
	CSSFloatingTab( wxAuiNotebook* parent );
	virtual ~CSSFloatingTab();

private:
	wxAuiNotebook* m_parent;
};

//This is the base class from which all the "debugger tab" utility windows will derive
class CSSDebuggerTabBase : public CSSFloatingTab
{
	wxDECLARE_CLASS( CSSDebuggerTabBase );

public:
	CSSDebuggerTabBase( wxAuiNotebook* parent );
	virtual ~CSSDebuggerTabBase();

	virtual void Connect( const wxChar* ip ) = 0;
	virtual void CopyToClipboard() = 0;
	virtual void DebuggingStopped() = 0;
	virtual void Refresh() = 0;

	virtual bool Paste( const wxString& text );
};

#endif // __DEBUGGER_TAB_BASE_H__
