
#include "build.h"
#include "dialogEditorSceneCtrl.h"
#include "dialogEditor.h"

#include "dialogEditorActions.h"

#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"

#include "../../common/engine/gameResource.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneGraphBlock.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneDebugger.h"
#include "../../common/game/storySceneDisplayInterface.h"
#include "storyScenePreviewPlayer.h"
#include "../../common/game/storySceneControlPartsUtil.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CEdSceneCtrl::CEdSceneCtrl()
	: m_player( NULL )
	, m_editor( NULL )
	, m_scene( NULL )
	, m_sceneController( NULL )
	, m_csWorldInterface( NULL )
{

}

CStoryScenePreviewPlayer* CEdSceneCtrl::CreatePlayer( const CStorySceneInput* sugestedInput )
{
	m_sceneController = NULL;

	if ( m_scene->GetInputNames().Size() > 0 )
	{	
		const CStorySceneInput* input = nullptr;		
		if ( sugestedInput )
		{
			input = sugestedInput;
		} 
		else
		{
			const String inputName = m_scene->GetInputNames()[ 0 ];
			input = m_scene->FindInput( inputName );
		}		
		if ( input )
		{
			StorySceneControllerSetup setup;
			setup.m_mustPlay = true;
			setup.m_spawnAllActors = true;
			setup.m_useApprovedVoDurations = m_editor->GetConfigUseApprovedVoDurations();
			setup.m_scenePlayerClass = ClassID< CStoryScenePreviewPlayer >();
			setup.m_world = m_editor->GetWorld();
			setup.m_asyncLoading = false;			
			setup.m_contextActorsProvider = m_editor->ResetAndGetSceneContextActorsProvider();

			// Hack
			setup.m_hackFlowCtrl = m_editor->HACK_GetFlowCtrl();

			if ( m_csWorldInterface )
			{
				delete m_csWorldInterface;
			}
			m_csWorldInterface = new CCutsceneEmptyWorldInterface( m_editor->GetWorld() );

			// DIALOG_TOMSIN_TODO
			//setup.m_sceneDisplay = m_editor->GetStorySceneDisplay();
			setup.m_sceneDebugger = m_editor->GetStorySceneDebugger();
			setup.m_csWorldInterface = m_csWorldInterface;

			m_sceneController = GCommonGame->GetSystem< CStorySceneSystem >()->PlayInput( input, setup );
			if ( m_sceneController )
			{
				CStoryScenePreviewPlayer* player = SafeCast< CStoryScenePreviewPlayer >( m_sceneController->GetPlayer() );
				return player;
			}
		}
	}

	ASSERT( 0 );

	return NULL;
}

void CEdSceneCtrl::Init( CEdSceneEditor* e, const CStoryScene* s )
{
	m_editor = e;
	m_scene = s;

	m_player = CreatePlayer();
}

void CEdSceneCtrl::Destroy()
{
	ASSERT( m_player );

	if ( m_sceneController != nullptr )
	{
		m_sceneController->Stop( SCR_NONE );
		m_sceneController = nullptr;
	}

	GCommonGame->GetSystem< CStorySceneSystem >()->FreeFinished();

	//m_player->Stop();
	m_player = NULL;
}

Bool CEdSceneCtrl::IsValid() const
{
	return m_player != nullptr;
}

void CEdSceneCtrl::Refresh()
{
	ScenePlayerInputState state;
	state.RequestRestart();
	state.RequestTime( m_player->GetCurrentSectionTime() );
	state.RequestPause( m_player->IsPaused() );
	m_player->SetState( state );
}

ScenePlayerInputState CEdSceneCtrl::Rebuild( const CStorySceneSection* startFromSection )
{
	ScenePlayerInputState state;
	state.RequestSection( startFromSection );
	state.RequestTime( GetSectionTime() );
	state.RequestPause( m_player->IsPaused() );

	Bool wasContUsed = m_player->GetAutoGoToNextSection();
	auto previewGrid = m_player->GetPreviewGrid();

	m_sceneController->Stop( SCR_NONE );
	//m_player->Stop();

	GCommonGame->GetSystem< CStorySceneSystem >()->FreeFinished();

	m_editor->ClearAllSceneDataForActors();

	const CStorySceneInput* input = m_editor->HACK_GetFlowCtrl()->GetInputFor( startFromSection );
	m_player = CreatePlayer( input );
	if ( m_player )
	{
		if ( startFromSection )
		{
			m_player->SetState( state );
		}

		m_player->SetAutoGoToNextSection( wasContUsed );
		m_player->SetPreviewGrid( previewGrid );
	}

	return state;
}

Bool CEdSceneCtrl::IsChangingSection() const
{
	return m_player->IsChangingSection();
}

