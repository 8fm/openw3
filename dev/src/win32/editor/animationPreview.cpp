
#include "build.h"
#include "animationPreview.h"
#include "defaultCharactersIterator.h"
#include "callbackData.h"
#include "animationEditorUtils.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/worldTick.h"


#define ID_LOAD_ENTITY				4001
#define ID_LOAD_ENTITY_ITEM_UNDO	4102
#define ID_LOAD_ENTITY_ITEM			4103

//CGatheredResource resCEdAnimationPreviewFloor( TXT("engine\\meshes\\editor\\base_grid_10x10.w2mesh"), 0 );

//////////////////////////////////////////////////////////////////////////

CEdAnimationPreview::PlayedAnimation::PlayedAnimation()
	: m_bodyMimicFlag( true )
	, m_playedAnimation( nullptr )
{

}

void CEdAnimationPreview::PlayedAnimation::Set( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion ) 
{
	if ( IsBody() )
	{
		m_playedAnimation = ac->GetAnimatedSkeleton()->PlayAnimation( aniamtion );
	}
	else if ( m_slot.IsValid() )
	{
		SAnimationFullState animA;
		animA.m_state.m_animation = aniamtion->GetName();
		SAnimationFullState animB;
		animB.m_state.m_animation = aniamtion->GetName();
		m_slot.SetIdleAnimationToSample( animA, animB, 1.f );
	}
}

CPlayedSkeletalAnimation* CEdAnimationPreview::PlayedAnimation::SetBody( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion )
{
	if ( IsBody() )
	{
		m_playedAnimation = ac->GetAnimatedSkeleton()->PlayAnimation( aniamtion );
	}

	return m_playedAnimation;
}

void CEdAnimationPreview::PlayedAnimation::Reset()
{
	m_playedAnimation = nullptr;
}

Bool CEdAnimationPreview::PlayedAnimation::IsBody() const
{
	return m_bodyMimicFlag;
}

void CEdAnimationPreview::PlayedAnimation::SetMimic( CMimicComponent* mimicComponent ) 
{ 
	m_bodyMimicFlag = false;

	if ( mimicComponent->GetBehaviorStack() )
	{
		mimicComponent->GetBehaviorStack()->GetSlot( CName( TXT( "MIXER_SLOT_OVERRIDE" ) ), m_slot, false );
	}
}

//////////////////////////////////////////////////////////////////////////

const Vector		CEdAnimationPreview::CAMERA_POS_OFFSET = Vector( 0.f, 2.f, 1.5f );
const EulerAngles	CEdAnimationPreview::CAMERA_ROT_OFFSET = EulerAngles( 0.f, -10.f, 180.f );

CEdAnimationPreview::CEdAnimationPreview( wxWindow* parent, Bool timeOptions, CEdAnimationPreviewListener* listener )
	: CEdPreviewPanel( parent, false )
	, m_cameraSnapping( true )
	, m_prevPosition( Vector::ZERO_3D_POINT )
	, m_usePositionWraping( true )
	, m_timeMul( 1.f )
	, m_listener( listener )
	, m_contextMenu( NULL )
	, m_timeSilder( NULL )
	, m_pause( false )
	, m_forceOneFrame( 0 )
	, m_animationGraphEnabled( false )
	, m_component( nullptr )
{
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );

	if ( timeOptions )
	{
		CreateTimeOptions();
	}

	CreateFloor();

	BackToDefaultPosition();

	SetLightPosition( 0 );
}

CEdAnimationPreview::~CEdAnimationPreview()
{
	m_playedAnimation.Reset();

	HideGhosts();

	m_postprocesses.ClearPtr();

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdAnimationPreview::AddRotationWidgets()
{
	const Float rotateGismoSize = 0.5f;

	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 1, 0 ), Vector::EX, Color::RED, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 1, 0, 0 ), Vector::EY, Color::GREEN, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 0, 1 ), Vector::EZ, Color::BLUE, rotateGismoSize ) );

	m_widgetManager->SetWidgetSpace( RPWS_Local );

	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	m_widgetManager->SetWidgetMode( RPWM_Rotate );
}

void CEdAnimationPreview::AddTranslationWidgets()
{
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );

	m_widgetManager->SetWidgetSpace( RPWS_Local );

	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	m_widgetManager->SetWidgetMode( RPWM_Move );
}
void CEdAnimationPreview::AddScaleWidgets()
{
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScalePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );
	m_widgetManager->AddWidget( TXT("Scale"), new CViewportWidgetScaleUniform( Vector::EX, Vector::EY, Vector::EZ, Color::WHITE ) );

	m_widgetManager->SetWidgetSpace( RPWS_Local );

	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	m_widgetManager->SetWidgetMode( RPWM_Scale );
}

