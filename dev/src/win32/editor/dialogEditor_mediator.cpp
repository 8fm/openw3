
#include "build.h"
#include "dialogEditor.h"

#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"

#include "../../common/engine/extAnimCutsceneDialogEvent.h"
#include "../../common/engine/mimicComponent.h"

#include "../../common/game/storySceneGraph.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/game/storySceneEventControlRig.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/engine/gameResource.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/game/storySceneSectionPlayingPlan.h"
#include "../../common/game/sceneAnimEvents.h"
#include "../../common/game/storySceneEventInterpolation.h"
#include "../../common/game/storySceneEventLightProperties.h"

#include "dialogEditorActions.h"
#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"
#include "propertiesPageComponent.h"
#include "storyScenePreviewPlayer.h"
#include "dialogUtilityHandlers.h"
#include "dialogPreview.h"
#include "dialogEditorDialogsetPanel.h"
#include "voice.h"
#include "lipsyncDataSceneExporter.h"
#include "animFriend.h"
#include "dialogAnimationParameterEditor.h"
#include "dialogEditor_hacks.h"
#include "dialogEditorUtils.h"
#include "dialogTimeline_includes.h"
#include "controlRigPanel.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/tagManager.h"
#include "../../common/core/mathUtils.h"
#include "../../common/engine/particleComponent.h"
#include "../../common/engine/spotLightComponent.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/localizationManager.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

extern CGatheredResource resStoryIcon;
extern CGatheredResource resLightIcon;
 
//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnPreview_ViewportTick( Float timeDelta )
{
	const Float before = m_controller.GetSectionTime();

	CEdSceneEditor_Hacks::OnPreview_ViewportTick_ApplyDof( this );

	if ( m_worldCtrl.IsPreviewMode() )
	{
		ProcessViewportTick( timeDelta );
	}

	const Float after = m_controller.GetSectionTime();
	m_controlRigPanel->OnTick( before, after, timeDelta );
}

Bool CEdSceneEditor::OnPreview_CalculateCamera( IViewport* view, CRenderCamera &camera ) const
{
	if ( m_controller.IsValid() && m_worldCtrl.IsPreviewMode() )
	{
		return ProcessCalculateCamera( view, camera );
	}
	return false;
}

void CEdSceneEditor::OnPreview_GenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_worldCtrl.IsPreviewMode() )
	{
		ProcessGenerateFragments( view, frame );
	}

	const CStorySceneDialogsetInstance * dialogset = GetCurrentDialogsetInstance();
	if (  dialogset != NULL && frame->GetFrameInfo().IsShowFlagOn( SHOW_Sprites ) )
	{
		for ( Uint32 i = 0; i < dialogset->GetSlots().Size(); ++i )
		{
			CStorySceneDialogsetSlot* slot = dialogset->GetSlots()[ i ];
			if( !slot )
			{
				continue;
			}
			EngineTransform slotPlacement;
			CStorySceneDirectorPlacementHelper	placementHelper;
			EngineTransform slotTransform;
			placementHelper.Init( m_controller.GetPlayer() );
			placementHelper.GetSlotPlacement( slot, dialogset, slotPlacement );
			Matrix slotLocalToWorld;
			slotPlacement.CalcLocalToWorld( slotLocalToWorld );

			frame->AddDebugSphere( slotPlacement.GetPosition(), 0.25f, Matrix::IDENTITY, Color::BLUE );
			frame->AddDebugArrow( slotLocalToWorld, Vector::EY, 1.0f, Color::GREEN );
			frame->AddDebugText( slotPlacement.GetPosition(), slot->GetSlotName().AsString(), 0, 0, true, Color::LIGHT_BLUE );
		}
	}

	// HACK: make particle systems selectable
	// (Is there a better way to do this than to draw an invisible sprite?)
	static CBitmapTexture* effectIcon = resStoryIcon.LoadAndGet< CBitmapTexture >();	
	if( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		// effects
		const TDynArray< THandle< CEntity > >& effectEntities = m_actorsProvider->GetEffectsForEditor();
		for( Uint32 i=0; i<effectEntities.Size(); ++i )
		{
			if( CEntity* entity = effectEntities[ i ].Get() )
			{
				for ( ComponentIterator< CParticleComponent > it( entity ); it; ++it )
				{
					CParticleComponent* pc = (*it);
					frame->AddSprite( pc->GetWorldPosition(), 0.25f, Color::WHITE, pc->GetHitProxyID(), effectIcon );
				}
			}
		}
	}

	// light icons
	static CBitmapTexture* lightIcon = resLightIcon.LoadAndGet< CBitmapTexture >();
	const TDynArray< THandle< CEntity > >& lightEntities = m_actorsProvider->GetLightsForEditor();
	for( Uint32 i=0; i<lightEntities.Size(); ++i )
	{
		if( CEntity* entity = lightEntities[ i ].Get() )
		{
			for ( ComponentIterator< CLightComponent > it( entity ); it; ++it )
			{
				CLightComponent* light = (*it);
				frame->AddSprite( light->GetWorldPosition(), 0.25f, Color::WHITE, light->GetHitProxyID(), lightIcon );

				// light is selected?
				if( m_keyframeCtrl.m_selectedEntity == entity )
				{
					if( light->IsA< CSpotLightComponent >() )
					{
						// spot light
						CSpotLightComponent* spotLight = Cast< CSpotLightComponent >( light );
						frame->AddDebugCone( light->GetLocalToWorld(), light->GetRadius(), spotLight->GetInnerAngle(), spotLight->GetOuterAngle() );
					}
					else
					{
						// point light
						frame->AddDebugSphere( light->GetWorldPosition(), light->GetRadius(), Matrix::IDENTITY, Color::WHITE );
					}
				}
			}
		}
	}
}

Bool CEdSceneEditor::OnPreview_ViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data, Bool& moveViewportCam )
{
	return ProcessViewportInput( view, key, action, data, moveViewportCam );
}

void CEdSceneEditor::OnPreview_FreeCameraMode()
{
	m_camera.SetFreeMode();

	SaveCameraToDefinition( m_camera.GetSelectedDefinition() );

	m_cameraProperties->SetObject( m_camera.GetSelectedDefinition() );
	
	OnCameraModeChanged();
}

void CEdSceneEditor::OnPreview_PreviewCameraMode()
{
	if ( m_camera.IsEditMode() )
	{
		RefreshPlayer();
	}

	m_camera.SetPreviewMode();

	OnCameraModeChanged();
}

void CEdSceneEditor::OnPreview_EditCameraMode()
{
	m_cameraProperties->SetNoObject();

	m_timeline->CancelSelection();

	long itemToDeselect = GetCustomCamerasList()->GetNextItem(-1, wxLIST_NEXT_ALL,  wxLIST_STATE_SELECTED);
	GetCustomCamerasList()->SetItemState(itemToDeselect, 0, wxLIST_STATE_SELECTED);

	m_camera.SetEditMode();

	OnCameraModeChanged();
}

