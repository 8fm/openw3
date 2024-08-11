
#include "build.h"
#include "dialogEditor.h"

#include "../../common/game/storySceneGraph.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventCameraAnimation.h"
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
#include "../../common/game/storySceneEventStartBlendToGameplayCamera.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/game/storySceneEventControlRig.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneUtils.h"
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
#include "../../common/game/storySceneSectionPlayingPlan.h"
#include "../../common/game/sceneAnimEvents.h"
#include "../../common/game/storySceneEventInterpolation.h"
#include "../../common/game/storySceneEventLightProperties.h"
#include "../../common/game/StorySceneEventCameraLight.h"
#include "../../common/game/storySceneEventOpenDoor.h"

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
#include "../../common/engine/gameResource.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/engine/particleComponent.h"
#include "../../common/engine/extAnimCutsceneDialogEvent.h"
#include "../../common/engine/mimicComponent.h"

#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/mathUtils.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( placement )

void CEdSceneEditor::OnTimeline_EventChanged( const CStorySceneSection* section, CStorySceneEvent* event, const CName& propertyName )
{
	SCENE_ASSERT( event );
	if ( !event )
	{
		return;
	}
	
	event->OnPreviewPropertyChanged( m_controller.GetPlayer(), propertyName );

	if ( event->GetClass()->IsA< CStorySceneEventAnimation >() ||  event->GetClass()->IsA< CStorySceneEventCameraAnim >()
		|| event->GetClass()->IsA< CStorySceneEventMimicsAnim >() )
	{
		if ( propertyName == CNAME( friendlyName ) || propertyName == CNAME( animationName ) )
		{
			RunLaterOnce( [this](){ RefreshPropertiesPage(); } );
			RebuildPlayer();
		}
		else if ( propertyName == CNAME( weight ) )
		{
			m_controlRequest.RequestRefresh();
		}
	}
	else if ( event->GetClass()->IsA< CStorySceneEventDuration >() && propertyName == CNAME( duration ) )
	{
		m_controlRequest.RequestRebuild();
	}
	else if ( event->GetClass()->IsA< CStorySceneEventCustomCamera >() || event->GetClass()->IsA< CStorySceneEventCustomCameraInstance >() )
	{
		if ( !m_camera.IsPreviewMode() )
		{
			UpdateCameraFromDefinition( m_camera.GetSelectedDefinition() );
		}
	}
	else if( event->GetClass()->IsA< CStorySceneEventScenePropPlacement >() && propertyName == CNAME( placement ) )
	{
		m_controlRequest.RequestRefresh();	
	}
	else if ( event->GetClass()->IsA< CStorySceneEventChangePose >() )
	{
		m_controlRequest.RequestRebuild();
	}
	else if ( event->GetClass()->IsA( ClassID< CStorySceneEventOverridePlacement >() ) && propertyName == TXT( "placement" ) )
	{
		RefreshPlayer();
	}
	else if ( event->GetClass()->IsA< CStorySceneEventPoseKey >() || event->GetClass()->IsA< CStorySceneEventAttachPropToSlot >()
		|| event->GetClass()->IsA< CStorySceneEventLookAtDuration >() || event->GetClass()->IsA< CStorySceneEventLookAt >() || event->GetClass()->IsA< CStorySceneEventGameplayLookAt >() )
	{
		RefreshPlayer();
	}
	else if( event->GetClass()->IsA< CStorySceneEventCameraLight >() || event->GetClass()->IsA< CStorySceneEventOpenDoor >())
	{
		m_controlRequest.RequestRefresh();	
	}
}

void CEdSceneEditor::OnTimeline_EventChanged( const CStorySceneSection* section, CStorySceneEvent* e )
{
	m_controlRequest.RequestRebuild();
}

