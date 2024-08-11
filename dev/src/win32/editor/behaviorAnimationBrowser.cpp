
#include "build.h"
#include "behaviorAnimationBrowser.h"
#include "behaviorEditor.h"

BEGIN_EVENT_TABLE( CEdBehaviorAnimationBrowserPanel, CEdBehaviorEditorSimplePanel )
END_EVENT_TABLE()

CEdBehaviorAnimationBrowserPanel::CEdBehaviorAnimationBrowserPanel( CEdBehaviorEditor* editor, wxWindow* window )
	: CEdBehaviorEditorSimplePanel( editor, window )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	SEdAnimationTreeBrowserSettings s;
	m_browser = new CEdAnimationTreeBrowser( this, s );

	sizer->Add( m_browser, 1, wxEXPAND|wxALL, 0 );

	SetSizer( sizer );
	Layout();

	LoadEntityForPreview();
}

wxAuiPaneInfo CEdBehaviorAnimationBrowserPanel::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	//info.Floatable( true ).Float().MinSize( 200, 100 ).BestSize( 375, 300 ).Dockable( false );

	return info;
}

void CEdBehaviorAnimationBrowserPanel::OnLoadEntity()
{
	LoadEntityForPreview();
}

void CEdBehaviorAnimationBrowserPanel::LoadEntityForPreview()
{
	m_browser->CloneAndUseAnimatedComponent( GetEditor()->GetAnimatedComponent() );
}