EScenePreviewCameraMode CEdSceneEditor::OnPreview_GetCameraMode()
{
	return m_camera.GetCurrentMode();
}

void CEdSceneEditor::OnPreview_CameraMoved()
{
	OnViewportCameraMoved();
}

Bool CEdSceneEditor::OnPreview_CreateCustomCameraFromView()
{
	return CreateCustomCameraFromView();
}

Bool CEdSceneEditor::OnPreview_PlayInMainWorld( Bool ingame )
{
	return PlayInMainWorld( ingame );
}

void CEdSceneEditor::OnPreview_OpenCutscene()
{
	CStorySceneCutsceneSection* csSection = Cast< CStorySceneCutsceneSection >( m_controller.GetCurrentSection() );
	if ( csSection )
	{
		CCutsceneTemplate* csTempl = csSection->GetCsTemplate();
		if ( csTempl )
		{
			TDynArray< SCutsceneActorLine > lines;
			csSection->GetDialogLines( lines );

			CEdCutsceneEditor* editor = new CEdCutsceneEditor( 0, csTempl, &lines );
			editor->Show();
		}
	}
}

void CEdSceneEditor::OnPreview_AddLight()
{
	String lightName;

	const CStorySceneLight* lastLight( nullptr );
	const TDynArray< CStorySceneLight* >& sceneLights = const_cast< TDynArray< CStorySceneLight* >& >( m_storyScene->GetSceneLightDefinitions() );
	if ( sceneLights.Size() > 0 )
	{
		lastLight = sceneLights[ sceneLights.SizeInt()-1 ];
		
		String name = lastLight->m_id.AsString();
		
		size_t place;
		if ( name.FindCharacter( '_', place ) )
		{
			String left = name.StringBefore( TXT("_"), true );
			String right = name.StringAfter( TXT("_"), true );

			Int32 number = 0;
			if ( FromString( right, number ) )
			{
				number++;
				right = ToString( number );

				lightName = left + TXT("_") + right;
			}
		}
	}

	if ( InputBox( m_preview, TXT("Add light"), TXT("Light name:"), lightName ) )
	{
		CStorySceneLight* ld = CreateObject< CStorySceneLight >( m_storyScene );
		if ( lastLight )
		{
			ld->CopyFrom( *lastLight );
		}

		ld->m_id = CName( lightName );

		m_storyScene->AddLightDefinition( ld );
		RebuildLights();
	}
}