void CEdSceneEditor::OnTimeline_AddEvent( const CStorySceneSection* section, CStorySceneEvent* e, CStorySceneSectionVariantId variantId, Bool loadDataFromPreviousEvents /* = true */ )
{
	ASSERT( section == m_controller.GetCurrentSection() );
	if( section == m_controller.GetCurrentSection() )
	{
		m_controller.GetCurrentSection()->AddEvent( e, variantId );

		m_controlRequest.RequestRebuild();
	}

	if ( e->GetClass()->IsA< CStorySceneEventCustomCameraInstance >() )
	{
		RefreshCamerasList();
	}

	if ( loadDataFromPreviousEvents )
	{
		// TODO: variants - look for prev event of type only in specified variant?
		if( CStorySceneEventLightProperties* le = Cast< CStorySceneEventLightProperties >( e ) )
		{
			if( const CStorySceneEventLightProperties* prev = FindPrevEventOfType( le ) )
			{
				le->LoadDataFromOtherEvent( prev );
			}
		}
	}
}

void CEdSceneEditor::OnTimeline_RemoveEvent( const CStorySceneSection* section, CStorySceneEvent* e )
{
	ASSERT( section == m_controller.GetCurrentSection() );
	if( section == m_controller.GetCurrentSection() )
	{
		RemoveEvent( e );
	}
}

void CEdSceneEditor::OnTimeline_AddSceneElement( const CStorySceneSection* section, CStorySceneElement* e, Uint32 place )
{
	ASSERT( section == m_controller.GetCurrentSection() );
	if( section == m_controller.GetCurrentSection() )
	{
		m_controller.GetCurrentSection()->AddSceneElement( e, place );

		m_controlRequest.RequestRebuild();
		m_controlRequest.RequestRefreshTimeline();
	}
}

void CEdSceneEditor::OnTimeline_ApproveElementDuration( const CStorySceneSection* section, CStorySceneSectionVariantId variantId, CStorySceneElement* e, Float duration )
{
	RED_FATAL_ASSERT( section == m_controller.GetCurrentSection(), "CEdSceneEditor::OnTimeline_ApproveElementDuration(): section is not current section." );
	m_controller.GetCurrentSection()->ApproveElementDuration( variantId, e->GetElementID(), duration );
}

void CEdSceneEditor::OnTimeline_RemoveElement( const CStorySceneSection* section, CStorySceneElement* e )
{
	ASSERT( section == m_controller.GetCurrentSection() );
	if( section == m_controller.GetCurrentSection() )
	{
		m_controller.GetCurrentSection()->RemoveElement( e );

		m_controlRequest.RequestRebuild();
		m_controlRequest.RequestRefreshTimeline();
	}
}

