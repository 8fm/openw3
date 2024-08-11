#include "build.h"
#include "dialogEditor.h"

#include "dialogEditorActions.h"

#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"

#include "../../common/engine/gameResource.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneGraphBlock.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneDebugger.h"
#include "../../common/game/storySceneCameraDefinition.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"

#include "storyScenePreviewPlayer.h"
#include "dialogUtilityHandlers.h"
#include "dialogPreview.h"
#include "dialogEditorDialogsetPanel.h"
#include "voice.h"
#include "lipsyncDataSceneExporter.h"
#include "animFriend.h"
#include "dialogAnimationParameterEditor.h"
#include "dialogEditorChangeSlotPropertyEditor.h"
#include "../../common/game/storySceneUtils.h"
#include "../../common/core/feedback.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

void CEdSceneEditor::RefreshCamerasList()
{
	TDynArray< StorySceneCameraDefinition > cameras = m_storyScene->GetCameraDefinitions();

	class Pred
	{
	public:
		Bool operator()( const StorySceneCameraDefinition& a, const StorySceneCameraDefinition& b ) const
		{
			return a.m_cameraName.AsString() < b.m_cameraName.AsString();
		}
	};

	Sort( cameras.Begin(), cameras.End(), Pred() );

	m_customCamerasList->Freeze();
	m_customCamerasList->ClearAll();

	for( TDynArray< StorySceneCameraDefinition >::const_iterator cameraIter = cameras.Begin();
		cameraIter != cameras.End(); ++cameraIter )
	{
		const StorySceneCameraDefinition& camera = *cameraIter;
		m_customCamerasList->InsertItem( 0, camera.m_cameraName.AsString().AsChar() );
	}

	m_customCamerasList->Thaw();
}

void CEdSceneEditor::OnCameraModeChanged()
{
	/*CBehaviorGraphStack* behaviorStack ;
	if( m_dialogPreviewCamera && m_dialogPreviewCamera->GetRootAnimatedComponent() && m_dialogPreviewCamera->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		behaviorStack = m_dialogPreviewCamera->GetRootAnimatedComponent()->GetBehaviorStack();
		if( m_cameraMode == SPCM_PREVIEW )
		{
			behaviorStack->SetBehaviorVariable( CCamera::DOF_OVERRIDE, 0.0f );
			ApplyCurrentPreviewState();
		}
		else
		{
			behaviorStack->SetBehaviorVariable ( CCamera::DOF_OVERRIDE, 1.0f );
			UpdateCameraFromDefinition();
		}
	}*/

	m_cameraProperties->SetReadOnly( m_camera.IsPreviewMode() && !m_camera.IsGameplayMode() );

	m_preview->ToggleCameraTool( m_camera.IsPreviewMode(), m_camera.IsFreeMode(), m_camera.IsEditMode() );
	m_timeline->ToggleCameraTool( m_camera.IsPreviewMode(), m_camera.IsFreeMode(), m_camera.IsEditMode() );

	m_preview->SlowDownCameraInputs( m_camera.IsEditMode() );
}

void CEdSceneEditor::OnCustomCamerasListDblClick( wxMouseEvent & event )
{
	const CStorySceneSection* section = m_controller.GetCurrentSection();
	if( section && section->IsA< CStorySceneCutsceneSection >() )
	{
		wxMessageBox( TXT( "Camera event can't be created in cutscene section as camera is controlled by cutscene itself." ) );
		return;
	}

	const long index = m_customCamerasList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( index != -1 )
	{
		String cameraName = m_customCamerasList->GetItemText( index );

		m_timeline->CreateCustomCameraInstance( CName( cameraName ) );
	}
}

void CEdSceneEditor::OnCameraDefinitionChanged( const StorySceneCameraDefinition* def )
{
	if ( m_camera.IsPreviewMode() )
	{
		m_controller.ReloadCamera( def );
	}
}

void CEdSceneEditor::OnCameraDefinitionChanged( wxCommandEvent& event )
{
	if ( !m_camera.IsPreviewMode() )
	{
		UpdateCameraFromDefinition( m_camera.GetSelectedDefinition() );
	}

	const CEdPropertiesPage::SPropertyEventData* propertyData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	if ( propertyData && propertyData->m_page == m_cameraProperties )
	{ 
		if ( const StorySceneCameraDefinition* def = propertyData->m_typedObject.As< StorySceneCameraDefinition >() )
		{
			OnCameraDefinitionChanged( def );
		}
	}

}