void CEdSceneEditor::OnPreview_GenerateFragmentsForCurrentCamera( IViewport* view, CRenderFrame* frame )
{
	if ( DialogTimelineItems::CTimelineItemEnhancedCameraBlend* blend = dynamic_cast< DialogTimelineItems::CTimelineItemEnhancedCameraBlend* >( m_timeline->FindDurationBlendItemByEventClass( ClassID< CStorySceneEventEnhancedCameraBlend >(), m_timeline->GetCurrentPosition() ) ) )
	{
		blend->GenerateCameraFragments( view, frame );
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnWorldCtrl_ViewportTick( Float timeDelta )
{
	ProcessViewportTick( timeDelta );
}

Bool CEdSceneEditor::OnWorldCtrl_CalculateCamera( IViewport* view, CRenderCamera &camera ) const
{
	return ProcessCalculateCamera( view, camera );
}

void CEdSceneEditor::OnWorldCtrl_GenerateFragments( IViewport *view, CRenderFrame *frame )
{
	ProcessGenerateFragments( view, frame );
}

void CEdSceneEditor::OnWorldCtrl_PreModeChanged()
{
	CCurveEntity::DeleteEditors( GetWorld(), false );

	m_controller.DeactivateCustomEnv();

	m_helperEntitiesCtrl.DeselectAllHelpers();
}

void CEdSceneEditor::OnWorldCtrl_PostModeChanged()
{
	// remember current section (as this will be forgotten during following operations)
	const CStorySceneSection* section = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( section, "CEdSceneEditor::OnWorldCtrl_PostModeChanged(): current section is nullptr." );

	m_actorsProvider->Rebuild( m_storyScene );
	m_helperEntitiesCtrl.Refresh();
	m_keyframeCtrl.RecreateDefaultHelper();

	RebuildPlayer( section );
	m_timeline->RequestRebuild();
}

void CEdSceneEditor::OnWorldCtrl_CameraMoved()
{
	OnViewportCameraMoved();
}

Bool CEdSceneEditor::OnWorldCtrl_ViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Bool moveViewportCam = true;
	if ( ProcessViewportInput( view, key, action, data, moveViewportCam ) )
	{
		return true;
	}

	if ( moveViewportCam )
	{
		m_camera.OnWorldViewportInput( key, data );
	}

	return false;
};

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnScreenplayPanel_ElementPanelFocus( const CStorySceneSection* section, CStorySceneElement* e )
{
	m_controlRequest.RequestPropBrowserObject( e );
	m_controller.Pause();
}

void CEdSceneEditor::OnScreenplayPanel_ChoiceLineChildFocus( const CStorySceneSection* section, CStorySceneChoiceLine* line )
{
	m_controlRequest.RequestPropBrowserObject( line );
	m_controller.Pause();
}

void CEdSceneEditor::OnScreenplayPanel_SectionPanelSectionFocus( const CStorySceneSection* section )
{
	RequestSection( section );
	m_controller.Pause();
}

void CEdSceneEditor::OnScreenplayPanel_SectionPanelSectionChildFocus( CStorySceneSection* section )
{
	RequestSection( section );

	wxWindow* focusedWindow = wxWindow::FindFocus();
	if ( focusedWindow && focusedWindow->IsKindOf( CLASSINFO( wxTextCtrl ) ) == true )
	{
		wxTextCtrl* textField = static_cast<wxTextCtrl*>( focusedWindow );
		textField->SetInsertionPointEnd();
		m_controlRequest.RequestPropBrowserObject( section );
	}

	m_controller.Pause();
}

void CEdSceneEditor::OnScreenplayPanel_ChoicePanelClick( CStorySceneChoice* ch )
{
	m_controlRequest.RequestPropBrowserObject( ch );
}

void CEdSceneEditor::OnScreenplayPanel_PostChangeSceneElements()
{
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
	m_controller.Pause();
}

void CEdSceneEditor::OnScreenplayPanel_RemoveSection( CStorySceneSection* section )
{
	if ( m_controller.GetCurrentSection() == section )
	{
		CStorySceneSection* newSection = GetPrecedingSection( section );
		if ( newSection )
		{
			m_controlRequest.RequestSection( newSection );
			m_controlRequest.RequestRefreshTimeline();
			m_controller.Pause();
		}
		else
		{
			m_controlRequest.RequestFreeze();
			m_controlRequest.RequestRefreshTimeline();
			m_controller.Pause();
		}

		m_storyScene->RemoveSection( section );
	}
}

CName CEdSceneEditor::OnScreenplayPanel_GetPrevSpeakerName( const CStorySceneElement* currElement ) const
{
	return m_controller.GetPrevSpeakerName( currElement );
}

const CStorySceneLine* CEdSceneEditor::OnScreenplayPanel_GetPrevLine( const CStorySceneElement* currElement )
{
	return m_controller.GetPrevLine( currElement );
}

void CEdSceneEditor::OnScreenplayPanel_ApplyChanges()
{
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnScreenplayPanel_RequestRebuildImmediate()
{
	RebuildPlayer();
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnDefinitions_PropertiesChanged( wxCommandEvent& event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	String propName = eventData->m_propertyName.AsString();

	if( propName == TXT( "sceneTemplates" ) ||
		propName == TXT( "sceneProps" ) ||
		propName == TXT( "sceneEffects" ) ||
		propName == TXT( "sceneLights" ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Definitions Property Changed: %s" ), propName.AsChar() );

		OnPropListChanged( event );
		{
			m_actorsProvider->Rebuild( m_storyScene );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnGraph_AddSceneBlock( CStorySceneGraphBlock* block )
{
	RebuildPlayer();
	m_timeline->RefreshSection();
}

void CEdSceneEditor::OnGraph_AddSceneBlocks( const TDynArray< CStorySceneGraphBlock* >& blocks )
{
	RebuildPlayer();

	m_timeline->RefreshSection();
}

void CEdSceneEditor::OnGraph_RemoveSceneBlocks( const TDynArray< CStorySceneGraphBlock* >& blocks )
{
	const CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnGraph_RemoveSceneBlocks(): currentSection must not be nullptr." );

	// check if current section was removed
	Bool removedCurrentSection = false;
	for( const CStorySceneGraphBlock* block : blocks )
	{
		if( CStorySceneSection* section = Cast< CStorySceneSection >( block->GetControlPart() ) )
		{
			if( currentSection == section )
			{
				removedCurrentSection = true;
				break;
			}
		}
	}

	// if current section was removed then choose some other section to which we will switch
	RED_FATAL_ASSERT( m_storyScene->GetNumberOfSections() > 0, "CEdSceneEditor::OnGraph_RemoveSceneBlocks(): scene has to have at least one section." );
	const CStorySceneSection* sectionToSwitchTo = removedCurrentSection? m_storyScene->GetSection( 0 ) : currentSection;

	RebuildPlayer( sectionToSwitchTo );
	m_timeline->SetSection( sectionToSwitchTo );
}

Int32 CEdSceneEditor::OnGraph_GetNumBlocks( const CClass* blockClass ) const
{
	Int32 count = 0;

	const TDynArray< CGraphBlock* >& blocks = m_storyScene->GetGraph()->GraphGetBlocks();

	const Uint32 size = blocks.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CGraphBlock* block = blocks[ i ];
		if ( block && block->IsA( blockClass ) )
		{
			++count;
		}
	}

	return count;
}

Bool CEdSceneEditor::OnGraph_IsBlockActivated( const CStorySceneControlPart* cp ) const
{
	return m_flowCtrl.IsBlockActivated( cp );
}

Float CEdSceneEditor::OnGraph_GetBlockActivationAlpha( const CStorySceneControlPart* cp ) const
{
	return m_flowCtrl.GetBlockActivationAlpha( cp );
}

void CEdSceneEditor::OnGraph_ToggleSelectedInputLinkElement()
{
	m_flowCtrl.RecalcFlow( Hack_GetPlayer() );
}

void CEdSceneEditor::OnGraph_ToggleSelectedOutputLinkElement()
{
}

CName CEdSceneEditor::OnGraph_GetCurrentDialogsetName() const
{
	return m_controller.GetCurrentDialogsetInstanceIfValid() ? m_controller.GetCurrentDialogsetInstanceIfValid()->GetName() : CName::NONE;
}

//////////////////////////////////////////////////////////////////////////

const CStorySceneSection* CEdSceneEditor::OnTempLipsyncDlg_GetCurrentSection() const
{
	return m_controller.GetCurrentSection();	
}

CStoryScene* CEdSceneEditor::OnTempLipsyncDlg_GetScene() const
{
	return GetStoryScene();
}

void CEdSceneEditor::OnTempLipsyncDlg_RefreshLipsyncs()
{
	// TODO: maybe we could refresh lipsyncs in a more lightweight way?
	RebuildPlayer();
	m_controlRequest.RequestRefreshTimeline();
}

//////////////////////////////////////////////////////////////////////////

const THashMap< CName, THandle< CEntity > >& CEdSceneEditor::OnEditMimicsDialog_GetActorMap() const
{
	return m_controller.GetActorMap();
}

CActor* CEdSceneEditor::OnEditMimicsDialog_GetSceneActor( CName actorName )
{
	return GetSceneActor( actorName );
}

//////////////////////////////////////////////////////////////////////////

const CStorySceneSection* CEdSceneEditor::OnImportW2StringsDlg_GetCurrentSection() const
{
	return m_controller.GetCurrentSection();
}

void CEdSceneEditor::OnImportW2StringsDlg_RefreshScreenplay() const
{
	m_screenplayPanel->RefreshDialog();
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnDialogsetPanel_SlotPropertyChanged()
{
	RebuildPlayer_Lazy();
}

void CEdSceneEditor::OnDialogsetPanel_PropertyChanged()
{
	m_actorsProvider->Refresh( m_storyScene );
}

//////////////////////////////////////////////////////////////////////////

CAnimatedComponent* CEdSceneEditor::OnCameraAnimationSelector_GetCameraComponent()
{
	return GetCameraComponent();
}

CAnimatedComponent* CEdSceneEditor::OnMimicsAnimationSelector_GetHeadComponentForActor( const CName& actorName )
{
	return GetHeadComponentForActor( actorName );
}

CAnimatedComponent* CEdSceneEditor::OnMimicsGestureAnimationSelector_GetHeadComponentForActor( const CName& actorName )
{
	return GetHeadComponentForActor( actorName );
}

CAnimatedComponent* CEdSceneEditor::OnMimicPoseSelector_GetHeadComponentForActor( const CName& actorName )
{
	return GetHeadComponentForActor( actorName );
}

CAnimatedComponent*	CEdSceneEditor::OnDialogAnimationFilter_GetBodyComponentForActor( const CName& actorName )
{
	return GetAnimatedComponentForActor( actorName );
}

CAnimatedComponent*	CEdSceneEditor::OnDialogAnimationFilter_GetMimicsComponentForActor( const CName& actorName )
{
	return GetHeadComponentForActor( actorName );
}

Bool CEdSceneEditor::OnDialogAnimationFilter_GetPreviousActorAnimationState( const CName& actorName, SStorySceneActorAnimationState& state ) const
{
	return m_controller.GetPreviousActorAnimationState( actorName, state );
}


const TDynArray<CName> CEdSceneEditor::OnVoicetagSelector_GetActorIds( Int32 actorTypes ) const
{
	return m_controller.GetActorIds( actorTypes );
}


Bool CEdSceneEditor::OnTimeline_GetPreviousActorAnimationState( const CName& actorName, SStorySceneActorAnimationState& state ) const
{
	return m_controller.GetPreviousActorAnimationState( actorName, state );
}

const CStorySceneSection* CEdSceneEditor::OnDialogLineSelector_GetCurrentSection() const
{
	return m_controller.GetCurrentSection();
}

Bool CEdSceneEditor::OnTimeline_UseLocColors() const
{
	return m_locCtrl.IsEnabled();
}

wxColor CEdSceneEditor::OnTimeline_FindLocColor( Uint32 stringId ) const
{
	return m_locCtrl.FindColorForLocString( stringId );
}

const IStorySceneElementInstanceData* CEdSceneEditor::OnTimeline_FindElementInstance( const CStorySceneElement* element ) const
{
	return m_controller.FindElementInstance( element );
}

Bool CEdSceneEditor::OnTimeline_IsTimelineEditingEnabled() const
{
	return TimelineEditingEnabled();
}

//////////////////////////////////////////////////////////////////////////

const THashMap< CName, THandle< CEntity > >& CEdSceneEditor::OnDebugger_GetActorMap()
{
	return m_controller.GetActorMap();
}

Bool CEdSceneEditor::OnDebugger_GetActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_controller.GetCurrentActorAnimationState( actor, out );
}

void CEdSceneEditor::OnDebugger_GetActorAnimationNames( const CName& actor, TDynArray< CName >& animations ) const
{
	m_controller.GetActorAnimationNames( actor, animations );
}

CAnimatedComponent* CEdSceneEditor::OnDebugger_GetAnimatedComponentForActor( const CName& actorName )
{
	return GetAnimatedComponentForActor( actorName );
}

Matrix CEdSceneEditor::OnDebugger_GetActorPosition( const CName& actorName ) const
{
	return m_controller.GetActorPosition( actorName );
}

Matrix CEdSceneEditor::OnDebugger_GetDialogPosition() const
{
	return m_controller.GetDialogPosition();
}

//////////////////////////////////////////////////////////////////////////

const CStorySceneLine* CEdSceneEditor::OnEvtGenerator_GetPrevLine( const CStorySceneElement* currElement )
{
	return m_controller.GetPrevLine( currElement );
}

void CEdSceneEditor::OnEvtGenerator_GetMarkedLockedEvents( TDynArray< CStorySceneEvent*>& marked, TDynArray< CStorySceneEvent*>& locked )
{
	m_timeline->GetMarkedLockedEvents( marked, locked );
}

Bool CEdSceneEditor::OnEvtGenerator_GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos /*= NULL */, Float* voiceEndPos /*= NULL*/ )
{
	return GetVoiceDataPositions( forLine, maxAmpPos,  voiceStartPos, voiceEndPos );
}

EngineTransform CEdSceneEditor::OnEvtGenerator_GetCurrentScenePlacement()
{
	return m_controller.GetCurrentScenePlacement();
}

Bool CEdSceneEditor::OnEvtGenerator_IsTimelineTrackLocked( const String& trackName ) const
{
	return m_timeline->IsTrackLocked( trackName );
}

void CEdSceneEditor::OnEvtGenerator_GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const
{
	GetMimicIdleData( data, poseWeight, actor );
}

void CEdSceneEditor::OnEvtGenerator_PinTimelineTrack( const String& track )
{
	m_timeline->PinTrack( track );
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnHelperEntity_SetEditString( const String& str )
{
	m_preview->SetEditString( str );
}


Bool CEdSceneEditor::OnHelperEntity_IsSomethingFromScene( const CComponent* c ) const
{
	// DIALOG_TOMSIN_TODO - na razie tylko aktorzy ale potem dowolne obiekty
	return m_actorsProvider->IsSomethingFromActors( c );
}

void CEdSceneEditor::OnHelperEntity_ClearSelection()
{
	m_keyframeCtrl.SelectEntity( nullptr, false );
}

CNode* CEdSceneEditor::OnHelperEntity_SelectObjectFromScene( const CComponent* c )
{
	CEntity* sceneEntity = StorySceneEditorUtils::ExtractEntityFromComponent( c );

	// de-select any select keyframes at this point
	m_timeline->CancelSelection();
	m_timeline->RefreshSection();	

	Bool processed = false;
	CNode* hEnt = nullptr;
	CStorySceneLight* lightDef = sceneEntity ? m_storyScene->GetSceneLightDefinition( CName( sceneEntity->GetName() ) ) : nullptr;
	if ( lightDef )
	{
		processed = true;

		//Uncommenting these lines will enable selecting lights in scene preview
		//
		//if ( !IsDetailPageOpened( PAGE_PROPERTY ) )
		//{
		//	SelectDetailPage( PAGE_PROPERTY );
		//}

		//SetPropertiesBrowserObject( lightDef, nullptr );

		//if( CActor* actor = GetSceneActor( lightDef->GetAttachmentActor() ) )
		//{
		//	SReferenceFrameSettings frameInfo( SReferenceFrameSettings::RF_LocalFrame );
		//	frameInfo.m_boneName = lightDef->GetAttachmentBone();
		//	frameInfo.m_parentComp = actor->GetRootAnimatedComponent();
		//	hEnt = m_keyframeCtrl.SelectEntity( sceneEntity, false, frameInfo );
		//	processed = true;
		//}		
	}
	if ( !processed )
	{
		hEnt = m_keyframeCtrl.SelectEntity( sceneEntity, false );
	}	

	// set preview edit text
	m_preview->SetEditString( String::EMPTY );
	if( hEnt )
	{
		String editString( TXT( "EDIT: " ) );
		editString += GetEntityIdStr( sceneEntity );
		m_preview->SetEditString( editString );
	}

	//CActor* sceneActor = Cast< CActor > ( sceneEntity );
	//m_controlRigPanel->OnActorSelectionChange( sceneActor );
	
	return hEnt;
}

CNode* CEdSceneEditor::OnHelperEntity_SelectObjectFromWorld( const CComponent* c )
{
	// CARL TODO: Doing scene placement first, world placement will follow

/*	if ( !CanModifyEntityFromWorld( c->GetEntity() ) )
	{
		return nullptr;
	}

	CEdSceneHelperEntity* hEnt = nullptr;

	Bool found = false;

	CEntity* actor = c->GetEntity();

	TDynArray< const CStorySceneEventWorldPropPlacement* > evts;
	if ( m_controller.FindEventsByTime( m_controller.GetSectionTime(), evts ) )
	{
		for ( Uint32 i=0; i<evts.Size(); ++i )
		{
			const CStorySceneEventWorldPropPlacement* e = evts[ i ];
			if ( e->GetEntityHandle().Get() == actor )
			{
				SCENE_ASSERT( !found );
				found = true;

				SelectItemWithAnimation( e );

				hEnt = m_helperEntitiesCtrl.FindHelperById( e->GetGUID() );
			}
		}
	}

	if ( !found )
	{
		CStorySceneEventWorldPropPlacement* e = Cast< CStorySceneEventWorldPropPlacement >( CreateAndSelectEventAndItemWithAnimation( ClassID< CStorySceneEventWorldPropPlacement >() ) );
		if ( e )
		{
			e->SetEntityHandle( actor );
			hEnt = m_helperEntitiesCtrl.FindHelperById( e->GetGUID() );
		}
	}
	*/
	return nullptr;
}

void CEdSceneEditor::OnHelperEntity_GenerateFragmentsForLight( CName lightId, CRenderFrame* frame )
{
	if( CEntity* ent = m_controller.GetSceneEntity( lightId ) )
	{
		for( ComponentIterator<CLightComponent> it( ent ); it; ++it )
		{
			EShowFlags flags = SHOW_LightsBBoxes;
			(*it)->OnGenerateEditorFragments( frame, flags );
		}
	}
}

void CEdSceneEditor::OnHelperEntity_GenerateFragmentsForDimmers( CName lightId, CRenderFrame* frame )
{
	if( CEntity* ent = m_controller.GetSceneEntity( lightId ) )
	{
		for( ComponentIterator<CDimmerComponent> it( ent ); it; ++it )
		{
			// the dimmer's oriented box is shown only if a dimmer node is selected which doesn't occur with the dimmer entity helper selected,
			// hence we need to generate this box here for the dimmer connected to currently selected entity helper
			frame->AddDebugBox( Box( Vector( -1.f, -1.f, -1.f ), Vector( 1.f, 1.f, 1.f ) ), (*it)->GetLocalToWorld(), Color::LIGHT_GREEN );
		}
	}
}


Bool CEdSceneEditor::OnHelperEntity_FloatingHelpersEnabled() const
{
	return m_helperEntitiesCtrl.FloatingHelpersEnabled();
}


void CEdSceneEditor::OnHelperEntity_SelectionChanged()
{
	TDynArray< CEdSceneHelperEntity* > entities;
	m_helperEntitiesCtrl.CollectSelectedHelpers( entities );

	m_controlRigPanel->OnPreviewSelectionChanged( entities );
}

void CEdSceneEditor::OnHelperEntity_RefreshProperty( const CGUID& id, Bool pos, Bool rot, Bool scale )
{
	if( !m_keyframeCtrl.OnHelperEntityRefreshedProperty( id, pos, rot, scale ) )
	{
		//m_controlRequest.RequestRefresh();
		RefreshPlayer();
	}
}

CWorld* CEdSceneEditor::OnHelperEntity_GetWorld()
{
	return GetWorld();
}

Bool CEdSceneEditor::OnHelperEntity_IsPointOnScreen( const Vector& pointWS, Vector& pointOnScreen ) const
{
	return m_camera.IsPointOnScreen( pointWS, pointOnScreen );
}

//////////////////////////////////////////////////////////////////////////

CStorySceneEvent* CEdSceneEditor::OnControlRig_FindSelectedEvent( const CClass* c, const CName& actorId )
{
	return FindSelectedEvent( c, actorId );
}

void CEdSceneEditor::OnControlRig_SetPreviewEditString( const String& txt, EEditStringId strid /*= EDIT_Default */ )
{
	m_preview->SetEditString( txt, strid );
}

void CEdSceneEditor::OnControlRig_HelperEntityCreate( const CGUID& id, EngineTransform& transformWS, Bool& dirtyFlag, const String& name, const CEdSceneHelperShapeSettings* s )
{
	m_helperEntitiesCtrl.CreateHelper( id, transformWS, dirtyFlag, s )->SetName( name );
}

void CEdSceneEditor::OnControlRig_HelperEntityDestroy( const CGUID& id )
{
	m_helperEntitiesCtrl.DestroyHelpers( id );
}

void CEdSceneEditor::OnControlRig_HelperEntityVisible( const CGUID& id )
{
	m_helperEntitiesCtrl.Select( id );
}

void CEdSceneEditor::OnControlRig_HelperEntitySelect( const CGUID& id )
{
	CEdSceneHelperEntity* e = m_helperEntitiesCtrl.FindHelperById( id );
	if ( e && e->GetLayer()->GetWorld()->GetSelectionManager() )
	{
		e->GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
		e->GetLayer()->GetWorld()->GetSelectionManager()->Select( e );
	}
}

Bool CEdSceneEditor::OnControlRig_HelperEntityIsSelected( const CGUID& id )
{
	CEdSceneHelperEntity* e = m_helperEntitiesCtrl.FindHelperById( id );
	return e && e->IsSelected();
}

void CEdSceneEditor::OnControlRig_HelperEntityUpdateTransform( const CGUID& id, const EngineTransform& transformWS )
{
	m_helperEntitiesCtrl.UpdateRawPlacement( id, transformWS );
}

void CEdSceneEditor::OnControlRig_HelperEntityUpdateTransform( const CGUID& id, const EngineTransform& transformWS, const EngineTransform& refTransformWS )
{
	m_helperEntitiesCtrl.UpdateRawPlacement( id, transformWS, refTransformWS );
}

void CEdSceneEditor::OnControlRig_RefreshPlayer()
{
	m_controlRequest.RequestRefresh();
}

Bool CEdSceneEditor::OnControlRig_GetCurrIdleAnimationName( CName actorId, CName& animName, Float& animTime ) const
{
	return m_controller.GetActorCurrIdleAnimationNameAndTime( actorId, animName, animTime );
}

Bool CEdSceneEditor::OnControlRig_GetParentAnimationName( const CStorySceneEvent* e, CName& animName, Float& animTime ) const
{
	if ( e->HasLinkParent() )
	{
		if ( const CStorySceneEvent* linkParent = m_timeline->FindEvent( e->GetLinkParentGUID() ) )
		{
			if ( linkParent->GetClass()->IsA< CStorySceneEventAnimClip >() )
			{
				const CStorySceneEventAnimClip* ac = static_cast< const CStorySceneEventAnimClip* >( linkParent );
				animName = ac->GetAnimationName();
			}
		}
	}

	return animName != CName::NONE && e->GetSubject() != CName::NONE ? m_controller.GetActorCurrAnimationTime( e->GetSubject(), animName, animTime ) : false;
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneEditor::OnKeyframeCtrl_HelperEntityCreate( const CGUID& id, EngineTransform& transformWS )
{
	m_helperEntitiesCtrl.CreateHelper( id, transformWS );

	// TODO - why do we need this?
	m_helperEntitiesCtrl.DeselectAllHelpers();

	CEdSceneHelperEntity* hEnt = m_helperEntitiesCtrl.FindHelperById( id );
	if( hEnt )
	{
		hEnt->SetColor( Color( 255, 64, 0, 255 ) );
	}
}

CEdSceneHelperEntity* CEdSceneEditor::OnKeyframeCtrl_FindHelperEntity( const CGUID& id )
{
	return m_helperEntitiesCtrl.FindHelperById( id );
}

void CEdSceneEditor::OnKeyframeCtrl_DeselectAllHelperEntities()
{
	m_helperEntitiesCtrl.DeselectAllHelpers();
}

Bool CEdSceneEditor::OnKeyframeCtrl_HasAnyEventNow( const CClass* c, CName id )
{
	return FindEditorEvent( c, id, nullptr ) != nullptr;
}

CStorySceneEvent* CEdSceneEditor::OnKeyframeCtrl_GetSelectedEvent()
{
	TDynArray< CStorySceneEvent* > evts;
	m_timeline->GetSelectedEvents( evts );
	return evts.Size() == 1 ? evts[ 0 ] : nullptr;
}

CStorySceneEvent* CEdSceneEditor::OnKeyframeCtrl_CloneEvent( const CStorySceneEvent* e )
{
	// TODO - this is temporary code
	if ( ITimelineItem* evtItem = m_timeline->FindItemEvent( e ) )
	{
		TDynArray< ITimelineItem* > items;
		TDynArray< ITimelineItem* > pastedItems;

		items.PushBack( evtItem );
		const Float time = m_controller.GetSectionTime();

		m_timeline->CopyItems( items, false );
		m_timeline->PasteItems( pastedItems, CEdTimeline::PIM_CustomTime, time, false );

		SCENE_ASSERT( pastedItems.Size() == 1 );
		if ( pastedItems.Size() == 1 )
		{
			if ( DialogTimelineItems::CTimelineItemEvent* sceneEvt = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( pastedItems[ 0 ] ) )
			{
				return sceneEvt->GetEvent();
			}
		}
	}

	SCENE_ASSERT( 0 );

	return nullptr;
}

EngineTransform CEdSceneEditor::OnHelperEntity_GetSceneToWorld() const
{
	return m_controller.GetCurrentScenePlacement();
}

const CActor* CEdSceneEditor::OnKeyframeCtrl_AsSceneActor( const CEntity* e ) const
{
	return AsSceneActor( e );
}

const CEntity* CEdSceneEditor::OnKeyframeCtrl_AsSceneProp( const CEntity* e ) const
{
	return AsSceneProp( e );
}

const CEntity* CEdSceneEditor::OnKeyframeCtrl_AsSceneLight( const CEntity* e ) const
{
	return AsSceneLight( e );
}

//////////////////////////////////////////////////////////////////////////

Vector CEdSceneEditor::OnActorsProvider_CalcLightSpawnPositionSS() const
{
	return CalcBestSpawnPositionSS();
}

EngineTransform CEdSceneEditor::OnActorsProvider_GetSceneToWorld() const
{
	return m_controller.GetCurrentScenePlacement();
}

CWorld* CEdSceneEditor::OnActorsProvider_GetWorld() const
{
	return GetWorld();
}

void CEdSceneEditor::OnActorsProvider_AddExtraActors( TDynArray< THandle< CEntity > >& actors, TDynArray< THandle< CEntity > >& props ) const
{
	m_screenshotCtrl.AddExtraActorsToScene( actors, props );
}

//////////////////////////////////////////////////////////////////////////

Bool CEdSceneEditor::OnScreenshotPanel_ForceMainWorld()
{
	return PlayInGameplayWorld( true );
}

CWorld* CEdSceneEditor::OnScreenshotPanel_GetWorld()
{
	return GetWorld();
}

CStoryScene* CEdSceneEditor::OnScreenshotPanel_GetScene()
{
	return m_storyScene;
}

CStorySceneDialogsetInstance* CEdSceneEditor::OnScreenshotPanel_GetCurrentDialogset()
{
	return const_cast< CStorySceneDialogsetInstance* >( m_controller.GetCurrentDialogsetInstanceIfValid() );
}

EngineTransform CEdSceneEditor::OnScreenshotPanel_GetCurrentDialogsetTransform() const
{
	return m_controller.GetCurrentScenePlacement();
}

void CEdSceneEditor::OnScreenshotPanel_AddEntityToScene()
{
	m_actorsProvider->Rebuild( m_storyScene );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

//////////////////////////////////////////////////////////////////////////

const CStorySceneSection* CEdSceneEditor::OnMainControlPanel_GetCurrentSection() const
{
	return m_controller.GetCurrentSection();
}

void CEdSceneEditor::OnMainControlPanel_RequestSectionVariant( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_RequestSectionVariant(): no section." );

	currentSection->SetVariantChosenInEditor( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();

	// Switch locale to base locale of chosen variant.
	const Uint32 localeId = currentSection->GetVariantBaseLocale( variantId );
	String localeStr;
	SLocalizationManager::GetInstance().FindLocaleStr( localeId, localeStr );
	wxCommandEvent ev;
	ev.SetString( localeStr.AsChar() );
	m_locPrevLang = SLocalizationManager::GetInstance().GetCurrentLocale();
	wxTheFrame->OnChangeLanguage( ev );
}

void CEdSceneEditor::OnMainControlPanel_SetVariantBase( CStorySceneSectionVariantId variantId, Uint32 localeId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_SetVariantBase(): no section." );

	currentSection->SetVariantBaseLocale( variantId, localeId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnMainControlPanel_SetVariantForcedInEditor( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_SetVariantForcedInEditor(): no section." );

	currentSection->SetVariantForcedInEditor( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnMainControlPanel_SetVariantAsDefault( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_SetVariantAsDefault(): no section." );

	currentSection->SetDefaultVariant( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

/*

\param localeId Locale on which new variant will be based.
*/
void CEdSceneEditor::OnMainControlPanel_RequestCreateEmptyVariant( Uint32 baseLocaleId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_RequestCreateEmptyVariant(): no section." );

	String baseLocaleStr;
	SLocalizationManager::GetInstance().FindLocaleStr( baseLocaleId, baseLocaleStr );

	CStorySceneSectionVariantId variantId = currentSection->CreateVariant( baseLocaleId );

	// Calculate and approve duration of all scene elements, including choice element.
	for( Uint32 iElement = 0, numElements = currentSection->GetNumberOfElements(); iElement < numElements; ++iElement )
	{
		const CStorySceneElement* element = currentSection->GetElement( iElement );
		const Float duration = element->CalculateDuration( baseLocaleStr );
		currentSection->ApproveElementDuration( variantId, element->GetElementID(), duration );
	}
	if( const CStorySceneChoice* choice = currentSection->GetChoice() )
	{
		const Float duration = choice->CalculateDuration( baseLocaleStr );
		currentSection->ApproveElementDuration( variantId, choice->GetElementID(), duration );
	}

	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

CStorySceneSectionVariantId CEdSceneEditor::OnMainControlPanel_RequestCloneVariant( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_RequestCloneVariant(): no section." );

	const CStorySceneSectionVariantId newVariantId = currentSection->CloneVariant( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();

	return newVariantId;
}

void CEdSceneEditor::OnMainControlPanel_RequestDeleteVariant( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_RequestDeleteVariant(): no section." );

	// TODO: assert DestroySectionVariant() preconditions.
	currentSection->DestroyVariant( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnMainControlPanel_RequestDeleteAllEvents( CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_RequestDeleteAllEvents(): no section." );

	currentSection->RemoveAllEvents( variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnMainControlPanel_SetLocaleVariantMapping( Uint32 localeId, CStorySceneSectionVariantId variantId )
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_SetLocaleVariantMapping(): no section." );

	currentSection->SetLocaleVariantMapping( localeId, variantId );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

/*
Updates current variant.

Updating variant means two things:
1. Updating element durations stored in variant so they match current durations of elements in current locale.
   Note it's important whether current section view uses "approved VO durations" or "local VO durations".
2. Changing variant language to current language.
*/
void CEdSceneEditor::OnMainControlPanel_UpdateCurrentVariantBase()
{
	CStorySceneSection* currentSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currentSection, "CEdSceneEditor::OnMainControlPanel_UpdateCurrentVariantBase(): no section." );

	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId currentVariantId = currentSection->GetVariantUsedByLocale( currentLocaleId );

	currentSection->SetVariantBaseLocale( currentVariantId, currentLocaleId );

	// Update element durations stored in variant.
	for( Uint32 iElement = 0, numElements = currentSection->GetNumberOfElements(); iElement < numElements; ++iElement )
	{
		CStorySceneElement* element = currentSection->GetElement( iElement );

		// Update durations of all elements included by section playing plan (elements that are not
		// both schedulable and playable are not included and there's no instance data for them).
		const IStorySceneElementInstanceData* elementInst = m_controller.FindElementInstance( element );
		if( elementInst )
		{
			currentSection->ApproveElementDuration( currentVariantId, element->GetElementID(), elementInst->GetDuration() );
		}
	}

	// Update choice element duration stored in variant (but only if section has one and if its included by section playing plan).
	if( CStorySceneElement* element = currentSection->GetChoice() )
	{
		const IStorySceneElementInstanceData* elementInst = m_controller.FindElementInstance( element );
		if( elementInst )
		{
			currentSection->ApproveElementDuration( currentVariantId, element->GetElementID(), elementInst->GetDuration() );
		}
	}

	// Bake scale of variant events.
	for( auto evGuid : currentSection->GetEvents( currentVariantId ) )
	{
		// Note that we're not reseting event instance scaling factor.
		// After baking scale we do rebuild - instance scaling factor will be set again.
		CStorySceneEvent* ev = currentSection->GetEvent( evGuid );
		const Float scalingFactor = m_controller.GetEventScalingFactor( *ev );
		ev->BakeScale( scalingFactor );
	}

	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

/*
Creates unscaled variant from current view.

\return 0 - success.
        1 - We've encountered element whose duration was never approved. User should approve it and try again.
		2 - Result view would be too short to fit all events. User should add pause at the and and try again.
*/
Int32 CEdSceneEditor::OnMainControlPanel_CreateUnscaledVariantFromCurrentView()
{
	enum ResultStatus : Int32
	{
		success = 0,
		invalidElApprDur = 1,			// We've encountered element whose duration was never approved. User should approve it and try again.
		rsltViewTooShort = 2			// Result view would be too short to fit all events. User should add pause at the end and try again.
	};

	// Let's define some terms:
	//
	// 1. apprView: Approved view of current variant.
	//              Locale of apprView is always the same as base locale of current variant.
	//              apprView is using approved VO durations (which may or may not be the same as local VO durations).
	//              We like absolute positions and durations of events in this view.
	//
	// 2. currView: Current view of current variant.
	//              Locale of currView may or may not be the same as locale of apprView.
	//              currView is using local VO durations, possibly modified by adding some silence to line elements.
	//              Comparing this view to apprView:
	//                - Elements have different start times and durations.
	//                - Events have the same relative positions, different absolute positions, different durations.
	//              We don't like positions and durations of events in this view.
	//
	// 3. rsltView: Approved view of result variant - that's what we want to create.
	//              Locale of rsltView is the same as locale in currView.
	//              rsltView is using local VO durations.
	//              Comparing this view to apprView:
	//                - Elements have different start times and durations.
	//                - Events have different relative positions, the same absolute positions and durations

	CStorySceneSection* currSection = m_controller.GetCurrentSection();
	RED_FATAL_ASSERT( currSection, "CEdSceneEditor::OnMainControlPanel_CreateUnscaledVariantFromCurrentView(): no section." );

	// Create list of scene elements that are included in section playing plan. Choice element is also included.
	TDynArray< CStorySceneElement* > sectionElements;
	sectionElements.Reserve( currSection->GetNumberOfElements() );
	for( CStorySceneElement* element : currSection->GetElements() )
	{
		if( const Bool elementInPlayingPlan = ( m_controller.FindElementInstance( element ) != nullptr ) )
		{
			sectionElements.PushBack( element );
		}
	}
	if( CStorySceneElement* const choice = currSection->GetChoice() )
	{
		if( const Bool elementInPlayingPlan = ( m_controller.FindElementInstance( choice ) != nullptr ) )
		{
			sectionElements.PushBack( choice );
		}
	}

	// Get info about current view.
	const Uint32 currViewLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId currVariantId = currSection->GetVariantUsedByLocale( currViewLocaleId );

	// Make sure that all elements in apprVew have valid approved duration. If there are any elements
	// whose duration was never approved then ask the user to approve their duration and try again.
	for( Uint32 iElement = 0, numElements = sectionElements.Size(); iElement < numElements; ++iElement )
	{
		const CStorySceneElement& element = *sectionElements[ iElement ];
		const Float apprViewElementDuration = currSection->GetElementApprovedDuration( currVariantId, element.GetElementID() );
		if( const Bool elementDurationWasNeverApproved = ( apprViewElementDuration < 0.0f ) )
		{
			return ResultStatus::invalidElApprDur;
		}
	}

	struct ElementInfo
	{
		ElementInfo() : m_startTime( -1.0f ), m_duration( -1.0f ) {}
		ElementInfo( Float startTime, Float duration ) : m_startTime( startTime ), m_duration( duration ) {}

		Float m_startTime;
		Float m_duration;
	};

	// Get info about scene elements in apprView and rsltView.
	THashMap< String, ElementInfo > apprViewElements;
	THashMap< String, ElementInfo > rsltViewElements;
	{
		Float apprViewElementStartTime = 0.0f;
		Float rsltViewElementStartTime = 0.0f;

		for( Uint32 iElement = 0, numElements = sectionElements.Size(); iElement < numElements; ++iElement )
		{
			const CStorySceneElement& element = *sectionElements[ iElement ];
			const IStorySceneElementInstanceData& elementInst = *m_controller.FindElementInstance( &element );

			// Calculate duration of element in rsltView. Duration of element in rsltView and currView
			// is the same except for extra silence that may have been added to line element in currView.
			const Float currViewElementDuration = elementInst.GetDuration();
			Float currViewElementSilence = 0.0f;
			if( element.IsA< CStorySceneLine >() )
			{
				const StorySceneLineInstanceData& lineInst = static_cast< const StorySceneLineInstanceData& >( elementInst );
				Float leadingSilence = 0.0f;
				Float trailingSilence = 0.0f;
				lineInst.GetTimeOffsets( leadingSilence, trailingSilence );
				currViewElementSilence = leadingSilence + trailingSilence;
			}
			const Float rsltViewElementDuration = currViewElementDuration - currViewElementSilence;

			const Float apprViewElementDuration = currSection->GetElementApprovedDuration( currVariantId, element.GetElementID() );
			RED_FATAL_ASSERT( apprViewElementDuration >= 0.0f, "" ); // We've made sure at the beginning that this won't happen.

			ElementInfo rsltViewElementInfo( rsltViewElementStartTime, rsltViewElementDuration );
			ElementInfo apprViewElementInfo( apprViewElementStartTime, apprViewElementDuration);

			rsltViewElements.Insert( element.GetElementID(), rsltViewElementInfo );
			apprViewElements.Insert( element.GetElementID(), apprViewElementInfo );

			// calculate start times of next element
			rsltViewElementStartTime += rsltViewElementInfo.m_duration;
			apprViewElementStartTime += apprViewElementInfo.m_duration;
		}
	}

	{
		// Find rightmost event and compute its start time in apprView.
		Float apprViewMaxEvStartTime = 0.0f;
		for( CGUID evGuid : currSection->GetEvents( currVariantId ) )
		{
			const CStorySceneEvent& ev = *currSection->GetEvent( evGuid );

			const ElementInfo apprViewElementInfo = apprViewElements[ ev.GetSceneElement()->GetElementID() ];
			const Float apprViewEvStartTime = apprViewElementInfo.m_startTime + ev.GetStartPosition() * apprViewElementInfo.m_duration;
			if( apprViewEvStartTime > apprViewMaxEvStartTime )
			{
				apprViewMaxEvStartTime = apprViewEvStartTime;
			}
		}

		// Calculate duration of rsltView.
		const ElementInfo rsltViewLastElementInfo = rsltViewElements[ sectionElements.Back()->GetElementID() ];
		const Float rsltViewDuration = rsltViewLastElementInfo.m_startTime + rsltViewLastElementInfo.m_duration;

		// Check whether we'll be able to fit rightmost event in rsltView. Note that we don't care
		// whether duration events fully fit inside rsltView - we only care about their start times.
		if( const Bool rsltViewTooShort = ( apprViewMaxEvStartTime >= rsltViewDuration ) )
		{
			return ResultStatus::rsltViewTooShort;
		}
	}

	// Create result variant as a clone of current variant.
	const CStorySceneSectionVariantId rsltVariantId = currSection->CloneVariant( currVariantId );
	currSection->SetVariantBaseLocale( rsltVariantId, currViewLocaleId );

	// Approve durations of scene elements in rsltView.
	for( Uint32 iElement = 0, numElements = sectionElements.Size(); iElement < numElements; ++iElement )
	{
		const String& elementId = sectionElements[ iElement ]->GetElementID();
		currSection->ApproveElementDuration( rsltVariantId, elementId, rsltViewElements[ elementId ].m_duration );
	}
	
	/*
	Finds scene element at specified time in rsltView.
	*/
	auto rsltViewGetElementAt = [&]( Float time ) -> CStorySceneElement*
	{
		for( CStorySceneElement* element : sectionElements )
		{
			const ElementInfo elementInfo = rsltViewElements[ element->GetElementID() ];
			if( elementInfo.m_startTime <= time && time < elementInfo.m_startTime + elementInfo.m_duration )
			{
				return element;
			}
		}

		return nullptr;
	};

	// Adjust relative positions of events and, if needed, change their parent scene elements.
	for( CGUID evGuid : currSection->GetEvents( rsltVariantId ) )
	{
		CStorySceneEvent& ev = *currSection->GetEvent( evGuid );
		
		// Get element that is a parent of event in apprView (it's the same in currView).
		const CStorySceneElement& apprViewEvParent = *ev.GetSceneElement();
		RED_FATAL_ASSERT( apprViewElements.KeyExist( apprViewEvParent.GetElementID() ), "" ); // Assert that event is associated with element that is included in playing plan.

		// Calculate start time of event in apprView.
		const ElementInfo apprViewEvParentInfo = apprViewElements[ apprViewEvParent.GetElementID() ];
		const Float apprViewEvStartTime = apprViewEvParentInfo.m_startTime + ev.GetStartPosition() * apprViewEvParentInfo.m_duration;

		// Find scene element that exists in rsltView at calculated position. Make that element parent of current event.
		CStorySceneElement* const rsltViewEvParent = rsltViewGetElementAt( apprViewEvStartTime );
		RED_FATAL_ASSERT( rsltViewEvParent, "" ); // We know that apprView is long enough so this should never happen.
		ev.SetSceneElement( rsltViewEvParent );

		// Set relative position of event so that its start time in rsltView is the same as in apprView.
		const ElementInfo rsltViewEvParentInfo = rsltViewElements[ rsltViewEvParent->GetElementID() ];
		const Float rsltViewEvStartPos = ( apprViewEvStartTime - rsltViewEvParentInfo.m_startTime ) / rsltViewEvParentInfo.m_duration;
		RED_FATAL_ASSERT( rsltViewEvStartPos >= 0.0f && rsltViewEvStartPos < 1.0f, "" );
		ev.SetStartPosition( rsltViewEvStartPos );

		// Note that we intentionally don't update positions of events that depend on position of current event - this loop updates all events
		// independently of each other and the results are ok. Also, we intentionally don't bake scale of events - we want durations in rsltView
		// to be the same as in apprView.
	}

	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();

	return ResultStatus::success;
}

Bool CEdSceneEditor::OnMainControlPanel_LocalVoMatchApprovedVoInCurrentSectionVariant() const
{
	return m_controller.LocalVoMatchApprovedVoInCurrentSectionVariant();
}

void CEdSceneEditor::OnMainControlPanel_RequestChangeLocale( Uint32 localeId )
{
	String localeStr;
	SLocalizationManager::GetInstance().FindLocaleStr( localeId, localeStr );

	wxCommandEvent ev;
	ev.SetString( localeStr.AsChar() );

	m_locPrevLang = SLocalizationManager::GetInstance().GetCurrentLocale();
	
	wxTheFrame->OnChangeLanguage( ev );
}

void CEdSceneEditor::OnMainControlPanel_RequestRebuildImmediate()
{
	RebuildPlayer();
}

void CEdSceneEditor::OnMainControlPanel_SetConfigUseApprovedDurations( Bool state )
{
	SetConfigUseApprovedVoDurations( state );
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