void CEdSceneEditor::OnTimeline_SelectionChanged( const CStorySceneSection* section, ITimelineItem* item, Bool goToEvent )
{
	ASSERT( section );
	ASSERT( section == m_controller.GetCurrentSection() );

	ResetMimicControlRigObject();
	ResetBodyControlRigObject();

	if( !item )
	{
		m_helperEntitiesCtrl.DeselectAllHelpers();
		ResetPropertiesBrowserObject();
		SelectDetailPage( "Section" );
		return;
	}
	else if( DialogTimelineItems::CTimelineItemBlocking* itemSceneElement = dynamic_cast< DialogTimelineItems::CTimelineItemBlocking* >( item ) )
	{
		m_helperEntitiesCtrl.DeselectAllHelpers();
		ResetPropertiesBrowserObject();
		SelectDetailPage( "Properties" );
		return;
	}
	else if( DialogTimelineItems::CTimelineItemEvent* itemSceneEvent = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( item ) )
	{
		CStorySceneEvent* e = itemSceneEvent->GetEvent();

		CAnimatedComponent* comp = GetAnimatedComponentForActor( e->GetSubject() ); // TODO supports mimics
		CEntity* sceneEntity = m_controller.GetSceneEntity( e->GetSubject() );

		Bool openCameraPage = false;
		Bool openCRigPage = false;

		if ( e->GetClass()->IsA< CStorySceneEventCustomCameraInstance >() )
		{
			if ( section == m_controller.GetCurrentSection() && !m_camera.IsPreviewMode() )
			{
				ActivateCameraFromList( static_cast< CStorySceneEventCustomCameraInstance* >( e )->GetCustomCameraName() );
			}
			else if ( section == m_controller.GetCurrentSection() && m_camera.IsPreviewMode() )
			{
				CName cameraName = static_cast< CStorySceneEventCustomCameraInstance* >( e )->GetCustomCameraName();
				m_cameraProperties->SetObject( m_storyScene->GetCameraDefinition( cameraName ) );
			}
			openCameraPage = true;
		}
		else if ( e->GetClass()->IsA< CStorySceneEventCustomCamera >() )
		{
			if ( section == m_controller.GetCurrentSection() && !m_camera.IsPreviewMode() )
			{
				ActivateCustomCamera(  static_cast< CStorySceneEventCustomCamera* >( e ) );
			}
			else if ( section == m_controller.GetCurrentSection() && m_camera.IsPreviewMode() )
			{
				m_cameraProperties->SetObject( static_cast< CStorySceneEventCustomCamera* >( e )->GetCameraDefinition() );
			}
			openCameraPage = true;
		}
		else if ( e->GetClass()->IsA< CStorySceneEventPoseKey >() )
		{
			SetMimicControlRigObject( static_cast< CStorySceneEventPoseKey* >( e ) );

			SetBodyControlRigObject( static_cast< CStorySceneEventPoseKey* >( e ), Cast< CActor >( comp->GetEntity() ) );
			openCRigPage = true;
		}

		const Bool evtWithAnimation = SetTreeAnimationSelection( e );

		SetPropertiesBrowserObject( e, comp );

		if ( !(evtWithAnimation && IsAnyAnimTreeOpen()) )
		{
			wxString tab = wxT( "Properties" );
			if( openCRigPage )
			{
				tab = wxT( "Control rig" );
			}
			if( openCameraPage )
			{
				tab = wxT( "Cameras" );
			}
			SelectDetailPage( tab );
		}

		// select the entity (if there is one associated with the event)
		if( sceneEntity )
		{
			m_keyframeCtrl.SelectEntity( sceneEntity, goToEvent );

			// set preview edit string
			String editString = String::Printf( TXT( "EDIT KEYFRAME: %s - %s @  %.2f" ), sceneEntity->GetName().AsChar(), e->GetClass()->GetName().AsChar(), e->GetStartPosition() );
			m_preview->SetEditString( editString );
		}


		if( goToEvent )
		{
			GoToEvent( e );
		}

		// select associated helper
		CEdSceneHelperEntity* helper = m_helperEntitiesCtrl.FindHelperById( e->GetGUID() );
		if ( !helper )
		{
			helper = CreateHelperEntityForEvent( e );
		}
		if( helper )
		{
			Bool selectHelper = UpdateHelperEntityForEvent( e, helper );
			CSelectionManager* selectionMgr = GetWorld()->GetSelectionManager();
			CSelectionManager::CSelectionTransaction transaction(*selectionMgr);
			selectionMgr->DeselectAll();
			if ( selectHelper )
			{
				selectionMgr->Select( helper );
			}
		}
	}
}

void CEdSceneEditor::OnTimeline_RequestSetTime( wxCommandEvent& event )
{
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	m_controlRequest.RequestTime( clientData->GetData() );
}

void CEdSceneEditor::OnTimeline_RequestRebuild()
{
	m_controlRequest.RequestRebuild();
}

void CEdSceneEditor::OnTimeline_RequestRefreshTimeline()
{
	m_controlRequest.RequestRefreshTimeline();
}

Bool CEdSceneEditor::OnTimeline_CreateCustomCameraFromView()
{
	return CreateCustomCameraFromView();
}

void CEdSceneEditor::OnTimeline_GetCurrentCameraDefinition( StorySceneCameraDefinition& cameraDef )
{
	SaveCameraToDefinition( &cameraDef );
}

