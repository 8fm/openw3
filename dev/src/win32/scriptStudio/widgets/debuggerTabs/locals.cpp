/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "locals.h"

wxIMPLEMENT_CLASS( CSSLocalsDebuggerTab, CSSVariablesTabBase );

CSSLocalsDebuggerTab::CSSLocalsDebuggerTab( wxAuiNotebook* parent )
:	CSSVariablesTabBase( parent )
{

}

CSSLocalsDebuggerTab::~CSSLocalsDebuggerTab()
{

}

void CSSLocalsDebuggerTab::Refresh()
{
	RequestUpdate( 0 );
}

void CSSLocalsDebuggerTab::OnStackFrameSelected( CCallstackFrameSelectedEvent& event )
{
	RequestUpdate( event.GetFrame() );

	event.Skip();
}

void CSSLocalsDebuggerTab::RequestUpdate( Red::System::Uint32 stackFrameIndex )
{
	wxTreeItemId root = m_tree->GetRootItem();

	m_currentStackFrameIndex = stackFrameIndex;

	CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( root ) );

	// Request new variables
	m_stamp = UPDATE_STAMP;
	m_helper.RequestLocals( m_currentStackFrameIndex, m_stamp, wxEmptyString, data->id );
}

Red::System::Uint32 CSSLocalsDebuggerTab::GetItemExpansionStamp()
{
	return STACK_STAMP;
}
