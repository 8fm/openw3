/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef __DEBUGGER_TAB_CALLSTACK_H__
#define __DEBUGGER_TAB_CALLSTACK_H__

#include "base.h"

struct SFrame;
struct SNativeFrame;
class CCallstackUpdateEvent;

class CSSCallstackDebuggerTab : public CSSDebuggerTabBase
{
	wxDECLARE_CLASS( CSSCallstackDebuggerTab );

	enum EMenuId
	{
		emid_CopyAll = wxID_HIGHEST,
		emid_CopySelection
	};

	enum EColumn
	{
		Col_Icon = 0,
		Col_Location
	};

	enum EIcon
	{
		Icon_Invalid = -1,

		Icon_Head = 0,
		Icon_Local,

		Icon_Max
	};

	struct LocationInfo
	{
		wxString			m_file;
		Red::System::Uint32	m_line;
	};

public:
	CSSCallstackDebuggerTab( wxAuiNotebook* parent );
	virtual ~CSSCallstackDebuggerTab();

	virtual void Connect( const wxChar* ip ) override final;
	virtual void DebuggingStopped() override final;
	virtual void Refresh() override final;

	virtual bool Paste( const wxString& text );

	void OnCallstackEvent( CCallstackUpdateEvent& event );

private:
	void OnContextMenu( wxContextMenuEvent& event );

	virtual void CopyToClipboard() override final;
	void OnCopyAll( const wxCommandEvent& event );
	void OnCopySelection( const wxCommandEvent& event );
	void CopyAll();
	void CopySelection();
	wxString Extract( int startItem, int endItem );
	void PushToClipboard( const wxString& value );

	wxString FormatFrame( const SFrame& frame ) const;
	wxString FormatFrame( const SNativeFrame& frame ) const;
	void AppendFrame( const wxString& location, const wxColour& colour, const LocationInfo* info = nullptr );

	void ActivateStackFrame( int newActiveStackFrame );
	void OnItemActivated( wxListEvent& event );
	void OnItemDeleted( wxListEvent& event );

private:
	wxListView* m_listView;

	int m_activeStackFrame;
};

#endif // __DEBUGGER_TAB_LOCALS_H__