Bool CEdSceneCtrl::ForceState( const ScenePlayerInputState& state )
{
	ScenePlayerInputState newState( state );
	newState.RequestPause( m_player->IsPaused() );

	const CStorySceneInput* newInput = m_editor->HACK_GetFlowCtrl()->GetInputFor( state.m_section2 );
	const CStorySceneInput* oldInput = m_editor->HACK_GetFlowCtrl()->GetInputFor( m_player->GetCurrentSection() );

	if ( newInput != oldInput && newInput )
	{
		Rebuild( state.m_section2 );
	}

	return m_player->SetState( newState );
}

Bool CEdSceneCtrl::SetState( const ScenePlayerInputState& state )
{
	return m_player->SetState( state );
}

void CEdSceneCtrl::Tick( Float dt )
{	
	Bool ret = m_player->Tick( dt );
}

Float CEdSceneCtrl::GetSectionTime() const
{
	return m_player->GetCurrentSectionTime();
}

void CEdSceneCtrl::TogglePause()
{
	if ( m_player )
	{
		m_player->TogglePause();
	}
}

void CEdSceneCtrl::Pause()
{
	if ( m_player )
	{
		m_player->Pause( true );
	}
}

Bool CEdSceneCtrl::IsPaused() const
{
	return m_player->IsPaused();
}

void CEdSceneCtrl::Freeze()
{
	m_player->Freeze();
}

void CEdSceneCtrl::RestartSection()
{
	ScenePlayerInputState state;
	state.RequestTime( 0.f );
	m_player->SetState( state );
	m_player->Pause( true );
	m_player->HandleOnEndSoundEvents(nullptr);
	m_player->HandleOnSkipSoundEvents();
}

void CEdSceneCtrl::PlayOneFrame()
{
	static Float dt = 1.f / 30.f;
	m_player->Pause( false );
	m_player->Tick( dt );
	m_player->Pause( true );
}

Bool CEdSceneCtrl::CalculateCamera( IViewport* view, CRenderCamera &camera ) const
{
	return m_player && m_player->CalculateCamera( view, camera );
}

EngineTransform	CEdSceneCtrl::GetCurrentScenePlacement() const
{
	return m_player ? m_player->GetCurrentScenePlacement() : EngineTransform();
}

const CStorySceneSection* CEdSceneCtrl::GetCurrentSection() const
{
	return m_player->GetCurrentSection();
}

CStorySceneSection* CEdSceneCtrl::GetCurrentSection()
{
	return m_player->GetCurrentSection();
}

const CStorySceneDialogsetInstance* CEdSceneCtrl::GetCurrentDialogsetInstance() const
{
	return m_player->GetCurrentDialogsetInstance();
}

const CStorySceneDialogsetInstance* CEdSceneCtrl::GetCurrentDialogsetInstanceIfValid() const
{
	return m_player->GetCurrentDialogsetInstanceIfValid();
}

CActor* CEdSceneCtrl::GetSceneActor( const CName& actorName )
{
	return Cast< CActor >( m_player->GetSceneActorEntity( actorName ) );
}

CEntity* CEdSceneCtrl::GetSceneEntity( const CName& actorName )
{
	return const_cast< CEntity* >( static_cast< const CEdSceneCtrl*>( this )->GetSceneEntity( actorName ) );
}

const CEntity* CEdSceneCtrl::GetSceneEntity( const CName& actorName ) const
{
	// TODO: this should be refactored to use the actor name to look up in a THashMap<> or similar...

	const CEntity* entity = m_player->GetSceneActorEntity( actorName );
	entity = ( !entity ? m_player->GetScenePropEntity( actorName ) : entity );
	entity = ( !entity ? m_player->GetSceneEffectEntity( actorName ) : entity );
	entity = ( !entity ? m_player->GetSceneLightEntity( actorName ) : entity );
	return entity;	
}

CCamera* CEdSceneCtrl::GetCamera()
{
	return m_player->GetSceneCamera();
}

const CActor* CEdSceneCtrl::AsSceneActor( const CEntity* e ) const
{
	return m_player->AsSceneActor( e );
}

const CEntity* CEdSceneCtrl::AsSceneProp( const CEntity* e ) const
{
	return m_player->AsSceneProp( e );
}

const CEntity* CEdSceneCtrl::AsSceneLight( const CEntity* e ) const
{
	return m_player->AsSceneLight( e );
}

StorySceneCameraState CEdSceneCtrl::GetCameraState() const
{
	return m_player->GetCurrentCameraState();
}

Bool CEdSceneCtrl::GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_player->GetCurrentActorAnimationState( actor, out );
}

Bool CEdSceneCtrl::GetCurrentLightState( const CName& actor, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const
{
	return m_player->GetCurrentLightState( actor, out, outPos );
}

Bool CEdSceneCtrl::GetPreviousActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_player->GetPreviousActorAnimationState( actor, out );
}

