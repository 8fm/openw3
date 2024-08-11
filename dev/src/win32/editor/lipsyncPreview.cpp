
#include "build.h"
#include "lipsyncPreview.h"
#include "../../common/game/actor.h"
#include "../../common/game/actorSpeech.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/viewport.h"

CGatheredResource resHead_1( TXT("characters\\templates\\witcher\\witcher.w2ent"), 0 );
CGatheredResource resHead_2( TXT("templates\\characters\\appearances\\main npc\\triss\\triss.w2ent"), 0 );
CGatheredResource resHead_3( TXT("templates\\characters\\appearances\\main npc\\filippa\\filippa.w2ent"), 0 );
CGatheredResource resHead_4( TXT("templates\\characters\\appearances\\main npc\\roche\\roche.w2ent"), 0 );
CGatheredResource resHead_5( TXT("templates\\characters\\appearances\\main npc\\dandelion\\dandelion.w2ent"), 0 );
CGatheredResource resHead_6( TXT("templates\\characters\\appearances\\main npc\\zoltan\\zoltan.w2ent"), 0 );
CGatheredResource resHead_7( TXT("templates\\characters\\appearances\\crowd\\citizen_rich\\citizen_rich.w2ent"), 0 );
CGatheredResource resHead_8( TXT("templates\\characters\\appearances\\crowd\\temeria_knight\\temeria_knight.w2ent"), 0 );

#define	ID_LS_LOAD_ENTITY_SEL	3001
#define	ID_LS_LOAD_ENTITY_1		3002
#define	ID_LS_LOAD_ENTITY_2		3003
#define	ID_LS_LOAD_ENTITY_3		3004
#define	ID_LS_LOAD_ENTITY_4		3005
#define	ID_LS_LOAD_ENTITY_5		3006
#define	ID_LS_LOAD_ENTITY_6		3007
#define	ID_LS_LOAD_ENTITY_7		3008
#define	ID_LS_LOAD_ENTITY_8		3009

BEGIN_EVENT_TABLE( CEdLipsyncPreview, wxFrame )
	//EVT_MENU( XRCID( "first" ), CEdLipsyncPreview::OnStartPose )
	//EVT_MENU( XRCID( "prev"), CEdLipsyncPreview::OnPrevPose )	
	//EVT_MENU( XRCID( "next" ), CEdLipsyncPreview::OnNextPose )
	//EVT_MENU( XRCID( "toolSkeleton" ), CEdLipsyncPreview::OnShowSkeleton )
	EVT_BUTTON( XRCID( "btnSelect" ), CEdLipsyncPreview::OnManualSelect )
	END_EVENT_TABLE()

	CEdLipsyncPreview::CEdLipsyncPreview( wxWindow* parent )
	: m_previewEntity( NULL )
	, m_list( NULL )
{
	// Load window
	wxXmlResource::Get()->LoadFrame( this, parent, wxT( "LipsyncPreview" ) );

	// Set window parameters
	SetTitle( wxT( "Lipsync Preview" ) );
	SetSize( 800, 600 );

	{
		// Create preview
		wxPanel* rp = XRCCTRL( *this, "previewPanel", wxPanel );
		ASSERT( rp != NULL );
		m_preview = new CEdAnimBrowserPreview( rp, NULL, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		m_preview->SetCameraPosition( Vector( 0.f, 0.5f, 1.7f ) );
		m_preview->SetCameraRotation( EulerAngles( 0, -5, 180 ) );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
	}

	{
		// Tree & list
		wxTreeCtrl* tree = XRCCTRL( *this, "playTree", wxTreeCtrl );
		wxListBox* list = XRCCTRL( *this, "playList", wxListBox );

		m_list = new CEdLipsyncPreviewPlaylist( tree, list );
	}

	{
		// Create preview popup menu
		wxMenu* previewMenu = new wxMenu();

		previewMenu->Append( ID_LS_LOAD_ENTITY_SEL, TXT("Use selected entity") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_SEL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadSelEntity ), NULL, this ); 

		previewMenu->AppendSeparator();

		previewMenu->Append( ID_LS_LOAD_ENTITY_1, TXT("Geralt") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_2, TXT("Tiss") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_3, TXT("Filippa") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_4, TXT("Roche") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_5, TXT("Dandelion") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_6, TXT("Zoltan") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_6, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_7, TXT("Citizen") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_7, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this ); 

		previewMenu->Append( ID_LS_LOAD_ENTITY_8, TXT("Knight") );
		previewMenu->Connect( ID_LS_LOAD_ENTITY_8, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdLipsyncPreview::OnLoadEntity ), NULL, this );

		m_preview->SetContextMenu( previewMenu );
	}

	if ( m_previewEntity == NULL )
	{
		LoadEntity( resHead_1.GetPath().ToString() );
	}
}

