/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mimicFacesEditor.h"

/*
#include "skeletonEditor.h"

CGatheredResource resDefaultHead( TXT("gameplay\\globals\\sounds\\event_replace.csv"), 0 );

#define	ID_MIMIC_LOAD_ENTITY	3001

BEGIN_EVENT_TABLE( CEdMimicFacesEditor, wxFrame )
	EVT_MENU( XRCID( "first" ), CEdMimicFacesEditor::OnStartPose )
	EVT_MENU( XRCID( "prev"), CEdMimicFacesEditor::OnPrevPose )	
	EVT_MENU( XRCID( "next" ), CEdMimicFacesEditor::OnNextPose )
	EVT_MENU( XRCID( "toolSkeleton" ), CEdMimicFacesEditor::OnShowSkeleton )
END_EVENT_TABLE()

CEdMimicFacesEditor::CEdMimicFacesEditor( wxWindow* parent, CResource* mimicFaces )
	: m_mimicFaces( NULL )
	, m_preview( NULL )
	, m_headComponent( NULL )
	, m_previewEntity( NULL )
	, m_currPose( 0 )
{
	m_mimicFaces = SafeCast< CMimicFaces >( mimicFaces );
	m_mimicFaces->AddToRootSet();

	ASSERT( m_mimicFaces->GetFile() );

	// Load window
	wxXmlResource::Get()->LoadFrame( this, parent, wxT( "MimicFacesEditor" ) );

	// Set window parameters
	SetTitle( String::Printf( TXT( "Mimic Faces Editor - %s" ),
		m_mimicFaces->GetFile()->GetDepotPath().AsChar() ).AsChar() );
	SetSize( 500, 450 );

	{
		// Create preview
		wxPanel* rp = XRCCTRL( *this, "previewPanel", wxPanel );
		ASSERT( rp != NULL );
		m_preview = new CEdAnimBrowserPreview( rp, this, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( m_preview, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();

		m_preview->SetCameraPosition( Vector( 0.f, 0.5f, 0.4f ) );
		m_preview->SetCameraRotation( EulerAngles( 0, -5, 180 ) );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
	}

	{
		// Create preview popup menu
		wxMenu* previewMenu = new wxMenu();
		previewMenu->Append( ID_MIMIC_LOAD_ENTITY, TXT("Use selected entity") );
		previewMenu->Connect( ID_MIMIC_LOAD_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdMimicFacesEditor::OnLoadEntity ), NULL, this ); 
		m_preview->SetContextMenu( previewMenu );
	}

	{
		wxSlider* sliderWeight = XRCCTRL( *this, "sliderWeight", wxSlider );
		sliderWeight->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdMimicFacesEditor::OnSliderUpdate ), NULL, this );
		sliderWeight->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdMimicFacesEditor::OnSliderUpdate ), NULL, this );
	}

	// Create skeleton icons
	CreateSkeletonIcons();

	LoadOptionsFromConfig();

	if ( m_previewEntity == NULL )
	{
		LoadEntity( resDefaultHead.GetPath().ToString() );
	}
}

CEdMimicFacesEditor::~CEdMimicFacesEditor()
{
	UnloadEntity();

	SaveOptionsToConfig();

	m_mimicFaces->RemoveFromRootSet();
	m_mimicFaces = NULL;
}

void CEdMimicFacesEditor::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	CSkeleton* skeleton = m_mimicFaces->GetMimicSkeleton();

	if ( m_visualPose.Size() > 0 && skeleton )
	{
		SkeletonRenderingUtils::DrawSkeleton( m_visualPose, skeleton, Color( 255, 255, 255 ), frame );
	}
}

void CEdMimicFacesEditor::OnLoadEntity( wxCommandEvent& event )
{
	String selectedResource;

	if ( GetActiveResource( selectedResource ) )
	{
		LoadEntity( selectedResource );
	}
}

void CEdMimicFacesEditor::LoadEntity( const String &entName )
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
	TDynArray< CHeadComponent* > animComponents;
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
		m_headComponent = animComponents[0];
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
				m_headComponent = animComponents[i];
				break;
			}
		}
	}

	ASSERT( m_headComponent != NULL );

	// Disable motion extraction
	m_headComponent->SetUseExtractedMotion( false );
	m_headComponent->SetUseExtractedTrajectory( true );

	// Tick world to allow behaviours process
	CWorldTickInfo info( m_preview->GetPreviewWorld(), 0.1f );
	m_preview->GetPreviewWorld()->Tick( info );

	if ( m_headComponent->GetEntity()->GetRootAnimatedComponent() && m_headComponent->GetEntity()->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		m_headComponent->GetEntity()->GetRootAnimatedComponent()->GetBehaviorStack()->Deactivate();
	}

	if ( m_headComponent && m_headComponent->GetBehaviorStack() )
	{
		m_headComponent->GetBehaviorStack()->Deactivate();
	}

	// Disable ragdoll
	m_headComponent->EnableRagdoll( false );

	// Update preview
	m_preview->OnLoadEntity( m_previewEntity );
	m_preview->SetAnimatedComponent( m_headComponent );
	m_preview->SetPlayedAnimation( NULL );

	m_currPose = 0;
	UpdatePose();
}

void CEdMimicFacesEditor::UnloadEntity()
{
	if ( m_previewEntity == NULL )
	{
		return;
	}

	// Unload entity from world
	m_preview->GetPreviewWorld()->DelayedActions();
	m_previewEntity->Destroy();
	m_preview->GetPreviewWorld()->DelayedActions(); 
	m_headComponent = NULL;
	m_previewEntity = NULL;

	m_preview->OnUnloadEntity();
	m_preview->SetAnimatedComponent( NULL );
	m_preview->SetPlayedAnimation( NULL );
}

void CEdMimicFacesEditor::SaveOptionsToConfig()
{
	String identifier = TXT("/Frames/MimicFacesEditor/");

	SaveLayout( identifier );

	if ( m_previewEntity )
	{
		identifier = identifier + m_mimicFaces->GetFile()->GetDepotPath();

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		config.RemeberCurrentPath();
		config.SetPath( identifier );

		// Entity
		CEntityTemplate* t = Cast< CEntityTemplate >( m_previewEntity->GetTemplate() );

		config.Write( TXT("Entity"), t->GetFile()->GetDepotPath() );

		config.RestoreCurrentPath();
	}
}

void CEdMimicFacesEditor::LoadOptionsFromConfig()
{
	String identifier = TXT("/Frames/MimicFacesEditor/");

	LoadLayout( identifier );

	identifier = identifier + m_mimicFaces->GetFile()->GetDepotPath();

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.RemeberCurrentPath();
	config.SetPath( identifier );

	String depotPath = config.Read( TXT("Entity"), String::EMPTY );
	if ( depotPath.Empty() == false )
	{
		LoadEntity( depotPath );
	}

	config.RestoreCurrentPath();
}

void CEdMimicFacesEditor::SaveSession( CConfigurationManager &config )
{
	wxSmartLayoutFrame::SaveSession( config );
}

void CEdMimicFacesEditor::RestoreSession( CConfigurationManager &config )
{
	wxSmartLayoutFrame::RestoreSession( config );
}

wxToolBar* CEdMimicFacesEditor::GetSkeletonToolbar()
{
	wxToolBar* toolbar = XRCCTRL( *this, "controlToolbar", wxToolBar );
	ASSERT( toolbar );
	return toolbar;
}

namespace
{
	Bool IsSkeletonEqual( const CSkeleton* s1, const CSkeleton* s2 )
	{
		return	s1->GetBonesNum() == s2->GetBonesNum() &&
				s1->GetTracksNum() == s2->GetTracksNum();
	}
}

EHeadState CEdMimicFacesEditor::GetDesiredHeadState() const
{
	if ( m_headComponent )
	{
		if ( m_headComponent->GetMimicSkeletonHigh() && 
			IsSkeletonEqual( m_headComponent->GetMimicSkeletonHigh(), m_mimicFaces->GetMimicSkeleton() ) )
		{
			return HS_MimicHigh;
		}
	}

	return HS_None;
}

void CEdMimicFacesEditor::UpdatePose()
{
	wxStaticText* text = XRCCTRL( *this, "textPose", wxStaticText );

	String str = ToString( m_currPose );
	String strTotal = ToString( m_mimicFaces->GetMimicPoseNum() );
	String final = str + TXT("/") + strTotal;

	if ( m_headComponent && m_headComponent->GetMimicSkeleton() )
	{
		CSkeleton* mimicSkeleton = m_headComponent->GetMimicSkeleton();
		String trackName = m_currPose > 0 ? mimicSkeleton->GetTrackName( m_currPose - 1 ) : TXT("<base>");

		final += TXT(" - ")  + trackName;
	}

	text->SetLabel( final.AsChar() );

	ApplyPose();
}

void CEdMimicFacesEditor::ApplyPose()
{
	if ( m_headComponent && m_headComponent->GetBehaviorStack() )
	{
		m_headComponent->GetBehaviorStack()->Deactivate();
	}

	CSkeleton* skeleton = m_mimicFaces->GetMimicSkeleton();
	if ( skeleton )
	{
		// T pose
		SBehaviorGraphOutput poseT( skeleton->GetBonesNum(), skeleton->GetTracksNum() );
		poseT.Reset( skeleton );

		// Additive pose
		SBehaviorGraphOutput poseAdd;

		if ( GetWeight() < 0.001f )
		{
			//...
		}
		else if ( m_mimicFaces->GetMimicPose( m_currPose, poseAdd ) )
		{
			m_mimicFaces->AddPose( poseT, poseAdd, GetWeight() );
		}
		else
		{
			String msg = String::Printf( TXT("Mimic pose hasn't got pose '%d'"), m_currPose );
			wxMessageBox( msg.AsChar(), wxT("Warinng"), wxOK | wxCENTRE | wxICON_WARNING );
		}

		poseT.Touch();

		m_visualPose.Resize( skeleton->GetBonesNum() );
		poseT.GetBonesModelSpace( skeleton, m_visualPose );

		if ( m_headComponent )
		{
			EHeadState state = GetDesiredHeadState();

			if ( state == HS_None )
			{
				String msg = TXT("Head couldn't change state to mimic state from this resource");
				wxMessageBox( msg.AsChar(), wxT("Message"), wxOK | wxCENTRE );
				return;
			}

			//if ( m_headComponent->SetState( state ) )
			if ( m_headComponent->ForceState( state ) )
			{
				m_preview->GetPreviewWorld()->GetTickManager()->Remove( m_headComponent );

				{
					// Tick world to allow behaviours process
					CWorldTickInfo info( m_preview->GetPreviewWorld(), 0.1f );
					m_preview->GetPreviewWorld()->Tick( info );
				}

				{

					if ( state == HS_MimicHigh )
					{
						m_headComponent->ForceMimicPoseHigh( poseT );
					}
					else
					{
						m_headComponent->ForceMimicPoseLow( poseT );
					}

					m_headComponent->ForceUpdateTransform();
					m_headComponent->ForceUpdateBounds();
				}
			}
			else
			{
				String msg = TXT("Head couldn't change state to ");

				if ( state == HS_MimicHigh )
				{
					msg += TXT("Mimic High");
				}
				else if ( state == HS_NormalMimicLow )
				{
					msg += TXT("Mimic Low");
				}
				else
				{
					ASSERT( 0 );
				}

				wxMessageBox( msg.AsChar(), wxT("Warinng"), wxOK | wxCENTRE | wxICON_WARNING );
			}
		}
	}
}

Float CEdMimicFacesEditor::GetWeight() const
{
	wxSlider* sliderWeight = XRCCTRL( *this, "sliderWeight", wxSlider );
	return ( (Float)sliderWeight->GetValue() ) / 100.f;
}

void CEdMimicFacesEditor::OnSliderUpdate( wxCommandEvent& event )
{
	UpdatePose();
}

void CEdMimicFacesEditor::OnShowSkeleton( wxCommandEvent& event )
{
	if ( m_mimicFaces->GetMimicSkeleton() )
	{
		CEdSkeletonDialog dlg( this, wxT("Mimic skeleton"), m_mimicFaces->GetMimicSkeleton() );
		dlg.DoModal();
	}
}

void CEdMimicFacesEditor::OnStartPose( wxCommandEvent& event )
{
	m_currPose = 0;
	UpdatePose();
}

void CEdMimicFacesEditor::OnEndPose( wxCommandEvent& event )
{
	m_currPose = m_mimicFaces->GetMimicPoseNum() - 1;
	UpdatePose();
}

void CEdMimicFacesEditor::OnNextPose( wxCommandEvent& event )
{
	if ( m_currPose == m_mimicFaces->GetMimicPoseNum() - 1 )
	{
		m_currPose = 0;
	}
	else
	{
		m_currPose = Clamp< Uint32 >( m_currPose + 1, 0, m_mimicFaces->GetMimicPoseNum() - 1 );
	}

	UpdatePose();
}

void CEdMimicFacesEditor::OnPrevPose( wxCommandEvent& event )
{
	if ( m_currPose == 0 )
	{
		m_currPose = m_mimicFaces->GetMimicPoseNum() - 1;
	}
	else
	{
		m_currPose = Clamp< Uint32 >( m_currPose - 1, 0, m_mimicFaces->GetMimicPoseNum() - 1 );
	}

	UpdatePose();
}

//////////////////////////////////////////////////////////////////////////

#define ID_OK	4001

BEGIN_EVENT_TABLE( CEdSkeletonDialog, wxDialog )
	EVT_BUTTON( ID_OK, CEdSkeletonDialog::OnOk )
END_EVENT_TABLE()

CEdSkeletonDialog::CEdSkeletonDialog( wxWindow* parent, wxString caption, CSkeleton* skeleton )
	: wxDialog( parent, wxID_ANY, caption )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	wxTreeCtrl* treeBones = new wxTreeCtrl( this );
	wxTreeCtrl* treeTracks = new wxTreeCtrl( this );

	wxButton* btnOk = new wxButton( this, ID_OK, wxT("Ok") );

	CEdSkeletonEditor::FillSkeletonTrees( skeleton, treeBones, treeTracks );

	sizer->Add( treeBones, 3, wxALL | wxEXPAND, 5 );
	sizer->Add( treeTracks, 1, wxALL | wxEXPAND, 5 );
	sizer->Add( btnOk, 0, wxALL, 5 );

	SetSizer( sizer );

	SetWindowStyle( wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

	SetMinSize( wxSize( 100, 100 ) );
	SetSize( 250, 400 );

	Layout();
	Refresh();
}

Int32 CEdSkeletonDialog::DoModal()
{
	return wxDialog::ShowModal();
}

void CEdSkeletonDialog::OnOk( wxCommandEvent& event )
{
	EndDialog( wxID_OK );
}
*/