void CEdAnimationPreview::SetTranslationWidgetMode()
{
	m_widgetManager->SetWidgetMode( RPWM_Move );
}

void CEdAnimationPreview::SetRotationWidgetMode()
{
	m_widgetManager->SetWidgetMode( RPWM_Rotate );
}

void CEdAnimationPreview::ToggleWidgetMode()
{
	if ( m_widgetManager->GetWidgetMode() == RPWM_Move )
	{
		SetRotationWidgetMode();
	}
	else
	{
		SetTranslationWidgetMode();
	}
}

void CEdAnimationPreview::SetWidgetModelSpace()
{
	m_widgetManager->SetWidgetSpace( RPWS_Global );
}

void CEdAnimationPreview::SetWidgetLocalSpace()
{
	m_widgetManager->SetWidgetSpace( RPWS_Local );
}

void CEdAnimationPreview::EnableWidgets()
{
	m_widgetManager->EnableWidgets( true );
}

void CEdAnimationPreview::DisableWidgets()
{
	m_widgetManager->EnableWidgets( false );
}

CAnimatedComponent* CEdAnimationPreview::GetAnimatedComponent() const
{
	return m_component;
}

void CEdAnimationPreview::RefreshWorld()
{
	CWorldTickInfo info( m_previewWorld, 0.f );
	info.m_updatePhysics = false;
	m_previewWorld->Tick( info );
}

void CEdAnimationPreview::Pause( Bool flag )
{
	m_pause = flag;
}

Bool CEdAnimationPreview::IsPaused() const
{
	return m_pause;
}

void CEdAnimationPreview::SetTimeMul( Float factor )
{
	m_timeMul = factor;
}

void CEdAnimationPreview::ForceOneFrame()
{
	m_forceOneFrame = 1;
}

void CEdAnimationPreview::ForceOneFrameBack()
{
	m_forceOneFrame = -1;
}

void CEdAnimationPreview::SetDefaultContextMenu()
{
	m_contextMenu = new wxMenu();
	m_contextMenu->Append( ID_LOAD_ENTITY, TXT("Use selected entity") );
	m_contextMenu->Connect( ID_LOAD_ENTITY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationPreview::OnLoadPreviewEntity ), NULL, this ); 

	Uint32 counter = 0;

	for ( DefaultCharactersIterator it; it; ++it )
	{
		counter++;

		m_contextMenu->Append( ID_LOAD_ENTITY + counter, it.GetName().AsChar() );
		m_contextMenu->Connect( ID_LOAD_ENTITY + counter, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationPreview::OnLoadPreviewEntity ), NULL, this ); 
	}

	{
		wxMenu* subMenu = new wxMenu();

		counter = 0;
		for ( AnimPreviewItemIterator it; it; ++it )
		{
			counter++;

			subMenu->Append( ID_LOAD_ENTITY_ITEM + counter, (*it).AsChar() );
			m_contextMenu->Connect( ID_LOAD_ENTITY_ITEM + counter, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationPreview::OnLoadPreviewEntityItem ), NULL, this );
		}

		subMenu->AppendSeparator();

		subMenu->Append( ID_LOAD_ENTITY_ITEM_UNDO, wxT("Undo") );
		m_contextMenu->Connect( ID_LOAD_ENTITY_ITEM_UNDO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdAnimationPreview::OnLoadPreviewEntityItem ), NULL, this );

		m_contextMenu->AppendSubMenu( subMenu, wxT("Add items") );
	}
}

void CEdAnimationPreview::SetCustomContextMenu( wxMenu* contextMenu )
{
	m_contextMenu = contextMenu;
}

void CEdAnimationPreview::HandleContextMenu( Int32 x, Int32 y )
{
	CEdPreviewPanel::HandleContextMenu( x, y );

	if ( m_contextMenu )
	{
		PopupMenu( m_contextMenu, x, y );
	}
}

void CEdAnimationPreview::HandleSelection( const TDynArray< CHitProxyObject* >& object )
{
	CEdPreviewPanel::HandleSelection( object );

	if ( m_listener )
	{
		m_listener->OnPreviewHandleSelection( object );
	}
}


