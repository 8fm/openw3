/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/core/clipboardBase.h"

#include "behaviorEditor.h"
#include "behaviorAnimationLister.h"

BEGIN_EVENT_TABLE( CEdBehaviorGraphAnimationLister, CEdBehaviorEditorSimplePanel )
	EVT_BUTTON( XRCID( "buttListAnims" ), CEdBehaviorGraphAnimationLister::OnListAnims )
	EVT_BUTTON( XRCID( "buttCopyToClipboard" ), CEdBehaviorGraphAnimationLister::OnCopyToClipboard )
END_EVENT_TABLE()

CEdBehaviorGraphAnimationLister::CEdBehaviorGraphAnimationLister( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorAnimationLister") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_list = XRCCTRL( *this, "list", wxListCtrl );
	
	SetSizer( sizer );	
	Layout();

	m_list->InsertColumn( LIST_ANIM_NAME, wxT("Animation") );

	FillList();
}

wxAuiPaneInfo CEdBehaviorGraphAnimationLister::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Floatable( true ).Float().MinSize( GetMinSize() ).Dockable( false );

	return info;
}

void CEdBehaviorGraphAnimationLister::OnListAnims( wxCommandEvent& event )
{
	FillList();
}

void CEdBehaviorGraphAnimationLister::OnCopyToClipboard( wxCommandEvent& event )
{
	String allAnims;
	int itemCount = m_list->GetItemCount();
	for ( int idx = 0; idx < itemCount; ++ idx )
	{
		if ( ! allAnims.Empty() )
		{
			allAnims += TXT("\n");
		}
		allAnims += m_list->GetItemText( idx, LIST_ANIM_NAME ).wc_str();
	}
	if ( GClipboard )
	{
		GClipboard->Copy(allAnims);
	}
}

void CEdBehaviorGraphAnimationLister::FillList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();

	if ( CBehaviorGraph* graph = GetBehaviorGraph() )
	{
		TDynArray< CBehaviorGraphNode* > nodes;
		graph->GetAllNodes(nodes);

		TDynArray< CName > usedAnims;
		for ( auto iNode = nodes.Begin(); iNode != nodes.End(); ++ iNode )
		{
			TDynArray< CName > anims;
			(*iNode)->CollectUsedAnimations(anims);
			for ( auto iAnim = anims.Begin(); iAnim != anims.End(); ++ iAnim )
			{
				usedAnims.PushBackUnique( *iAnim );
			}
		}

		Int32 idx = 0;
		for ( auto iAnim = usedAnims.Begin(); iAnim != usedAnims.End(); ++ iAnim, ++ idx )
		{
			m_list->InsertItem( idx, iAnim->AsString().AsChar() );
			m_list->SetItem( idx, LIST_ANIM_NAME, iAnim->AsString().AsChar() );
		}
	}

	m_list->SetColumnWidth( LIST_ANIM_NAME, wxLIST_AUTOSIZE );

	m_list->Thaw();
}

void CEdBehaviorGraphAnimationLister::OnReset()
{
	Clear();
	FillList();
}

void CEdBehaviorGraphAnimationLister::OnInstanceReload()
{
	Clear();
	FillList();
}

void CEdBehaviorGraphAnimationLister::OnPanelClose()
{
	Clear();
}

void CEdBehaviorGraphAnimationLister::Clear()
{
	m_list->DeleteAllItems();
}