Bool CEdSceneCtrl::GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight )  const
{
	return m_player->GetCurrentActorAnimationMimicState( actor, mimicEmoState, mimicLayerEyes, mimicLayerPose, mimicLayerAnim, poseWeight );
}

void CEdSceneCtrl::GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const
{
	return m_player->GetActorAnimationNames( actor, out );
}

Bool CEdSceneCtrl::GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const
{
	return m_player->GetActorCurrIdleAnimationNameAndTime( actorId, animName, animTime );
}

Bool CEdSceneCtrl::GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const
{
	return m_player->GetActorCurrAnimationTime( actorId, animName, animTime );
}

void CEdSceneCtrl::GetVoiceTagsForCurrentSetting( TDynArray< CName >& vt ) const
{
	m_player->GetVoiceTagsForCurrentSetting( vt );
}

Matrix CEdSceneCtrl::GetActorPosition( const CName& actor ) const
{
	return m_player->GetActorPosition( actor );
}

Matrix CEdSceneCtrl::GetDialogPosition() const
{
	return m_player->GetDialogPosition();
}

Bool CEdSceneCtrl::ReloadCamera( const StorySceneCameraDefinition* cameraDefinition )
{
	return m_player->ReloadCamera( cameraDefinition );
}

CName CEdSceneCtrl::GetPrevSpeakerName( const CStorySceneElement* currElement ) const
{
	return m_player->GetPrevSpeakerName( currElement );
}

const CStorySceneLine* CEdSceneCtrl::GetPrevLine( const CStorySceneElement* currElement )
{
	return m_player->GetPrevLine( currElement );
}

const CStoryScenePreviewPlayer* CEdSceneCtrl::GetPlayer() const
{
	return m_player;
}

CStoryScenePreviewPlayer* CEdSceneCtrl::HACK_GetPlayer()
{
	return m_player;
}

const IStorySceneElementInstanceData* CEdSceneCtrl::FindElementInstance( const CStorySceneElement* element ) const
{
	return m_player->FindElementInstance( element );
}

Bool CEdSceneCtrl::LocalVoMatchApprovedVoInCurrentSectionVariant() const
{
	return m_player->LocalVoMatchApprovedVoInCurrentSectionVariant();
}

Bool CEdSceneCtrl::UseApprovedDurations() const
{
	return m_player->UseApprovedVoDurations();
}

void CEdSceneCtrl::SetAutoGoToNextSection( Bool flag )
{
	m_player->SetAutoGoToNextSection( flag );
}

void CEdSceneCtrl::PlaySpeechSound( Bool flag )
{
	m_player->PlaySpeechSound( flag );
}

const THashMap< CName, THandle< CEntity > >& CEdSceneCtrl::GetActorMap() const
{
	return m_player->GetSceneActors();
}

void CEdSceneCtrl::SetPreviewGrid( SSPreviewPlayerCompGrid grid )
{
	m_player->SetPreviewGrid( grid );
}

Bool CEdSceneCtrl::FindEventsByTime( const CClass* c, Float time, TDynArray< CStorySceneEvent* >& out )
{
	return m_player->FindEventsByTime( c, time, out );
}

Bool CEdSceneCtrl::FindEventsByType( const CClass* c, TDynArray< CStorySceneEvent* >& out )
{
	return m_player->FindEventsByType( c, out );
}

void CEdSceneCtrl::GetEventAbsTime( const CStorySceneEvent* e, Float& time, Float& duration ) const
{
	m_player->GetEventAbsTime( e, time, duration );
}

Float CEdSceneCtrl::GetEventScalingFactor( const CStorySceneEvent& e ) const
{
	return m_player->GetEventScalingFactor( e );
}

Float CEdSceneCtrl::GetEventDuration( const CStorySceneEvent& e ) const
{
	return m_player->GetEventDuration( e );
}

void CEdSceneCtrl::SetEventDuration( const CStorySceneEvent& e, Float duration ) const
{
	m_player->SetEventDuration( e, duration );
}

Float CEdSceneCtrl::GetEventStartTime( const CStorySceneEvent& e ) const
{
	return m_player->GetEventStartTime( e );
}

void CEdSceneCtrl::SetEventStartTime( const CStorySceneEvent& e, Float startTime ) const
{
	m_player->SetEventStartTime( e, startTime );
}

void CEdSceneCtrl::SetCameraAdjustedDebugFrame( Bool adjusted )
{
	m_player->SetCameraAdjustedDebugFrame( adjusted );
}

TDynArray< CName > CEdSceneCtrl::GetActorIds( Int32 actorTypes ) const
{
	return m_player->GetActorIds( actorTypes );	
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