Bool CEdSceneEditor::OnTimeline_CreateCustomCameraEnhancedBlendInterpolated()
{
	return CreateCustomCameraEnhancedBlendInterpolated();
}

Bool CEdSceneEditor::OnTimeline_CreatePlacementOverrideBlendInterpolated()
{
	const Float position = m_timeline->GetCurrentPosition();
	DialogTimelineItems::CTimelineItemOverridePlacementBlend* blend = dynamic_cast< DialogTimelineItems::CTimelineItemOverridePlacementBlend* >( m_timeline->FindDurationBlendItemByEventClass( ClassID< CStorySceneOverridePlacementBlend >(), position ) );
	if ( !blend )
	{
		return false;
	}
	CStorySceneOverridePlacementBlend* blendEvent = static_cast< CStorySceneOverridePlacementBlend* >( blend->GetEvent() );

	const Float localPosition = position - blend->GetStart();
	EngineTransform transform;
	blendEvent->GetTransformAt( localPosition, transform );

	return m_timeline->CreateOverridePlacementEvent( blendEvent->GetTrackName(), blendEvent->GetSubject(), &transform, blend ) != NULL;
}

void CEdSceneEditor::OnTimeline_StartEventChanging( const CStorySceneSection* section, CStorySceneEvent* e )
{
	ASSERT( m_controller.GetCurrentSection() == section );
	m_controlRequest.InfoStartEventChanging( e );
}

void CEdSceneEditor::OnTimeline_StateChanged()
{
	m_controlRequest.RequestRebuild();
}

Bool CEdSceneEditor::OnTimeline_GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_controller.GetCurrentActorAnimationState( actor, out );
}

CStorySceneDialogsetSlot* CEdSceneEditor::OnTimeline_GetAndChangeActorDialogsetSlot( const CName& actor ) const
{
	const CStorySceneDialogsetInstance* dialogset = GetCurrentDialogsetInstance();
	if ( dialogset )
	{
		return dialogset->GetSlotByActorName( actor );
	}

	return nullptr;
}

const CStorySceneLight* CEdSceneEditor::OnTimeline_GetLightDefinition( const CName& lightId ) const
{
	return m_storyScene->GetSceneLightDefinition( lightId );
}

CStorySceneLight* CEdSceneEditor::OnTimeline_GetAndChangeLightDefinition( const CName& lightId ) const
{
	return m_storyScene->GetSceneLightDefinition( lightId );
}

CName CEdSceneEditor::OnTimeline_GetPrevSpeakerName( CStorySceneElement* currElement ) const
{
	return m_controller.GetPrevSpeakerName( currElement );
}

void CEdSceneEditor::OnTimeline_GetEventTime( const CStorySceneEvent* e, Float& start, Float& duration ) const
{
	m_controller.GetEventAbsTime( e, start, duration );
}

Float CEdSceneEditor::OnTimeline_GetEventInstanceDuration( const CStorySceneEvent& e ) const
{
	return m_controller.GetEventDuration( e );
}

void CEdSceneEditor::OnTimeline_SetEventInstanceDuration( const CStorySceneEvent& e, Float duration ) const
{
	m_controller.SetEventDuration( e, duration );
}

Float CEdSceneEditor::OnTimeline_GetEventInstanceStartTime( const CStorySceneEvent& e ) const
{
	return m_controller.GetEventStartTime( e );
}

void CEdSceneEditor::OnTimeline_SetEventInstanceStartTime( const CStorySceneEvent& e, Float startTime ) const
{
	m_controller.SetEventStartTime( e, startTime );
}

Float CEdSceneEditor::OnTimeline_GetEventScalingFactor( const CStorySceneEvent& e ) const
{
	return m_controller.GetEventScalingFactor( e );
}

