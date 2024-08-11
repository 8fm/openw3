#include "build.h"
#include "dropTarget.h"
#include "assetBrowser.h"
#include "animBrowser.h"

#include "../../common/core/depot.h"

// Drop target
CDropTarget::CDropTarget()
	: m_window( nullptr )
{
}

CDropTarget::CDropTarget( wxWindow *window )
	: m_window( window )
{
	// note: drop target is owned by the window
	m_window->SetDropTarget( new CDropTargetHelper( this ) );
}

void CDropTarget::SetDropTargetWindow( wxWindow *window )
{
	if ( m_window )
	{	// remove the old drop target
		m_window->SetDropTarget( nullptr );
	}

	m_window = window;

	if ( m_window )
	{
		m_window->SetDropTarget( new CDropTargetHelper( this ) );
	}
}

CDropTarget::~CDropTarget()
{
	if ( m_window )
	{
		m_window->SetDropTarget( nullptr );
	}
}

// Drop target helper
CDropTargetHelper::CDropTargetHelper( CDropTarget *owner )
: m_owner( owner )
, m_resourcesExtracted( false )
, m_action( wxDragNone )
{
}

CDropTargetHelper::~CDropTargetHelper()
{
}

wxTextDataObject* CDropTarget::GetDraggedDataObject()
{
    return static_cast< CDropTargetHelper* >( m_window->GetDropTarget() )->GetDataObject();
}

const TDynArray< CResource* > &CDropTarget::GetDraggedResources()
{
    return static_cast< CDropTargetHelper* >( m_window->GetDropTarget() )->GetResources();
}

wxDragResult CDropTarget::GetDraggedAction()
{
    return static_cast< CDropTargetHelper* >( m_window->GetDropTarget() )->GetResult();
}

wxTextDataObject* CDropTargetHelper::GetDataObject()
{
    if ( !GetData() )
        return NULL;

    return (wxTextDataObject *)m_dataObject;
}

wxDragResult CDropTargetHelper::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    m_resources.Clear();
    m_resourcesExtracted = false;

	m_owner->OnEnter(x,y);
	return def;
}

wxDragResult CDropTargetHelper::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    return m_action = m_owner->OnDragOver(x,y, def);
}

const TDynArray< CResource* > &CDropTargetHelper::GetResources()
{
    if (m_resourcesExtracted)
        return m_resources;

    m_resourcesExtracted = true;
    m_resources.Clear();

    wxTextDataObject *data = GetDataObject();
    
    String items( data->GetText() );
    String left, right;
    items.Split( TXT(":"), &left, &right );

    if( left == TXT("Resources") )
    {
        TDynArray< String > parts;
        right.Slice( parts, TXT( ";" ) );
        if( !parts.Size() )
            return m_resources;

        for( Uint32 i=0; i<parts.Size(); i++ )
        {
            String fileName = parts[ i ];
            if( fileName.Empty() )
                continue;
            CResource* r = GDepot->LoadResource( fileName );
            if( r )
                m_resources.PushBack( r );
        }
    }

    return m_resources;
}


void CDropTargetHelper::OnLeave()
{
	m_owner->OnLeave();
}

bool CDropTargetHelper::OnDropText( wxCoord x, wxCoord y, const wxString& data )
{
	String items( data );
	String left, right;
	items.Split( TXT(":"), &left, &right );

	if( left == TXT("Resources") )
	{
		TDynArray< String > parts;
		right.Slice( parts, TXT( ";" ) );
		if( !parts.Size() )
			return false;

		TDynArray< CResource* > res;

		for( Uint32 i=0; i<parts.Size(); i++ )
		{
			String fileName = parts[ i ];
			if( fileName.Empty() )
				continue;
			CResource* r = GDepot->LoadResource( fileName );
			if( r )
				res.PushBack( r );
		}

		if( res.Size() )
			return m_owner->OnDropResources( x, y, res );
		else
			return false;
	}

	return m_owner->OnDropText( x, y, items );
}

// Drop target for Depot Tree
Bool CDropTargetTreeDepot::OnDropResources(wxCoord x, wxCoord y, TDynArray<CResource*> &resources)
{
	// You can't drag layer - if you want to load layer you have to load layer info too
	// If you load layer alone it will be damaged
	bool isLayer = false;
	for (Int32 i=(Int32)resources.Size()-1; i>=0; i--)
	{
		if (resources[i]->IsA<CLayer>())
		{
			resources.Remove(resources[i]);
			isLayer = true;
		}
	}
	if (isLayer)
	{
		wxString msg = wxT("\nLayers file couldn't be copied.\n\n");
		wxMessageDialog dialog( 0, msg, wxT("Warning"), wxOK | wxICON_WARNING );
		dialog.ShowModal();
	}

	// Nothing to drag, exit
	if (resources.Empty()) return false;

	// Find directory
	wxTreeItemId item = m_owner->HitTest( wxPoint(x,y) );

	if (item.IsOk())
	{
		m_owner->SetItemDropHighlight(item, false);

		CDirectory*	dir = ( static_cast<wxTreeItemDataDir*>(m_owner->GetItemData(item)) )->GetDirectory();
		if ( !dir )
		{
			return false;
		}

		// All input resource files have the same directory
		CDirectory* oldDir = (resources[0])->GetFile()->GetDirectory();

		if (dir == oldDir)
		{
			return true;
		}

		// Confirm drag
		{
			wxString msg = wxT("Do you want to move files to ") + wxString::Format(_T("%s"), dir->GetName().AsChar() ) + wxT("?");

			wxMessageDialog dialog( 0, msg, wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			if ( dialog.ShowModal() == wxID_NO ) 
				return false;	
		}

		// Copy
		SVersionControlWrapper::GetInstance().MoveFiles(dir, resources);

		// Update
		oldDir->Repopulate();
		m_browser->UpdateResourceList();

		return true;
	}

	return false;
}

void CDropTargetTreeDepot::OnEnter(wxCoord x, wxCoord y) 
{
}

void CDropTargetTreeDepot::OnLeave()
{
	if (m_item.IsOk())
	{
		m_owner->SetItemDropHighlight(m_item, false);
	}
}

void CDropTargetTreeDepot::OnDragOver(wxCoord x, wxCoord y)
{
	wxPoint point(x,y);
	wxTreeItemId item = m_owner->HitTest( point );
	if (item.IsOk())
	{
		if (m_item.IsOk())
		{
			m_owner->SetItemDropHighlight(m_item, false);
		}
		m_item = item;
		m_owner->SetItemDropHighlight(m_item, true);
	}
}

// Drop target for Animation browser Notebook
Bool CDropTargetAnimBrowserNotebook::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
	TDynArray<CSkeletalAnimation*> anims;

	for (Uint32 i=0; i<resources.Size(); i++)
	{
		if( resources[i]->IsA< CSkeletalAnimation >() )
		{
			CSkeletalAnimation *skeletalAnim = ( CSkeletalAnimation* )resources[i];
			anims.PushBack(skeletalAnim);
		}
	}

	m_browser->AddAnimations(anims);

	return true;
}

void CDropTargetAnimBrowserNotebook::OnEnter(wxCoord x, wxCoord y)
{

}