void CEdSceneEditor::OnCameraSelectedFromList( wxCommandEvent& event )
{
	const long item = m_customCamerasList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if ( item != -1 && !m_camera.IsPreviewMode() )
	{
		CName cameraName( m_customCamerasList->GetItemText( item ) );

		StorySceneCameraDefinition* cameraDefinition = m_storyScene->GetCameraDefinition( cameraName );
		ASSERT( cameraDefinition );

		if( m_camera.IsEditMode() && m_timeline->IsAnySelected() && cameraDefinition )
		{
			TDynArray<CStorySceneEvent*> events;
			m_timeline->GetSelectedEvents( events );
			for ( Uint32 i = 0; i < events.Size() ; ++i  )
			{
				CStorySceneEventCustomCamera* cam1 = Cast<CStorySceneEventCustomCamera>( events[i] );
				if ( cam1 )
				{
					cam1->SetCameraDefinition( *cameraDefinition );
					continue;
				}
				CStorySceneEventCustomCameraInstance* cam2 = Cast<CStorySceneEventCustomCameraInstance>( events[i] );
				if ( cam2 )
				{
					cam2->SetCustomCameraName( cameraName );
				}
			}		
		}
		ActivateCameraFromDefinition( cameraDefinition );
	}
	else if ( item != -1 )
	{
		CName cameraName( m_customCamerasList->GetItemText( item ) );

		StorySceneCameraDefinition* cameraDefinition = m_storyScene->GetCameraDefinition( cameraName );
		ASSERT( cameraDefinition );

		m_cameraProperties->SetObject( cameraDefinition );
	}
}

void CEdSceneEditor::OnRightClickCameraFromList( wxCommandEvent& event )
{
	wxMenu* menu = new wxMenu();

	menu->Append( 1113, wxT( "Duplicate camera" ) );
	menu->Connect( 1113, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdSceneEditor::OnDuplicateCamera ), NULL, this );

	menu->AppendSeparator();

	menu->Append( 1112, wxT( "Delete item" ) );
	menu->Connect( 1112, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler( CEdSceneEditor::OnDeleteCameraFromList ), NULL, this );
	
	menu->AppendSeparator();
	menu->Append( 1111, wxT( "Delete all items" ) );
	menu->Connect( 1111, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneEditor::OnDeleteAllCamerasFromList ), NULL, this );
	menu->Append( 1115, wxT( "Delete unused items" ) );
	menu->Connect( 1115, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneEditor::OnDeleteUnusedCameras ), NULL, this );

	menu->AppendSeparator();
	menu->Append( 1114, wxT( "Disable adjust for all cameras" ) );
	menu->Connect( 1114, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneEditor::OnDisableAdjustInCameraDefs ), NULL, this );
	menu->Append( 1114, wxT( "Enable adjust for all cameras" ) );
	menu->Connect( 1114, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneEditor::OnEnableAdjustInCameraDefs ), NULL, this );

	PopupMenu( menu );
}

void CEdSceneEditor::OnDeleteAllCamerasFromList( wxCommandEvent& event )
{
	auto camDefs = m_storyScene->GetCameraDefinitions();
	for( StorySceneCameraDefinition& def : camDefs )
	{
		if( ! DeleteCameraFromList( def.m_cameraName ) )
		{
			break;
		}
	}	
	RefreshCamerasList();
}

void CEdSceneEditor::OnDeleteUnusedCameras( wxCommandEvent& event )
{
	if ( !GFeedback->AskYesNo( TXT("Are you sure you want to remove unused cameras?") ) )
	{
		return;
	}

	auto camDefs = m_storyScene->GetCameraDefinitions();
	Int32 deletedCameras = 0;
	for( StorySceneCameraDefinition& def : camDefs )
	{
		String dummy;
		if ( !IsCameraUsedInEvent( def.m_cameraName, dummy ) )
		{
			m_storyScene->DeleteCameraDefinition( def.m_cameraName );
			++deletedCameras;
		}
	}	
	RefreshCamerasList();
	GFeedback->ShowMsg( TXT("Unused cameras removed"), TXT("Removed %i unused cameras"), deletedCameras );
}

void CEdSceneEditor::OnDuplicateCamera( wxCommandEvent& event ) 
{ 
	DuplicateCamera(); 
}

void CEdSceneEditor::DuplicateCamera()
{
	const long item = m_customCamerasList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if( item == -1 )
	{
		return;
	}
	
	String camName = m_customCamerasList->GetItemText( item );
	const StorySceneCameraDefinition* refCameraDefinition = m_storyScene->GetCameraDefinition( CName(camName) );
	
	if ( !refCameraDefinition )
	{
		ASSERT( refCameraDefinition );
		return;
	}

	StorySceneCameraDefinition cameraDefinition = *refCameraDefinition;

	// Create new name check if camera with this name is not already added
	camName += TXT("_c");
	for( int i = 0 ; m_storyScene->GetCameraDefinition( CName(camName) ) != NULL ; i++ )
	{
		camName = m_customCamerasList->GetItemText( item ) + TXT("_c") + ToString(i);
	}
	
	cameraDefinition.m_cameraName = CName(camName);
	cameraDefinition.ParseCameraParams();
	m_storyScene->AddCameraDefinition( cameraDefinition );

	RefreshCamerasList();
}