void CEdSceneEditor::OnTimeline_GetDisableDialogLookatEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEvents, Bool mimicAnim )
{
	CActor* actor = GetSceneActor( actorName );
	if ( actor == nullptr )
	{
		return;
	}
	const CAnimatedComponent* ac = mimicAnim ? actor->GetMimicComponent() : actor->GetRootAnimatedComponent();
	if ( ac == nullptr )
	{
		return;
	}
	dialogAnimEvents.Clear();
	CSkeletalAnimationContainer* animations = ac->GetAnimationContainer();
	if( animations)
	{
		CSkeletalAnimationSetEntry* anim =animations->FindAnimation( animName );
		if( anim )
		{
			TDynArray< CExtAnimDisableDialogLookatEvent* > events;
			Float animDuration = anim->GetDuration();
			anim->GetEventsOfType( events );
			for( Uint32 i = 0; i < events.Size(); ++i )
			{
				dialogAnimEvents.PushBack( TPair<Float,Float>( events[i]->GetStartTime(), events[i]->GetStartTime() + events[i]->GetDuration() ));
			}
		}
	}
}

void CEdSceneEditor::OnTimeline_GetKeyPoseMarkersEventsPositions( CName actorName, CName animName, TDynArray< Float >& dialogAnimEvents, Bool mimicAnim )
{
	CActor* actor = GetSceneActor( actorName );
	if ( actor == nullptr )
	{
		return;
	}
	const CAnimatedComponent* ac = mimicAnim ? actor->GetMimicComponent() : actor->GetRootAnimatedComponent();
	if ( ac == nullptr )
	{
		return;
	}
	dialogAnimEvents.Clear();
	CSkeletalAnimationContainer* animations = ac->GetAnimationContainer();
	if( animations)
	{
		CSkeletalAnimationSetEntry* anim =animations->FindAnimation( animName );
		if( anim )
		{
			TDynArray< CExtAnimDialogKeyPoseMarker* > events;
			anim->GetEventsOfType( events );

			for( Uint32 i = 0; i < events.Size(); ++i )
			{
				dialogAnimEvents.PushBack( events[i]->GetStartTime() );
			}
		}
	}
}

void CEdSceneEditor::OnTimeline_GetKeyPoseDurationsEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEventsTrans, TDynArray< TPair< Float, Float > >& dialogAnimEventsPoses, Bool mimicAnim )
{
	dialogAnimEventsTrans.Clear();
	dialogAnimEventsPoses.Clear();

	const CActor* actor = GetSceneActor( actorName );
	if ( actor == nullptr )
	{
		return;
	}

	const CAnimatedComponent* ac = mimicAnim ? actor->GetMimicComponent() : actor->GetRootAnimatedComponent();
	if ( ac == nullptr )
	{
		return;
	}

	const CSkeletalAnimationContainer* animations = ac->GetAnimationContainer();
	if ( animations )
	{
		const CSkeletalAnimationSetEntry* anim = animations->FindAnimation( animName );
		if( anim )
		{
			TDynArray< CExtAnimDialogKeyPoseDuration* > events;
			anim->GetEventsOfType( events );

			for( Uint32 i = 0; i < events.Size(); ++i )
			{
				CExtAnimDialogKeyPoseDuration* e = events[i];
				if ( e->IsTransition() )
				{
					dialogAnimEventsTrans.PushBack( TPair< Float, Float >( e->GetStartTime(), e->GetStartTime() + e->GetDuration() ) );
				}
				if ( e->IsKeyPose() )
				{
					dialogAnimEventsPoses.PushBack( TPair< Float, Float >( e->GetStartTime(), e->GetStartTime() + e->GetDuration() ) );
				}
			}
		}
	}
}

Bool CEdSceneEditor::OnTimeline_GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos /*= NULL */, Float* voiceEndPos /*= NULL*/ )
{
	return GetVoiceDataPositions( forLine, maxAmpPos,  voiceStartPos, voiceEndPos );
}

const CAnimatedComponent* CEdSceneEditor::OnTimeline_GetBodyComponentForActor( const CName& actor )
{
	return GetAnimatedComponentForActor( actor );
}