void CEdAnimationPreview::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{
		// Selection has changed
		typedef CSelectionManager::SSelectionEventData SEventData;
		const SEventData& eventData = GetEventData< SEventData >( data );
		if ( eventData.m_world == m_previewWorld )
		{
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );

			if ( nodes.Size() > 0 )
			{
				m_widgetManager->EnableWidgets( nodes.Size() > 0 );
			}
		}
	}
}

void CEdAnimationPreview::SaveSession( CConfigurationManager &config )
{
	CEdPreviewPanel::SaveSession( config );
}

void CEdAnimationPreview::RestoreSession( CConfigurationManager &config )
{
	CEdPreviewPanel::RestoreSession( config );
}

void CEdAnimationPreview::OnViewportTick( IViewport* view, Float timeDelta )
{
	if ( m_pause )
	{
		return;
	}

	static const Float minStepTime = 0.0000001f;

	Float dt = Max( !m_pause ? m_timeMul * timeDelta : 0.f, minStepTime );

	if ( m_forceOneFrame != 0 )
	{
		dt = m_forceOneFrame == 1 ? 1.f / 30.f : -1.f / 30.f;

		if ( m_forceOneFrame == 1 )
		{
			CEdPreviewPanel::OnViewportTick( view, dt );
		}
		else
		{
			if ( m_component && m_component->GetBehaviorStack() )
			{
				m_component->GetBehaviorStack()->Update( m_component->GetBehaviorGraphUpdateContext(), dt );
				m_component->ForceBehaviorPose();
			}
			CEdPreviewPanel::OnViewportTick( view, minStepTime );
		}

		m_forceOneFrame = 0;
	}
	else
	{
		CEdPreviewPanel::OnViewportTick( view, dt );
	}

	if ( ShouldBackToDefaultPosition() )
	{
		BackToDefaultPosition();
	}

	SnapCamera();

	if ( m_component )
	{
		m_ghostContainer.UpdateGhosts( dt, m_component );
	}

	if ( m_listener )
	{
		m_listener->OnPreviewTick( dt );
	}

	SItemEntityManager::GetInstance().OnTick( dt );
}

Bool CEdAnimationPreview::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( m_pause )
	{
		return false;
	}

	if ( m_listener )
	{
		m_listener->OnPreviewViewportInput( view, key, action, data );
	}

	if ( key == IK_MouseZ )
	{
		Float currentFov = GetCameraFov();

		if( data > 0.0f )
		{
			currentFov -= 2.5f;
		}
		else
		{
			currentFov += 2.5f;
		}

		currentFov = Clamp( currentFov, 1.0f, 180.0f );
		SetCameraFov( currentFov );

		return true;
	}

	return CEdPreviewPanel::OnViewportInput( view, key, action, data );
};

void CEdAnimationPreview::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_pause )
	{
		return;
	}

	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	frame->AddDebugAxisOnScreen( 0.2f, 0.2f, Matrix::IDENTITY, 0.1f );

	if ( m_listener )
	{
		m_listener->OnPreviewGenerateFragments( frame );
	}

	if ( m_component )
	{
		m_ghostContainer.Draw( frame, m_component );
	}

	if ( m_playedAnimation.IsBody() )
	{
		const Uint32 pSize = m_postprocesses.Size();
		for ( Uint32 i=0; i<pSize; ++i )
		{
			m_postprocesses[ i ]->OnPreviewPostProcessGenerateFragments( frame, m_component, m_playedAnimation.m_playedAnimation );
		}
	}
}

void CEdAnimationPreview::CloneAndUseAnimatedComponent( const CAnimatedComponent* animatedComponent )
{
	if ( animatedComponent )
	{
		CloneEntityFromComponent( animatedComponent );
	}
	else
	{
		m_component = NULL;
	}

	if ( m_component )
	{
		SetInitialCameraPosition();
	}
	else
	{
		BackToDefaultPosition();
	}
}

void CEdAnimationPreview::LoadEntity( const String& fileName, const String& component )
{
	ASSERT( !m_component );

	CEntityTemplate* entityTemplate = LoadResource< CEntityTemplate>( fileName );
	if ( !entityTemplate )
	{
		return;
	}

	EntitySpawnInfo einfo;
	einfo.m_template = entityTemplate;
	einfo.m_name = TXT("PreviewEntity");

	CAnimatedComponent* newComponent = NULL;

	CEntity* entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );			
	if ( entity )
	{
		if ( !component.Empty() )
		{
			newComponent = Cast< CAnimatedComponent >( entity->FindComponent( component ) );
		}
		else
		{
			newComponent = entity->GetRootAnimatedComponent();
		}

		CWorldTickInfo info( GetPreviewWorld(), 0.1f );
		info.m_updatePhysics = true;
		GetPreviewWorld()->Tick( info );
	}

	SetupAndSetEntity( newComponent );

	m_ghostContainer.Reset();
}