Bool CEdSceneEditor::CreateCustomCameraFromView()
{
	// Deselect current selection from  cameras list
	const long itemToDeselect = m_customCamerasList->GetNextItem(-1, wxLIST_NEXT_ALL,  wxLIST_STATE_SELECTED);
	if ( itemToDeselect != -1 )
	{
		m_customCamerasList->SetItemState(itemToDeselect, 0, wxLIST_STATE_SELECTED);
	}

	StorySceneCameraDefinition cameraDef;
	SaveCameraToDefinition( &cameraDef );

	// DIALOG_TOMSIN_TODO - to w srodku tworzony jest event i callback ze stworzony do editora - to do zmiany, nie ma ukrywych tworzen obiektow!
	return m_timeline->CreateCustomCameraEvent( cameraDef ) != NULL;
}

Bool CEdSceneEditor::CreateCustomCameraEnhancedBlendInterpolated()
{
	// Deselect current selection from  cameras list
	const long itemToDeselect = m_customCamerasList->GetNextItem(-1, wxLIST_NEXT_ALL,  wxLIST_STATE_SELECTED);
	if ( itemToDeselect != -1 )
	{
		m_customCamerasList->SetItemState(itemToDeselect, 0, wxLIST_STATE_SELECTED);
	}

	const Float position = m_timeline->GetCurrentPosition();
	DialogTimelineItems::CTimelineItemEnhancedCameraBlend* blend = dynamic_cast< DialogTimelineItems::CTimelineItemEnhancedCameraBlend* >( m_timeline->FindDurationBlendItemByEventClass( ClassID< CStorySceneEventEnhancedCameraBlend >(), position ) );
	if ( !blend )
	{
		return false;
	}
	CStorySceneEventEnhancedCameraBlend* blendEvent = static_cast< CStorySceneEventEnhancedCameraBlend* >( blend->GetEvent() );

	const Float localPosition = position - blend->GetStart();
	StorySceneCameraDefinition cameraDef;
	blendEvent->GetCameraStateAt( localPosition, &cameraDef );

	return m_timeline->CreateCustomCameraEvent( cameraDef, blend ) != NULL;
}

Bool CEdSceneEditor::IsCameraUsedInEvent( CName name, String& sectionName ) const
{
	Uint32 sectionsNum = m_storyScene->GetNumberOfSections();
	for( Uint32 i = 0; i < sectionsNum; i++ )
	{
		auto events = m_storyScene->GetSection(i)->GetEventsFromAllVariants();
		for( const CStorySceneEvent* evt : events )
		{
			if ( const CStorySceneEventCustomCameraInstance* cam = Cast< const CStorySceneEventCustomCameraInstance >(evt) )
			{
				if ( cam->GetCustomCameraName() == name )
				{
					sectionName = m_storyScene->GetSection(i)->GetName();
					return true;
				}
			}
		}
	}
	return false;
}

Bool CEdSceneEditor::DeleteCameraFromList( CName name )
{
	String sectionName;
	if( IsCameraUsedInEvent( name, sectionName ) )
	{
		String message = TXT( "Cant delete item " ) + name.AsString() + TXT( ". Camera used in event in section " ) + sectionName;
		GFeedback->ShowError( message.AsChar() );
		return false;
	}

	return m_storyScene->DeleteCameraDefinition( name );
}


void CEdSceneEditor::OnDeleteCameraFromList( wxCommandEvent& event )
{
	const long item = m_customCamerasList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if( item != -1 )
	{
		CName name( m_customCamerasList->GetItemText( item ).c_str() );
		
		if( DeleteCameraFromList( name ) )
		{
			RefreshCamerasList();
		}		
	}
}

void CEdSceneEditor::ActivateCameraFromList( CName cameraName )
{
	long itemToDeselect = -1, itemToSelect = -1;
	itemToDeselect = m_customCamerasList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	itemToSelect = m_customCamerasList->FindItem( itemToSelect, cameraName.AsString().AsChar());

	if ( itemToDeselect != itemToSelect )
	{
		m_customCamerasList->SetItemState( itemToDeselect, 0, wxLIST_STATE_SELECTED );
		m_customCamerasList->SetItemState( itemToSelect, wxLIST_STATE_SELECTED| wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED );
		m_customCamerasList->EnsureVisible( itemToSelect );
	}
}

void CEdSceneEditor::ActivateCustomCamera( CStorySceneEventCustomCamera* camera )
{
	const long itemToDeselect = m_customCamerasList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	m_customCamerasList->SetItemState( itemToDeselect, 0, wxLIST_STATE_SELECTED );

	StorySceneCameraDefinition* cameraDefinition = camera->GetCameraDefinition();
	ActivateCameraFromDefinition( cameraDefinition, camera );
}