const CAnimatedComponent* CEdSceneEditor::OnTimeline_GetMimicComponentForActor( const CName& actor )
{
	return GetHeadComponentForActor( actor );
}

Float CEdSceneEditor::OnTimeline_GetBodyAnimationDuration( const CName& actorVoicetag, const CName& animationName )
{
	return GetBodyAnimationDuration( actorVoicetag, animationName );
}

Float CEdSceneEditor::OnTimeline_GetMimicAnimationDuration( const CName& actorVoicetag, const CName& animationName )
{
	return GetMimicAnimationDuration( actorVoicetag, animationName );
}

Float CEdSceneEditor::OnTimeline_GetCameraAnimationDuration( const CName& animationName )
{
	return GetCameraAnimationDuration( animationName );
}

Float CEdSceneEditor::OnTimeline_GetAnimationDurationFromEvent( const CStorySceneEventAnimClip* animClip )
{
	return GetAnimationDurationFromEvent( animClip );
}

Bool CEdSceneEditor::OnTimeline_GetActorPlacementLS( const CName& actorVoicetag, EngineTransform& outPlacement ) const
{
	const CEntity* sceneEntity = GetSceneEntity( actorVoicetag );
	if ( sceneEntity )
	{
		const EngineTransform scenePlacement = m_controller.GetCurrentScenePlacement();

		Matrix entityPlacementWS;			
		if( CHardAttachment* attachment = sceneEntity->GetTransformParent() )
		{
			attachment->CalcAttachedLocalToWorld( entityPlacementWS );
		}
		else
		{
			sceneEntity->GetTransform().CalcLocalToWorld( entityPlacementWS );
		}

		if ( !scenePlacement.IsIdentity() )
		{
			Matrix scenePlacementWorldToLocal;
			scenePlacement.CalcWorldToLocal( scenePlacementWorldToLocal );
			const Matrix entityPlacementLS = entityPlacementWS * scenePlacementWorldToLocal;
			outPlacement = EngineTransform( entityPlacementLS );
		}
		else
		{
			outPlacement = EngineTransform( entityPlacementWS );
		}

		return true;
	}

	return false;
}

Bool CEdSceneEditor::OnTimeline_GetActorPelvisPlacementLS( const CName& actorVoicetag, EngineTransform& outPlacement ) const
{
	const CEntity* sceneEntity = GetSceneEntity( actorVoicetag );
	if ( const CActor* actor = Cast< const CActor >( sceneEntity ) )
	{
		const Int32 pelvisIdx = actor->GetRootAnimatedComponent()->FindBoneByName( CNAME(pelvis) );
		if ( pelvisIdx != -1 )
		{
			Matrix rootWS = actor->GetRootAnimatedComponent()->GetBoneMatrixWorldSpace( 0 );
			Matrix boneWS = actor->GetRootAnimatedComponent()->GetBoneMatrixWorldSpace( pelvisIdx );

			Vector pos = boneWS.GetTranslation();
			pos.Z = rootWS.GetTranslation().Z;

			EulerAngles rot = boneWS.ToEulerAngles();
			rot.Pitch = 0.f;
			rot.Roll = 0.f;

			outPlacement.SetPosition( pos );
			outPlacement.SetRotation( rot );

			return true;
		}
	}

	return false;
}

EngineTransform CEdSceneEditor::OnTimeline_GetScenePlacement() const
{
	return m_controller.GetCurrentScenePlacement();
}

const StorySceneCameraDefinition* CEdSceneEditor::OnTimeline_GetCameraDefinition( CName cameraName ) const
{
	return m_storyScene ? m_storyScene->GetCameraDefinition( cameraName ) : nullptr;
}

CWorld* CEdSceneEditor::OnTimeline_GetWorld() const
{
	return GetWorld();
}

/*
Sets line as a background or non-background line.

\param line Line on which to operate. Must not be nullptr.
\param state True - line is to be set as background line, false - line is to be set as a non-background line.
*/
void CEdSceneEditor::OnTimeline_SetAsBackgroundLine( CStorySceneLine* line, Bool state )
{
	ASSERT( line );
	m_controlRequest.RequestSetAsBackgroundLine( line, state );
}