CEdLipsyncPreview::~CEdLipsyncPreview()
{
	ClearList();

	delete m_list;
	m_list = NULL;

	UnloadEntity();

	//SaveOptionsToConfig();
}

void CEdLipsyncPreview::Tick( Float timeDelta )
{
	if ( m_list && m_previewEntity )
	{
		CActor* actor = Cast< CActor >( m_previewEntity );
		if ( actor )
		{
			m_list->Update( actor );
		}
	}
}

void CEdLipsyncPreview::LoadEntity( const String &entName )
{
	/*
	// destroy previous entity
	UnloadEntity();

	// Load entity template
	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( entName );
	if ( entityTemplate == NULL )
	{
		ERR_EDITOR( TXT( "Failed to load %s entity" ), entName.AsChar() );
		return;
	}

	// Spawn preview actor
	EntitySpawnInfo einfo;
	einfo.m_template = entityTemplate;
	einfo.m_name = TXT( "PreviewEntity" );
	einfo.m_previewOnly = true;

	m_previewEntity = m_preview->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	if ( m_previewEntity == NULL )
	{
		return;
	}

	CActor* actor = Cast< CActor >( m_previewEntity );
	if ( !actor )
	{
		return;
	}

	// Select animated component
	CHeadComponent* headComponent = actor->GetHeadComponent();
	if ( !headComponent )
	{
		return;
	}

	// Disable motion extraction
	headComponent->SetUseExtractedMotion( false );
	headComponent->SetUseExtractedTrajectory( true );

	// Tick world to allow behaviours process
	CWorldTickInfo info( m_preview->GetPreviewWorld(), 0.1f );
	m_preview->GetPreviewWorld()->Tick( info );

	// Disable ragdoll
	headComponent->EnableRagdoll( false );

	// Update preview
	m_preview->OnLoadEntity( m_previewEntity );
	m_preview->SetAnimatedComponent( headComponent );
	m_preview->SetPlayedAnimation( NULL );
	*/
}

void CEdLipsyncPreview::UnloadEntity()
{
	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Unload entity from world
	m_preview->GetPreviewWorld()->DelayedActions();
	m_previewEntity->Destroy();
	m_preview->GetPreviewWorld()->DelayedActions();

	m_previewEntity = NULL;

	m_preview->OnUnloadEntity();
	m_preview->SetAnimatedComponent( NULL );
	m_preview->SetPlayedAnimation( NULL );
}

void CEdLipsyncPreview::ClearList()
{
	m_list->Clear();
}

void CEdLipsyncPreview::FillListManual()
{
	ClearList();

	CActor* actor = Cast< CActor >( m_previewEntity );
	if ( !actor )
	{
		return;
	}

	CEdFileDialog dlg;
	dlg.AddFormat( TXT( "wav" ), TXT( "Wav file(s)" ) );
	dlg.SetMultiselection( true );
	dlg.SetIniTag( TXT("CEdLipsyncPreview") );

	if ( !dlg.DoOpen( (HWND) GetHandle(), true ) ) 
	{
		return;
	}

	const TDynArray< String >& files = dlg.GetFiles();
	m_list->Fill( actor, files );
}