void CEdAnimationPreview::SetAnimation( CSkeletalAnimationSetEntry* animation )
{
	if ( m_component )
	{
		if ( animation )
		{
			m_playedAnimation.Set( m_component, animation );
		}
		else
		{
			ClearAnimation();
		}
	}
}

CPlayedSkeletalAnimation* CEdAnimationPreview::SetBodyAnimation( CSkeletalAnimationSetEntry* animation )
{
	if ( m_component )
	{
		if ( animation )
		{
			m_playedAnimation.SetBody( m_component, animation );
		}
		else
		{
			ClearAnimation();
		}
	}

	return m_playedAnimation.m_playedAnimation;
}

void CEdAnimationPreview::ClearAnimation()
{
	if ( m_component )
	{
		m_component->StopAllAnimationsOnSkeleton();
	}

	m_playedAnimation.Reset();
}

void CEdAnimationPreview::SetAnimationGraphEnabled( Bool state )
{
	m_animationGraphEnabled = state;
}

void CEdAnimationPreview::UseCameraSnapping( Bool flag )
{
	m_cameraSnapping = flag;
}

void CEdAnimationPreview::AddPostProcess( CEdAnimationPreviewPostProcess* process )
{
	m_postprocesses.PushBack( process );
}

void CEdAnimationPreview::SnapCamera()
{
	if ( m_component )
	{
		if ( m_cameraSnapping )
		{
			Vector pos = m_component->GetWorldPosition();

			m_cameraPosition = m_cameraPosition - m_prevPosition + pos;
			m_prevPosition = pos;
		}
		else
		{
			//m_cameraPosition = pos + CAMERA_POS_OFFSET;
		}
	}
	
	OnCameraMoved();
}

void CEdAnimationPreview::SetInitialCameraPosition()
{
	if ( m_component )
	{
		Box box;
		box.Clear();

		if ( CMimicComponent* m = Cast< CMimicComponent >( m_component ) )
		{
			const Int32 num = m->GetBonesNum();
			for ( Int32 i=0; i<num; ++i )
			{
				box.AddPoint( m->GetBoneMatrixWorldSpace( i ).GetTranslation() );
			}

			if ( !box.IsEmpty() )
			{
				SetCameraPosition( box.CalcCenter() + Vector( 0.4f, 2.5f, 0.9f ) * ( box.CalcSize().Mag3() * 0.4f ) );
				SetCameraRotation( EulerAngles( 0.f, -5.f, 175.f ) );
			}
		}
		else
		{
			box = m_component->GetEntity()->CalcBoundingBox();

			if ( !box.IsEmpty() )
			{
				SetCameraPosition( box.CalcCenter() + Vector( 0.6f, 2.f, 1.f ) * ( box.CalcSize().Mag3() * 0.2f ) );
				SetCameraRotation( EulerAngles( 0.f, -20.f, 160.f ) );
			}
		}

		m_prevPosition = m_component->GetWorldPosition();
	}
}

Bool CEdAnimationPreview::ShouldBackToDefaultPosition() const
{
	if ( m_usePositionWraping && m_component )
	{
		return m_component->GetWorldPositionRef().SquareMag3() > 100000.f;
	}
	return false;
}

void CEdAnimationPreview::BackToDefaultPosition()
{
	SetCameraPosition( CAMERA_POS_OFFSET );
	SetCameraRotation( CAMERA_ROT_OFFSET );

	if ( m_component )
	{
		m_component->GetEntity()->SetPosition( Vector::ZERO_3D_POINT );
		m_component->GetEntity()->SetRotation( EulerAngles::ZEROS );
	}

	m_prevPosition = Vector::ZERO_3D_POINT;
}