void CEdSceneEditor::OnTimeline_CameraBlendNewPlot( const CStorySceneCameraBlendEvent* e, const TDynArray< String >& tracks )
{
	const Float timeStep = 3.f * 1.f / 30.f;

	TDynArray< TDynArray< TPair< Vector2, Vector > > > data;

	// Be careful, low level stuff
	//const CStoryScenePreviewPlayer* player = m_controller.GetPlayer();
	//e->SampleData( *(player->GetCurrentPlayingPlan()->m_sectionInstanceData.m_data), player, timeStep, tracks, data, GFeedback );

	//SetFCurveDataToPlot( tracks, data, true );
}

void CEdSceneEditor::OnTimeline_InterpolationEventPlot( const CStorySceneEventInterpolation* interpolationEvent, const TDynArray< String >& tracks )
{
	const Float timeStep = 3.0f * 1.0f / 30.0f;

	TDynArray< TDynArray< TPair< Vector2, Vector > > > data;

	const CStoryScenePreviewPlayer* player = m_controller.GetPlayer();
	CSceneEventFunctionSimpleArgs args( *(player->GetCurrentPlayingPlan()->m_sectionInstanceData.m_data), const_cast< CStoryScenePreviewPlayer* >( player ) );
	interpolationEvent->SampleData( timeStep, data, args );

	SetFCurveDataToPlot( tracks, data, true );
}

void CEdSceneEditor::OnTimeline_OnItemPropertyChanged( wxCommandEvent& event )
{
	// light definition updated?
	const CEdPropertiesPage::SPropertyEventData* propertyData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );
	if ( propertyData && propertyData->m_typedObject.As< CStorySceneLight >()  )
	{ 
		CStorySceneLight* light = propertyData->m_typedObject.As< CStorySceneLight >();
		if ( propertyData->m_propertyName == TXT("lightId") || propertyData->m_propertyName == TXT("type") )
		{
			RebuildLights();
		}
		else
		{
			RefreshLights();
		}
	}
	else if ( propertyData && propertyData->m_typedObject.As< CStorySceneEventLightProperties >() )
	{
		CStorySceneEventLightProperties* le = propertyData->m_typedObject.As< CStorySceneEventLightProperties >();
		if ( propertyData->m_propertyName == TXT("color") )
		{
			if( le->GetUseEnvColor() )
			{
				//le->SetUseCustomColor();
				//propertyData->m_page->RefreshValues();
			}			
		}
		else if ( propertyData->m_propertyName == TXT( "useGlobalCoords" ) )
		{
			if( !le->IsAttached() )
			{
				EngineTransform sceneToWorldET = m_controller.GetCurrentScenePlacement();
				EngineTransform& evtPlacement = le->GetTransformRef();
				if ( le->UseGlobalCoords() )
				{				
					evtPlacement = StorySceneUtils::CalcWSFromSS( evtPlacement, sceneToWorldET );
				}
				else
				{
					evtPlacement = StorySceneUtils::CalcSSFromWS( evtPlacement, sceneToWorldET );
				}	
				UpdateHelperEntityForEvent( le );
			}
		}
		else if ( propertyData->m_propertyName == TXT( "attachment" ) )
		{
			if( CEntity* lightEnt = m_controller.GetSceneEntity( le->GetSubject() ) )
			{
				Bool isAttached = lightEnt->GetTransformParent() != nullptr;
				Bool willAttach = le->IsAttached();

				if( isAttached && !willAttach )
				{
					Matrix toWorld;
					lightEnt->GetLocalToWorld( toWorld );

					Matrix world2scene;
					EngineTransform sceneToWorld = m_controller.GetCurrentScenePlacement();
					sceneToWorld.CalcWorldToLocal( world2scene );

					Matrix transform( Matrix::IDENTITY );
					if ( le->UseGlobalCoords() )
					{
						transform = toWorld * world2scene;
					}
					else
					{
						transform = toWorld;
					}
							
					le->GetTransformRef() = EngineTransform( transform );
					RefreshPlayer();
				}
				else if( !isAttached && willAttach )
				{
					if( CEntity* actor = m_controller.GetSceneEntity( le->GetAttachmentActor() ) )
					{
						if( CAnimatedComponent* ac = actor->GetRootAnimatedComponent() )
						{
							Matrix scene2world;
							EngineTransform sceneToWorldET = m_controller.GetCurrentScenePlacement();
							sceneToWorldET.CalcLocalToWorld( scene2world );						
							Matrix world2local = StorySceneUtils::CalcL2WForAttachedObject( ac, le->GetAttachmentBone(), le->GetAttachmentFlags() ).FullInverted();

							Matrix transform;
							le->GetTransform().CalcLocalToWorld( transform );
							
							if ( le->UseGlobalCoords() )
							{
								transform = transform * world2local;
							}
							else
							{
								transform = transform * scene2world * world2local;
							}
							

							le->GetTransformRef() = EngineTransform( transform );
							RefreshPlayer();
						}	
					}				
				}
			}
			m_helperEntitiesCtrl.DeselectAllHelpers();			
		}
		RefreshLights();
	}
}

