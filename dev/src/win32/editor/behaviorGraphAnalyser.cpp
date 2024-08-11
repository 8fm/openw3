/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorGraphAnalyser.h"
#include "../../common/engine/behaviorGraphContext.h"


BEGIN_EVENT_TABLE( CEdBehaviorInstanceProfiler, CEdBehaviorEditorSimplePanel )
	EVT_TOGGLEBUTTON( XRCID( "buttConnect" ), CEdBehaviorInstanceProfiler::OnConnect )
END_EVENT_TABLE()

CEdBehaviorInstanceProfiler::CEdBehaviorInstanceProfiler( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_connect( false )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorGraphDataPanel") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );
	
	SetSizer( sizer );	
	Layout();
}

wxAuiPaneInfo CEdBehaviorInstanceProfiler::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Dockable( false ).Floatable( true ).Float().MinSize( GetMinSize() );

	return info;
}

void CEdBehaviorInstanceProfiler::OnConnect( wxCommandEvent& event )
{
	Connect( event.IsChecked() );
}

void CEdBehaviorInstanceProfiler::Connect( Bool flag )
{
	m_connect = flag;
	RefreshPanel();
}

void CEdBehaviorInstanceProfiler::OnReset()
{
	CBehaviorGraph* graph = GetBehaviorGraph();
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	TDynArray< CBehaviorGraphNode* > nodes;
	graph->GetAllNodes( nodes );

	{
		wxTextCtrl* edit = XRCCTRL( *this, "tInstanceName", wxTextCtrl );
		edit->SetLabel( instance->GetInstanceName().AsString().AsChar() );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tNodeNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), nodes.Size() );
		edit->SetLabel( str );
	}
	
	RefreshPanel();
}

void CEdBehaviorInstanceProfiler::OnInstanceReload()
{

}

void CEdBehaviorInstanceProfiler::OnTick( Float dt )
{
	if ( m_connect )
	{
		RefreshPanel();
	}
}

void CEdBehaviorInstanceProfiler::RefreshPanel()
{
	CBehaviorGraph* graph = GetBehaviorGraph();
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	const SBehaviorSampleContext* sampleContext = instance->GetAnimatedComponent()->GetBehaviorGraphSampleContext();
	if ( !sampleContext )
	{
		return;
	}

	TDynArray< CBehaviorGraphNode* > nodes;
	graph->GetAllNodes( nodes );

	//------------------------------------------------------------
	{
		Uint32 aNum = 0;
		for ( Uint32 i=0; i<nodes.Size(); i++ )
		{
			if ( nodes[i]->IsActive( *instance ) )
			{
				aNum++;
			}
		}

		wxTextCtrl* edit = XRCCTRL( *this, "tActiveNodeNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), aNum );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tPoseNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tPoseNumCurr", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tCachedPoseNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tCachedPoseNumCurr", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tMimicPoseNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tMimicPoseNumCurr", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tConNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), 0 );
		edit->SetLabel( str );
	}
	//------------------------------------------------------------
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tProEvents", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), instance->GetEventProcessedNum() );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tActivNot", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), instance->GetActivationNotificationNum() );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tDeactivNot", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), instance->GetDeactivationNotificationNum() );
		edit->SetLabel( str );
	}
	{
		wxTextCtrl* edit = XRCCTRL( *this, "tAnimEventsNum", wxTextCtrl );
		wxString str = wxString::Format( wxT("%d"), sampleContext->GetAnimEventsNum() );
		edit->SetLabel( str );
	}
}

//////////////////////////////////////////////////////////////////////////
