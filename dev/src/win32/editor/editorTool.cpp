/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "toolsPanel.h"

IMPLEMENT_ENGINE_CLASS( IEditorTool );

class CEditorToolContextMenu : public wxEvtHandler
{
public:
	CEditorToolContextMenu( IEditorTool *editorTool ) : m_editorTool( editorTool ) {}

	void OnContextMenu( wxCommandEvent& event )
	{
		wxTheFrame->GetToolsPanel()->CancelTool();
	}

	void HandleContextMenu()
	{
		wxMenu menu;

		menu.Append( 0, TXT("Close active tool") );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEditorToolContextMenu::OnContextMenu ), NULL, this );

		wxTheFrame->PopupMenu( &menu );
	}

private:
	IEditorTool *m_editorTool;
};

Bool IEditorTool::HandleContextMenu( Int32 x, Int32 y, const Vector& collision )
{
	CEditorToolContextMenu menu( this );

	menu.HandleContextMenu();

	return true;
}

void IEditorTool::GetDefaultAccelerator( Int32 &flags, Int32 &key ) const
{
    flags = 0;
    key = 0;
}