void CEdLipsyncPreview::FillListCsv()
{
	ClearList();


}

//////////////////////////////////////////////////////////////////////////

void CEdLipsyncPreview::OnLoadSelEntity( wxCommandEvent& event )
{
	String selectedResource;

	if ( GetActiveResource( selectedResource ) )
	{
		LoadEntity( selectedResource );
	}
}

void CEdLipsyncPreview::OnLoadEntity( wxCommandEvent& event )
{
	switch ( event.GetId() )
	{
	case ID_LS_LOAD_ENTITY_1:
		LoadEntity( resHead_1.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_2:
		LoadEntity( resHead_2.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_3:
		LoadEntity( resHead_3.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_4:
		LoadEntity( resHead_4.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_5:
		LoadEntity( resHead_5.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_6:
		LoadEntity( resHead_7.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_7:
		LoadEntity( resHead_7.GetPath().ToString() );
		return;
	case ID_LS_LOAD_ENTITY_8:
		LoadEntity( resHead_8.GetPath().ToString() );
		return;
	}
}

void CEdLipsyncPreview::OnManualSelect( wxCommandEvent& event )
{
	FillListManual();
}

//////////////////////////////////////////////////////////////////////////

CEdLipsyncPreviewPlaylist::CEdLipsyncPreviewPlaylist( wxTreeCtrl* tree, wxListBox* list )
	: m_tree( tree )
	, m_list( list )
	, m_latentAction( LA_None )
{
	SEL_COLOR = wxColor( 255, 128, 128 );

	m_currItem = wxTreeItemId();
}

CEdLipsyncPreviewPlaylist::~CEdLipsyncPreviewPlaylist()
{

}

wxTreeItemId CEdLipsyncPreviewPlaylist::FindTreeItemByActor( const String& actor ) const
{
	wxTreeItemIdValue cookie;
	for ( wxTreeItemId cur = m_tree->GetFirstChild( m_tree->GetRootItem(), cookie ); cur.IsOk(); cur = m_tree->GetNextChild( cur, cookie ) )
	{
		wxTreeItemData* data = m_tree->GetItemData( cur );
		if ( data )
		{
			TTreeItemDataWrapper< String >* lData = static_cast< TTreeItemDataWrapper< String >* >( data );
			if ( lData->GetData() == actor )
			{
				return cur;
			}
		}
	}

	return m_tree->AppendItem( m_tree->GetRootItem(), actor.AsChar(), -1, -1, new TTreeItemDataWrapper< String >( actor ) );
}

void CEdLipsyncPreviewPlaylist::Fill( CActor* actor, const TDynArray< String >& fileList )
{
	m_tree->Freeze();
	m_list->Freeze();

	m_tree->AddRoot( wxT("Lipsync") );
	DEFAULT_COLOR = m_tree->GetItemBackgroundColour( m_tree->GetRootItem() );

	for ( Uint32 i=0; i<fileList.Size(); ++i )
	{
		const String& absFile = fileList[ i ];
		String actor = absFile.StringAfter( TXT("VO_") ).StringBefore( TXT("_") );
		String name = absFile.StringAfter( TXT("\\"), true ).StringBefore( TXT(".") );

		wxTreeItemId treeItem = FindTreeItemByActor( actor );

		m_tree->AppendItem( treeItem, name.AsChar(), -1, -1, new CEdLipsyncPreviewPlaylistItem( actor, absFile ) );
	}

	m_tree->Thaw();
	m_list->Thaw();

	Reset( actor );
}

void CEdLipsyncPreviewPlaylist::Clear()
{
	m_tree->Freeze();
	m_list->Freeze();

	m_tree->DeleteAllItems();
	m_list->Clear();

	m_tree->Thaw();
	m_list->Thaw();
}

void CEdLipsyncPreviewPlaylist::Update( CActor* actor )
{
	if ( m_paused )
	{
		return;
	}

	if ( m_latentAction != LA_None )
	{
		switch ( m_latentAction )
		{
		case LA_Next:
			if ( FindNextItem() )
			{
				PlayItem( actor );
			}
			return;
		case LA_Prev:
			if ( FindPrevItem() )
			{
				PlayItem( actor );
			}
			return;
			//...
		}
	}

	if ( !actor->IsSpeaking() )
	{
		if ( FindNextItem() )
		{
			PlayItem( actor );
		}
	}
}

void CEdLipsyncPreviewPlaylist::Play( CActor* actor )
{
	ASSERT( m_paused );
	m_paused = false;

	actor->FreezeAllAnimatedComponents();
}

void CEdLipsyncPreviewPlaylist::Pause( CActor* actor )
{
	ASSERT( !m_paused );
	m_paused = true;

	actor->UnfreezeAllAnimatedComponents();
}

Bool CEdLipsyncPreviewPlaylist::IsPaused() const
{
	return m_paused;
}

void CEdLipsyncPreviewPlaylist::Reset( CActor* actor )
{
	if ( !m_currItem.IsOk() )
	{
		DeselectCurrItem();

		wxTreeItemIdValue cookie;
		wxTreeItemId currRoot = m_tree->GetFirstChild( m_tree->GetRootItem(), cookie );
		if ( currRoot.IsOk() )
		{
			wxTreeItemIdValue cookie2;
			m_currItem = m_tree->GetFirstChild( currRoot, cookie2 );

			if ( m_currItem.IsOk() )
			{
				PlayItem( actor );
			}
		}

		SelectCurrItem();
	}
}

Bool CEdLipsyncPreviewPlaylist::FindNextItem()
{
	if ( !m_currItem.IsOk() )
	{
		return false;
	}

	DeselectCurrItem();

	wxTreeItemId next = m_tree->GetNextSibling( m_currItem );
	if ( !next.IsOk() )
	{
		wxTreeItemId parent = m_tree->GetItemParent( m_currItem );
		if ( parent.IsOk() )
		{
			parent = m_tree->GetNextSibling( parent );
			if ( !parent.IsOk() )
			{
				wxTreeItemIdValue cookie;
				parent = m_tree->GetFirstChild( m_tree->GetRootItem(), cookie );
			}

			ASSERT( parent.IsOk() );

			if ( parent.IsOk() )
			{
				wxTreeItemIdValue cookie2;
				m_currItem = m_tree->GetFirstChild( parent, cookie2 );
				ASSERT( m_currItem.IsOk() );

				SelectCurrItem();

				return m_currItem.IsOk();
			}
		}
		else
		{
			ASSERT( parent.IsOk() );

			return false;
		}
	}

	m_currItem = next;

	SelectCurrItem();

	return true;
}

Bool CEdLipsyncPreviewPlaylist::FindPrevItem()
{
	return false;
}

void CEdLipsyncPreviewPlaylist::Next()
{
	m_latentAction = LA_Next;
}

void CEdLipsyncPreviewPlaylist::Prev()
{
	m_latentAction = LA_Prev;
}

void CEdLipsyncPreviewPlaylist::Select()
{

}

void CEdLipsyncPreviewPlaylist::PlayItem( CActor* actor )
{
	if ( m_currItem.IsOk() )
	{
		wxTreeItemData* data = m_tree->GetItemData( m_currItem );
		if ( data )
		{
			CEdLipsyncPreviewPlaylistItem* lData = static_cast< CEdLipsyncPreviewPlaylistItem* >( data );

			//...

			//Int32 flags = ASM_Voice | ASM_Lipsync;
			//actor->SpeakLine( 46600, "XXX", true, flags );
		}
	}
}

void CEdLipsyncPreviewPlaylist::SelectCurrItem()
{
	if ( m_currItem.IsOk() )
	{
		m_tree->SetItemBackgroundColour( m_currItem, SEL_COLOR );
	}
}

void CEdLipsyncPreviewPlaylist::DeselectCurrItem()
{
	if ( m_currItem.IsOk() )
	{
		m_tree->SetItemBackgroundColour( m_currItem, DEFAULT_COLOR );
	}
}