void CEdAnimationPreview::CreateTimeOptions()
{
	wxBoxSizer *sizer = new wxBoxSizer( wxHORIZONTAL );

	m_timeSilder = new wxSlider( this, -1, 100, 0, 200 );
	m_timeSilder->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdAnimationPreview::OnTimeSliderChanged ), NULL, this );
	m_timeSilder->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdAnimationPreview::OnTimeSliderChanged ), NULL, this );

	//wxButton* button00 = new wxButton( this, -1, TXT("-100"), wxDefaultPosition, wxSize( 30, -1 ) );
	//button00->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( -100 ), this ); 

	//wxButton* button0 = new wxButton( this, -1, TXT("-10"), wxDefaultPosition, wxSize( 30, -1 ) );
	//button0->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( -10 ), this ); 

	wxButton* button1 = new wxButton( this, -1, TXT("0"), wxDefaultPosition, wxSize( 30, -1 ) );
	button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( 0 ), this ); 

	wxButton* button2 = new wxButton( this, -1, TXT("10"), wxDefaultPosition, wxSize( 30, -1 ) );
	button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( 10 ), this ); 

	wxButton* button3 = new wxButton( this, -1, TXT("100"), wxDefaultPosition, wxSize( 30, -1 ) );
	button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( 100 ), this ); 

	wxButton* button4 = new wxButton( this, -1, TXT("200"), wxDefaultPosition, wxSize( 30, -1 ) );
	button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdAnimationPreview::OnBtnTime ), new TCallbackData< Int32 >( 200 ), this ); 

	sizer->Add( m_timeSilder, 1, wxALL | wxEXPAND, 0 );
	//sizer->Add( button00, 1, wxALL, 0 );
	//sizer->Add( button0, 1, wxALL, 0 );
	sizer->Add( button1, 0, wxALL, 0 );
	sizer->Add( button2, 0, wxALL, 0 );
	sizer->Add( button3, 0, wxALL, 0 );
	sizer->Add( button4, 0, wxALL, 0 );
	sizer->Layout();

	m_sizer->Add( sizer, 0, wxALL | wxEXPAND, 5 );
	m_sizer->Layout();
}

void CEdAnimationPreview::CreateFloor()
{
	/*EntitySpawnInfo sinfo;
	sinfo.m_name = TXT("Floor");
	CEntity* floor = GetWorld()->GetDynamicLayer()->CreateEntitySync( sinfo );
	if ( floor )
	{
		CMesh* mesh = resCEdAnimationPreviewFloor.LoadAndGet< CMesh >();

		for ( Float i=-100.f; i<=100.f; i+=10.f )
		{
			for ( Float j=-100.f; j<=100.f; j+=10.f )
			{
				CStaticMeshComponent* component = SafeCast< CStaticMeshComponent >( floor->CreateComponent( CStaticMeshComponent::GetStaticClass(), SComponentSpawnInfo() ) );
				component->SetMesh( mesh );
				component->SetPosition( Vector( i, j, 0.f ) );
			}
		}

		floor->ForceUpdateTransform();
		floor->ForceUpdateBounds();
	}*/
}

void CEdAnimationPreview::DestroyFloor()
{

}

void CEdAnimationPreview::UnloadEntity()
{
	if ( m_component )
	{
		m_component->GetEntity()->Destroy();
		m_component = NULL;
	}
}

void CEdAnimationPreview::SetupAndSetEntity( CAnimatedComponent *component )
{
	m_component = component;

	if ( m_component )
	{
		if ( CMimicComponent* m = Cast< CMimicComponent >( component ) )
		{
			m->MimicHighOn();

			if ( CAnimatedComponent* ac = m->GetEntity()->GetRootAnimatedComponent() )
			{
				if ( ac->GetBehaviorStack() )
				{
					ac->GetBehaviorStack()->Deactivate();
				}
			}

			if ( m->GetBehaviorStack() )
			{
				m->GetBehaviorStack()->Activate();
			}

			m_playedAnimation.SetMimic( m );
		}
		else
		{
			if ( !m_animationGraphEnabled && m_component->GetBehaviorStack() )
			{
				m_component->GetBehaviorStack()->Deactivate();
			}
		}

		AnimationEditorUtils::SetActorsItemsVisible( m_component->GetEntity() );

		AnimationEditorUtils::SetActorsMeshRepresentation( m_component->GetEntity() );
	}
}