void CEdSceneEditor::OnTimeline_RefreshPlayer()
{
	RefreshPlayer();
}

void CEdSceneEditor::OnTimeline_RebuildPlayer()
{
	RebuildPlayer();
}

Vector CEdSceneEditor::OnTimeline_CalcLightSpawnPositionSS() const
{
	return CalcBestSpawnPositionSS();
}

Vector CEdSceneEditor::OnTimeline_CalcLightSpawnPositionWS() const
{
	return CalcBestSpawnPositionWS();
}

void CEdSceneEditor::OnTimeline_GetVoiceTagsForCurrentSetting( TDynArray< CName >& vt ) const
{
	m_controller.GetVoiceTagsForCurrentSetting( vt );
}


void CEdSceneEditor::OnTimeline_GetCurrentLightState( CName lightId, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const
{
	m_controller.GetCurrentLightState( lightId, out, outPos );
}


void CEdSceneEditor::OnTimeline_GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const
{
	GetMimicIdleData( data, poseWeight, actor );
}

void CEdSceneEditor::OnTimeline_TrackSelectionChanged( const String& trackName )
{
	if ( m_detailsTabs->GetSelection() == PAGE_BODY_ANIMS )
	{
		RefreshBodyAnimTree();
	}
	else if ( m_detailsTabs->GetSelection() == PAGE_MIMICS_ANIMS )
	{
		RefreshMimicsAnimTree();
	}
}

Bool CEdSceneEditor::OnTimeline_AdjustCameraForActorHeight( const StorySceneCameraDefinition& def, EngineTransform* outAdjusted ) const
{ 
	return m_controller.GetPlayer()->AdjustCameraForActorHeight( def, outAdjusted, nullptr );
}

void CEdSceneEditor::OnTimeline_ToggleTimePause()
{
	ToggleTimePause();
}

void CEdSceneEditor::OnTimeline_GoToTime( Float time )
{
	m_controlRequest.RequestTime( time );
}

void CEdSceneEditor::OnTimeline_ToggleIsInteractiveEntityHelper()
{
	TDynArray< CEdSceneHelperEntity* > selectedEntities;
	m_helperEntitiesCtrl.CollectSelectedHelpers( selectedEntities );
	for ( CEdSceneHelperEntity* helperEntity : selectedEntities )
	{
		for ( CComponent* component : helperEntity->GetComponents() )
		{
			if ( component->IsA< CEdSceneHelperComponent >() )
			{
				CEdSceneHelperComponent* helperComponent = Cast< CEdSceneHelperComponent >( component );
				helperComponent->ToggleInteractive();
				break;
			}
		}
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