void CEdSceneEditor::OnTimeline_LockCamera( Bool lock )
{
	m_isCameraLocked = lock;
}

void CEdSceneEditor::ActivateCameraFromDefinition( StorySceneCameraDefinition* cameraDef, CStorySceneEvent* cameraDefOwner )
{
	ASSERT( !m_camera.IsPreviewMode() );
	ASSERT( cameraDef );
	if ( cameraDef )
	{
		m_camera.SelectDefinition( cameraDef, cameraDefOwner );

		Bool adjusted = UpdateCameraFromDefinition( m_camera.GetSelectedDefinition() );
		m_controller.SetCameraAdjustedDebugFrame( adjusted );
		m_cameraProperties->SetObject( m_camera.GetSelectedDefinition() );
	}
}

CActor* CEdSceneEditor::GetActorOnDialogsetSlot( CName slotName, Uint32 slotNumber /*= 0 */ )
{
	const CStorySceneDialogsetInstance* dialogset = GetCurrentDialogsetInstance();
	if ( dialogset )
	{
		CStorySceneDialogsetSlot* slot = dialogset->GetSlotByName( slotName );
		if( !slot )
		{
			slot = dialogset->GetSlotBySlotNumber( slotNumber );
		}
		else
		{
			return GetSceneActor( slot->GetActorName() );	
		}
	}

	return nullptr;
}

void CEdSceneEditor::OnDisableAdjustInCameraDefs( wxCommandEvent& event )
{
	TDynArray< StorySceneCameraDefinition>& camDefs = m_storyScene->GetCameraDefinitions();

	for ( Uint32 i = 0; i < camDefs.Size(); i++ )
	{
		camDefs[i].m_cameraAdjustVersion = 0;
	}
}

void CEdSceneEditor::OnEnableAdjustInCameraDefs( wxCommandEvent& event )
{
	TDynArray< StorySceneCameraDefinition>& camDefs = m_storyScene->GetCameraDefinitions();

	for ( Uint32 i = 0; i < camDefs.Size(); i++ )
	{
		camDefs[i].m_cameraAdjustVersion = 3;
	}
}

void CEdSceneEditor::SaveActorHeigthInCameraDef( StorySceneCameraDefinition* cameraDefinition )
{
	CActor* srcActor = GetActorOnDialogsetSlot( cameraDefinition->m_sourceSlotName, cameraDefinition->m_genParam.m_sourceSlot  );
	if ( srcActor )
	{	
		CName idleAnim;
		m_controller.GetPlayer()->GetActorIdleAnimation( srcActor->GetVoiceTag() , idleAnim );
		cameraDefinition->m_sourceEyesHeigth = StorySceneUtils::CalcEyesPosLS( srcActor, idleAnim ).Z;
	}		
	CActor* dstActor = GetActorOnDialogsetSlot( cameraDefinition->m_targetSlotName, cameraDefinition->m_genParam.m_targetSlot );
	if ( dstActor )
	{	
		CName idleAnim;
		m_controller.GetPlayer()->GetActorIdleAnimation( dstActor->GetVoiceTag() , idleAnim );
		cameraDefinition->m_targetEyesLS = StorySceneUtils::CalcEyesPosLS( dstActor, idleAnim );
	}	
	cameraDefinition->m_cameraAdjustVersion = 2;
}

void CEdSceneEditor::OnTimeline_SaveCameraToDefinition( StorySceneCameraDefinition* cameraDefinition )
{
	SaveCameraToDefinition( cameraDefinition );
}

void CEdSceneEditor::SaveCameraToDefinition( StorySceneCameraDefinition* cameraDefinition )
{
	if ( cameraDefinition )
	{
		Matrix sceneL2W;
		m_controller.GetCurrentScenePlacement().CalcLocalToWorld( sceneL2W );
		SaveActorHeigthInCameraDef( cameraDefinition );
		m_camera.WriteTo( cameraDefinition, sceneL2W );
	}

	m_cameraProperties->RefreshValues();
}

Bool CEdSceneEditor::UpdateCameraFromDefinition( StorySceneCameraDefinition* cameraDefinition )
{
	if ( m_isCameraLocked )
	{
		return false;
	}

	if ( cameraDefinition )
	{
		EngineTransform camAdjustedLS;
		Matrix cameraWS;
		Bool adjusted = m_controller.GetPlayer()->AdjustCameraForActorHeight( *cameraDefinition, &camAdjustedLS, &cameraWS );	
		cameraDefinition->m_cameraTransform = camAdjustedLS;
		SaveActorHeigthInCameraDef( cameraDefinition );
		m_camera.ReadFrom( cameraDefinition, cameraWS );
		m_controller.ReloadCamera( cameraDefinition );
		return adjusted;
	}
	return false;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
