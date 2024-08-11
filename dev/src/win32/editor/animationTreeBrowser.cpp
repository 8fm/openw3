
#include "build.h"
#include "animationTreeBrowser.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/viewport.h"

BEGIN_EVENT_TABLE( CEdAnimationTreeBrowser, wxPanel )
	EVT_TREE_SEL_CHANGED( XRCID( "tree" ), CEdAnimationTreeBrowser::OnTreeSelectionChanged )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "tree" ), CEdAnimationTreeBrowser::OnTreeItemActivated )
	EVT_TREE_BEGIN_DRAG( XRCID( "tree" ), CEdAnimationTreeBrowser::OnTreeItemBeginDrag )
END_EVENT_TABLE()

CEdAnimationTreeBrowser::CEdAnimationTreeBrowser( wxWindow* parent, const SEdAnimationTreeBrowserSettings& settings )
	: wxPanel( parent )
	, m_filter( settings.m_filter )
	, m_supportsDragAndDrop( settings.m_supportsDragAndDrop )
	, m_oneClickSelect( settings.m_oneClickSelect )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("AnimationTreeBrowser") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_tree = XRCCTRL( *this, "tree", wxTreeCtrl );

	if ( !settings.m_verticalStyle )
	{
		SetBrowserStyle( false );
	}

	{
		wxPanel* rp = XRCCTRL( *this, "previewPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		m_preview = new CEdAnimationPreview( rp, false );
		
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );

		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	{
		wxPanel* rp = XRCCTRL( *this, "propPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		PropertiesPageSettings settings;
		m_prop = new CEdPropertiesPage( rp, settings, nullptr );

		sizer1->Add( m_prop, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	m_propSetter = new CEdAnimationTreeBrowserPropSetter();

	SetSizer( sizer );	
	Layout();
}

CEdAnimationTreeBrowser::~CEdAnimationTreeBrowser()
{
	delete m_filter;
	delete m_propSetter;
}

void CEdAnimationTreeBrowser::LoadEntity( const String& fileName, const String& component )
{
	m_preview->LoadEntity( fileName, component );

	FillAnimationTree( m_preview->GetAnimatedComponent() );
}

void CEdAnimationTreeBrowser::UnloadEntity()
{
	m_preview->UnloadEntity();

	FillAnimationTree( m_preview->GetAnimatedComponent() );
}

void CEdAnimationTreeBrowser::CloneAndUseAnimatedComponent( const CAnimatedComponent* animatedComponent )
{
	if( GGame->IsActive() ) 
	{
		return;
	}

	m_preview->CloneAndUseAnimatedComponent( animatedComponent );

	FillAnimationTree( animatedComponent );
}

void CEdAnimationTreeBrowser::SetFilter( CEdAnimationTreeBrowserAnimFilter* filter )
{
	if ( m_filter )
	{
		delete m_filter;
	}

	m_filter = filter;

	if ( m_preview->GetAnimatedComponent() )
	{
		FillAnimationTree( m_preview->GetAnimatedComponent() );
	}
}

void CEdAnimationTreeBrowser::SetPropSetter( CEdAnimationTreeBrowserPropSetter* setter )
{
	if ( m_propSetter )
	{
		delete m_propSetter;
		m_propSetter = NULL;
	}

	m_propSetter = setter;
}

void CEdAnimationTreeBrowser::AddPreviewPostprocess( CEdAnimationPreviewPostProcess* postprocess )
{
	m_preview->AddPostProcess( postprocess );
}

void CEdAnimationTreeBrowser::SetBrowserStyle( Bool vertical )
{
	wxSplitterWindow* spliter = XRCCTRL( *this, "splitter", wxSplitterWindow );
	
	if ( vertical )
	{
		spliter->SetSplitMode( wxSPLIT_HORIZONTAL );
	}
	else
	{
		spliter->SetSplitMode( wxSPLIT_VERTICAL );
	}
}

void CEdAnimationTreeBrowser::SetAnimationGraphEnabled( Bool state )
{
	m_preview->SetAnimationGraphEnabled( state );
}

void CEdAnimationTreeBrowser::CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const
{
	TSkeletalAnimationSetsArray animSets;

	CAnimatedComponent* animatedComponent = m_preview->GetAnimatedComponent();

	if ( animatedComponent && animatedComponent->GetAnimationContainer() )
	{
		animSets = animatedComponent->GetAnimationContainer()->GetAnimationSets();
	}

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();
		if ( !set )
		{
			continue;
		}

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		set->GetAnimations( setAnims );

		for( Uint32 j=0; j<setAnims.Size(); ++j )
		{
			const CSkeletalAnimationSetEntry* currAnim = setAnims[ j ];
			if ( currAnim->GetAnimation() )
			{
				if ( m_filter && !m_filter->Test( currAnim ) )
				{
					continue;
				}

				anims.PushBack( currAnim );
			}
		}
	}
}

void CEdAnimationTreeBrowser::FillAnimationTree( const CAnimatedComponent* animatedComponent )
{
	TSkeletalAnimationSetsArray animSets;

	if ( animatedComponent && animatedComponent->GetAnimationContainer() )
	{
		animSets = animatedComponent->GetAnimationContainer()->GetAnimationSets();
	}

	const CSkeletalAnimationSetEntry* selectedAnimation = NULL;
	wxTreeItemId selectedItem = m_tree->GetSelection();
	wxTreeItemId itemToSelect;
	ASSERT( !itemToSelect.IsOk() );

	if ( selectedItem.IsOk() )
	{
		SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( selectedItem );
		if ( animData )
		{
			selectedAnimation = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		}
	}

	m_tree->Freeze();
	m_tree->DeleteAllItems();
	m_tree->AddRoot( TXT("Animations") );

	for( auto it = animSets.Begin(), end = animSets.End(); it != end; ++it )
	{
		CSkeletalAnimationSet* set = ( *it ).Get();
		if ( !set )
		{
			continue;
		}

		String setName = set->GetFile() ? set->GetFile()->GetDepotPath() : TXT("DUPA");

		Bool parentAdded = false;
		wxTreeItemId currSetItem;

		TDynArray< CSkeletalAnimationSetEntry* > setAnims;
		set->GetAnimations( setAnims );

		for( Uint32 j=0; j<setAnims.Size(); ++j )
		{
			CSkeletalAnimationSetEntry* currAnim = setAnims[ j ];
			if ( currAnim->GetAnimation() )
			{
				if ( m_filter && !m_filter->Test( currAnim ) )
				{
					continue;
				}

				wxString itemName( currAnim->GetAnimation()->GetName().AsString().AsChar() );

				if ( !parentAdded )
				{
					currSetItem = m_tree->AppendItem( m_tree->GetRootItem(), setName.AsChar() );
					m_tree->SetItemData( currSetItem, new SerializableItemWrapper( set ) );
					
					OnItemAdded( currSetItem, set );

					parentAdded = true;
				}

				wxTreeItemId item = m_tree->AppendItem( currSetItem, itemName, -1, -1, new SerializableItemWrapper( currAnim ) );
				if ( selectedAnimation && selectedAnimation == currAnim )
				{
					itemToSelect = item;
				}

				OnItemAdded( item, currAnim );
			}

			m_tree->SortChildren( currSetItem );
		}
	}

	m_tree->SortChildren( m_tree->GetRootItem() );
	m_tree->Expand( m_tree->GetRootItem() );
	m_tree->Thaw();

	if ( itemToSelect.IsOk() )
	{
		m_tree->SelectItem( itemToSelect, true );
	}
}

void CEdAnimationTreeBrowser::Pause()
{
	m_preview->Pause( true );
}

void CEdAnimationTreeBrowser::Unpause()
{
	m_preview->Pause( false );
}

void CEdAnimationTreeBrowser::SelectAnimation( const CName& animation )
{
	if ( m_preview->GetAnimatedComponent() && m_preview->GetAnimatedComponent()->GetAnimationContainer() )
	{
		CSkeletalAnimationSetEntry* animEntry = m_preview->GetAnimatedComponent()->GetAnimationContainer()->FindAnimation( animation );
		SelectAnimation( animEntry );

		if ( animEntry )
		{
			wxTreeItemId treeItemId = FindAnimationTreeItem( animEntry );
			m_tree->SelectItem( treeItemId );
		}
	}
}

void CEdAnimationTreeBrowser::SelectAnimation( const wxTreeItemId& selectedId )
{
	SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( selectedId );
	if ( animData )
	{
		CSkeletalAnimationSetEntry* entry = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		if ( entry )
		{
			SelectAnimation( entry );
		}
	}
}

void CEdAnimationTreeBrowser::SelectAnimation( CSkeletalAnimationSetEntry* animation )
{
	m_preview->SetAnimation( animation );

	if ( m_propSetter )
	{
		m_propSetter->Set( animation, m_prop );
	}

	OnSelectAnimation( animation );
}

wxTreeItemId CEdAnimationTreeBrowser::FindAnimationTreeItem( CSkeletalAnimationSetEntry *anim )
{
	TQueue< wxTreeItemId > items;
	items.Push( m_tree->GetRootItem() );

	while( !items.Empty() )
	{
		wxTreeItemId& currItem = items.Front();
		items.Pop();

		SerializableItemWrapper* animData = (SerializableItemWrapper*)( m_tree->GetItemData( currItem ) );
		if ( animData && animData->m_object == anim )
		{
			return currItem;
		}

		wxTreeItemIdValue cookie = 0;
		wxTreeItemId child = m_tree->GetFirstChild( currItem, cookie );
		while( child.IsOk() )
		{
			items.Push( child );
			child = m_tree->GetNextChild( currItem, cookie );
		}
	}

	return wxTreeItemId();
}

void CEdAnimationTreeBrowser::OnTreeSelectionChanged( wxTreeEvent& event )
{
	if ( m_oneClickSelect )
	{
		wxTreeItemId selectedId = event.GetItem();
		SelectAnimation( selectedId );
	}
}

void CEdAnimationTreeBrowser::OnTreeItemActivated( wxTreeEvent& event )
{
	if ( m_oneClickSelect )
	{
		if ( m_preview->IsPaused() )
		{
			m_preview->Pause( false );
		}
		else
		{
			m_preview->Pause( true );
		}
	}
	else
	{
		wxTreeItemId selectedId = event.GetItem();
		SelectAnimation( selectedId );
	}
}

void CEdAnimationTreeBrowser::OnTreeItemBeginDrag( wxTreeEvent &event )
{
	if ( !m_supportsDragAndDrop )
	{
		event.Veto();
		return;
	}

	wxTreeItemId selectedId = event.GetItem();

	SerializableItemWrapper* animData = (SerializableItemWrapper*)m_tree->GetItemData( selectedId );
	if ( animData )
	{
		CSkeletalAnimationSetEntry* entry = Cast< CSkeletalAnimationSetEntry >( animData->m_object );
		if ( entry )
		{
			CSkeletalAnimationSet* set = entry->GetAnimSet();

			String drop = set->GetDepotPath();
			drop += TXT(";");
			drop += entry->GetAnimation()->GetName().AsString();

			wxString wxDrop = drop.AsChar();

			wxTextDataObject myData( wxDrop );

			wxDropSource dragSource( myData, this );
			wxDragResult result = dragSource.DoDragDrop( wxDrag_DefaultMove );

			event.Allow();
		}
	}

	event.Veto();
}
