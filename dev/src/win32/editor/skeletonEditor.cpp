/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skeletonEditor.h"
#include "shapesPreviewItem.h"
#include "../../common/core/curveData.h"

#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/worldTick.h"


CGatheredResource resDefaultSkeleton( TXT("gameplay\\globals\\sounds\\event_replace.csv"), 0 );

//////////////////////////////////////////////////////////////////////////

CEdSkeletonPreview::CEdSkeletonPreview( wxWindow* parent, 
									    ISkeletonPreviewControl* skeletonPreviewControl, 
										const CSkeleton* skeleton,
										Bool allowRenderOptionsChange )
	: CEdAnimationPreview( parent, false )
	, m_skeleton( skeleton )
	, m_skeletonPreviewControl( skeletonPreviewControl )
	, m_shapesContainer( GetWorld(), true )
{
	AddRotationWidgets();
	AddScaleWidgets();
	AddTranslationWidgets();

	m_widgetManager->SetWidgetSpace( RPWS_Global );

	if ( m_skeleton )
	{
		const Int32 size = m_skeleton->GetBonesNum();

		m_skeletonPoseWS.Resize( size );

		for ( Int32 i=0; i<size; ++i )
		{
			m_skeletonPoseWS[ i ] = m_skeleton->GetBoneMatrixMS( i );
		}
	}
}

CEdSkeletonPreview::~CEdSkeletonPreview()
{
}

void CEdSkeletonPreview::HandleContextMenu( Int32 x, Int32 y )
{
	// Assemble menu
	wxMenu menu;
	m_shapesContainer.AppendMenu( &menu, this );
	PopupMenu( &menu );
}

void CEdSkeletonPreview::OnViewportTick( IViewport* view, Float timeDelta )
{
	CEdAnimationPreview::OnViewportTick( view, timeDelta );
}

void CEdSkeletonPreview::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	if ( wxGetKeyState( (wxKeyCode)'R' ) )
	{
		for ( TDynArray< CHitProxyObject* >::const_iterator it = objects.Begin(); it != objects.End(); ++it )
		{
			m_shapesContainer.AddToEdit( (*it)->GetHitObject() );
		}
	}
	m_shapesContainer.HandleItemSelection( objects );
}

void CEdSkeletonPreview::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	CEdAnimationPreview::OnViewportCalculateCamera( view, camera );
}

void CEdSkeletonPreview::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	frame->AddDebugScreenText( view->GetWidth() / 2, view->GetHeight() / 2, TXT("TODO") );

	if ( m_skeletonPreviewControl && m_skeleton )
	{
		SkeletonRenderingUtils::DrawSkeleton( m_skeletonPoseWS, m_skeleton, Color( 255, 255, 255 ), frame, true, false, true );

		if ( m_skeletonPreviewControl->IsSkeleton() )
		{
			//...
		}
	}

	m_shapesContainer.OnGenerateFragments( view, frame );
}
Bool CEdSkeletonPreview::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	CEdAnimationPreview::OnViewportInput( view, key, action, data );

	if ( key == IK_Alt )
	{
		if ( action == IACT_Press )
		{
			m_widgetManager->SetWidgetMode( RPWM_Rotate );
			m_widgetManager->EnableWidgets( true );
			m_widgetManager->SetWidgetSpace( RPWS_Global );
		}
		else
		{
			m_widgetManager->SetWidgetMode( RPWM_Move );
			m_widgetManager->EnableWidgets( true );
			m_widgetManager->SetWidgetSpace( RPWS_Global );
		}
	}
	if ( key == IK_Space )
	{
		if ( action == IACT_Press )
		{
			m_widgetManager->SetWidgetMode( RPWM_Scale );
			m_widgetManager->EnableWidgets( true );
			m_widgetManager->SetWidgetSpace( RPWS_Local );
		}
		else
		{
			m_widgetManager->SetWidgetMode( RPWM_Move );
			m_widgetManager->EnableWidgets( true );
			m_widgetManager->SetWidgetSpace( RPWS_Global );
		}
	}
	return true;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdSkeletonEditor, wxSmartLayoutFrame )
END_EVENT_TABLE()

