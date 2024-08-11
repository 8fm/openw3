
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

//RED_DEFINE_STATIC_NAME( UpdateSceneTree )
//RED_DEFINE_STATIC_NAME( ActiveWorldChanged )
//RED_DEFINE_STATIC_NAME( ShowReloadFilesDialog )

#include "reloadDialog.h"
#include "../../common/core/garbageCollector.h"

BEGIN_EVENT_TABLE( CEdFileReloadDialog, wxDialog )
	EVT_BUTTON( XRCID("Reload"),	CEdFileReloadDialog::OnReload )
	EVT_BUTTON( XRCID("Skip"),		CEdFileReloadDialog::OnSkip )
	EVT_COLLAPSIBLEPANE_CHANGED( wxID_ANY, CEdFileReloadDialog::OnPaneChanged )
END_EVENT_TABLE()

CEdFileReloadDialog::CEdFileReloadDialog()
	: m_ready(false)
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, NULL, TXT("FilesReloadDialog") );

	// Create collapsible panel
	{
		wxPanel* p = XRCCTRL( *this, "PanelChoose", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_collPane = new wxCollapsiblePane(p, wxID_ANY, wxT("Show files..."), wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE|wxNO_BORDER);
		wxWindow *win = m_collPane->GetPane();

		sizer->Add( m_collPane, 0, wxVERTICAL, 5);

		wxStaticText* textListEditor = new wxStaticText(win, wxID_ANY, wxT("Files in editors:"), wxPoint(10,10));
		m_listEditor = new wxListCtrl(win, wxID_ANY, wxPoint(10,30), wxSize(540,150), wxLC_REPORT);
		m_listEditor->InsertColumn(0, wxT("File"));
		m_listEditor->InsertColumn(1, wxT("Editor"));
		m_listEditor->SetColumnWidth(0, 230);
		m_listEditor->SetColumnWidth(1, 300);

		wxStaticText* textListAll = new wxStaticText(win, wxID_ANY, wxT("All files:"), wxPoint(10,200));
		m_listAll = new wxListCtrl(win, wxID_ANY, wxPoint(10,220), wxSize(540,150), wxLC_REPORT);
		m_listAll->InsertColumn(0, wxT("File"));
		m_listAll->SetColumnWidth(0, 530);

		p->SetSizer( sizer );
		p->Layout();

		// Fit coll panel
		wxCollapsiblePaneEvent fake;
		OnPaneChanged(fake);
	}
}

void CEdFileReloadDialog::OnPaneChanged( wxCollapsiblePaneEvent& event )
{
	wxWindow *win = m_collPane->GetPane();
	win->SetSize(wxSize(590,500));
	Fit();
	Layout();
}

void CEdFileReloadDialog::DoModal()
{
	if (m_ready)
	{
		ShowModal();
	}

	m_listEditor->DeleteAllItems();
	m_listAll->DeleteAllItems();

	m_allResources.Clear();

	if(!m_collPane->IsCollapsed())
	{
		m_collPane->Collapse(true);
		wxCollapsiblePaneEvent fake;
		OnPaneChanged(fake);
	}
	
	m_ready = false;
}

void CEdFileReloadDialog::AddResourceToReload(CResource* res)
{
	ASSERT(res);

	if (m_allResources.Find(res) == m_allResources.End())
	{
		Int32 pos = m_listAll->GetItemCount();
		wxString label = res->GetFile()->GetDepotPath().AsChar();
		m_listAll->InsertItem(pos, label);
	}

	m_allResources.Insert(res);

	if ( !m_ready )
	{
		m_ready = true;
		const Bool delayFlag = true;
		SEvents::GetInstance().QueueEvent( CNAME( ShowReloadFilesDialog ), CreateEventData( delayFlag ) );
	}
}

void CEdFileReloadDialog::AddResourceToReloadFromEditor( const CReloadFileInfo& info )
{
	CDiskFile* file = info.m_oldResource->GetFile();

	ASSERT(file && file->GetResource());
	
	String editorName = info.m_editor;
	Int32 pos = m_listEditor->GetItemCount();
	wxString label = file->GetDepotPath().AsChar();
	long temp = m_listEditor->InsertItem(pos, label, 0);
	m_listEditor->SetItemData(temp, pos);
	m_listEditor->SetItem(temp, 1, editorName.AsChar());

	if ( !m_ready )
	{
		m_ready = true;
		const Bool delayFlag = false;
		SEvents::GetInstance().QueueEvent( CNAME( ShowReloadFilesDialog ), CreateEventData( delayFlag ) );
	}
}

void CEdFileReloadDialog::OnReload( wxCommandEvent& event )
{
	CWorld* worldToReload = NULL;

	if (GGame && GGame->GetActiveWorld())
	{
		CWorld* world = GGame->GetActiveWorld();

		if (m_allResources.Find(world) != m_allResources.End())
		{
			worldToReload = world;
			worldToReload->SynchronizeLayersRemove(worldToReload->GetWorldLayers());
		}
	}

	for ( TSortedSet<CResource*, CResource::CompareFunc >::iterator it = m_allResources.Begin(); it != m_allResources.End(); ++it )
	{
		(*it)->Reload(false);
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( world ) 
	{
		world->DelayedActions();
		SEvents::GetInstance().DispatchEvent(CNAME( ActiveWorldChanged ), CreateEventData( world ) );
	}

	SEvents::GetInstance().DispatchEvent(CNAME( UpdateSceneTree ), NULL);

	SGarbageCollector::GetInstance().CollectNow();

	EndDialog(1);
}

void CEdFileReloadDialog::OnSkip( wxCommandEvent& event )
{
	EndDialog(0);
}
