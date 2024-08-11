/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __DEBUGGER_TAB_WATCH_H__
#define __DEBUGGER_TAB_WATCH_H__

#include "variables.h"
#include <functional>

class CCallstackFrameSelectedEvent;

class CSSWatchDebuggerTab : public CSSVariablesTabBase
{
	wxDECLARE_CLASS( CSSWatchDebuggerTab );

public:
	CSSWatchDebuggerTab( wxAuiNotebook* parent );
	virtual ~CSSWatchDebuggerTab();

	void OnStackFrameSelected( CCallstackFrameSelectedEvent& event );

private:
	static const Red::System::Uint32 WATCH_STAMP = 0x77746368; //"wtch"
	static const Red::System::Uint32 WATCH_EXPANSION_STAMP = 0x77657870; //"wexp"

	virtual Red::System::Uint32 GetItemExpansionStamp() override final;
	virtual void Refresh() override final;
	void RequestUpdate( Red::System::Uint32 stackFrameIndex );

	void OnStartLabelEdit( wxTreeEvent& event );
	void OnEndLabelEdit( wxTreeEvent& event );

	virtual void OnUserCreateNewItem() override final;
	virtual void OnUserStartEditItemName( const wxTreeItemId& item ) override final;

	wxTreeItemId CreateWatch( wxString& path, bool refresh = true );

	virtual bool Paste( const wxString& text ) override final;
	void OnPaste( wxClipboardTextEvent& event );
	void OnCopy( wxClipboardTextEvent& event );
	void OnMenuItemSelected( wxCommandEvent& event );

	virtual void OnKeyDown( wxTreeEvent& event ) override final;
	void OnRightClick( wxMouseEvent& event );

	void OnTreeDrag( wxTreeEvent& event );
	void OnTreeDrop( wxTreeEvent& event );
	wxTreeItemId m_draggedItem;

	wxCursor* m_dragCursor;

	class CSSWatchDebuggerDropTarget : public wxTextDropTarget
	{
	public:
		typedef std::function< bool( const wxString& ) > TCallback;

		CSSWatchDebuggerDropTarget( TCallback callback );
		virtual ~CSSWatchDebuggerDropTarget();

		virtual bool OnDropText( wxCoord x, wxCoord y, const wxString& data ) override final;

	private:
		TCallback m_callback;
	};
};

#endif // __DEBUGGER_TAB_WATCH_H__