CEdSkeletonEditor::CEdSkeletonEditor( wxWindow* parent, CSkeleton* skeleton )
	: m_skeleton( skeleton )
	, m_preview( NULL )
	, m_previewEntity( NULL )
	, m_animComponent( NULL )
{
	m_skeleton->AddToRootSet();

	ASSERT( m_skeleton->GetFile() );

	// Load window
	wxXmlResource::Get()->LoadFrame( this, parent, wxT( "SkeletonEditor" ) );

	// Set window parameters
	SetTitle( String::Printf( TXT( "Skeleton Editor - %s" ),
		m_skeleton->GetFile()->GetDepotPath().AsChar() ).AsChar() );
	SetSize( 530, 410 );

	{
		// Create preview
		wxPanel* rp = XRCCTRL( *this, "previewPanel", wxPanel );
		ASSERT( rp != NULL );
		m_preview = new CEdSkeletonPreview( rp, this, m_skeleton );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_preview, 1, wxEXPAND | wxALL, 5 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		m_preview->SetCameraPosition( Vector( 0.f, 0.5f, 0.4f ) );
		m_preview->SetCameraRotation( EulerAngles( 0, -5, 180 ) );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
	}

	{
		// Create property panel
		wxPanel* panelProperties = XRCCTRL( *this, "propPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		m_propertiesPage = new CEdPropertiesPage( panelProperties, PropertiesPageSettings(), nullptr );
		sizer->Add( m_propertiesPage, 1, wxEXPAND | wxALL, 5 );
		panelProperties->SetSizer( sizer );
		panelProperties->Layout();

		m_propertiesPage->SetObject( m_skeleton );
	}

	// Create skeleton icons
	CreateSkeletonIcons();

	LoadOptionsFromConfig();

	if ( m_previewEntity == NULL )
	{
		LoadEntity( resDefaultSkeleton.GetPath().ToString() );
	}

	GetSkeletonToolbar()->ToggleTool( TOOL_SKELETON, true );

	FillTrees();
}

CEdSkeletonEditor::~CEdSkeletonEditor()
{
	UnloadEntity();

	SaveOptionsToConfig();

	m_skeleton->RemoveFromRootSet();
	m_skeleton = NULL;
}

void CEdSkeletonEditor::OnLoadEntity( wxCommandEvent& event )
{
	String selectedResource;

	if ( GetActiveResource( selectedResource ) )
	{
		LoadEntity( selectedResource );
	}
}

void CEdSkeletonEditor::LoadEntity( const String &entName )
{
	// destroy previous entity
	UnloadEntity();

	// Load entity template
	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate >( entName );
	if ( entityTemplate == NULL )
	{
		ERR_EDITOR( TXT( "Failed to load %s entity" ), entName.AsChar() );
		return;
	}

	// Spawn entity
	EntitySpawnInfo einfo;
	einfo.m_template = entityTemplate;
	einfo.m_name = TXT( "PreviewEntity" );

	m_previewEntity = m_preview->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Select animated component
	TDynArray< CAnimatedComponent* > animComponents;
	CollectEntityComponents( m_previewEntity, animComponents );

	if ( animComponents.Size() == 0 )
	{
		String msg = TXT( "No head componets in entity!" );
		WARN_EDITOR( TXT("%s"), msg.AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warinng"), wxOK | wxCENTRE | wxICON_WARNING );
		UnloadEntity();
		return;
	}
	else if ( animComponents.Size() == 1 )
	{
		m_animComponent = animComponents[0];
	}
	else
	{
		// Create anim comp name list
		TDynArray< String > animCopmsName;

		// Fill list
		for ( Uint32 i=0; i<animComponents.Size(); i++ )
		{
			animCopmsName.PushBack( animComponents[i]->GetName() );
		}

		const String selectComponent = 
			InputComboBox( NULL, TXT( "Select head component" ),
			TXT( "Entity has got more then one head component. Choose one." ),
			animCopmsName[0], animCopmsName);

		// Find select component
		for ( Uint32 i=0; i < animCopmsName.Size(); i++ )
		{
			if ( animCopmsName[i] == selectComponent )
			{
				m_animComponent = animComponents[i];
				break;
			}
		}
	}

	ASSERT( m_animComponent != NULL );

	// Disable motion extraction
	m_animComponent->SetUseExtractedMotion( false );
	m_animComponent->SetUseExtractedTrajectory( true );

	// Tick world to allow behaviours process
	CWorldTickInfo info( m_preview->GetPreviewWorld(), 0.1f );
	m_preview->GetPreviewWorld()->Tick( info );
}

void CEdSkeletonEditor::UnloadEntity()
{
	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Unload entity from world
	m_preview->GetPreviewWorld()->DelayedActions();
	m_previewEntity->Destroy();
	m_preview->GetPreviewWorld()->DelayedActions(); 
	m_animComponent = NULL;
	m_previewEntity = NULL;
}

void CEdSkeletonEditor::SaveOptionsToConfig()
{
	String identifier = TXT("/Frames/SkeletonEditor/");

	SaveLayout( identifier );

	identifier = identifier + m_skeleton->GetFile()->GetDepotPath();

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, identifier);

	if ( m_previewEntity )
	{
		// Entity
		CEntityTemplate* t = Cast< CEntityTemplate >( m_previewEntity->GetTemplate() );

		config.Write( TXT("Entity"), t->GetFile()->GetDepotPath() );
	}

	// Splitters
	wxSplitterWindow* split1 = XRCCTRL( *this, "splitOne", wxSplitterWindow );
	config.Write( TXT("split1"), split1->GetSashPosition() );
}

void CEdSkeletonEditor::LoadOptionsFromConfig()
{
	String identifier = TXT("/Frames/SkeletonEditor/");

	LoadLayout( identifier );

	identifier = identifier + m_skeleton->GetFile()->GetDepotPath();

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, identifier );

	String depotPath = config.Read( TXT("Entity"), String::EMPTY );
	if ( depotPath.Empty() == false )
	{
		LoadEntity( depotPath );
	}

	// Splitters
	wxSplitterWindow* split1 = XRCCTRL( *this, "splitOne", wxSplitterWindow );
	Int32 pos1 = config.Read( TXT("split1"), 300 );
	split1->SetSashPosition( pos1 );
}