void CEdAnimationPreview::CloneEntityFromComponent( const CAnimatedComponent *originalComponent )
{
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	CEntity* prototype = originalComponent->GetEntity();

	DependencySavingContext context( prototype );
	if ( !saver.SaveObjects( context ) )
	{
		return;
	}

	UnloadEntity();

	LayerEntitiesArray pastedEntities;
	GetPreviewWorld()->GetDynamicLayer()->PasteSerializedEntities( buffer, pastedEntities, true, Vector( 0.0f, 0.0f, 0.0f ), EulerAngles( 0.0f, 0.0f, 0.0f ) );

	CAnimatedComponent* component = NULL;

	ASSERT( pastedEntities.Size() == 1 );
	if ( pastedEntities.Size() == 1 )
	{
		CEntity* entity = pastedEntities[0];
		component = entity->FindComponent< CAnimatedComponent >( originalComponent->GetName() );

		entity->SetPosition( Vector::ZERO_3D_POINT );
		entity->SetRotation( EulerAngles::ZEROS );
	}

	CWorldTickInfo info( GetPreviewWorld(), 0.1f );
	info.m_updatePhysics = true;
	GetPreviewWorld()->Tick( info );

	SetupAndSetEntity( component );
}

void CEdAnimationPreview::ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type )
{
	m_ghostContainer.InitGhosts( number, type );
}

void CEdAnimationPreview::HideGhosts()
{
	m_ghostContainer.DestroyGhosts();
}

Bool CEdAnimationPreview::HasGhosts() const
{
	return m_ghostContainer.HasGhosts();
}

void CEdAnimationPreview::OnLoadPreviewEntity( wxCommandEvent& event )
{
	String componentName = m_component ? m_component->GetName() : String::EMPTY;

	UnloadEntity();

	if ( event.GetId() > ID_LOAD_ENTITY )
	{
		const Int32 index = event.GetId() - ID_LOAD_ENTITY - 1;
		ASSERT( index >= 0);

		DefaultCharactersIterator it;
		it.GoTo( (Uint32)index );

		if ( it )
		{
			LoadEntity( it.GetPath(), componentName );
		}
	}
	else
	{
		// Selected
	}

	if ( m_listener )
	{
		m_listener->OnLoadPreviewEntity( m_component );
	}
}

void CEdAnimationPreview::OnLoadPreviewEntityItem( wxCommandEvent& event )
{
	if ( event.GetId() == ID_LOAD_ENTITY_ITEM_UNDO )
	{
		UndoItemDisplay();

		return;
	}

	const Int32 index = event.GetId() - ID_LOAD_ENTITY_ITEM - 1;
	ASSERT( index >= 0);

	AnimPreviewItemIterator it;
	it.GoTo( (Uint32)index );

	if ( it )
	{
		String itemName = it.GetName();
		EItemDisplaySlot slot = it.GetSlot();

		DisplayItem( CName( itemName ), slot );
	}
}

void CEdAnimationPreview::OnTimeSliderChanged( wxCommandEvent& event )
{
	SetTimeMul( (Float)m_timeSilder->GetValue() / 100.f );
}

void CEdAnimationPreview::OnBtnTime( wxCommandEvent& event )
{
	TCallbackData< Int32 >* data = static_cast< TCallbackData< Int32 >* >( event.m_callbackUserData );
	SetTimeMul( data->GetData() / 100.f );

	m_timeSilder->SetValue( Clamp( data->GetData(), 0, 200 ) );
}

CActor*	CEdAnimationPreview::GetActorEntity()
{
	return m_component ? Cast< CActor >( m_component->GetEntity() ) : NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CEdAnimationPreviewWithPreviewItems::CEdAnimationPreviewWithPreviewItems( wxWindow* parent, Bool timeOptions /* = false */, CEdAnimationPreviewListener* listener /* = NULL */ )
	: CEdAnimationPreview( parent, timeOptions, listener )
{
	AddTranslationWidgets();

	InitItemContainer();
}

CEdAnimationPreviewWithPreviewItems::~CEdAnimationPreviewWithPreviewItems()
{
	DestroyItemContainer();
}

CWorld* CEdAnimationPreviewWithPreviewItems::GetPreviewItemWorld() const
{
	return m_previewWorld;
}

void CEdAnimationPreviewWithPreviewItems::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CEdAnimationPreview::HandleSelection( objects );

	HandleItemSelection( objects );
}

void CEdAnimationPreviewWithPreviewItems::OnSelectItem( IPreviewItem* item )
{
	if ( m_listener )
	{
		m_listener->OnPreviewSelectPreviewItem( item );
	}
}

void CEdAnimationPreviewWithPreviewItems::OnDeselectAllItem()
{
	if ( m_listener )
	{
		m_listener->OnPreviewDeselectAllPreviewItems();
	}
}

//////////////////////////////////////////////////////////////////////////