wxToolBar* CEdSkeletonEditor::GetSkeletonToolbar()
{
	wxToolBar* toolbar = XRCCTRL( *this, "controls", wxToolBar );
	ASSERT( toolbar );
	return toolbar;
}

void CEdSkeletonEditor::FillTrees()
{
	wxTreeCtrl* treeBones = XRCCTRL( *this, "treeBones", wxTreeCtrl );
	wxTreeCtrl* treeTracks = XRCCTRL( *this, "treeTracks", wxTreeCtrl );

	FillSkeletonTrees( m_skeleton, treeBones, treeTracks, true );
}

void CEdSkeletonEditor::FillSkeletonTrees( CSkeleton* skeleton, wxTreeCtrl* treeBones, wxTreeCtrl* treeTracks, Bool showIndex )
{
	if ( treeBones )
	{
		treeBones->Freeze();
		treeBones->DeleteAllItems();
	}
	
	if ( treeTracks )
	{
		treeTracks->Freeze();
		treeTracks->DeleteAllItems();
	}

	//dex++: using generalized CSkeleton interface
	const Uint32 numBones = skeleton->GetBonesNum();
	if ( numBones > 0 )
	//dex-
	{
		// Bones
		if ( treeBones )
		{
			//dex++
			wxString itemName = skeleton->GetBoneNameAnsi(0);
			wxTreeItemId root = treeBones->AddRoot( itemName );
			FillBoneTree( skeleton, 0, root, treeBones, showIndex );
			//dex-
		}

		// Tracks
		if ( treeTracks )
		{
			//dex++
			FillTracksTree( skeleton, treeTracks, showIndex );
			//dex--
		}
	}

	if ( treeBones )
	{
		treeBones->ExpandAll();
		treeBones->Thaw();
	}

	if ( treeTracks )
	{
		treeTracks->ExpandAll();
		treeTracks->Thaw();
	}
}

//dex++
void CEdSkeletonEditor::FillBoneTree( CSkeleton* skeleton, Int32 parentIndex, wxTreeItemId& parent, wxTreeCtrl* tree, Bool showIndex )
{
	const Uint32 numBones = skeleton->GetBonesNum();
	for ( Uint32 i=0; i<numBones; i++ )
	{
		const Int32 boneParentIndex = skeleton->GetParentBoneIndex(i);
		if ( boneParentIndex == parentIndex )
		{
			wxString itemName = skeleton->GetBoneNameAnsi(i);
			
			if ( showIndex )
			{
				itemName += wxString::Format( wxT(" (%d)"), i );
			}

			wxTreeItemId item = tree->AppendItem( parent, itemName, 0 );

			if ( !tree->ItemHasChildren( parent ) ) 
			{
				tree->SetItemHasChildren( parent, true );
			}

			FillBoneTree( skeleton, i, item, tree, showIndex );
		}
	}
}
//dex--

//dex++
void CEdSkeletonEditor::FillTracksTree( CSkeleton* skeleton, wxTreeCtrl* tree, Bool showIndex )
{
	wxTreeItemId root = tree->AddRoot( wxT("Tracks") );

	const Uint32 numTracks = skeleton->GetTracksNum();
	for ( Uint32 i=0; i<numTracks; ++i )
	{
		wxString itemName = skeleton->GetTrackNameAnsi(i);

		if ( showIndex )
		{
			itemName += wxString::Format( wxT(" (%d)"), i );
		}

		tree->AppendItem( root, itemName );
	}
}
//dex--
