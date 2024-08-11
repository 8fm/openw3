/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storyScenePlayer.h"

#include "../core/gatheredResource.h"
#include "../core/depot.h"
#include "../core/fileSystemProfilerWrapper.h"

#include "../engine/behaviorGraphStack.h"
#include "../engine/soundStartData.h"
#include "../engine/deniedAreaComponent.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "../engine/camera.h"
#include "../engine/soundSystem.h"
#include "../engine/videoPlayer.h"
#include "../engine/tagManager.h"
#include "../engine/viewport.h"
#include "../engine/gameTimeManager.h"
#include "../engine/componentIterator.h"
#include "../engine/environmentManager.h"
#include "../engine/renderCommands.h"
#include "../engine/pathlibWorld.h"
#include "../engine/renderFrame.h"
#include "../engine/localizationManager.h"

#include "movableRepresentationPathAgent.h"
#include "movingPhysicalAgentComponent.h"
#include "actorsManager.h"
#include "gameWorld.h"
#include "factsDB.h"

#include "storySceneIncludes.h"
#include "storySceneEventMimicFilter.h"
#include "storySceneEventMimicPose.h"
#include "storySceneComponent.h"
#include "storySceneCutsceneSection.h"
#include "storySceneInput.h"
#include "storyScenePlaybackListener.h"
#include "storySceneRandomizer.h"
#include "storySceneScriptingBlock.h"
#include "storySceneSection.h"
#include "storySceneEvent.h"
#include "storySceneEventCustomCamera.h"
#include "storySceneEventCustomCameraInstance.h"
#include "storySceneEventDuration.h"
#include "storySceneSectionPlayingPlan.h"
#include "storySceneFlowCondition.h"
#include "storySceneFlowSwitch.h"
#include "storySceneItems.h"
#include "storySceneSectionOverrideCondition.h"
#include "storySceneSystem.h"
#include "sceneLog.h"
#include "storySceneChoiceLine.h"
#include "storySceneChoice.h"
#include "storySceneVideo.h"
#include "storySceneDisplayInterface.h"
#include "storySceneDebugger.h"
#include "storySceneEventFade.h"
#include "../engine/cutsceneInstance.h"
#include "binaryStorage.h"
#include "commonGame.h"

// For texture pre-streaming
#include "../engine/renderTextureStreamRequest.h"
#include "../engine/extAnimCutsceneBodyPartEvent.h"
#include "extAnimItemEvents.h"
#include "storySceneCutsceneSection.h"
#include "storySceneEventEquipItem.h"
#include "../engine/renderFramePrefetch.h"
#include "storySceneUtils.h"
#include "questsSystem.h"
#include "../engine/rigidMeshComponent.h"

IMPLEMENT_ENGINE_CLASS( CStoryScenePlayer );

RED_DEFINE_NAME( StorySceneDebugRefresh )

CGatheredResource resDialogsetBehavior( TXT("characters\\templates\\interaction\\dialogsets\\dialogsets.w2beh"), RGF_Startup );

const String DEFAULT_BLACKSCREEN_REASON( TXT( "Section not loaded" ) );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

// Add an item to an IRenderTextureStreamRequest. This can't be a part of the request, since items exist in the game project.
static void RequestTextureForItem( const CEntity* ent, CName itemName, IRenderTextureStreamRequest* request )
{
	// If we couldn't get an existing entity for the item, we'll try to scan it's entity template.
	CName appearance;
	CEntityTemplate* templ = nullptr;
	if ( ent )
	{
		const Bool forPlayer = ent->IsPlayer();
		if( const CGameplayEntity* gpent = Cast< const CGameplayEntity>(ent) )
		{
			// Template is not preloaded, load sync
			if( const CInventoryComponent* inv = gpent->GetInventoryComponent() )
			{
				const String& templateName = inv->GetTemplate( itemName );
				const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( templateName );
				
				templ = SItemEntityManager::GetInstance().GetPreloadedEntityTemplate( itemName, templatePath );
				if ( !templ && !templatePath.Empty() )
				{
					CResource* res = GDepot->FindResource( templatePath );
					templ = Cast< CEntityTemplate >( res );
				}
			}
		}

		if( const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( itemName ) )
		{
			appearance = itemDef->GetItemAppearanceName( forPlayer );
		}
	}

	if ( templ )
	{
		request->AddEntityTemplate( templ );
		request->AddEntityAppearance( templ, appearance );
	}
}


//////////////////////////////////////////////////////////////////////////

CStoryScenePlayer::InternalFlowState::InternalFlowState()
	: m_currentFlowElement( NULL )
	, m_activatedOutput( NULL )
	, m_currentSection( NULL )
	, m_prevSection( nullptr )
	, m_pendingNextSection( NULL )
	, m_isInTick( false )
	, m_skipLine( false )
	, m_justSkippedElement( false )
	, m_hasPendingChoice( false )
	, m_planId( -1 )
	, m_input( NULL )
	, m_sceneStarted( false )
	, m_resumeRequest( NULL )
	, m_timer( 0.f )
	, m_selectedChoice( NULL )
	, m_choiceToReturnTo( nullptr )
	, m_scheduledNextElements( 0 )
	, m_isFrozen( false )
	, m_sectionInputNumber( -1 )
	, m_timeMultiplier( 1.f )
	, m_sceneProgress( 0.f ) 
	, m_canSkipElement( false )
	, m_injectedScene( nullptr )
	, m_pendingNextSectionStage( PNSS_Reset )
	, m_sceneEndSkipped( false )
	, m_sceneEndTimer( 0.f )
	, m_sceneEndFadeSet( false )
{
#ifdef USE_SCENE_FLOW_MARKERS
	m_flowMarkers.Reserve( 100 );
#endif
}

CStoryScenePlayer::InternalFlowState::Intro::Intro()
	: m_state( IntroState_Initial )
	, m_time( 0.0f )
	, m_destSection( nullptr )
	, m_cutsceneInstance( nullptr )
	, m_fadeOutStarted( false )
{}

void CStoryScenePlayer::InternalFlowState::SetCurrentFlowElement( const CStorySceneLinkElement* e ) 
{ 
	m_currentFlowElement = e; 
}

void CStoryScenePlayer::InternalFlowState::ResetCurrentFlowElement()	
{ 
	m_currentFlowElement = NULL; 
}

void CStoryScenePlayer::InternalFlowState::SetActivatedOutput( const CStorySceneOutput* o )
{
	m_activatedOutput = o;
}

void CStoryScenePlayer::InternalFlowState::ResetActivatedOutput()
{
	m_activatedOutput = NULL;
}

void CStoryScenePlayer::InternalFlowState::SetCurrentSection( const CStorySceneSection* s )
{
	m_prevSection = m_currentSection;
	m_currentSection = s;
}

void CStoryScenePlayer::InternalFlowState::ResetCurrentSection()
{
	m_prevSection = nullptr;
	m_currentSection = nullptr;
}

void CStoryScenePlayer::InternalFlowState::SetPendingNextSection( const CStorySceneSection* s )
{
	SCENE_ASSERT__FIXME_LATER( !m_pendingNextSection );
	SCENE_ASSERT__FIXME_LATER( m_pendingNextSectionStage == PNSS_Reset );

	m_pendingNextSection = s;
	m_pendingNextSectionStage = PNSS_Init;
}

void CStoryScenePlayer::InternalFlowState::ResetPendingNextSection()
{
	SCENE_ASSERT__FIXME_LATER( m_pendingNextSection );
	SCENE_ASSERT__FIXME_LATER( m_pendingNextSectionStage == PNSS_Finish );

	m_pendingNextSection = nullptr;
	m_pendingNextSectionStage = PNSS_Reset;
}

void CStoryScenePlayer::InternalFlowState::SetSectionInputNumber( Int32 index )
{
	m_sectionInputNumber = index;
}

void CStoryScenePlayer::InternalFlowState::ResetSectionInputNumber()
{
	m_sectionInputNumber = -1;
}

#ifdef USE_SCENE_FLOW_MARKERS
void CStoryScenePlayer::InternalFlowState::AddFlowMarker( const FlowMarker& m )
{
	if ( m_flowMarkers.Size() < 100 )
	{
		m_flowMarkers.PushBack( m );
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

CStoryScenePlayer::CStoryScenePlayer()
	: m_isStopped(true)
	, m_storyScene( NULL )
	, m_scriptBlockThread( NULL )
	, m_isPrecached( false )
	, m_sceneController( NULL )
	, m_saveBlockId( -1 )
	, m_interceptionHelper( this )
	, m_allowTickDuringFade( false )
	, m_world( NULL )
	, m_player( NULL )
	, m_csWorldInterface( NULL )
	, m_switchedToGameplayCamera( false )
	, m_resetGameplayCameraOnOutput( true )
	, m_didStartBlendingOutLights( false )
	, m_useApprovedVoDurations( false )
	, m_frozenFrameSet( false )
	, m_blackscreenSet( false )
	, m_isDestoryed( false )
	, m_isPaused( 0 )
	, m_currentSectionTexRequest( nullptr )
	, m_currentBackgroundRequest( nullptr )
	, m_newBackgroundRequest( nullptr )
	, m_framePrefetch( nullptr )
	, m_sceneInProfiler( false )
	, m_IsBlockingMusic( false )
	, m_sceneIsOverridingListener( false )
	, m_sectionIsOverridingListener( false )
{
}

CStoryScenePlayer::~CStoryScenePlayer()
{
	SAFE_RELEASE( m_currentSectionTexRequest );
	SAFE_RELEASE( m_currentBackgroundRequest );
	SAFE_RELEASE( m_newBackgroundRequest );
	SAFE_RELEASE( m_framePrefetch );
}

void CStoryScenePlayer::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Scenes );
}

void CStoryScenePlayer::OnDetached( CWorld* world )
{
	if ( !m_isStopped )
	{
		m_sceneController->Stop( SCR_PLAYER_DETACHED );
	}

	// Reset binding to thread
	if ( m_scriptBlockThread )
	{
		m_scriptBlockThread->SetListener( NULL );
		m_scriptBlockThread = NULL;
		m_scriptBlock = NULL;
	}

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Scenes );

	TBaseClass::OnDetached( world );
}

void CStoryScenePlayer::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		m_sectionLoader.SerializeForGC( file );
	}
}

String CStoryScenePlayer::GetFriendlyName() const
{
	String str = TBaseClass::GetFriendlyName();
	if ( m_storyScene )
	{
		str += m_storyScene->GetFriendlyName();
	}

	return str;
}

Bool CStoryScenePlayer::Init( CStorySceneController& controller, const ScenePlayerPlayingContext& context )
{
	SCENE_ASSERT( !m_isDestoryed );

	ResetPlayer();

	// No input to start
	m_sceneController = &controller;
	if ( m_sceneController == nullptr )
	{
		SCENE_ASSERT__FIXME_LATER( 0 );
		SCENE_WARN( TXT("No input to start by scene player") );
		return false;
	}
	if ( m_sceneController->GetInput() == nullptr )
	{
		SCENE_ASSERT__FIXME_LATER( 0 );
		SCENE_WARN( TXT("No input to start by scene player") );
		return false;
	}
	if ( m_sceneController->GetInput()->GetScene() == nullptr )
	{
		SCENE_ASSERT__FIXME_LATER( 0 );
		SCENE_WARN( TXT("No input to start by scene player") );
		return false;
	}

	m_world = context.m_world;
	m_player = context.m_playerEntity;
	m_display = context.m_display;
	m_debugger = context.m_debugger;
	m_csWorldInterface = context.m_csWorldInterface;
	m_useApprovedVoDurations = context.m_useApprovedVoDurations;

	m_scene = m_sceneController->GetInput()->GetScene();

	GCommonGame->GetSystem< CQuestsSystem >()->GetContextDialogsForScene( m_scene, m_injectedScenes );
	
	SetupPlayerName( m_sceneController->GetInput() );

	m_isGameplay = m_sceneController->GetInput()->IsGameplay();
	m_storyScene = m_sceneController->GetInput()->GetScene();

	//Initialize sound banks loading
	const TDynArray< CName > & banksDependency = m_storyScene->GetBanksDependency();
	Uint32 banksCount = banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->QueueLoading();
	}

	// Fill actors map
	const TDynArray< CStorySceneController::SceneActorInfo > & mappedActors = m_sceneController->m_mappedActors;
	for ( Uint32 i = 0; i < mappedActors.Size(); ++i )
	{
		if ( mappedActors[ i ].m_actor.Get() != NULL )
		{
			m_sceneActorEntities.Insert( mappedActors[ i ].m_voicetag, mappedActors[ i ].m_actor );
		}
	}
	
	// Fill effects map
	const TDynArray< CStorySceneController::ScenePropInfo > & mappedEffects = m_sceneController->m_mappedEffects;
	for ( Uint32 i = 0; i < mappedEffects.Size(); ++i )
	{
		if ( mappedEffects[ i ].m_prop.Get() != NULL )
		{
			m_sceneEffectEntities.Insert( mappedEffects[ i ].m_id, mappedEffects[ i ].m_prop );
		}
	}

	// Fill light map
	const TDynArray< CStorySceneController::ScenePropInfo > & mappedLights = m_sceneController->m_mappedLights;
	for ( Uint32 i = 0; i < mappedLights.Size(); ++i )
	{
		if ( mappedLights[ i ].m_prop.Get() != NULL )
		{
			m_sceneLightEntities.Insert( mappedLights[ i ].m_id, mappedLights[ i ].m_prop );
		}
	}

	m_sceneDirector.Initialize( this, m_sceneController->GetInput() );
	m_sceneDirector.SetAllowCameraReactivation( false );

	m_sectionLoader.m_asyncLoading = context.m_asyncLoading;

	{
		// Layout jest w story scene
		//m_data = dataLayout.CreateBuffer( this, TXT( "CStoryScenePlayer" ) );
	}

	m_eventsExecutor.Init( &m_sceneActorEntities, &m_scenePropEntities, &m_sceneLightEntities, GetSceneCamera() );

	if ( IsSceneInGame() )
	{
		m_blackscreenSet = GGame ? GGame->HasBlackscreenRequested() : false;
	}

	OnSceneStarted();

	m_internalState.m_input = m_sceneController->GetInput();
	OnInit( context );

	Tick( 0.f );

	if ( !IsGameplay() )
	{
		UpdateInfoAboutSkipping( true );	
	}

	SCENE_LOG( TXT("%ls - started playing scene"), m_sceneController->GetInput()->GetFriendlyName().AsChar() );

	return true;
}

Bool CStoryScenePlayer::DetermineSceneBounds( const SStorySceneDialogsetInstanceCalcBoundDesc& desc, Vector& center, Box& box, TDynArray< Vector >& convex, const CStorySceneSection* section = nullptr ) const
{
	// Gather scene bounding vertices

	struct AddDialogSetInstanceSlotPositions
	{
		const SStorySceneDialogsetInstanceCalcBoundDesc& m_desc;
		const CStoryScenePlayer& m_storyScenePlayer;
		const CStorySceneSection* m_section;

		AddDialogSetInstanceSlotPositions( const CStoryScenePlayer& storyScenePlayer, const SStorySceneDialogsetInstanceCalcBoundDesc& desc, const CStorySceneSection* section )
			: m_desc( desc )
			, m_storyScenePlayer( storyScenePlayer )
			, m_section( section )
		{}

		void Perform( const CStorySceneDialogsetInstance* dialogSetInstance, TDynArray< Vector >& convex )
		{
			for ( const CStorySceneDialogsetSlot* slot : dialogSetInstance->GetSlots() )
			{
				const Vector& slotPosition = slot->GetSlotPlacement().GetPosition();

				// Add 4 points around the actual slot position so as to account for slot area (and make it an actual area, not a point or a line)

				convex.PushBack( slotPosition + Vector( m_desc.m_safeZone, 0.0f, 0.0f )		);
				convex.PushBack( slotPosition + Vector( -m_desc.m_safeZone, 0.0f, 0.0f )	);
				convex.PushBack( slotPosition + Vector( 0.0f, m_desc.m_safeZone, 0.0f )		);
				convex.PushBack( slotPosition + Vector( 0.0f, -m_desc.m_safeZone, 0.0f )	);
			}

			if ( m_desc.m_includeCameras )
			{
				if ( m_section )
				{
					for( CStorySceneEvent* evt : m_section->GetEventsFromAllVariants() )
					{
						if ( CStorySceneEventCustomCamera* cam = Cast<CStorySceneEventCustomCamera>( evt ) )
						{
							convex.PushBack( cam->GetCameraTranslation() );
						}
						else if ( CStorySceneEventCustomCameraInstance* camInstance = Cast<CStorySceneEventCustomCameraInstance>( evt ) )
						{
							convex.PushBack( camInstance->GetCameraDefinition()->m_cameraTransform.GetPosition() );
						}
					}

				}
			}
		}
	} addDialogSetInstanceSlotPositions( *this, desc, section );

	if ( desc.m_dialogSetInstance )
	{
		addDialogSetInstanceSlotPositions.Perform( desc.m_dialogSetInstance, convex );
	}
	else // Take into account all dialog set instances if none is provided
	{
		for ( const CStorySceneDialogsetInstance* it : m_storyScene->GetDialogsetInstances() )
		{
			addDialogSetInstanceSlotPositions.Perform( it, convex );
		}
	}

	// Compute 2D convex shape of the scene

	if ( convex.Size() < 3 )
	{
		return false;
	}
	Compute2DConvexHull( convex );

	if ( convex.Size() < 3 )
	{
		return false;
	}

	// Transform convex by parent transform

	Matrix parentMatrix;
	desc.m_parentTransform.CalcLocalToWorld( parentMatrix );
	for ( Uint32 i = 0; i < convex.Size(); ++i )
	{
		convex[ i ] = parentMatrix.TransformPoint( convex[ i ] );
	}

	// Figure out scene's center and its bounding box

	center = Vector::ZEROS;

	box.Clear();
	for ( auto it = convex.Begin(), end = convex.End(); it != end; ++it )
	{
		center += *it;
		box.AddPoint( *it );
	}

	center /= ( Float ) convex.Size();

	box.Max.Z = box.Min.Z + 2.0f; // Make the box 2 meters high

	return true;
}


Matrix CStoryScenePlayer::GetSceneSectionTransform( const CStorySceneSection* section ) const
{
	Matrix transform = Matrix::IDENTITY;
	if ( auto csSection = Cast< const CStorySceneCutsceneSection >( section ) )
	{
		csSection->GetCsTemplate()->GetCsPointMatrix( transform );
	}
	else
	{
		const CStorySceneDialogsetInstance* dialogSet = GetSceneDirector()->GetCurrentSettingsIfValid();
		if ( !dialogSet )
		{
			dialogSet = GetSceneDirector()->GetDesiredSettings();
		}

		CName nextDialogset = section->GetDialogsetChange();
		if ( nextDialogset )
		{
			dialogSet = section->GetScene()->GetDialogsetByName( nextDialogset );
		}

		if ( dialogSet )
		{
			EngineTransform sceneL2WTx;
			GetReferencePlacement( dialogSet, sceneL2WTx );
			sceneL2WTx.CalcLocalToWorld( transform );
		}
	}

	return transform;
}


void CStoryScenePlayer::Precache()
{
	// DIALOG_TOMSIN_TODO
	// Propably nothing more depends on this flag - check it and remove it
	m_isPrecached = true;
}

Float CStoryScenePlayer::GetCurrentSectionTime() const 
{ 
	if ( m_internalState.m_planId != -1 )
	{
		if ( const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId ) )
		{
			return plan->GetCurrentSectionTime();
		}
	}

	return 0.f; 
}

Bool CStoryScenePlayer::IsPlanInProgress() const
{
	return m_internalState.m_planId != -1;
}

#ifndef NO_EDITOR
void CStoryScenePlayer::Freeze()
{
	m_internalState.m_isFrozen = true;

	ChangeSection( NULL, true, false );

	FinishChangingSection();
}
#endif

Bool CStoryScenePlayer::ShouldGenerateEditorFragments( CRenderFrame* frame ) const
{
	const Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	const Vector currentPos = GetSceneDirector()->Debug_GetCurrentScenePlacement().GetPosition();
	const Float distFromCam = frame->GetFrameInfo().m_camera.GetPosition().DistanceSquaredTo( currentPos );
	return distFromCam < Red::Math::MSqr( dist );
}

void CStoryScenePlayer::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	m_sceneDirector.OnGenerateEditorFragments( frame, flags );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Scenes ) )
	{
		Vector direction1 = m_internalState.m_cameraState.m_rawRotation.TransformVector( Vector::EY );
		frame->AddDebug3DArrow( m_internalState.m_cameraState.m_rawPosition, direction1 , 0.1f, 0.004f, 0.008f, 0.01f, Color::YELLOW, Color::YELLOW );

		Vector direction2 = m_internalState.m_cameraState.m_rotation.TransformVector( Vector::EY ) * 1.25f;
		frame->AddDebug3DArrow( m_internalState.m_cameraState.m_position, direction2 , 0.1f, 0.004f, 0.008f, 0.01f, Color::GREEN, Color::GREEN );

		// Bounding convex

		Vector sceneCenter;
		Box sceneBox;
		TDynArray< Vector > sceneConvex;

		const CStorySceneDialogsetInstance* dialogSetInstance = GetSceneDirector()->GetCurrentSceneDialogInstance();

		SStorySceneDialogsetInstanceCalcBoundDesc desc;
		desc.m_dialogSetInstance = m_sceneDirector.GetCurrentSceneDialogInstance();
		desc.m_parentTransform = GetSceneDirector()->Debug_GetCurrentScenePlacement();
		desc.m_safeZone = dialogSetInstance ? dialogSetInstance->GetSafePlacementRadius() : CStorySceneDialogsetInstance::GetDefaultSafePlacementRadius();
		desc.m_includeCameras = dialogSetInstance ? dialogSetInstance->AreCamerasUsedForBoundsCalculation() : false;
		if ( DetermineSceneBounds( desc, sceneCenter, sceneBox, sceneConvex ) )
		{
			const Vector secondLineOffset( 0.0f, 0.0f, 1.0f );

			for ( Uint32 i = 0; i < sceneConvex.Size(); ++i )
			{
				const Uint32 j = ( i + 1 ) % sceneConvex.Size();

				frame->AddDebugLine( sceneConvex[ i ], sceneConvex[ j ], Color::GREEN );
				frame->AddDebugLine( sceneConvex[ i ] + secondLineOffset, sceneConvex[ j ] + secondLineOffset, Color::GREEN );
			}
		}
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) )
	{
		String msg;
		if ( m_debugStats.ShowMsg( msg ) )
		{
			Color c( 255, 0, 0, m_debugStats.GetAlphaAsChar() );

			const Int32 x = 30;
			const Int32 y = 60;

			frame->AddDebugRect( x, y-15, 20, 20, c );
			frame->AddDebugScreenText( x + 30, y, msg, c );
		}
	}
}


void CStoryScenePlayer::UpdatePlaybackListeners()
{
	for ( TDynArray< IStoryScenePlaybackListener* >::iterator addListenerIter = m_playbackListenersToAdd.Begin(); 
		addListenerIter != m_playbackListenersToAdd.End(); ++addListenerIter )
	{
		m_playbackListeners.PushBackUnique( *addListenerIter );
	}
	m_playbackListenersToAdd.Clear();

	for ( TDynArray< IStoryScenePlaybackListener* >::iterator removListenerIter = m_playbackListenersToRemove.Begin(); 
		removListenerIter != m_playbackListenersToRemove.End(); ++removListenerIter )
	{
		m_playbackListeners.Remove( *removListenerIter );
	}
	m_playbackListenersToRemove.Clear();
}

RED_DEFINE_STATIC_NAME( OnTutorialMessageForChoiceLineChosen )

void CStoryScenePlayer::OnInjectedSceneFinished( const CStorySceneOutput* output, const CStoryScene* injectedScene, const CStoryScene* targetScene )
{
	RED_FATAL_ASSERT( m_internalState.m_injectedScene, "CStoryScenePlayer::OnInjectedSceneFinished" );
	RED_FATAL_ASSERT( targetScene, "CStoryScenePlayer::OnInjectedSceneFinished" );
	RED_FATAL_ASSERT( injectedScene, "CStoryScenePlayer::OnInjectedSceneFinished" );
	RED_FATAL_ASSERT( m_internalState.m_injectedScene == injectedScene, "CStoryScenePlayer::OnInjectedSceneFinished" );

	const CStoryScene* s = GetStoryScene();
	RED_ASSERT( s );

	SCENE_ASSERT( s->CheckSectionsIdsCollision( injectedScene ) );

	TSoftHandle< CStoryScene > injectedSceneH( const_cast< CStoryScene* >( injectedScene ) );
	TSoftHandle< CStoryScene > targetSceneH( const_cast< CStoryScene* >( targetScene ) );

	for ( auto it = m_playbackListeners.Begin(), end = m_playbackListeners.End(); it != end; ++it )
	{
		( *it )->OnInjectedReturnDialogEnd( this, output->GetOutputName(), injectedSceneH, targetSceneH );
	}

	m_internalState.m_injectedScene = nullptr;
}

void CStoryScenePlayer::OnInjectedSceneStarted( const CStoryScene* injectedScene )
{
	SCENE_ASSERT( injectedScene );
	SCENE_ASSERT( !m_internalState.m_injectedScene );
	RED_FATAL_ASSERT( injectedScene, "CStoryScenePlayer::OnInjectedSceneStarted" );
	RED_FATAL_ASSERT( !m_internalState.m_injectedScene, "CStoryScenePlayer::OnInjectedSceneStarted" );

	m_internalState.m_injectedScene = injectedScene;
}

Bool CStoryScenePlayer::SignalAcceptChoice( const SSceneChoice& choosenLine )
{
	SCENE_ASSERT( !m_internalState.m_isInTick );
	SCENE_ASSERT( HasPendingChoice() );

	if ( choosenLine.link != NULL )
	{

		if ( choosenLine.m_injectedChoice )
		{
			OnInjectedSceneStarted( choosenLine.link->FindParent<CStoryScene>() );
		}

		m_internalState.m_selectedChoice = choosenLine.link;
		if( choosenLine.m_choiceToReturnTo )
		{
			m_internalState.m_choiceToReturnTo = choosenLine.m_choiceToReturnTo;
		}		

		if ( m_debugger )
		{
			m_debugger->SignalAcceptChoice( choosenLine );
		}

		GGame->CallEvent( CNAME( OnTutorialMessageForChoiceLineChosen ), (Int32)choosenLine.m_dialogAction );

		return true;
	}

	return false;
}

Bool CStoryScenePlayer::SignalSkipLine()
{
	SCENE_ASSERT( !m_internalState.m_isInTick );

	if ( m_internalState.m_sceneProgress < GGame->GetGameplayConfig().m_sceneIgnoreInputDuration )
	{
		return true;
	}

	m_internalState.m_skipLine = true;

	if ( m_debugger )
	{
		m_debugger->SignalSkipLine();
	}

	return true;
}

void CStoryScenePlayer::Pause( Bool shouldPause, EPauseChannel channel )
{
	Bool wasPaused = IsPaused();

	if ( shouldPause )
	{
		m_isPaused = m_isPaused | channel;
	}
	else
	{
		m_isPaused = m_isPaused & ~channel;
	}

	if ( IsPaused() != wasPaused )
	{
		if ( shouldPause )
		{
			SCENE_LOG( TXT("Scene paused %ls"), GetFriendlyName().AsChar() );
		}
		else
		{
			SCENE_LOG( TXT("Unpausing scene %ls"), GetFriendlyName().AsChar() );
		}

		OnPaused();
	}		
}

void CStoryScenePlayer::TogglePause()
{
	Pause( !IsPaused() );
}

Bool CStoryScenePlayer::HasLoadedSection() const
{
	return m_sectionLoader.HasLoadedSection();
}

void CStoryScenePlayer::OnPaused()
{
	if ( m_internalState.m_planId != -1 )
	{
		CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if ( plan )
		{
			CStorySceneSectionPlayingPlan::InstanceData& instance = plan->m_sectionInstanceData;

			const Uint32 elemIndex = plan->m_currElementIndex;

			if ( elemIndex < plan->GetNumElements() )
			{
				IStorySceneElementInstanceData* elem = instance.m_elemData[ elemIndex ];
				SCENE_ASSERT( elem );

				elem->Pause( IsPaused() );
#ifndef NO_EDITOR_EVENT_SYSTEM
				if ( !IsPaused() )
				{
					SEvents::GetInstance().QueueEvent( CNAME( AudioTrackStarted ), nullptr );
				}
#endif
			}
		}
	}
}

void CStoryScenePlayer::OnScriptThreadKilled( CScriptThread* thread, Bool finished )
{
	SCENE_ASSERT( m_scriptBlock );
	SCENE_ASSERT( m_scriptBlockThread == thread );

	// Reset shit
	const CStorySceneScript* block = m_scriptBlock;
	m_scriptBlock = NULL;
	m_scriptBlockThread = NULL;	

	const CStorySceneLinkElement* resumeFrom = NULL;
	const TDynArray< CStorySceneLinkElement* >& links = block->GetOutLinks();

	if ( m_scriptReturnValue.GetType() && m_scriptReturnValue.GetType()->GetName() == GetTypeName< Bool >() )
	{
		// Resume flow using the result
		Bool resultValue = *( Bool* ) m_scriptReturnValue.Data();
		SCENE_ASSERT( links.Size() == 2 )
		if ( resultValue )
		{
			SCENE_LOG( TXT("Thread killed with 'true' for script block '%ls' in '%ls'"), block->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );
			if ( links.Size() > 1 )
			{
				resumeFrom = links[ CStorySceneScript::TRUE_LINK_INDEX ];
			}			
		}
		else
		{
			SCENE_LOG( TXT("Thread killed with 'false' for script block '%ls' in '%ls'"), block->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );
			if( links.Size() > 0 )
			{
				resumeFrom = links[ CStorySceneScript::FALSE_LINK_INDEX ];
			}		
		}
	}
	else if( m_scriptReturnValue.GetType() && m_scriptReturnValue.GetType()->GetType() == RT_Enum )
	{
		Uint32 resultValue = *( Uint32* ) m_scriptReturnValue.Data();
		Uint32 mask = ( 1 <<  m_scriptReturnValue.GetType()->GetSize() * 8 ) - 1;
		resultValue &= mask;
		SCENE_ASSERT( resultValue < links.Size() && resultValue >= 0 )
		resumeFrom = links[ resultValue ];
	}
	else
	{
		// Resume from generic output
		SCENE_LOG( TXT("Thread killed for script block '%ls' in '%ls'"), block->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );
		resumeFrom = block->GetNextElement();
	}
	
	if ( resumeFrom != NULL )
	{
		ResumeSceneFlow( resumeFrom );
		Pause( false, EPC_ScriptBlock );
	}
	
	TBaseClass::OnScriptThreadKilled(thread, finished);
}

Bool CStoryScenePlayer::ShouldTickDuringFading() const
{
	return m_allowTickDuringFade || ( m_internalState.m_currentSection && m_internalState.m_currentSection->ShouldTickDuringFading() ? true : false );
}

RED_DEFINE_STATIC_NAME( ShouldRestoreItemsForPlayer )

void CStoryScenePlayer::EndPlayingScene( Bool isStopped )
{
	if ( m_internalState.m_planId != -1 )
	{
		CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if ( plan && plan->GetCurrElement() )
		{
			plan->GetCurrElement()->Stop();
		}
	}

	ResetAllActorsHiResShadow();
	ResetActorLodOverrides();

	// Release texture requests
	SAFE_RELEASE( m_currentSectionTexRequest );
	SAFE_RELEASE( m_currentBackgroundRequest );
	SAFE_RELEASE( m_newBackgroundRequest );

	// DIALOG_TOMSIN_TODO - WTF???
	{
		ChangeSection( NULL, true, false );

		FinishChangingSection();

		CycleStreamingCamera( m_internalState.m_prevSection, nullptr );
	}

	// Close waiting thread
	if ( m_scriptBlockThread )
	{
		// Log this
		if ( m_scriptBlock )
		{
			SCENE_LOG( TXT("Thread stopped for script block '%ls' in '%ls'"), m_scriptBlock->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );
		}
		
		// Stop the thread
		m_scriptBlockThread->SetListener( NULL );
		m_scriptBlockThread->ForceKill();
		m_scriptBlockThread = NULL;
	}

	if ( m_sceneDirector.IsEnabled() == true )
	{
		m_sceneDirector.DisableDirector();
	}
	
	if( isStopped )
	{
		// DIALOG_TOMSIN_TODO
		// Po choice mam problem z hudem
		// Force toggling the dialog hud off when scene is stopped in the middle (no FinishChangingSection will be called to do this for us)
		if ( !IsGameplay() )
			{
				ToggleHud( false );
			}

		// Internal event
		OnSceneEnded();
	}

	m_eventsExecutor.Deinit( this );


	for ( TPair< CName, THandle< CEntity > >& iter : m_scenePropEntities )
	{
		if( iter.m_second && m_sceneController->IsPropDestroyedAfterScene( iter.m_first ))
		{
			iter.m_second->SetDisableAllDissolves( true );
			iter.m_second->Destroy();
		}		
	}

	for ( TPair< CName, THandle< CEntity > >& iter : m_sceneLightEntities )
	{
		if( iter.m_second && m_sceneController->IsLightDestroyedAfterScene( iter.m_first ) )
		{
			iter.m_second->Destroy();
		}		
	}

	for ( TPair< CName, THandle< CEntity > >& iter : m_sceneEffectEntities )
	{
		if( iter.m_second && m_sceneController->IsEffectDestroyedAfterScene( iter.m_first ))
		{
			iter.m_second->Destroy();
		}		
	}

	UnloadSceneTemplates();

	// Signal that story scene playback has stopped
	SignalPlaybackEnd( isStopped ); 	
	m_sceneController->Stop( isStopped ? SCR_STOPPED : SCR_SCENE_ENDED  );// DIALOG_TOMSIN_TODO - player->Destory() is inside -> be careful
}

void CStoryScenePlayer::UnloadSceneTemplates()
{
	const TDynArray< CStorySceneActor* >& expectedActors = m_storyScene->GetSceneActorsDefinitions();
	for ( TDynArray< CStorySceneActor* >::const_iterator expectedActorIter = expectedActors.Begin();
		expectedActorIter != expectedActors.End(); ++expectedActorIter )
	{
		CStorySceneActor* actorDef = ( *expectedActorIter );
		if( actorDef )
		{
			actorDef->m_entityTemplate.Release();
		}
	}

	const TDynArray< CStorySceneProp* >& expectedProps = m_storyScene->GetScenePropDefinitions();
	for ( TDynArray< CStorySceneProp* >::const_iterator expectedPropIter = expectedProps.Begin();
		expectedPropIter != expectedProps.End(); ++expectedPropIter )
	{
		CStorySceneProp* propDef = ( *expectedPropIter );
		if( propDef )
		{
			propDef->m_entityTemplate.Release();
		}
	}
}

Bool CStoryScenePlayer::IsInWorld( const CWorld* world ) const
{
	return GetLayer()->GetWorld() == world;
}

void CStoryScenePlayer::SetDestroyFlag()
{
	SCENE_ASSERT( !m_isDestoryed );
	m_isDestoryed = true;
}

Bool CStoryScenePlayer::HasDestroyFlag() const
{
	return m_isDestoryed;
}

Bool CStoryScenePlayer::IsStopped() const
{
	return m_isStopped;
}

void CStoryScenePlayer::Stop()
{
	if ( m_isStopped )
	{
		return;
	}

	SCENE_ASSERT( !m_isDestoryed );

	m_sceneDirector.InformNPCsThatSceneIsFinished();

	EndPlayingScene( true );

	// Clear other tables
	m_sceneActorEntities.Clear();
	m_scenePropEntities.Clear();

	// Reset flags
	m_isPaused = 0;
	m_isStopped = true;

	m_animContainer.UnloadAllAnimations();
}

void CStoryScenePlayer::SchedulePlayNextSceneElement()
{
	m_internalState.m_scheduledNextElements++;
}

void CStoryScenePlayer::ResumeSceneFlow( const CStorySceneLinkElement* flowPoint )
{
	m_internalState.m_resumeRequest = flowPoint;
}

void CStoryScenePlayer::SetDeniedAreasEnabled( Bool enabled )
{
	// Enable all denied areas
	for ( ComponentIterator< CDeniedAreaComponent > it( this ); it; ++it )
	{
		(*it)->SetEnabled( enabled );
	}
}

void CStoryScenePlayer::ChangeSection( const CStorySceneSection* newSection, Bool destroyOldPlan, Bool checkFlow )
{
	SCENE_ASSERT( !m_isDestoryed );

	if ( m_interceptionHelper.OnChangeSection() ) // DIALOG_TOMSIN_TODO - to jakis hack, nie wiem o co chodzi, jezeli to nie jest dowolone to nie powinno sie wolac
	{
		return;
	}

	if ( m_internalState.m_currentSection && newSection )
	{
		SCENE_ASSERT( m_internalState.m_currentSection != newSection );
		SCENE_ASSERT( m_internalState.m_currentSection->GetSectionUniqueId() != newSection->GetSectionUniqueId() );
	}

	Float fadeDuration( 0.f );
	if ( ShouldFadeOutBetweenSections( m_internalState.m_currentSection, newSection, fadeDuration ) )
	{
		if ( GGame->GetGameplayConfig().m_useFrozenFrameInsteadOfBlackscreen )
		{
			if ( !m_frozenFrameSet )
			{
				SceneFrozenFrameStart();
			}
		}
		else
		{
			if ( !m_blackscreenSet )
			{
				const String currName = m_internalState.m_currentSection ? m_internalState.m_currentSection->GetName() : TXT("<None>");
				const String nextName = m_internalState.m_pendingNextSection ? m_internalState.m_pendingNextSection->GetName() : TXT("<None>");
				String reason = String::Printf( TXT("curr '%ls' next '%ls' - ShouldFadeOutBetweenSections"), currName.AsChar(), nextName.AsChar() );

				if ( m_internalState.m_activatedOutput )
				{
					reason += TXT("[with output]");

					SceneFadeOut( reason, 0.f, m_internalState.m_activatedOutput->BlackscreenColor() );
				}
				else
				{
					SceneFadeOut( reason, 0.0f );
				}
			}
		}
	}

	if ( m_debugger )
	{
		m_debugger->OnChangeSection( m_internalState.m_currentSection, m_internalState.m_pendingNextSection );
	}

	Int32 planToDestroyId = -1;
	if ( destroyOldPlan )
	{
		planToDestroyId = m_internalState.m_planId;
	}
	else if ( CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId ) )
	{
		plan->SetPreventRemoval( true );
	}

	// Tell scene system that we ended playing non-gameplay scene
	if( m_internalState.m_currentSection && !m_internalState.m_currentSection->IsGameplay() )
	{
		OnNonGameplaySceneEnded(newSection);
	}
	
	if ( newSection != NULL && !newSection->IsGameplay() )
	{
		this->CallEvent( CNAME( OnBlockingScenePrepare ) );

		for ( THashMap< CName, THandle< CEntity > >::iterator actorIter = m_sceneActorEntities.Begin();
			actorIter != m_sceneActorEntities.End(); ++actorIter )
		{
			THandle< CEntity >& actorHandle = actorIter->m_second;
			CEntity *actor = actorHandle.Get();
			if ( actor == NULL || actor == m_player )
			{
				continue;
			}
			// We process player independently - no need to do this here

			actor->CallEvent( CNAME( OnBlockingScenePrepare ) );
		}

		// freeze/unfreeze player
		TogglePlayerMovement( false );
	}	

	// Begin changing section
	m_internalState.SetPendingNextSection( newSection );

	// Send request for new playing plan
	if ( IsInGame() )
	{
		SCENE_ASSERT( !checkFlow ); // This is super important assert!
	}

	m_sceneDirector.CancelTeleportNPCsJob();

	if ( m_internalState.m_pendingNextSection )
	{
		CName dialogsetName = m_internalState.m_pendingNextSection->GetDialogsetChange();

		if ( checkFlow )
		{
			RequestSectionPlayingPlan_StartFlow( m_internalState.m_pendingNextSection, dialogsetName );
		}
		else
		{
			RequestSectionPlayingPlan_ContinueFlow( m_internalState.m_pendingNextSection );
		}

		if ( dialogsetName )
		{
			const CStoryScene* scene = m_internalState.m_pendingNextSection->GetScene() ? m_internalState.m_pendingNextSection->GetScene() : GetStoryScene();
			SCENE_ASSERT( scene );

			m_sceneDirector.ChangeDesiredSetting( scene->GetDialogsetByName( dialogsetName ) );
		}
		else
		{
			m_sceneDirector.KeepCurrentSetting();
		}

		if ( !m_internalState.m_pendingNextSection->IsGameplay() && m_sceneDirector.GetDesiredSettings() )
		{
			SCENE_ASSERT( HasValidDialogsetFor( m_internalState.m_pendingNextSection, m_sceneDirector.GetDesiredSettings() ) );
		}
	}
	else
	{
		m_sceneDirector.ChangeDesiredSetting( nullptr );
	}
	
	const Bool wasBlocking = m_internalState.m_currentSection && !m_internalState.m_currentSection->IsGameplay();
	const Bool isBlocking = ( m_internalState.m_pendingNextSection && !m_internalState.m_pendingNextSection->IsGameplay() ) || m_scriptBlockThread ;
	
	// Toggle HUD
	if ( wasBlocking != isBlocking )
	{
		ToggleHud( isBlocking );

		// GI_2013_DEMO_HACK: Activate game camera one frame earlier to allow it to be sampled, 
		// so if you want to switch to combat state combat camera can use it for smooth actviation
		Float fadeDuration( 0.f );
		if ( newSection == NULL && ShouldFadeOutBetweenSections( m_internalState.m_currentSection, newSection, fadeDuration ) == false )
		{
			m_sceneDirector.DeactivateCameras();
		}
	}

	if ( planToDestroyId != -1 )
	{
		const Int32 pendingPlanId = m_internalState.m_pendingNextSection ? m_sectionLoader.GetPlayingPlanId( m_internalState.m_pendingNextSection ) : -1;
		SCENE_ASSERT( planToDestroyId != pendingPlanId );

		if ( m_internalState.m_pendingNextSection )
		{
			SCENE_ASSERT( m_sectionLoader.HasPlanRequested( m_internalState.m_pendingNextSection ) );
		}

		EndPlayingSectionPlan( planToDestroyId );

		if ( m_internalState.m_pendingNextSection )
		{
			SCENE_ASSERT( m_sectionLoader.HasPlanRequested( m_internalState.m_pendingNextSection ) );
		}
	}

	if ( m_internalState.m_pendingNextSection )
	{
		m_sceneDirector.ClearAreaForSection( m_internalState.m_pendingNextSection );
	}

	if ( m_internalState.m_pendingNextSection )
	{
		SCENE_ASSERT( m_sectionLoader.HasPlanRequested( m_internalState.m_pendingNextSection ) );
	}
}

Bool CStoryScenePlayer::GetBlockSceneArea() const
{
	return m_internalState.m_input->GetBlockSceneArea();
}

Bool CStoryScenePlayer::GetEnableDestroyDeadActorsAround() const
{
	return m_internalState.m_input->GetEnableDestroyDeadActorsAround();
}

void CStoryScenePlayer::StartBlendingOutLights( Float blendTime, const Bool disableDof )
{
	ASSERT( IsSceneInGame() )

	if ( m_didStartBlendingOutLights )
	{
		return;
	}
	m_didStartBlendingOutLights = true;

	m_eventsExecutor.DeactivateCustomEnv( this, blendTime );

	CStorySceneSystem::SFinishedSceneBlendData data;
	data.m_blendTime = blendTime;
	data.m_disableDof = disableDof;
	data.m_sceneLightEntities = m_sceneController->GetMappedLights();
	
	for ( CStorySceneController::ScenePropInfo& info : data.m_sceneLightEntities )
	{
		m_sceneController->MarkLightForDestruction( info.m_id, false );
	}

	if( CWorld* world = GetLayer()->GetWorld() )
	{
		if( CEnvironmentManager* envMgr = world->GetEnvironmentManager() )
		{
			data.m_envSetupToBlendFrom = envMgr->GetCameraLightsModifers();
		}		
	}
	GCommonGame->GetSystem< CStorySceneSystem >()->SetFinishedSceneBlendData( data );
}


void CStoryScenePlayer::ChangeSection_StartBackgroundTextureRequest()
{
	CTimeCounter timer;

	if ( !m_internalState.m_pendingNextSection )
	{
		return;
	}


	RED_ASSERT( m_newBackgroundRequest == nullptr );
	SAFE_RELEASE( m_newBackgroundRequest );
	m_newBackgroundRequest = GRender->CreateTextureStreamRequest( true );

	// HACK : Scan for any appearance changes or anything like that, so we can get any new textures from those streaming
	// as well. We won't wait on those textures, just get them started so maybe they'll be ready later.
	if ( m_internalState.m_pendingNextSection->IsA< CStorySceneCutsceneSection >() )
	{
		const CStorySceneCutsceneSection* csSection = static_cast< const CStorySceneCutsceneSection* >( m_internalState.m_pendingNextSection );

		if ( CCutsceneTemplate* csTemplate = csSection->GetCsTemplate() )
		{
			TDynArray< CExtAnimEvent* > events;

			const Uint32 numAnimations = csTemplate->GetNumAnimations();
			for ( Uint32 i = 0; i < numAnimations; ++i )
			{
				CName animName = csTemplate->GetAnimationName( i );
				String actorName = csTemplate->GetActorName( animName );
				const SCutsceneActorDef* actorDef = csTemplate->GetActorDefinition( actorName );

				CEntity* ent = actorDef ? GetSceneActorEntity( actorDef->m_voiceTag ) : nullptr;

				csTemplate->GetEventsForAnimation( animName, events );
				for ( const CExtAnimEvent* evt : events )
				{
					// Appearance change
					if ( ent != nullptr && IsExactlyType< CExtAnimCutsceneBodyPartEvent >( evt ) )
					{
						const CExtAnimCutsceneBodyPartEvent* bpEvt = static_cast< const CExtAnimCutsceneBodyPartEvent* >( evt );
						CName appearance = bpEvt->GetAppearanceName();

						m_newBackgroundRequest->AddEntityAppearance( ent->GetEntityTemplate(), appearance );
					}
					// Item change
					else if ( IsExactlyType< CExtAnimItemEvent >( evt ) )
					{
						const CExtAnimItemEvent* itemEvt = static_cast< const CExtAnimItemEvent* >( evt );

						// Only care about mounting a new item.
						if ( itemEvt->GetAction() != IA_Unmount )
						{
							CName itemName = itemEvt->GetItemName();
							RequestTextureForItem( ent, itemName, m_newBackgroundRequest );
						}
					}
				}
				events.ClearFast();
			}
		}
	}


	{
		CStorySceneSectionVariantId variantId = m_internalState.m_pendingNextSection->GetVariantUsedByLocale( SLocalizationManager::GetInstance().GetCurrentLocaleId() );
		const auto& events = m_internalState.m_pendingNextSection->GetEvents( variantId );
		for ( CGUID evtGuid : events )
		{
			const CStorySceneEvent* evt = m_internalState.m_pendingNextSection->GetEvent( evtGuid );
			if ( IsExactlyType< CStorySceneEventEquipItem >( evt ) )
			{
				const CStorySceneEventEquipItem* equipEvt = static_cast< const CStorySceneEventEquipItem* >( evt );
				CName actorName = equipEvt->GetActorName();
				if ( CEntity* ent = GetSceneActorEntity( actorName ) )
				{
					CName leftItem = equipEvt->GetLeftItem();
					CName rightItem = equipEvt->GetRightItem();

					RequestTextureForItem( ent, leftItem, m_newBackgroundRequest );
					RequestTextureForItem( ent, rightItem, m_newBackgroundRequest );
				}
			}
		}
	}


	// Fire off the streaming request. We can fire and forget here, we aren't going to wait until it finishes.
	// Even if the request is initially ready (i.e. empty), we won't release it, so we don't accidentally try to do it again.
	if ( !m_newBackgroundRequest->IsReady() )
	{
		( new CRenderCommand_StartTextureStreamRequest( m_newBackgroundRequest ) )->Commit();
	}
}

namespace
{
	void PreprocessSectionItems( const CStorySceneSection* section, CStoryScenePlayer* player )
	{
		CStorySceneSectionVariantId variantId = section->GetVariantUsedByLocale( SLocalizationManager::GetInstance().GetCurrentLocaleId() );
		const TDynArray< CGUID >& events =  section->GetEvents( variantId );
		for( const CGUID& guid : events )
		{
			if( const CStorySceneEventEquipItem* evt = Cast< const CStorySceneEventEquipItem >( section->GetEvent( guid ) ) )
			{
				evt->PreprocessItems( player );
			}
		}
	}
}


IRenderFramePrefetch* CStoryScenePlayer::CreateFramePrefetch( const CStorySceneSectionPlayingPlan* playingPlan, const CStorySceneSectionPlayingPlan::CameraMarker& marker, const Matrix& sceneTransform )
{
	const Matrix camMatrixWS = marker.m_cameraMatrixSceneLocal * sceneTransform;

	IRenderScene* renderScene = GetSceneWorld()->GetRenderSceneEx();

	CRenderFrameInfo frameInfo( GGame->GetViewport() );

	frameInfo.m_camera.Set( camMatrixWS.GetTranslation(), camMatrixWS.ToEulerAngles(), marker.m_cameraFov, frameInfo.m_camera.GetAspect(), frameInfo.m_camera.GetNearPlane(), frameInfo.m_camera.GetFarPlane() );
	frameInfo.m_occlusionCamera = frameInfo.m_camera;
	frameInfo.UpdateMatrices();

	CRenderFrame* renderFrame = GRender->CreateFrame( nullptr, frameInfo );
	IRenderFramePrefetch* framePrefetch = GRender->CreateRenderFramePrefetch( renderFrame, renderScene );
	renderFrame->Release();

	return framePrefetch;
}


IRenderFramePrefetch* CStoryScenePlayer::StartSectionFirstFramePrefetch( const CStorySceneSectionPlayingPlan* playingPlan )
{
	// Find the first marker, with a time early enough to need prefetching before the start of the section.
	const CStorySceneSectionPlayingPlan::CameraMarker* firstMarker = nullptr;
	for ( const auto& marker : playingPlan->m_sectionInstanceData.m_cachedCameraMarkers )
	{
		if ( marker.m_eventTime <= CameraPrefetch_PRE_TIMEOFFSET )
		{
			if ( firstMarker == nullptr || marker.m_eventTime < firstMarker->m_eventTime )
			{
				firstMarker = &marker;
			}
		}
	}

	IRenderFramePrefetch* prefetch = nullptr;

	// If we found one, fire off a prefetch for it.
	if ( firstMarker != nullptr )
	{
		Matrix sceneL2W = GetSceneSectionTransform( playingPlan->m_section );

		prefetch = CreateFramePrefetch( playingPlan, *firstMarker, sceneL2W );
		if ( prefetch != nullptr )
		{
			( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();
		}
	}

	return prefetch;
}

void CStoryScenePlayer::StartSectionEarlyFramePrefetches( const CStorySceneSectionPlayingPlan* playingPlan )
{
	// Collect all events that are early enough to be prefetched at the start of the section.
	TDynArray< const CStorySceneSectionPlayingPlan::CameraMarker* > sectionStartMarkers;
	for ( const auto& marker : playingPlan->m_sectionInstanceData.m_cachedCameraMarkers )
	{
		if ( marker.m_eventTime <= CameraPrefetch_PRE_TIMEOFFSET )
		{
			sectionStartMarkers.PushBack( &marker );
		}
	}

	// If there's more than one, fire off prefetches for all but the earliest. The earliest one should be handled separately
	// by StartSectionFirstFramePrefetch.
	if ( sectionStartMarkers.Size() > 1 )
	{
		Matrix sceneL2W = GetSceneSectionTransform( playingPlan->m_section );

		// Sort the markers.
		Sort( sectionStartMarkers.Begin(), sectionStartMarkers.End(), []( const CStorySceneSectionPlayingPlan::CameraMarker* a, const CStorySceneSectionPlayingPlan::CameraMarker* b ) {
			return a->m_eventTime < b->m_eventTime;
		});

		for ( Uint32 i = 1; i < sectionStartMarkers.Size(); ++i )
		{
			auto marker = sectionStartMarkers[ i ];
			IRenderFramePrefetch* prefetch = CreateFramePrefetch( playingPlan, *marker, sceneL2W );
			if ( prefetch != nullptr )
			{
				( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();
				prefetch->Release();
			}
		}
	}
}


Bool CStoryScenePlayer::FinishChangingSection()
{
	SCENE_ASSERT( !m_isDestoryed );

	const Bool wasBlocking = m_internalState.m_currentSection && !m_internalState.m_currentSection->IsGameplay();
	const Bool isBlocking = ( m_internalState.m_pendingNextSection && !m_internalState.m_pendingNextSection->IsGameplay() ) || m_scriptBlockThread ;

	const CStorySceneSection* debuggerPrev = m_internalState.m_currentSection;
	const CStorySceneSection* debuggerNext = m_internalState.m_pendingNextSection;

	CStorySceneSectionPlayingPlan* pendingPlayingPlan( nullptr );

	if ( m_internalState.m_pendingNextSection )
	{
		if ( m_internalState.m_pendingNextSectionStage == PNSS_Init )
		{
			m_internalState.m_pendingNextSectionStage = PNSS_WaitingForPlan;

#ifdef USE_STORY_SCENE_LOADING_STATS
			m_internalState.m_pendingNextSectionTimer.ResetTimer();
#endif
		}

		// Plan
		{
			if ( m_internalState.m_pendingNextSectionStage == PNSS_WaitingForPlan )
			{
				pendingPlayingPlan = GetSectionPlayingPlan( m_internalState.m_pendingNextSection );
				if ( !pendingPlayingPlan )
				{
					// Request load of playing plan ASAP
					const Bool asap = m_sectionLoader.MakeRequestASAP( m_internalState.m_pendingNextSection );
					SCENE_ASSERT( asap );
				}
				else
				{
					m_internalState.m_pendingNextSectionStage = PNSS_PlanIsReady;
				}
			}
			else
			{
				pendingPlayingPlan = GetSectionPlayingPlan( m_internalState.m_pendingNextSection );
				SCENE_ASSERT( pendingPlayingPlan );
			}

			if ( m_internalState.m_pendingNextSectionStage == PNSS_PlanIsReady )
			{
#ifdef USE_STORY_SCENE_LOADING_STATS
				SCENE_ASSERT( pendingPlayingPlan->m_loadingStats.IsInitialized() );
				pendingPlayingPlan->m_loadingStats.StartB();
				pendingPlayingPlan->m_loadingStats.SetDurationForPlan( (Float)m_internalState.m_pendingNextSectionTimer.GetTimePeriodMS() );
				BEGIN_SS_LOADING( pendingPlayingPlan->m_loadingStats, "BlockedSSLoading" );
				BEGIN_SS_LOADING( pendingPlayingPlan->m_loadingStats, "WaitingPart" );
#endif

				m_internalState.m_pendingNextSectionStage = PNSS_StartLoadingTextures;
			}
		}

		// Textures
		{
			if ( m_internalState.m_pendingNextSectionStage == PNSS_StartLoadingTextures )
			{
				BEGIN_SS_LOADING( pendingPlayingPlan->m_loadingStats, "TextureStreaming" );

				SAFE_RELEASE( m_currentSectionTexRequest );
				SAFE_RELEASE( m_framePrefetch );

				// Make sure we have textures ready.
				if ( m_sectionLoader.IsAsync() && pendingPlayingPlan && !m_internalState.m_pendingNextSection->IsGameplay() )
				{
					// Take the texture request from the playing plan. We need to hold on to this for the duration of the section, so we
					// don't end up losing those textures.
					m_currentSectionTexRequest = pendingPlayingPlan->GetTextureStreamRequest();
					pendingPlayingPlan->SetTextureStreamRequest( nullptr );

					// Get the frame prefetch from this chosen section. We keep it alive and don't start a new one, to avoid the
					// overhead of having to re-run the scene query. It should be at least similar now... maybe some actors have
					// moved, but they have locked textures anyways.
					m_framePrefetch = pendingPlayingPlan->GetRenderFramePrefetch();
					pendingPlayingPlan->SetRenderFramePrefetch( nullptr );

					m_internalState.m_pendingNextSectionStage = PNSS_WaitingForTextures;
				}
				else
				{
					m_internalState.m_pendingNextSectionStage = PNSS_TexturesAreReady;
				}
			}

			if ( m_internalState.m_pendingNextSectionStage == PNSS_WaitingForTextures )
			{
				Bool wait( false );

				// If we have a texture request from the pending section, or a prefetch for initial frames, wait for them to finish.
				if (   ( pendingPlayingPlan->GetTextureStreamRequest() && !pendingPlayingPlan->GetTextureStreamRequest()->IsReady() )
					|| ( m_framePrefetch && !m_framePrefetch->IsFinished() ) )
				{
					wait = true;

					SS_LOADING_ADD_COUNTER( pendingPlayingPlan->m_loadingStats, "TextureStreaming" );
				}

				//...

				if ( !wait )
				{
					m_internalState.m_pendingNextSectionStage = PNSS_TexturesAreReady;
				}
			}

			if ( m_internalState.m_pendingNextSectionStage == PNSS_TexturesAreReady )
			{
				END_SS_LOADING( pendingPlayingPlan->m_loadingStats, "TextureStreaming" );

				// If we haven't made the extra texture request, do so now. We don't need to wait on them, just get them started.
				if ( pendingPlayingPlan && m_newBackgroundRequest == nullptr && !m_internalState.m_pendingNextSection->IsGameplay() )
				{
					// NOTE : We don't do this until _after_ the main texture request has completed, so that this background request
					// doesn't clog the texture streaming, preventing us from getting the section started.
					if ( !pendingPlayingPlan->GetTextureStreamRequest() || pendingPlayingPlan->GetTextureStreamRequest()->IsReady() )
					{
						SS_LOADING_SCOPE( pendingPlayingPlan->m_loadingStats, "StartBackgroundTextureRequest" );

						ChangeSection_StartBackgroundTextureRequest();

						// Start prefetches for the other early camera changes, but don't wait on them or anything.
						StartSectionEarlyFramePrefetches( pendingPlayingPlan );
					}
				}

				m_internalState.m_pendingNextSectionStage = PNSS_StartHud;
			}
		}

		// Hud
		{
			if ( m_internalState.m_pendingNextSectionStage == PNSS_StartHud )
			{
				BEGIN_SS_LOADING( pendingPlayingPlan->m_loadingStats, "IsHudReady" );

				m_internalState.m_pendingNextSectionStage = PNSS_WaitingForHud;
			}

			if ( m_internalState.m_pendingNextSectionStage == PNSS_WaitingForHud )
			{
				Bool wait( false );
				if ( m_internalState.m_pendingNextSection != NULL 
					&& m_internalState.m_pendingNextSection->IsGameplay() == false 
					&& IsHudReady() == false )
				{
					wait = true; // Wait a bit to ensure dialog hud existence;

					SS_LOADING_ADD_COUNTER( pendingPlayingPlan->m_loadingStats, "IsHudReady" );
				}

				if ( !wait )
				{
					END_SS_LOADING( pendingPlayingPlan->m_loadingStats, "IsHudReady" );

					m_internalState.m_pendingNextSectionStage = PNSS_StartNPCTeleport;
				}
			}
		}

		// NPC teleport job
		if ( m_internalState.m_pendingNextSectionStage == PNSS_StartNPCTeleport )
		{
			BEGIN_SS_LOADING( pendingPlayingPlan->m_loadingStats, "NPCTeleportJob" );

			m_internalState.m_pendingNextSectionStage = PNSS_WaitingForNPCTeleport;
		}

		if ( m_internalState.m_pendingNextSectionStage == PNSS_WaitingForNPCTeleport )
		{
			// Wait for NPCs teleport job to complete
			if ( !m_sceneDirector.IsNPCTeleportJobInProgress() )
			{
				END_SS_LOADING( pendingPlayingPlan->m_loadingStats, "NPCTeleportJob" );

				m_internalState.m_pendingNextSectionStage = PNSS_Finish;
			}
			else
			{
				SS_LOADING_ADD_COUNTER( pendingPlayingPlan->m_loadingStats, "NPCTeleportJob" );
				
				SCENE_ASSERT( IsSceneInGame() );
			}
		}

		const Bool canFinish = m_internalState.m_pendingNextSectionStage == PNSS_Finish;
		if ( canFinish == false )
		{
			// Set blackscreen while plan is loaded but only if it can be taken off
			if ( m_internalState.m_currentSection && m_internalState.m_pendingNextSection
				&& !m_internalState.m_currentSection->IsGameplay() && !m_internalState.m_pendingNextSection->IsGameplay() 
				/*&& !( m_internalState.m_currentSection->UsesSetting() && m_internalState.m_pendingNextSection->UsesSetting() ) */
				)
			{
				SCENE_LOG( TXT( "%ls - setting frozen frame on finishing section change because requested section(%ls) is not yet loaded" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), m_internalState.m_pendingNextSection->GetName().AsChar() );

				if ( !m_frozenFrameSet )
				{
					SceneFrozenFrameStart();
				}
			}

			return false;
		}

		// Forget all other branches if we have choosen a plan
		//if ( IsInOverrideMode() == false )
		{
			m_sectionLoader.ForgetPreloadedPlans( pendingPlayingPlan );
		}
		
		{
			SS_LOADING_SCOPE( pendingPlayingPlan->m_loadingStats, "EnsureNextElementsHaveSpeeches" );

			m_sectionLoader.EnsureNextElementsHaveSpeeches( pendingPlayingPlan );
		}

		END_SS_LOADING( pendingPlayingPlan->m_loadingStats, "WaitingPart" );
	}

#ifdef USE_STORY_SCENE_LOADING_STATS
	CStorySceneLoadingStat* stats = pendingPlayingPlan ? &(pendingPlayingPlan->m_loadingStats) : nullptr;
#endif

	BEGIN_SS_LOADING_PTR( stats, "SyncPart" );

	//dex++: update hires shadow on actors in the new section
	{
		SS_LOADING_SCOPE_PTR( stats, "HiResShadow" );

		// disable hi-res shadows on current actors
		ResetAllActorsHiResShadow();

		// if the new section is blocking section (non gameplay section) then get actors in that section and enable hi-res shadows for them
		if ( m_internalState.m_pendingNextSection && !m_internalState.m_pendingNextSection->IsGameplay() )
		{
			// process normal actors
			for ( THashMap< CName, THandle< CEntity > >::iterator actorIter = m_sceneActorEntities.Begin();
				actorIter != m_sceneActorEntities.End(); ++actorIter )
			{
				THandle< CEntity > actorHandle = actorIter->m_second;
				if ( CActor* actor = Cast< CActor >( actorHandle.Get() ) )
				{
					if ( ShouldActorHaveHiResShadows( actor ) )
					{
						SetActorHiResShadow( actor, true );
					}
				}
			}

			// always include witchers
			if ( CPlayer* player = m_player.Get() )
			{
				SetActorHiResShadow( player, true );
			}
		}
	}
	//dex--

	if( CWorld* world = GetLayer()->GetWorld() )
	{
		SS_LOADING_SCOPE_PTR( stats, "CameraLights" );

		CEnvironmentManager* mgr = world->GetEnvironmentManager();
		if( mgr )
		{		
			if( !IsGameplay() )
			{
				mgr->SetDistantLightOverride( m_internalState.m_pendingNextSection ? m_internalState.m_pendingNextSection->GetDistantLightOverride() : -1.f );
				mgr->SetHiResShadowMapExtents( m_internalState.m_pendingNextSection ? m_internalState.m_pendingNextSection->GetMaxBoxExtentsToApplyHiResShadows() : -1.f );
			}			

			if( isBlocking && !wasBlocking ) // scene started
			{ 
				// inform environment manager that scene is being played
				SCameraLightsModifiersSetup cameraSetup;
				cameraSetup.SetModifiersAllIdentityOneEnabled( ECLT_Scene );
				cameraSetup.SetScenesSystemActiveFactor( 1.0f );
				mgr->SetCameraLightsModifiers( cameraSetup );
			}
			else if( !isBlocking && wasBlocking && !GCommonGame->GetSystem< CStorySceneSystem >()->IsBlendingFinishedScene() ) // to gameplay
			{
				const CStorySceneOutput* out = m_internalState.m_activatedOutput;
				if ( out && out->GetGameplayCameraBlendTime() > 0.f && out->GetLightsBlendTime() )
				{ 
					StartBlendingOutLights( out->GetLightsBlendTime() );
				}
				else
				{
					SCameraLightsModifiersSetup cameraSetup;
					cameraSetup.SetModifiersAllIdentityOneEnabled( ECLT_Gameplay );
					cameraSetup.SetScenesSystemActiveFactor( 0.f );
					mgr->SetCameraLightsModifiers( cameraSetup );
				}
			}
		}
	}

	if ( isBlocking != wasBlocking )
	{
		SS_LOADING_SCOPE_PTR( stats, "ProcessSceneStateForActors" );

		if ( isBlocking )
		{
			m_sceneDirector.ActivateCameras();
		}
		else
		{
			m_sceneDirector.DeactivateCameras();
		}

		ProcessSceneStateForActors( isBlocking );

		if ( isBlocking )
		{
			{
				// close this in scope to ensure releasing of the handle being created here
				this->CallEvent< THandle< CStoryScene > >( CNAME( OnBlockingSceneStarted ), GetStoryScene() );
			}
		}
		else
		{
			{
				this->CallEvent< THandle< CStorySceneOutput > >( CNAME( OnBlockingSceneEnded ), m_internalState.m_activatedOutput );
			}
		}
		
		if ( GGame->GetGameplayConfig().m_enableSimplePriorityLoadingInScenes == true )
		{
			//CJobIOThread::SetPassesByPriorityCompletionProcedure( !isBlocking );
		}
	}

	if ( ShouldFadeInBetweenSections( m_internalState.m_currentSection, m_internalState.m_pendingNextSection ) )
	{
		SS_LOADING_SCOPE_PTR( stats, "ShouldFadeInBetweenSections" );

		//SCENE_LOG( TXT( "%ls - Fading in after changing section to %ls" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), m_internalState.m_pendingNextSection != NULL ? m_internalState.m_pendingNextSection->GetName().AsChar() : TXT( "END" ) );
		
		const String currName = m_internalState.m_currentSection ? m_internalState.m_currentSection->GetName() : TXT("<None>");
		const String nextName = m_internalState.m_pendingNextSection ? m_internalState.m_pendingNextSection->GetName() : TXT("<None>");
		const String reason = String::Printf( TXT("curr '%ls' next '%ls' - ShouldFadeInBetweenSections"), currName.AsChar(), nextName.AsChar() );
		SceneFadeIn( reason );

		SCENE_ASSERT( !m_blackscreenSet );
	}

	{
		SS_LOADING_SCOPE_PTR( stats, "JunkA" );

		if ( m_frozenFrameSet )
		{
			SceneFrozenFrameEnd();
		}

		m_internalState.SetCurrentSection( m_internalState.m_pendingNextSection );
	}

	if ( m_internalState.m_pendingNextSection != NULL )
	{
		SS_LOADING_SCOPE_PTR( stats, "ChangeSceneSettingAndResetActors" );

		ChangeSceneSettingAndResetActors();
	}

	if ( m_internalState.m_currentSection )
	{
		SS_LOADING_SCOPE_PTR( stats, "CurrentSectionSetup" );

		if ( m_internalState.m_prevSection )
		{
			DisableStreamingLockdown( m_internalState.m_prevSection );
		}

		if ( m_isGameplay == false )
		{
			OnSectionStarted( m_internalState.m_currentSection, m_internalState.m_prevSection );
			EnableStreamingLockdown( m_internalState.m_currentSection );
		}

		SetSceneCameraMovable( m_internalState.m_currentSection->CanMoveCamera() );

		if( !m_internalState.m_currentSection->IsGameplay() )
		{
			// Tell scene system that we are playing non-gameplay scene
			OnNonGameplaySceneStarted();
		}
		else
		{
			GGame->GameplaySceneStarted(); 
		}

		if ( pendingPlayingPlan != NULL )
		{
			// Request plans for following sections
			const TDynArray< const CStorySceneSection* >& nextSections = pendingPlayingPlan->GetNextSections();
			for ( TDynArray< const CStorySceneSection* >::const_iterator nextSectionIter = nextSections.Begin();
				nextSectionIter != nextSections.End(); ++nextSectionIter )
			{
				RequestSectionPlayingPlan_ContinueFlow( *nextSectionIter );
			}
		}

		PreprocessSectionItems( m_internalState.m_currentSection, this );
	}
	else if ( !m_internalState.m_pendingNextSection && m_internalState.m_prevSection )
	{
		SS_LOADING_SCOPE_PTR( stats, "CurrentSectionSetup" );

		DisableStreamingLockdown( m_internalState.m_prevSection );
	}

	{
		SS_LOADING_SCOPE_PTR( stats, "JunkB" );

		m_internalState.ResetPendingNextSection();

		if ( m_debugger )
		{
			m_debugger->OnFinishChangingSection( debuggerPrev, debuggerNext );
		}

		SCENE_ASSERT( !m_frozenFrameSet );
	}

	{
		SS_LOADING_SCOPE_PTR( stats, "Renderer_SAFE_RELEASE" );

		SAFE_RELEASE( m_framePrefetch );

		SAFE_RELEASE( m_currentBackgroundRequest );
		m_currentBackgroundRequest = m_newBackgroundRequest;
		m_newBackgroundRequest = nullptr;

		if ( IsSceneInGame() )
		{
			GCommonGame->GetActiveWorld()->GetCameraDirector()->SetCameraResetDisabled( false );
		}
	}

	END_SS_LOADING_PTR( stats, "SyncPart" );

#ifdef USE_STORY_SCENE_LOADING_STATS
	if ( pendingPlayingPlan )
	{
		END_SS_LOADING( pendingPlayingPlan->m_loadingStats, "BlockedSSLoading" );
		pendingPlayingPlan->m_loadingStats.StopB();
		pendingPlayingPlan->m_loadingStats.PrintToLog();
	}
#endif

	return true;
}

Bool CStoryScenePlayer::IsSectionChanging()
{
	return m_internalState.m_currentSection != m_internalState.m_pendingNextSection;
}

void CStoryScenePlayer::EndPlayingSectionPlan( Int32 id )
{
	CStorySceneSectionPlayingPlan* playingPlan = m_sectionLoader.FindPlanById( id );

	SCENE_ASSERT( playingPlan != NULL )

	if ( playingPlan->HasElements() && playingPlan->GetCurrElement() )
	{
		playingPlan->GetCurrElement()->ForceStop();
	}

	OnSectionEnded( playingPlan->GetSection() );

	Int32 playingPlanId = playingPlan->m_id;

	CStoryScene* scene = playingPlan->GetSection()->FindParent< CStoryScene >();
	if ( scene )
	{
		VERIFY( m_sectionLoader.ForgetPlayingPlan( playingPlan->GetSection()->GetSectionUniqueId() ) );
	}
	else
	{
		RED_FATAL_ASSERT( !IsSceneInGame(), "CStoryScenePlayer::EndPlayingSectionPlan calles editor code" );

		// DIALOG_TOMSIN_TODO - hack przez undo - undo przepina parenta i ta sekcja juz nie ma parenta sceny!
		//Uint64 CStorySceneSection::GetSectionUniqueId() const
		
			Uint64 uniqueId = 0;
			CStoryScene* parentScene = m_storyScene.Get();
			if ( parentScene != NULL )
			{
				uniqueId = parentScene->GetSceneIdNumber();
				uniqueId = uniqueId << 32;
			}
			uniqueId |= playingPlan->GetSection()->GetSectionId();

		VERIFY( m_sectionLoader.ForgetPlayingPlan( uniqueId ) );
	}

	SCENE_ASSERT( !m_sectionLoader.HasPlanById( playingPlanId ) );

	if ( m_internalState.m_planId != -1 )
	{
		SCENE_ASSERT( m_internalState.m_planId == playingPlanId );
		m_internalState.m_planId = -1;
	}
}

const CStorySceneLinkElement* CStoryScenePlayer::ProcessScriptBlock( const CStorySceneScript* script, Int32& _immediateResult )
{
	// Prevent from reentering
	SCENE_ASSERT( !m_scriptBlock );
	SCENE_ASSERT( !m_scriptBlockThread );		//!< Current thread being processed

	_immediateResult = -1;

	// Get function, if no function found report error
	CFunction* function = script->GetFunction();
	if ( !function )
	{
		SCENE_WARN( TXT("Missing scene script function '%ls' in '%ls'"), script->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Initialize thread return value
	if ( function->GetReturnValue() )
	{
		IRTTIType* returnType = function->GetReturnValue()->GetType();
		m_scriptReturnValue.Reset( returnType );
	}

	// Process the scene
	Int32 immediateResult = -1;
	CScriptThread* scriptThread = script->Execute( this, m_scriptReturnValue.Data(), immediateResult );

	// Script thread was created
	if ( scriptThread )
	{
		// Info
		SCENE_LOG( TXT("Thread started for script block '%ls' in '%ls'"), script->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar() );

		// Setup thread
		m_scriptBlockThread = scriptThread;
		m_scriptBlock = script;

		// Link
		scriptThread->SetListener( this );

		// Pause scene
		//ChangeSection( NULL );
		SCENE_LOG( TXT( "%ls - Pausing scene for processing of latent script" ), m_sceneController->GetInput()->GetFriendlyName().AsChar() );
		Pause( true, EPC_ScriptBlock );

		return NULL;
	}

	// Process result in place
	const TDynArray< CStorySceneLinkElement* >& links = script->GetOutLinks();
	SCENE_LOG( TXT("immediateResult from script block '%ls' in '%ls': %i"), script->GetFunctionName().AsString().AsChar(), GetFriendlyName().AsChar(), immediateResult );
	if ( immediateResult >= 0 && immediateResult < links.SizeInt() )
	{
		_immediateResult = immediateResult;
		return links[ immediateResult ];
	}
	else
	{
		return script->GetNextElement();
	}
}

Int32 CStoryScenePlayer::FindInputIndexFromTo( const CStorySceneLinkElement* from, const CStorySceneSection* to, Bool forced ) const
{
	SCENE_ASSERT__FIXME_LATER( from );
	SCENE_ASSERT( to );

	if ( from && to )
	{
		const CStorySceneSection* fromSection = Cast< const CStorySceneSection >( from->GetParent() );

		const Uint32 size = to->GetInputPathLinks().Size();
		if ( from && size > 0 )
		{
			for ( Uint32 i=0; i<size; ++i )
			{
				if ( fromSection )
				{
					if ( const CStorySceneLinkElement* link = fromSection->GetInputPathLinkElement( i ) )
					{
						const CStorySceneLinkElement* next = link->GetNextElement();
						if ( next && ( next == to || next->FindParent< CStorySceneSection >() == to ) )
						{
							/*TDynArray< CStorySceneLinkElement* > temp = link->GetLinkedElements();
							for ( CStorySceneLinkElement* item : temp )
							{
								if( ! Cast< CStorySceneControlPart > ( item ) )
								{
									item = Cast< CStorySceneLinkElement >( item->GetParent() );
								}
								if ( item )
								{
									out.PushBack( item );
								}							
							}

							return;*/
						}
					}
				}

				if ( to->GetInputPathLinkElement( i ) )
				{
					const TDynArray< CStorySceneLinkElement* >& arr = to->GetInputPathLinkElement( i )->GetLinkedElements();
				
					const Uint32 arrSize = arr.Size();
					SCENE_ASSERT( arrSize == 1 );

					for ( Uint32 j=0; j<arrSize; ++j )
					{
						if ( arr[ j ] == from )
						{
							return (Int32)i;
						}
					}
				}
			}

			if ( !forced )
			{
				SCENE_ASSERT( 0 );
			}
		}
	}
	
	SCENE_ASSERT__FIXME_LATER( 0 );
	return -1;
}

const CStorySceneSection* CStoryScenePlayer::EvaluateControlChain( const CStorySceneLinkElement* part, InternalFlowState& flowState, Bool forced )
{
	return EvaluateControlChain( NULL, flowState.m_currentFlowElement, part, flowState, forced );
}

Bool CStoryScenePlayer::EvaluateFlowCondition( const CStorySceneFlowCondition* condition ) const
{
	return condition->IsFulfilled();
}

const CStorySceneSection* CStoryScenePlayer::EvaluateControlChain( const CStorySceneLinkElement* start, const CStorySceneLinkElement* prev, const CStorySceneLinkElement* part, InternalFlowState& flowState, Bool forced )
{
	// Part should be valid
	if ( start == part )
	{
		return NULL;
	}
	else if ( start == NULL )
	{
		start = part;
	}

	if ( part )
	{
		flowState.ResetCurrentFlowElement();
		flowState.ResetSectionInputNumber();

		// A flow condition
		if ( const CStorySceneFlowCondition* condition = Cast< CStorySceneFlowCondition >( part ) )
		{
			if ( condition )
			{
#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
#endif
				// Evaluate flow control to choose the path to follow
				const CStorySceneLinkElement* nextElement = NULL;				
				if ( EvaluateFlowCondition( condition ) )
				{
					nextElement = condition->GetTrueLink();

#ifdef USE_SCENE_FLOW_MARKERS
					marker.m_desc = String::Printf( TXT("Flow condition: true") );
					marker.m_outputNum = 0;
#endif
				}
				else
				{
					nextElement = condition->GetFalseLink();

#ifdef USE_SCENE_FLOW_MARKERS
					marker.m_desc = String::Printf( TXT("Flow condition: false") );
					marker.m_outputNum = 0;
#endif
				}

#ifdef USE_SCENE_FLOW_MARKERS
				flowState.AddFlowMarker( marker );
#endif
				// Recurse
				return EvaluateControlChain( start, part, nextElement, flowState, false );
			}
			return NULL;
		}
		if ( const CStorySceneFlowSwitch* switchFlow = Cast< CStorySceneFlowSwitch >( part ) )
		{
			Int32 chosenPath = -1;
			CStorySceneLinkElement* nextElement = switchFlow->ChoosePathToFollow( chosenPath );
			if ( nextElement )
			{
				#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
				if ( chosenPath == -1 )
				{
					marker.m_desc = TXT("Flow switch: default case");
				}
				else
				{
					marker.m_desc = String::Printf( TXT("Flow switch: case [%d]"), chosenPath );
				}
				marker.m_outputNum = chosenPath;
				flowState.AddFlowMarker( marker );
				#endif

				return EvaluateControlChain( start, part, nextElement, flowState, false );
			}
		}
		if ( const CStorySceneRandomizer* randomizer = Cast< const CStorySceneRandomizer >( part ) )
		{
			Uint32 numOutputs = randomizer->GetOutputs().Size();
			if ( numOutputs == 0 )
			{
				SCENE_WARN( TXT( "Randomizer block has no outputs (%ls)" ), m_storyScene->GetFriendlyName().AsChar() );
				return NULL;
			}

			//Uint32 selOutput  = IRand() % numOutputs;
			Int32 outputindex = 0;
			CStorySceneLinkElement * next = randomizer->GetRandomOutput( outputindex )->GetNextElement();

			if ( next == NULL )
			{
				SCENE_WARN( TXT( "Randomizer block has selected scene with disconnected output (%ls)" ), m_storyScene->GetFriendlyName().AsChar() );
				return NULL;
			}

			#ifdef USE_SCENE_FLOW_MARKERS
			InternalFlowState::FlowMarker marker;
			marker.m_element = part;
			marker.m_desc = String::Printf( TXT("Randomizer: [%d]"), outputindex );
			marker.m_outputNum = outputindex;
			flowState.AddFlowMarker( marker );
			#endif

			return EvaluateControlChain( start, part, next, flowState, false );
		}

		// Simple section
		if ( const CStorySceneSection* section = Cast< const CStorySceneSection >( part ))
		{
			flowState.SetCurrentFlowElement( part );
			
			if ( section != NULL ) 
			{
				Int32 index = FindInputIndexFromTo( prev, section, forced );
				flowState.SetSectionInputNumber( index );

				#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
				marker.m_desc = String::Printf( TXT("Section: [%ls], input [%d]"), section->GetName().AsChar(), index );
				marker.m_outputNum = index;
				flowState.AddFlowMarker( marker );
				#endif

				return section;
			}
			return NULL;
		}

		if ( const CStorySceneScript* script = Cast< const CStorySceneScript >( part ) )
		{
			Int32 immediateResult = -1;
			const CStorySceneLinkElement* nextLink = ProcessScriptBlock( script, immediateResult );

			#ifdef USE_SCENE_FLOW_MARKERS
			InternalFlowState::FlowMarker marker;
			marker.m_element = part;
			if ( immediateResult != -1 )
			{
				marker.m_desc = String::Printf( TXT("Script: [%ls], result [%d]"), script->GetFunctionName().AsChar(), immediateResult );
			}
			else
			{
				marker.m_desc = String::Printf( TXT("Script: [%ls]"), script->GetFunctionName().AsChar() );
			}
			marker.m_outputNum = immediateResult;
			flowState.AddFlowMarker( marker );
			#endif

			return EvaluateControlChain( start, part, nextLink, flowState, false );
		}

		if ( const CStorySceneOutput* output = Cast< CStorySceneOutput >( part ) )
		{
			if ( flowState.m_choiceToReturnTo )
			{
				const CStorySceneSection* injectedSection = flowState.m_currentSection;
				CStorySceneSection* sectionToReturnTo = flowState.m_choiceToReturnTo->GetSection();
				SCENE_ASSERT( sectionToReturnTo );
				SCENE_ASSERT( injectedSection );

				CStoryScene* ssTarget = sectionToReturnTo ? sectionToReturnTo->GetScene() : nullptr;
				CStoryScene* ssInjected = injectedSection ? injectedSection->GetScene() : nullptr;
				SCENE_ASSERT( ssTarget != ssInjected );

				flowState.m_choiceToReturnTo = nullptr;

				OnInjectedSceneFinished( output, ssInjected, ssTarget );

				#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
				marker.m_desc = String::Printf( TXT("Output: [%ls], returned to section: [%ls]"), output->GetName().AsChar(), sectionToReturnTo->GetName().AsChar() );
				marker.m_outputNum = 0;
				flowState.AddFlowMarker( marker );
				#endif

				return EvaluateControlChain( sectionToReturnTo, flowState, false );
			}
			else
			{
				flowState.SetActivatedOutput( output );

				if ( m_debugger )
				{
					m_debugger->OnFinishedAtOutput( output );
				}

				#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
				marker.m_desc = String::Printf( TXT("Output: [%ls]"), output->GetName().AsChar() );
				marker.m_outputNum = 0;
				flowState.AddFlowMarker( marker );
				#endif

				// DIALOG_TOMSIN_TODO
				// WTF???
				Pause( false );

				return NULL;
			}
		}

		// Just a link, follow it
		if ( const CStorySceneSection* section = Cast< CStorySceneSection >( part->GetParent() ) )
		{
			flowState.SetCurrentFlowElement( part );	
			if ( section->IsValid() )
			{
				Int32 index = FindInputIndexFromTo( prev, section, forced );
				flowState.SetSectionInputNumber( index );

#ifdef USE_SCENE_FLOW_MARKERS
				InternalFlowState::FlowMarker marker;
				marker.m_element = part;
				marker.m_desc = String::Printf( TXT("Section: [%ls]"), section->GetName().AsChar() );
				marker.m_outputNum = 0;
				flowState.AddFlowMarker( marker );
#endif

				return section;
			}
		}


		return EvaluateControlChain( start, part, part->GetNextElement(), flowState, false );
	}

	// Invalid part
	return NULL;
}

Bool CStoryScenePlayer::HasPendingChoice()
{
	return m_internalState.m_hasPendingChoice;
	/*return m_internalState.m_currentElementsInstances.Empty() == false 
		&& m_internalState.m_currentElementsInstances.Back().m_first->GetElement()->IsA< CStorySceneChoice >();*/
}

Bool CStoryScenePlayer::ShouldActorHaveHiResShadows( const CActor* a ) const
{
	const CName voiceTag = a->GetVoiceTag();
	if ( voiceTag )
	{
		if ( const CStorySceneActor* exActor = m_storyScene->GetActorDescriptionForVoicetag( voiceTag ) )
		{
			return exActor->m_useHiresShadows;
		}
	}
	return false;
}

void CStoryScenePlayer::SetActorHiResShadow( CActor* a, Bool flag )
{
	const Bool hasShadows = m_actorsWithHiResShadows.Exist( a );
	if ( hasShadows != flag )
	{
		if ( flag )
		{
			SCENE_ASSERT( !hasShadows );

#ifndef NO_EDITOR
			const Bool ret = a->SetAndCheckHiResShadows( true );
			SCENE_ASSERT( ret );
#else
			a->SetHiResShadows( true );
#endif
			m_actorsWithHiResShadows.PushBack( a );
		}
		else
		{
			SCENE_ASSERT( hasShadows );
#ifndef NO_EDITOR
			const Bool ret = a->SetAndCheckHiResShadows( false );
			SCENE_ASSERT( ret );
#else
			a->SetHiResShadows( false );
#endif
			SCENE_VERIFY( m_actorsWithHiResShadows.RemoveFast( a ) );
		}
	}

	SCENE_ASSERT( a->UseHiResShadows() == flag );
}

void CStoryScenePlayer::ResetAllActorsHiResShadow()
{
	const Uint32 size = m_actorsWithHiResShadows.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( CActor* a = m_actorsWithHiResShadows[i].Get() )
		{
			a->SetHiResShadows( false );
		}
	}

	m_actorsWithHiResShadows.Clear();
}

void CStoryScenePlayer::SetActorLodOverride( CActor* a, Bool forceHighestLod, Bool disableAutoHide )
{
	for( ComponentIterator< CMeshTypeComponent > it( a ); it; ++it )
	{
		CMeshTypeComponent* comp = *it;
		comp->SetForcedHighestLOD( forceHighestLod );
		comp->SetForceNoAutohide( disableAutoHide );
	}

	a->SetIsVisibleFromFar( disableAutoHide );

	if( const Bool lodOverrideEnabled = ( forceHighestLod || disableAutoHide ) )
	{
		m_actorsWithLodOverride.PushBackUnique( a );
	}
	else
	{
		m_actorsWithLodOverride.Remove( a );
	}
}

/*
Resets actor LOD overrides.

Important: this function doesn't reset "LOD override" to what it was when SetActorLodOverride() for given actor was called.
Instead, it acts as if SetActorLodOverride( actor, false, false ) was called.
*/
void CStoryScenePlayer::ResetActorLodOverrides()
{
	TDynArray< THandle< CActor > > actorList = m_actorsWithLodOverride;

	for( Uint32 iActor = 0, numActors = actorList.Size(); iActor < numActors; ++iActor )
	{
		if( CActor* a = actorList[ iActor ].Get() )
		{
			SetActorLodOverride( a, false, false );
		}
	}

	RED_ASSERT( m_actorsWithLodOverride.Empty() );
}

Bool CStoryScenePlayer::ShouldActorHaveMimicOn( const CActor* a ) const
{
	const CName voiceTag = a->GetVoiceTag();
	if ( voiceTag )
	{
		if ( const CStorySceneActor* exActor = m_storyScene->GetActorDescriptionForVoicetag( voiceTag ) )
		{
			return exActor->m_useMimic;
		}
	}
	return true;
}

void CStoryScenePlayer::SetActorMimicOn( CActor* a, Bool flag )
{
	const Bool hasMimics = m_actorsWithMimicOn.Exist( a );
	const Bool hasMimicsSet = a->HasMimic();

	SCENE_ASSERT__FIXME_LATER( hasMimics == hasMimicsSet );

	if ( hasMimicsSet != flag )
	{
		if ( flag )
		{
			SCENE_ASSERT( !hasMimicsSet );
			a->MimicOn();
			m_actorsWithMimicOn.PushBackUnique( a );
		}
		else
		{
			SCENE_ASSERT( hasMimicsSet );
			a->MimicOff();
			SCENE_VERIFY( m_actorsWithMimicOn.RemoveFast( a ) );
		}
	}
}

void CStoryScenePlayer::ResetAllActorsMimicOn()
{
	const Uint32 size = m_actorsWithMimicOn.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( CActor* a = m_actorsWithMimicOn[i].Get() )
		{
			SCENE_ASSERT( a->HasMimic() );
			if ( a->HasMimic() )
			{
				a->MimicOff();
			}
		}
	}

	m_actorsWithMimicOn.Clear();
}

Bool CStoryScenePlayer::IsGameplayNow() const
{
	if ( m_internalState.m_currentSection != NULL )
	{
		return m_internalState.m_currentSection->IsGameplay();
	}
	return true;
}

const CGUID& CStoryScenePlayer::GetActorsSlotID( const CName& actor ) const
{
	return m_sceneDirector.GetActorsSlotID( actor );
}

Bool CStoryScenePlayer::GetActorIdleAnimation( const CName& actor, CName& out ) const
{
	return m_eventsExecutor.GetCurrentActorIdleAnimation( actor, out );
}

Bool CStoryScenePlayer::IsActorOptional( CName id ) const
{
	return m_sceneController->IsActorOptional( id );
}

#ifndef NO_EDITOR

Bool CStoryScenePlayer::GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_eventsExecutor.GetCurrentActorState( actor, out );
}

Bool CStoryScenePlayer::GetCurrentLightState( const CName& actor, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const
{
	return m_eventsExecutor.GetCurrentLightState( actor, out, outPos );
}

Bool CStoryScenePlayer::GetPreviousActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const
{
	return m_eventsExecutor.GetPreviousActorState( actor, out );
}

Bool CStoryScenePlayer::GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight ) const
{
	return m_eventsExecutor.GetCurrentActorAnimationMimicState( actor, mimicEmoState, mimicLayerEyes, mimicLayerPose, mimicLayerAnim, poseWeight );
}

Matrix CStoryScenePlayer::GetActorPosition( const CName& actor ) const
{
	return m_sceneDirector.GetActorTrajectory( actor );
}

Matrix CStoryScenePlayer::GetDialogPosition() const
{
	EngineTransform trans = m_sceneDirector.GetCurrentScenePlacement();
	Matrix maxt;
	trans.CalcLocalToWorld( maxt );
	return maxt;
}

#endif

void CStoryScenePlayer::OnSceneStarted()
{
	const CStorySceneInput* input = m_sceneController->GetInput();
	SCENE_ASSERT( input != NULL );

	// surpass the global water rendering if needed
	if( m_world )
	{
		m_world->SetWaterVisible( m_world->IsWaterShaderEnabled(), GetStoryScene()->IsWaterRenderingSurpassed() );
	}

	if ( m_isGameplay == false )
	{
		String lockReason = String::Printf( TXT("Scene '%ls':%ls"), GetStoryScene()->GetDepotPath().AsChar(), input->GetName().AsChar() );
		CreateNoSaveLock( lockReason );

#ifdef RED_PROFILE_FILE_SYSTEM
		RED_ASSERT( m_sceneInProfiler == false, TXT("Scene already playing") );
		RedIOProfiler::ProfileSceneStarted( lockReason.AsChar() );
		m_sceneInProfiler = true;
#endif
	}

	if(GetStoryScene()->ShouldMuteSpeechUnderWater())
	{
		GSoundSystem->IncrementUnderwaterSpeechMutingCount();
	}
}

void CStoryScenePlayer::OnSceneEnded()
{
#ifndef RED_FINAL_BUILD
	SetDebugScreenText( TXT("") );
#endif

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_sceneInProfiler )
	{
		RedIOProfiler::ProfileSceneEnded();
		m_sceneInProfiler = false;
	}
#endif

	// Release save lock
	if ( m_saveBlockId >= 0 )
	{
		ReleaseNoSaveLock();
	}

	// remove surpass from the global water rendering
	if( m_world )
	{
		m_world->SetWaterVisible( m_world->IsWaterShaderEnabled(), false );
	}

	//Release sounds
	const TDynArray< CName > & banksDependency = m_storyScene->GetBanksDependency();
	Uint32 banksCount = banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->Unload();
	}

	m_sectionLoader.ClearPreloads();
	UnhideAllNonSceneActors();

	if(GetStoryScene()->ShouldMuteSpeechUnderWater())
	{
		GSoundSystem->DecrementUnderwaterSpeechMutingCount();
	}
}

void CStoryScenePlayer::DisableStreamingLockdown( const CStorySceneSection* section )
{
	if ( section->IsGameplay() == false )
	{
		// streaming lock
		if ( section->GetStreamingAreaTag() && IsSceneInGame() )
		{
			// todo: would be nice to have something unique that's not so slow to compute
			GGame->DisableStreamingLockdown( section->GetStreamingAreaTag() );
		}
	}
}

void CStoryScenePlayer::EnableStreamingLockdown( const CStorySceneSection* section )
{
	if ( section->IsGameplay() == false )
	{
		// streaming lock
		if ( section->GetStreamingAreaTag() && IsSceneInGame() )
		{
			// todo: would be nice to have something unique that's not so slow to compute
			const String& idName = section->GetFriendlyName();
			GGame->EnableStreamingLockdown( idName, section->GetStreamingAreaTag() );
		}
	}
}

void CStoryScenePlayer::CycleStreamingCamera( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection )
{
	// Clear up previous section
	if( prevSection && prevSection->GetStreamingUseCameraPosition() )
	{
		const String lockName = prevSection->GetFriendlyName(); 
		GGame->DisableCameraBasedStreaming( lockName );
	}

	// streaming camera override
	if ( nextSection && nextSection->GetStreamingUseCameraPosition() )
{
		const Float distance = nextSection->GetStreamingCameraAllowedJumpDistance();
		const String lockName = nextSection->GetFriendlyName();

		GGame->EnableCameraBasedStreaming( lockName, distance, distance*1.2f ); // magic number :(
	}
}

void CStoryScenePlayer::OnSectionStarted( const CStorySceneSection* section, const CStorySceneSection* previousSection )
{
	if ( section && section->IsGameplay() == false )
	{
#ifndef RED_FINAL_BUILD
		SetDebugScreenText( String(TXT("Current section: ") ) + section->GetName() );
#endif

#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSceneSectionStarted( section->GetName().AsChar() );
#endif

		if ( m_sceneDirector.IsEnabled() == false )
		{
			m_sceneDirector.EnableDirector();
		}
		m_sceneDirector.FinalizeTeleportNPCsJob();

		for ( THashMap< CName, THandle< CEntity > >::iterator actorIter = m_sceneActorEntities.Begin();
			actorIter != m_sceneActorEntities.End(); ++actorIter )
		{
			THandle< CEntity >& actorHandle = actorIter->m_second;
			CActor *actor = Cast< CActor >( actorHandle.Get() );
			if ( actor == NULL )
			{
				continue;
			}

			actor->SetCrossSafeZoneEnabled( true );
		}

		CycleStreamingCamera( previousSection, section );

		RED_ASSERT( m_world != nullptr );

		if ( m_world )
			m_world->GetEnvironmentManager()->SetHiResShadowMapExtents( section->GetMaxBoxExtentsToApplyHiResShadows() );
	}	
}	

void CStoryScenePlayer::OnSectionEnded( const CStorySceneSection* section )
{
#ifndef RED_FINAL_BUILD
	SetDebugScreenText( String(TXT("") ) );
#endif

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( section && section->IsGameplay() == false )
	{
		RedIOProfiler::ProfileSceneSectionEnded();

		RED_ASSERT( m_world != nullptr );

		if ( m_world )
			m_world->GetEnvironmentManager()->SetHiResShadowMapExtents( -1.0f );
	}
#endif

	ClearDeniedAreas();
	UnhideAllNonSceneActors();
	m_sceneDirector.CancelTeleportNPCsJob();
}

void CStoryScenePlayer::SignalPlaybackEnd( Bool stopped )
{
	UpdatePlaybackListeners();
	
	TDynArray< IStoryScenePlaybackListener* > list = m_playbackListeners;
	for ( TDynArray< IStoryScenePlaybackListener* >::iterator listenerIter = list.Begin(); listenerIter != list.End(); ++listenerIter )
	{
		if ( ( *listenerIter ) == NULL )
		{
			continue;
		}
		( *listenerIter )->OnEnd( this, stopped );
	}
}


void CStoryScenePlayer::ProcessSceneStateForActors( Bool active )
{
	for ( auto it : m_sceneActorEntities )
	{
		CEntity* actor = it.m_second.Get();
		if ( actor && actor != m_player )
		{
			SetSceneStateOnActor( it.m_second, active );
		}		
	}

	for( auto it : m_scenePropEntities )
	{
		if( CEntity* actor = it.m_second.Get() )
		{
			SetSceneStateOnProp( it.m_first, it.m_second, active );	
		}
	}

	if ( CPlayer* player = m_player.Get() )
	{
		player->SetPlayerMovable( !active, active );

		THandle< CEntity > playerH( m_player );
		SetSceneStateOnActor( playerH, active );
	}
}

CGatheredResource resSceneBehGraph( TXT("gameplay\\behaviors\\pc\\behaviorgraph\\pc_dialog.w2beh"), RGF_Startup );

void CStoryScenePlayer::SetSceneBehaviourInst( CEntity* ent, Bool enable, Bool addGraphIfMissing )
{
	CAnimatedComponent* animatedComponent = ent->GetRootAnimatedComponent();
	CBehaviorGraphStack* behaviorStack = animatedComponent ? animatedComponent->GetBehaviorStack() : NULL;

	if ( enable )
	{
		if ( behaviorStack && !behaviorStack->HasInstance( CNAME( StoryScene ) ) )
		{
			if ( behaviorStack->HasInstanceSlot( CNAME( StoryScene ) ) )
			{
				behaviorStack->AttachBehaviorInstance( CNAME( StoryScene ) );
			}
			else if( addGraphIfMissing )
			{
				CBehaviorGraph* behGraph = resSceneBehGraph.LoadAndGet< CBehaviorGraph >();
				behaviorStack->AttachBehaviorInstance( behGraph, CNAME( StoryScene ) );
			}
		}
	}
	else
	{
		if ( behaviorStack && behaviorStack->HasInstance( CNAME( StoryScene ) ) == true )
		{
			behaviorStack->DetachBehaviorInstance( CNAME( StoryScene ) );
		}
	}
}

void CStoryScenePlayer::SetSceneStateOnProp( CName id, const THandle< CEntity >& actorHandle, Bool enable )
{
	CEntity *ent = actorHandle.Get();
	if ( ent == NULL )
	{
		return;
	}
	ent->SetForceNoLOD( enable );

	for( ComponentIterator<CRigidMeshComponent> iter( ent ); iter; ++iter )
	{
		(*iter)->SetEnabled( false );
	}

	if( CAnimatedComponent* animatedComponent = ent->GetRootAnimatedComponent() )
	{
		if( enable )
		{
			animatedComponent->OnCinematicStorySceneStarted();
		}
		else
		{
			animatedComponent->OnCinematicStorySceneEnded();
		}	
	}

	if ( const CStorySceneProp* propDef = GetStoryScene()->GetPropDefinition( id ) )
	{
		CAnimatedComponent* animatedComponent = ent->GetRootAnimatedComponent();
		CBehaviorGraphStack* behaviorStack = animatedComponent ? animatedComponent->GetBehaviorStack() : NULL;

		if ( enable && behaviorStack )
		{
			if( propDef->m_forceBehaviorGraph )
			{
				behaviorStack->ActivateBehaviorInstances( propDef->m_forceBehaviorGraph );
			}
			if( propDef->m_resetBehaviorGraph )
			{
				behaviorStack->Reset();
			}	
		}

		IActorInterface* actor = ent->QueryActorInterface();	
		if( propDef->m_useMimics && actor )
		{
			if( enable )
			{
				actor->MimicOn();
			}
			else
			{
				actor->MimicOff();
			}
		}				
	}
	SetSceneBehaviourInst( ent, enable, true );
}

Bool CStoryScenePlayer::FindSafeSpotForActor( const THandle< CEntity >& entity, const Vector inPos, Vector& outPos )
{
	CActor* actor = Cast< CActor >( entity.Get() );
	if ( !actor )
	{
		return false;
	}

	return StorySceneUtils::DoTraceZTest( actor, inPos, outPos );
}

Bool CStoryScenePlayer::SetSceneStateOnActor( const THandle< CEntity >& actorHandle, Bool enable, Vector* teleportedTransform )
{
	Bool teleported = false;

	CActor *actor = Cast< CActor >( actorHandle.Get() );
	if ( actor == NULL )
	{
		return teleported;
	}

	const Bool wasLockedByScene = actor->IsLockedByScene();
// 	if ( !enable && !wasLockedByScene )
// 	{
// 		return teleported;
// 	}

	actor->SetSceneLock( enable, false );

	actor->SetCrossSafeZoneEnabled( enable );

	actor->SetForceNoLOD( enable );

	if ( enable )
	{
		actor->CallEvent< THandle< CStoryScene > >( CNAME( OnBlockingSceneStarted ), GetStoryScene() );
		if ( ShouldActorHaveMimicOn( actor ) )
		{
			SetActorMimicOn( actor, true );
		}
	}
	else
	{
		SetActorMimicOn( actor, false );

		actor->DisableDialogsLookAts( 1.f );

		if ( m_eventsExecutor.IsEntityPositionControlledByScene( actor ) )
		{
			const Vector& actorPosition = actor->GetRootAnimatedComponent() ? actor->GetRootAnimatedComponent()->GetThisFrameTempPositionWSRef() : actor->GetWorldPositionRef();

			Vector safePosition;
			if ( FindSafeSpotForActor( actor, actorPosition, safePosition ) )
			{
				if ( !Vector::Near3( actorPosition, safePosition, 0.05f ) ) // 5cm
				{
					ERR_GAME( TXT("Actor '%ls' will be teleported to a safe position after scene. Cs Point [%1.2f, %1.2f, %1.2f], safe position [%1.2f, %1.2f, %1.2f]")
						, actor->GetFriendlyName().AsChar()
						, actorPosition.X, actorPosition.Y, actorPosition.Z
						, safePosition.X, safePosition.Y, safePosition.Z );
					SCENE_ERROR( TXT("Actor '%ls' will be teleported to a safe position after scene. Cs Point [%1.2f, %1.2f, %1.2f], safe position [%1.2f, %1.2f, %1.2f]")
						, actor->GetFriendlyName().AsChar()
						, actorPosition.X, actorPosition.Y, actorPosition.Z
						, safePosition.X, safePosition.Y, safePosition.Z );
				}

				CMovingAgentComponent* actorMovingAgent = actor->GetMovingAgentComponent();
				ASSERT( actorMovingAgent );

				actorMovingAgent->CancelMove();

				EulerAngles newRotation = actorMovingAgent->GetTeleportedRotationOrWSRotation();
				newRotation.Pitch = 0.0f;
				newRotation.Roll = 0.0f;

				if ( teleportedTransform )
				{
					SCENE_ASSERT( safePosition.IsOk() );
					*teleportedTransform = safePosition;
				}
				else
				{
					actor->Teleport( safePosition, newRotation );
				}

				teleported = true;
			}
		}

		actor->CallEvent< THandle< CStorySceneOutput > >( CNAME( OnBlockingSceneEnded ), m_internalState.m_activatedOutput );
	}

	SetSceneBehaviourInst( actor, enable );

	return teleported;
}

void CStoryScenePlayer::SetupPlayerName( const CStorySceneInput* startInput )
{
	// Take base name from section name
	m_name = startInput->GetName();

	// Add a file name
	if ( startInput->GetScene()->GetFile() )
	{
		m_name += TXT(" in ");
		m_name += CFilePath( startInput->GetScene()->GetFriendlyName() ).GetFileName();
	}
}

void CStoryScenePlayer::ResetPlayer()
{
	m_isPaused = 0;
	m_isStopped = false;
	m_sceneController = nullptr;
	m_storyScene = nullptr;
	m_saveBlockId = -1;
	m_interceptionHelper.OnResetPlayer();

	m_internalState = InternalFlowState();
}

void CStoryScenePlayer::ChangeSceneSettingWithoutResetActors()
{
	//CName dialogsetName = FindDialogsetForSection( m_internalState.m_currentSection );
	//const CStorySceneDialogsetInstance* currentDialogsetInstance = GetCurrentStoryScene()->GetDialogsetByName( dialogsetName );

	m_sceneDirector.FinishChangingSettings();
}

void CStoryScenePlayer::ChangeSceneSettingAndResetActors()
{
	ChangeSceneSettingWithoutResetActors();

	ResetAllSceneEntitiesState();
}

void CStoryScenePlayer::ResetAllSceneEntitiesState()
{
	if ( m_internalState.m_currentSection )
	{
		const CStorySceneDialogsetInstance* currentDialogsetInstance = m_sceneDirector.GetCurrentSettings();

		Bool forceDialogset = m_internalState.m_currentSection->ShouldForceDialogset() && currentDialogsetInstance;

		OnPreResetAllSceneEntitiesState( forceDialogset, m_internalState.m_currentSection, currentDialogsetInstance );

		Bool isCutscene = Cast<CStorySceneCutsceneSection>( m_internalState.m_currentSection ) != nullptr;
		Bool wasCutscene = Cast<CStorySceneCutsceneSection>( m_internalState.m_prevSection ) != nullptr;
		Bool isGameplay = m_internalState.m_currentSection->IsGameplay();
		Bool wasGameplay = !m_internalState.m_prevSection || m_internalState.m_prevSection->IsGameplay();

		m_eventsExecutor.ForceResetForAllEntities( currentDialogsetInstance, forceDialogset, isGameplay, wasGameplay, isCutscene, wasCutscene, this, m_debugger );
	}
}

void CStoryScenePlayer::AddPlaybackListener( IStoryScenePlaybackListener* listener )
{
	SCENE_ASSERT__FIXME_LATER( !m_internalState.m_isInTick );
	m_playbackListenersToAdd.PushBackUnique( listener );
}

void CStoryScenePlayer::RemovePlaybackListener( IStoryScenePlaybackListener* listener )
{
	SCENE_ASSERT__FIXME_LATER( !m_internalState.m_isInTick );
	m_playbackListenersToRemove.PushBackUnique( listener );
}

CStorySceneSectionPlayingPlan* CStoryScenePlayer::GetCurrentPlayingPlan()
{
	return m_sectionLoader.FindPlanById( m_internalState.m_planId );
}

const CStorySceneSectionPlayingPlan* CStoryScenePlayer::GetCurrentPlayingPlan() const
{
	return m_sectionLoader.FindPlanById( m_internalState.m_planId );
}

Bool CStoryScenePlayer::RegisterActor( const CName& voiceTag, CEntity* actor )
{
	SCENE_ASSERT( actor != NULL );
	if( actor == NULL )
	{
		return false;
	}

	// Assert that actor with specified voicetag is not already registered.
	SCENE_ASSERT( !m_sceneActorEntities.KeyExist( voiceTag ) );

	LOG_GAME( TXT( "ScenePlayer: Registering actor '%ls' under voiceTag '%ls'" ),
		actor->GetName().AsChar(), voiceTag.AsString().AsChar() );

	THandle< CEntity > actorHandle( actor );
	return m_sceneActorEntities.Insert( voiceTag, actorHandle );
}

Bool CStoryScenePlayer::RegisterProp( const CName& id, CEntity* prop )
{
	SCENE_ASSERT( prop != NULL );
	if( prop == NULL )
	{
		return false;
	}

	LOG_GAME( TXT( "ScenePlayer: Registering prop '%ls' with ID '%ls'" ), prop->GetName().AsChar(), id.AsString().AsChar() );

	THandle< CEntity > propHandle( prop );
	return m_scenePropEntities.Insert( id, propHandle );
}

Bool CStoryScenePlayer::RegisterEffect( const CName& id, CEntity* effect )
{
	SCENE_ASSERT( effect != NULL );
	if( effect == NULL )
	{
		return false;
	}

	LOG_GAME( TXT( "ScenePlayer: Registering effect '%ls' with ID '%ls'" ), effect->GetName().AsChar(), id.AsString().AsChar() );

	THandle< CEntity > effectHandle( effect );
	return m_sceneEffectEntities.Insert( id, effectHandle );
}

Bool CStoryScenePlayer::RegisterLight( const CName& id, CEntity* light )
{
	SCENE_ASSERT( light != NULL );
	if( light == NULL )
	{
		return false;
	}

	LOG_GAME( TXT( "ScenePlayer: Registering light '%ls' with ID '%ls'" ), light->GetName().AsChar(), id.AsString().AsChar() );

	THandle< CEntity > lightHandle( light );
	return m_sceneLightEntities.Insert( id, light );
}

void CStoryScenePlayer::UnregisterActor( const CName& voiceTag )
{
	// Assert that actor with specified voicetag is registered.
	SCENE_ASSERT( m_sceneActorEntities.KeyExist( voiceTag ) );

	// If cutscene is not setup correctly we should make sure actor will not be left in scene state
	// Bad cutscene can register actors if they have voicetags even if the scene itself doesn't use them
	CActor* registeredActor = GetMappedActor( voiceTag );
	if ( registeredActor != NULL )
	{
		registeredActor->CallEvent< THandle< CStorySceneOutput > >( CNAME( OnBlockingSceneEnded ), m_internalState.m_activatedOutput );
		m_sceneController->UnregisterSceneActor( registeredActor, true );
	}

	LOG_GAME( TXT( "ScenePlayer: Unregistering voiceTag '%ls'" ), voiceTag.AsString().AsChar() );
	m_sceneActorEntities.Erase( voiceTag );
}

void CStoryScenePlayer::UnregisterProp( const CName& id )
{
	LOG_GAME( TXT( "ScenePlayer: Unregistering prop '%ls'" ), id.AsString().AsChar() );
	m_scenePropEntities.Erase( id );
}

void CStoryScenePlayer::UnregisterEffect( const CName& id )
{
	LOG_GAME( TXT( "ScenePlayer: Unregistering effect '%ls'" ), id.AsString().AsChar() );
	m_sceneEffectEntities.Erase( id );
}

void CStoryScenePlayer::UnregisterLight( const CName& id )
{
	LOG_GAME( TXT( "ScenePlayer: Unregistering light '%ls'" ), id.AsString().AsChar() );
	m_sceneLightEntities.Erase( id );
}

CActor* CStoryScenePlayer::GetMappedActor( CName voiceTag )
{
	return Cast< CActor >( GetSceneActorEntity( voiceTag ) );
}

THandle<CEntity>*	CStoryScenePlayer::GetSceneActorEntityHandle( const CName& actorName )
{
	return m_sceneActorEntities.FindPtr( actorName );
}

CEntity* CStoryScenePlayer::SpawnSceneActorEntity( const CName& actorName )
{
	THandle< CEntity >* entityHandle = m_sceneActorEntities.FindPtr( actorName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	else if ( true )
	{
		CActor* actor = nullptr;
		if( const CStorySceneActor* aDef = GetCurrentStoryScene()->GetSceneActorDefinition( actorName ) )
		{
			if( !aDef->m_forceSpawn )
			{
				actor = m_sceneDirector.SearchForActorInCommunity( actorName );			
			}
			if ( actor )
			{
				AddNewActor( actor, actorName, true );
			}		
			else if ( !IsActorOptional( actorName ) )
			{
				actor = m_sceneDirector.SpawnActorOnDemand( actorName );
				AddNewActor( actor, actorName, false );
			}				
			return actor;
		}		
	}
	return NULL;
}

CEntity* CStoryScenePlayer::SpawnScenePropEntity( const CName& propName )
{
	THandle< CEntity >* entityHandle = m_scenePropEntities.FindPtr( propName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	else if ( true )
	{
		CEntity* prop = m_sceneDirector.SpawnPropOnDemand( propName );
		AddNewProp( prop, propName );
		m_sceneController->RegisterSpawnedProp( propName, prop );
		return prop;
	}
	return NULL;
}

CEntity* CStoryScenePlayer::SpawnSceneEffectEntity( const CName& fxName )
{
	THandle< CEntity >* entityHandle = m_sceneEffectEntities.FindPtr( fxName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	else if ( true )
	{
		CEntity* prop = m_sceneDirector.SpawnEffectOnDemand( fxName );
		AddNewEffect( prop, fxName );
		m_sceneController->RegisterSpawnedEffect( fxName, prop );
		return prop;
	}
	return NULL;
}

CEntity* CStoryScenePlayer::SpawnSceneLightEntity( const CName& lightName )
{
	THandle< CEntity >* entityHandle = m_sceneLightEntities.FindPtr( lightName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	else if ( true )
	{
		CEntity* prop = m_sceneDirector.SpawnLightOnDemand( lightName );
		AddNewLight( prop, lightName );
		m_sceneController->RegisterSpawnedLight( lightName, prop );
		return prop;
	}
	return NULL;
}

const CEntity* CStoryScenePlayer::GetSceneActorEntity( const CName& actorName ) const
{
	const THandle< CEntity >* entityHandle = m_sceneActorEntities.FindPtr( actorName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

CEntity* CStoryScenePlayer::GetSceneActorEntity( const CName& actorName )
{
	THandle< CEntity >* entityHandle = m_sceneActorEntities.FindPtr( actorName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

const CEntity* CStoryScenePlayer::GetScenePropEntity( const CName& propName ) const
{
	const THandle< CEntity >* entityHandle = m_scenePropEntities.FindPtr( propName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

CEntity* CStoryScenePlayer::GetScenePropEntity( const CName& propName )
{
	THandle< CEntity >* entityHandle = m_scenePropEntities.FindPtr( propName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

const CEntity* CStoryScenePlayer::GetSceneEffectEntity( const CName& fxName ) const
{
	const THandle< CEntity >* entityHandle = m_sceneEffectEntities.FindPtr( fxName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

const CEntity* CStoryScenePlayer::GetSceneLightEntity( const CName& lightName ) const
{
	const THandle< CEntity >* entityHandle = m_sceneLightEntities.FindPtr( lightName );
	if ( entityHandle )
	{
		return entityHandle->Get();
	}
	return NULL;
}

void CStoryScenePlayer::AddNewActor( CActor* actor, CName voiceTag, Bool dontDestroyAfterScene )
{
	if ( actor != NULL )
	{
		THandle< CEntity > actorHandle( actor );

		actor->AddStoryScene( m_sceneController );
		m_sceneController->RegisterNewSceneActor( actor, voiceTag, dontDestroyAfterScene );

		if ( m_internalState.m_currentSection != NULL && m_internalState.m_currentSection->IsGameplay() == false )
		{
			SetSceneStateOnActor( actorHandle, true );
		}

		RegisterActor( voiceTag, actor );
	}
}

void CStoryScenePlayer::AddNewProp( CEntity* prop, CName id )
{
	if ( prop != NULL )
	{
		RegisterProp( id, prop );

		THandle< CEntity > actorHandle( prop );
		if ( m_internalState.m_currentSection != NULL && m_internalState.m_currentSection->IsGameplay() == false )			
		{
			SetSceneStateOnProp( id, actorHandle, true );
		}
	}

}

void CStoryScenePlayer::AddNewEffect( CEntity* effect, CName id )
{
	if ( effect != NULL )
	{
		RegisterEffect( id, effect );
	}
}

void CStoryScenePlayer::AddNewLight( CEntity* light, CName id )
{
	if ( light != NULL )
	{
		RegisterLight( id, light );
	}
}

Bool CStoryScenePlayer::GetActorsUsedInCurrentSection( TDynArray< THandle< CEntity> >& sectionActors ) const
{
	const CStorySceneSection* section = m_internalState.m_currentSection;
	SCENE_ASSERT( section );
	if ( section )
	{
		return false;
	}

	TDynArray< CName > sectionVoicetags;
	section->GetVoiceTags( sectionVoicetags );

	if ( section->UsesSetting() == true )
	{
		const CStorySceneDialogsetInstance* setting = m_sceneDirector.GetCurrentSettings();
		if ( setting != NULL )
		{
			setting->GetAllActorNames( sectionVoicetags );
		}
	}

	for ( Uint32 i = 0; i < sectionVoicetags.Size(); ++i )
	{
		CName& voicetag = sectionVoicetags[ i ];
		
		THandle< CEntity > actorHandle;
		m_sceneActorEntities.Find( voicetag, actorHandle );

		if ( actorHandle.Get() )
		{
			sectionActors.PushBack( actorHandle );
		}
		else
		{
			SCENE_WARN( TXT( "Cannot map actor %ls in section '%ls' in scene '%ls'" ), 
				voicetag.AsString().AsChar(), section->GetName().AsChar(), GetName().AsChar() );
		}
	}
	if ( sectionActors.Size() != sectionVoicetags.Size() )
	{
		SCENE_WARN( TXT( "Cannot map all actors of section '%ls' in scene '%ls'" ), 
			section->GetName().AsChar(), section->GetScene()->GetFriendlyName().AsChar() );
		return false;
	}
	return true;
}

void CStoryScenePlayer::GetVoiceTagsForCurrentSetting( TDynArray< CName >& voicetags, Bool append /*= true */ ) const
{
	const CStorySceneDialogsetInstance* setting = m_sceneDirector.GetCurrentSettings();
	if ( setting )
	{
		const TDynArray< CStorySceneDialogsetSlot* >& dialogsetSlots = setting->GetSlots();
		for ( TDynArray< CStorySceneDialogsetSlot* >::const_iterator slotIter = dialogsetSlots.Begin(); slotIter != dialogsetSlots.End(); ++slotIter )
		{
			if ( *slotIter != NULL && (*slotIter)->GetActorName() != CName::NONE )
			{
				voicetags.PushBackUnique( (*slotIter)->GetActorName() );
			}
		}
	}
	else
	{
		SCENE_ASSERT( setting );
		SCENE_ASSERT( m_internalState.m_currentSection );
		if ( m_internalState.m_currentSection )
		{
			m_internalState.m_currentSection->GetVoiceTags( voicetags, append );
		}
	}
}

Bool CStoryScenePlayer::AddDeniedArea( const TDynArray<Vector>& localSpaceConvex, const EngineTransform& worldTransform )
{
	SDeniedAreaSpawnData deniedAreaSpawnData;
	deniedAreaSpawnData.m_collisionType = PLC_Immediate;
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_customData = &deniedAreaSpawnData;

	CDeniedAreaComponent* deniedArea = static_cast< CDeniedAreaComponent* >( CreateComponent( CDeniedAreaComponent::GetStaticClass(), spawnInfo ) );
	deniedArea->SetPosition( worldTransform.GetPosition() );
	deniedArea->SetRotation( worldTransform.GetRotation() );
	deniedArea->SetLocalPoints( localSpaceConvex, true );
	return true;
}

void CStoryScenePlayer::ClearDeniedAreas()
{
	TDynArray< CDeniedAreaComponent* > deniedAreas;
	for ( ComponentIterator< CDeniedAreaComponent > it( this ); it; ++it )
	{
		deniedAreas.PushBack( *it );
	}
	for ( Uint32 i = 0; i < deniedAreas.Size(); ++i )
	{
		DestroyComponent( deniedAreas[ i ] );
	}
	SCENE_LOG( TXT("Removed %u denied areas from scene player"), deniedAreas.Size() );
}

void CStoryScenePlayer::CheckPauseConditions()
{
	Bool shouldPause = false;
	Bool shouldStop = false;

	CPlayer* player = m_player.Get();
	if ( player && player->IsAlive() == false )
	{
		// Every scene should stop if player is dead
		shouldStop = true;
	}

	if ( m_internalState.m_currentSection != NULL && m_internalState.m_currentSection->ShouldPauseInCombat() == true )
	{

		TDynArray< THandle< CEntity > > sectionActors;
		Bool allSectionActorsPresent = GetActorsUsedInCurrentSection( sectionActors );

		shouldPause = false;
		shouldStop = allSectionActorsPresent == false;

		for ( TDynArray< THandle< CEntity > >::iterator actorIter  = sectionActors.Begin();
			actorIter != sectionActors.End(); ++actorIter )
		{
			CActor *actor = Cast< CActor >( actorIter->Get() );
			if ( actor == NULL || actor->IsAlive() == false )
			{
				SCENE_LOG( TXT( "%ls - Suggested stop for scene because one of the actors is no longer alive."), m_sceneController->GetInput()->GetFriendlyName().AsChar() );
				shouldStop = true;
				break;
			}
			if ( actor->IsInCombat() == true )
			{
				if ( IsPaused() == false )
				{
					SCENE_LOG( TXT( "%ls - Suggested pause for scene because actor(%ls) is in combat (pause may yet be ignored)"), m_sceneController->GetInput()->GetFriendlyName().AsChar(), actor->GetName().AsChar() );
				}
				shouldPause = true;
				break;
			}
		}
	}

	// Gameplay sections should pause, when player is busy in non-gameplay scenes
	// (but other than current)
	if( player && m_internalState.m_currentSection != NULL && m_internalState.m_currentSection->CanPlayDuringBlockingScenes() == false )
	{
		Bool isInNonGameplayScene = false;
		const TDynArray< CStorySceneController* >& scenes = player->GetStoryScenes();
		for( TDynArray< CStorySceneController* >::const_iterator sceneIter = scenes.Begin();
			sceneIter != scenes.End(); ++sceneIter )
		{
			CStorySceneController* controller = *sceneIter;
			SCENE_ASSERT( controller != NULL );
			SCENE_ASSERT( controller->GetInput() != NULL );

			// Ignore the same scene
			if( controller->GetPlayer() == this )
			{
				continue;
			}

			isInNonGameplayScene |= ! controller->GetInput()->IsGameplay();
		}

		shouldPause |= isInNonGameplayScene;
	}

	if ( IsGameplay() == true && IsSectionChanging() == false )
	{
		if ( shouldStop == true && m_isStopped == false )
		{
			Stop();
		}
		Pause( shouldPause );
	}
}

Bool CStoryScenePlayer::IsSceneBlackscreenSet() const
{
	return m_blackscreenSet;
}

void CStoryScenePlayer::SetSceneBlackscreen( Bool flag, const String& reason )
{ 
	GCommonGame->GetSystem< CStorySceneSystem >()->Blackscreen( GetName(), flag, reason );
	m_blackscreenSet = flag;
}

void CStoryScenePlayer::SceneFadeIn( const String& reason, Float duration, const Color& color )
{
	SCENE_ASSERT( m_blackscreenSet );
	GCommonGame->GetSystem< CStorySceneSystem >()->Fade( GetName(), reason, true, duration, color );
	m_blackscreenSet = false;
}

void CStoryScenePlayer::SceneFadeOut( const String& reason, Float duration, const Color& color )
{
	SCENE_ASSERT( !m_blackscreenSet );
	GCommonGame->GetSystem< CStorySceneSystem >()->Fade( GetName(), reason, false, duration, color );
	m_blackscreenSet = true;
}

void CStoryScenePlayer::SceneFrozenFrameStart()
{
	static Float blur = 4.f;
	static Float duration = 10.f;

	SCENE_ASSERT( !m_frozenFrameSet );
	( new CRenderCommand_ShowLoadingScreenBlur( blur, duration ) )->Commit();
	m_frozenFrameSet = true;
}

void CStoryScenePlayer::SceneFrozenFrameEnd()
{
	SCENE_ASSERT( m_frozenFrameSet );
	( new CRenderCommand_HideLoadingScreenBlur( 1.0 ) )->Commit();
	m_frozenFrameSet = false;
}

void CStoryScenePlayer::EnableTickDuringFade( Bool enable )
{
	m_allowTickDuringFade = enable;
}

Bool CStoryScenePlayer::IsDialogSetPlacementSafe( EngineTransform& transform, const TDynArray< Vector >& sceneConvex, const Box& sceneBox, const TDynArray< Vector >& dialogSetPoints, const TDynArray< CEntity* >& sceneActors, Bool isInterior, Bool checkConvex, Bool& isAcceptableAsFallbackOnly ) const
{
	CPathLibWorld* pathLibWorld = GCommonGame->GetActiveWorld()->GetPathLibWorld();

	// Check if the placement is on navmesh

	if ( checkConvex )
	{
		// Fast point test first

		if ( !pathLibWorld->TestLocation( transform.GetPosition(), PathLib::CT_DEFAULT ) )
		{
			return false;
		}

		// Full convex test

		if ( !pathLibWorld->ConvexHullCollisionTest( sceneConvex, sceneBox, transform.GetPosition() ) )
		{
			return false;
		}
	}
	else
	{
		// Just check all dialog set points

		const Float personalSpace = 0.3f;

		for ( const Vector& point : dialogSetPoints )
		{
			if ( !pathLibWorld->TestLocation( point + transform.GetPosition(), personalSpace, PathLib::CT_DEFAULT ) )
			{
				return false;
			}
		}
	}

	// Determine height variance

	Bool heightCheckFailed = false;
	Float minHeight = FLT_MAX;
	Float maxHeight = -FLT_MAX;

	auto processSceneVertexFunc =
		[ &pathLibWorld, &minHeight, &maxHeight, &heightCheckFailed, &transform ] ( const Vector& vertex )
	{
		Float height;
		if ( !pathLibWorld->ComputeHeight( vertex + transform.GetPosition(), height ) )
		{
			heightCheckFailed = true;
			return;
		}

		minHeight = Min( minHeight, height );
		maxHeight = Max( maxHeight, height );
	};

	if ( heightCheckFailed )
	{
		return false;
	}

#if 0 // Considering scene convex would significantly increase height variance making it less likely to find safe placement
	std::for_each( sceneConvex.Begin(), sceneConvex.End(), processSceneVertexFunc );
#endif
	std::for_each( dialogSetPoints.Begin(), dialogSetPoints.End(), processSceneVertexFunc );

	// Snap to ground

	const Float acceptableHeightVariance = 0.2f;
	const Float heightVariance = maxHeight - minHeight;
	if ( heightVariance > acceptableHeightVariance )
	{
		return false;
	}

	const Float originalZ = transform.GetPosition().Z;

	Vector snappedPosition = transform.GetPosition();
	snappedPosition.Z = ( minHeight + maxHeight ) * 0.5f;
	transform.SetPosition( snappedPosition );

	// Check height difference

	const Float heightDifference = Abs( originalZ - snappedPosition.Z );

	const Float acceptableHeightDifferenceWrtOriginalHeight = 10.0f;
	if ( heightDifference > acceptableHeightDifferenceWrtOriginalHeight )
	{
		return false; // Not acceptable even as a fallback
	}

	const Float acceptableHeightDifferenceWrtOriginalHeightFallback = 3.0f;
	isAcceptableAsFallbackOnly = heightDifference > acceptableHeightDifferenceWrtOriginalHeightFallback;

	// Interior test (must match expected interiorness)

	RED_ASSERT( !dialogSetPoints.Empty() );
	const Bool firstDialogSetPointInside = GCommonGame->IsPositionInInterior( dialogSetPoints[ 0 ] + transform.GetPosition() );
	if ( firstDialogSetPointInside != isInterior )
	{
		return false;
	}
	for ( Uint32 i = 1; i < dialogSetPoints.Size(); ++i )
	{
		if ( firstDialogSetPointInside != GCommonGame->IsPositionInInterior( dialogSetPoints[ i ] + transform.GetPosition() ) )
		{
			return false;
		}
	}

	// Check if the placement collides with non-scene actors

	struct SearchActorsFunctor
	{
		enum { SORT_OUTPUT = 0 };

		const TDynArray< CEntity* >& m_sceneActors;
		const TDynArray< Vector >& m_sceneConvex;
		Vector m_transformPos;
		Bool m_failed;

		SearchActorsFunctor( const TDynArray< CEntity* >& sceneActors, const TDynArray< Vector >& sceneConvex, const Vector& transformPos )
			: m_sceneActors( sceneActors )
			, m_sceneConvex( sceneConvex )
			, m_transformPos( transformPos )
			, m_failed( false )
		{}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& ptr )
		{
			// Fail if non-scene actor is inside placement convex

			CActor* actor = ptr.Get();
			if ( !actor || m_sceneActors.Exist( actor ) )
			{
				return true;
			}
			if ( !IsPointInsideConvexShape( actor->GetWorldPositionRef() - m_transformPos, m_sceneConvex ) )
			{
				return true;
			}

			// Fail - stop searching

			m_failed = true;
			return false;
		}

		Bool DidFail() const { return m_failed; }

	} searchActorsFunctor( sceneActors, sceneConvex, transform.GetPosition() );

	Box sceneWorldBox = sceneBox;
	sceneWorldBox.Min += transform.GetPosition();
	sceneWorldBox.Max += transform.GetPosition();
	GCommonGame->GetActorsManager()->TQuery( searchActorsFunctor, sceneWorldBox, true, nullptr, 0 );
	if ( searchActorsFunctor.DidFail() )
	{
		return false;
	}

	return true;
}

Bool CStoryScenePlayer::FindSafeDialogSetPlacement( const CStorySceneDialogsetInstance* dialogSetInstance, const EngineTransform& initialScenePlacement, TDynArray< CNode* >& taggedNodes, EngineTransform& placementTransform ) const
{
	// Collect scene actors and check if they're all inside

	Bool isInterior = true;
	TDynArray< CEntity* > sceneActors;
	auto& actors = const_cast< CStorySceneController* >( GetSceneController() )->GetMappedActors();
	for ( const CStorySceneController::SceneActorInfo& sceneActor : actors )
	{
		if ( CEntity* entity = sceneActor.m_actor.Get() )
		{
			sceneActors.PushBack( entity );

			if ( !GCommonGame->IsPositionInInterior( entity->GetWorldPositionRef() ) )
			{
				isInterior = false;
			}
		}
	}

	SCENE_LOG( TXT( "Started looking for safe dialog placement [dialog = %s]%s" ), dialogSetInstance->GetName().AsChar(), isInterior ? TXT(" in interior") : TXT("") );

	// Get scene bounds (with final rotation but no translation; translation to be determined)

	EngineTransform scenePlayerRotation = initialScenePlacement;
	scenePlayerRotation.SetPosition( Vector::ZEROS );

	EngineTransform scenePlayerTranslation = initialScenePlacement;
	scenePlayerTranslation.SetRotation( EulerAngles::ZEROS );

	Vector sceneCenter;
	Box sceneBox;
	TDynArray< Vector > sceneConvex;

	SStorySceneDialogsetInstanceCalcBoundDesc desc;
	desc.m_dialogSetInstance = dialogSetInstance;
	desc.m_parentTransform = scenePlayerRotation;
	desc.m_safeZone = dialogSetInstance->GetSafePlacementRadius();
	desc.m_includeCameras = dialogSetInstance->AreCamerasUsedForBoundsCalculation();
	if ( !DetermineSceneBounds( desc, sceneCenter, sceneBox, sceneConvex ) )
	{
		SCENE_LOG( TXT( "Stopped looking for safe dialog placement, reason: failed to determine convex bounds of the scene [dialog = %s]" ), dialogSetInstance->GetName().AsChar() );
		return false;
	}

	// Get dialog set positions (with final rotation but no translation; translation to be determined)

	TDynArray< Vector > dialogSetPositions;
	dialogSetPositions.Reserve( dialogSetInstance->GetSlots().Size() );
	for ( const CStorySceneDialogsetSlot* slot : dialogSetInstance->GetSlots() )
	{
		const Vector rotatedSlotPosition = scenePlayerRotation.TransformPoint( slot->GetSlotPlacement().GetPosition() );
		dialogSetPositions.PushBack( rotatedSlotPosition );
	}

	// Check initial/default placement

	Bool isAcceptableAsFallbackOnly = false;
	if ( IsDialogSetPlacementSafe( scenePlayerTranslation, sceneConvex, sceneBox, dialogSetPositions, sceneActors, isInterior, true, isAcceptableAsFallbackOnly ) )
	{
		placementTransform = initialScenePlacement; // We're good - no need to search for safe placement
		SCENE_LOG( TXT( "Finished looking for safe dialog placement - initial placement was OK [dialog = %s]" ), dialogSetInstance->GetName().AsChar() );
		return true;
	}

	SCENE_LOG( TXT( "Initial dialog placement not safe - continuing search... [dialog = %s]" ), dialogSetInstance->GetName().AsChar() );

	// Enable more granular space search for interiors

	const Float searchTimeLimit = 0.333f;
	const Float distanceCheckStep = isInterior ? 0.2f : 1.0f;
	const Float halfDistanceCheckStepInv = 0.5f / distanceCheckStep;
	const Float initialCheckDistance = isInterior ? 0.2f : 1.0f;

	// Check locations around initial/default placement

	Bool foundFallbackPlacement = false;
	Float fallbackPlacementDistance = 0.0f;
	EngineTransform fallbackPlacementTransform;

	Float distance = initialCheckDistance;
	CTimeCounter timer;
	Uint32 numTries = 0;
	while ( timer.GetTimePeriod() < searchTimeLimit )
	{
		// Check points around circle

		const Float circumference = 2.0f * M_PI * distance;
		const Uint32 numAngleSteps = Max( ( Uint32 ) 4, ( Uint32 ) ( circumference * halfDistanceCheckStepInv ) );

		const Float angleStep = 2.0f * M_PI / numAngleSteps;
		Float angle = 0.0f;

		for ( Uint32 i = 0; ( i < numAngleSteps && timer.GetTimePeriod() < searchTimeLimit ); ++i, ++numTries, angle += angleStep )
		{
			// Set up placement transform

			const Vector offset( Red::Math::MSin( angle ) * distance, Red::Math::MCos( angle ) * distance, 0.0f );
			EngineTransform offsetScenePlayerTransform;
			offsetScenePlayerTransform.SetPosition( scenePlayerTranslation.GetPosition() + offset );

			// Check placement

			if ( IsDialogSetPlacementSafe( offsetScenePlayerTransform, sceneConvex, sceneBox, dialogSetPositions, sceneActors, isInterior, true, isAcceptableAsFallbackOnly ) )
			{
				if ( isAcceptableAsFallbackOnly )
				{
					foundFallbackPlacement = true;
					fallbackPlacementDistance = distance;
					fallbackPlacementTransform.SetPosition( offsetScenePlayerTransform.GetPosition() );
					fallbackPlacementTransform.SetRotation( scenePlayerRotation.GetRotation() );
					continue;
				}
				else
				{
					placementTransform.SetPosition( offsetScenePlayerTransform.GetPosition() );
					placementTransform.SetRotation( scenePlayerRotation.GetRotation() );

					const Float timeMS = ( Float ) timer.GetTimePeriodMS();
					SCENE_LOG( TXT( "Finished looking for safe dialog placement - found safe placement at (%f, %f) offset = (%f, %f) and distance = %f in %f ms after %d tries [dialog = %s]" ),
						offsetScenePlayerTransform.GetPosition().X, offsetScenePlayerTransform.GetPosition().Y,
						offset.X, offset.Y,
						distance,
						timeMS,
						numTries,
						dialogSetInstance->GetName().AsChar() );
					return true;
				}
			}

			// Fallback placement check - only check slot positions, not the whole convex

			if ( !foundFallbackPlacement && IsDialogSetPlacementSafe( offsetScenePlayerTransform, sceneConvex, sceneBox, dialogSetPositions, sceneActors, isInterior, false, isAcceptableAsFallbackOnly ) )
			{
				foundFallbackPlacement = true;
				fallbackPlacementDistance = distance;
				fallbackPlacementTransform.SetPosition( offsetScenePlayerTransform.GetPosition() );
				fallbackPlacementTransform.SetRotation( scenePlayerRotation.GetRotation() );
			}
		}

		// Move further away by fixed distance

		distance += distanceCheckStep;
	}

	// Check if we have a fallback

	if ( foundFallbackPlacement )
	{
		placementTransform.SetPosition( fallbackPlacementTransform.GetPosition() );
		placementTransform.SetRotation( fallbackPlacementTransform.GetRotation() );

		const Float timeMS = ( Float ) timer.GetTimePeriodMS();
		SCENE_LOG( TXT( "Finished looking for safe dialog placement - found safe fallback placement at (%f, %f) and distance = %f in %f ms after %d tries [dialog = %s]" ),
			placementTransform.GetPosition().X, placementTransform.GetPosition().Y,
			fallbackPlacementDistance,
			timeMS,
			numTries,
			dialogSetInstance->GetName().AsChar() );
		return true;
	}

	// Failed to find safe placement

	SCENE_LOG( TXT( "Stopped looking for safe dialog placement, reason: exceeded time limit of %f after %d tries [dialog = %s]" ), searchTimeLimit, numTries, dialogSetInstance->GetName().AsChar() );
	return false;
}

Bool CStoryScenePlayer::GetReferencePlacement( const CStorySceneDialogsetInstance* dialogSetInstance, EngineTransform& transform ) const
{
	transform.Identity();

	if ( dialogSetInstance->GetPlacementTag().Empty() )
	{
		return false;
	}

	// Find existing or determine reference placement for a particular dialog set instance

	if ( const EngineTransform* p = m_referencePlacements.FindPtr( dialogSetInstance ) )
	{
		transform = *p;
		return true;
	}
	else
	{
		// Get tagged nodes
		
		CWorld* world = GetLayer()->GetWorld();
		TDynArray< CNode* > taggedNodes;

		const TagList& tags = dialogSetInstance->GetPlacementTag();
		CName placementTag = m_internalState.m_input->GetDialogsetPlacementTag();
		if ( placementTag && tags.HasTag( placementTag ) )
		{
			world->GetTagManager()->CollectTaggedNodes( placementTag, taggedNodes, BCTO_MatchAll );
		}
		else
		{
			world->GetTagManager()->CollectTaggedNodes( tags, taggedNodes, BCTO_MatchAll );
		}

		SCENE_ASSERT( taggedNodes.Size() <= 1 );
		if ( IsSceneInGame() )
		{
			SCENE_ASSERT( taggedNodes.Size() > 0 );
		}

		// Determine default (unsafe) placement (default to player transform; attempt to use tagged node entities' transform)

		EngineTransform placementTransform;

		CEntity* playerEntity = GGame->GetPlayerEntity();
		if ( playerEntity )
		{
			placementTransform.Init( playerEntity->GetLocalToWorld() );
		}

		Float minDist = NumericLimits<Float>::Max();
		for( auto node : taggedNodes )
		{
			const Float distance = playerEntity ? playerEntity->GetWorldPositionRef().DistanceSquaredTo( node->GetWorldPositionRef() ) : 0.0f;
			if ( distance < minDist )
			{
				minDist = distance;
				placementTransform.Init( node->GetLocalToWorld() );
			}
		}

		m_referencePlacements.Insert( dialogSetInstance, placementTransform );

		// Check if safe placement search is required

		Bool doSearchSafePlacement = IsInGame() && dialogSetInstance->GetFindSafePlacement();
		if ( doSearchSafePlacement )
		{
			// Is this dialog(set)-around-actor?

			for ( CNode* taggedNode : taggedNodes )
			{
				if ( !taggedNode->IsA< CActor >() )
				{
					doSearchSafePlacement = false;
					break;
				}
			}
		}

		// If this is dialog(set)-around-actor, then find safe spot for it

		EngineTransform safePlacementTransform;
		if ( doSearchSafePlacement && FindSafeDialogSetPlacement( dialogSetInstance, placementTransform, taggedNodes, safePlacementTransform ) ) 
		{
			m_referencePlacements[ dialogSetInstance ] = safePlacementTransform;
		}

		transform = m_referencePlacements[ dialogSetInstance ];
		return true;
	}

	return false;
}

Bool CStoryScenePlayer::CanUseDefaultScenePlacement() const
{
	SCENE_ASSERT( GGame );
	SCENE_ASSERT( GGame->IsActive() );

	return true;
}

EngineTransform CStoryScenePlayer::InformAndGetDefaultScenePlacement() const
{
	SCENE_ASSERT( CanUseDefaultScenePlacement() );
	SCENE_ASSERT( GGame );
	SCENE_ASSERT( GGame->IsActive() );

	// TODO
	// Write debug info

	return GGame && GGame->IsActive() && GGame->GetPlayerEntity() ? GGame->GetPlayerEntity()->GetTransform() : EngineTransform();
}

void CStoryScenePlayer::InformAboutScenePlacement( const TDynArray< CNode* >& taggedNodes ) const
{
	// Write debug info if size > 1
}

const CStorySceneEvent* CStoryScenePlayer::FindEventByGUID( const CGUID& id, Int32* outIndex ) const
{
	if ( m_internalState.m_currentSection )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		SCENE_ASSERT( plan );

		if ( plan )
		{
			const CStorySceneSectionPlayingPlan::InstanceData& instance = plan->m_sectionInstanceData;

			const Uint32 size = instance.m_evts.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				const CStorySceneEvent* e = instance.m_evts[ i ];
				SCENE_ASSERT( e );

				if ( e && e->GetGUID() == id )
				{
					if ( outIndex )
					{
						*outIndex = (Int32)i;
					}

					return e;
				}
			}
		}
	}

	return nullptr;
}

const CStorySceneEvent* CStoryScenePlayer::FindEventByIndex( Int32 index ) const
{
	if ( m_internalState.m_currentSection )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		SCENE_ASSERT( plan );

		if ( plan )
		{
			const CStorySceneSectionPlayingPlan::InstanceData& instance = plan->m_sectionInstanceData;

			if ( index < instance.m_evts.SizeInt() )
			{
				return instance.m_evts[ index ];
			}
		}
	}

	return nullptr;
}

const CStorySceneDialogsetInstance* CStoryScenePlayer::GetDialogsetForEvent( const CStorySceneEvent* e ) const
{
	return m_sceneDirector.GetCurrentSettings();
}

void CStoryScenePlayer::GetEventAbsTime( const CStorySceneEvent* e, Float& time, Float& duration ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	if ( m_internalState.m_planId != -1 )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		SCENE_ASSERT( e->GetSceneElement()->GetSection() == plan->GetSection() )
		if ( plan )
		{
			for ( Uint32 i = 0, numElements = plan->GetNumElements(); i < numElements; ++i )
			{
				if ( plan->m_sectionInstanceData.m_eventData[ i ].m_first->GetElement() == e->GetSceneElement() )
				{
					const Float elementStartTime = plan->m_sectionInstanceData.m_elemData[ i ]->GetStartTime();
					const Float elementDur = plan->m_sectionInstanceData.m_elemData[ i ]->GetDuration();
					time = elementStartTime + elementDur * e->GetStartPosition();
					duration = e->GetInstanceDuration( *plan->m_sectionInstanceData.m_data );

					return;
				}
			}
		}
	}

	time = 0.f;
	duration = 1.f;

	SCENE_ASSERT( 0 );
}

Float CStoryScenePlayer::GetEventScalingFactor( const CStorySceneEvent& e ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan ); // this should not fire, right?
	SCENE_ASSERT( e.GetSceneElement()->GetSection() == plan->GetSection() )

	return e.GetInstanceScalingFactor( *plan->m_sectionInstanceData.m_data );
}

Float CStoryScenePlayer::GetEventDuration( const CStorySceneEvent& e ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan ); // this should not fire, right?
	SCENE_ASSERT( e.GetSceneElement()->GetSection() == plan->GetSection() )
	
	return e.GetInstanceDuration( *plan->m_sectionInstanceData.m_data );
}

void CStoryScenePlayer::SetEventDuration( const CStorySceneEvent& e, Float duration ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan ); // this should not fire, right?
	SCENE_ASSERT( e.GetSceneElement()->GetSection() == plan->GetSection() )

	e.SetInstanceDuration( *plan->m_sectionInstanceData.m_data, duration );
}

Float CStoryScenePlayer::GetEventStartTime( const CStorySceneEvent& e ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan ); // this should not fire, right?
	SCENE_ASSERT( e.GetSceneElement()->GetSection() == plan->GetSection() )

	return e.GetInstanceStartTime( *plan->m_sectionInstanceData.m_data );
}

void CStoryScenePlayer::SetEventStartTime( const CStorySceneEvent& e, Float startTime ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
	SCENE_ASSERT( plan ); // this should not fire, right?
	SCENE_ASSERT( e.GetSceneElement()->GetSection() == plan->GetSection() )

	return e.SetInstanceStartTime( *plan->m_sectionInstanceData.m_data, startTime );
}

const CStorySceneLinkElement* CStoryScenePlayer::GetSectionNextElement( const CStorySceneLinkElement* sectionElement ) const
{
	if ( sectionElement->IsA< CStorySceneSection >() )
	{
		const CStorySceneSection* section = static_cast< const CStorySceneSection* >( sectionElement );
		return GetSectionNextElement( section );
	}
	else
	{
		return NULL;
	}
}

const CStorySceneLinkElement* CStoryScenePlayer::GetSectionNextElement( const CStorySceneSection* section ) const
{
	SCENE_ASSERT( section == m_internalState.m_currentSection );

	if ( section != m_internalState.m_currentSection )
	{
		return section->GetNextElement();
	}

	if ( m_internalState.m_sectionInputNumber >= 0 )
	{
		const Uint32 size = m_internalState.m_currentSection->GetInputPathLinks().Size();

		SCENE_ASSERT( size > (Uint32)m_internalState.m_sectionInputNumber );

		if ( size > (Uint32)m_internalState.m_sectionInputNumber )
		{
			Uint32 index = (Uint32)m_internalState.m_sectionInputNumber;
			return m_internalState.m_currentSection->GetInputPathLinkElement( index );
		}
	}

	SCENE_ASSERT( m_internalState.m_sectionInputNumber == -1 );

	return m_internalState.m_currentSection->GetNextElement();
}

const CStorySceneInput* CStoryScenePlayer::HACK_GetInput() const
{
	return m_internalState.m_input;
}

#ifndef RED_FINAL_BUILD
void CStoryScenePlayer::SetDebugScreenText(const String & arg) const
{
	GCommonGame->GetSystem< CStorySceneSystem >()->SetDebugScreenText(arg);
}
void CStoryScenePlayer::AddDebugScreenText(const String & arg) const
{
	GCommonGame->GetSystem< CStorySceneSystem >()->AddDebugScreenText(arg);
}
void CStoryScenePlayer::ClearDebugScreenText() const
{
	GCommonGame->GetSystem< CStorySceneSystem >()->ClearDebugScreenText();
}
#endif

namespace
{
	Bool HasSactionFadeEvent( const CStorySceneSection* s, CStorySceneSectionVariantId variantId )
	{
		const CStorySceneElement* firstElem = s->GetNumberOfElements() > 0 ? s->GetElement( 0 ) : nullptr;

		for ( CGUID evGuid : s->GetEvents( variantId ) )
		{
			const CStorySceneEvent* e = s->GetEvent( evGuid );
			if ( e->GetSceneElement() == firstElem && e->GetStartPosition() < 0.01f && e->GetClass()->IsA< CStorySceneEventFade >() )
			{
				const CStorySceneEventFade* fadeEvt = static_cast< const CStorySceneEventFade* >( e );
				if ( !fadeEvt->IsFadeIn() )
				{
					return true;
				}
			}
		}

		return false;
	}
}

Bool CStoryScenePlayer::ShouldFadeOutBetweenSections( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection, Float& duration ) const
{
#if 1
	/*if ( m_blackscreenSet )
	{
		return false;
	}
	else */if ( prevSection &&
		 !nextSection &&
		 m_internalState.m_activatedOutput && m_internalState.m_activatedOutput->ShouldEndWithBlackscreen() )
	{
		SCENE_LOG( TXT( "%ls - Fading out because of finishing last section[%ls endsWithBlackscreen == true )]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), prevSection->GetName().AsChar() );
		duration = 0.f;
		return true;
	}
	else if ( IsSceneInGame() && !prevSection && nextSection && !nextSection->IsGameplay() && !HasSactionFadeEvent( nextSection, nextSection->GetVariantUsedByLocale( SLocalizationManager::GetInstance().GetCurrentLocaleId() ) ) )
	{
		SCENE_LOG( TXT( "%ls - Fading out because of setting first section" ), m_sceneController->GetInput()->GetFriendlyName().AsChar() );
		duration = 0.2f;
		return true;
	}
	return false;
#else
	SCENE_ASSERT( prevSection != NULL || nextSection != NULL );
	if ( prevSection == NULL && nextSection == NULL )
	{
		return false;
	}
	Bool shouldFadeOut = false;
	if ( prevSection == NULL )
	{
		shouldFadeOut = ( nextSection->IsGameplay() == false && nextSection->ShouldFadeInOnBeginning() == true );
		if ( shouldFadeOut == true )
		{
			SCENE_LOG( TXT( "%ls - Fading out because of setting first section [%ls: IsGameplay() == false && ShouldFadeInOnBegginning() == true]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), nextSection->GetName().AsChar() );
		}
	}
	else if ( nextSection == NULL )
	{
		Bool sceneEndsWithBlackscreen = ( m_internalState.m_activatedOutput != NULL ) ? m_internalState.m_activatedOutput->ShouldEndWithBlackscreen() : false;
		shouldFadeOut = ( prevSection->IsGameplay() == false && prevSection->ShouldFadeOutOnEnd() == true ) || ( prevSection->HasFadeOut() == false && sceneEndsWithBlackscreen == true );

		if ( shouldFadeOut == true )
		{
			SCENE_LOG( TXT( "%ls - Fading out because of finishing last section[%ls : (IsGameplay() == false && ShouldFadeOutOnEnd() == true ) || ( HasFadeOut() == false && sceneEndsWithBlackscreen == true )]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), prevSection->GetName().AsChar() );
		}
	}
	else if ( prevSection->IsGameplay() == false && nextSection->IsGameplay() == false )
	{
		if ( prevSection->ShouldFadeOutOnEnd() == false || nextSection->ShouldFadeInOnBeginning() == false )
		{
			return false;
		}
		shouldFadeOut = m_sceneDirector.IsContinousTransitionPossible( nextSection->GetDialogsetChange() ) == false;	

		if ( shouldFadeOut == true )
		{
			SCENE_LOG
			(
				TXT( "%ls - Fading out between two non-gameplay sections where smooth dialogset change is not possible [%ls:%ls -> %ls:%ls]" ),
				m_sceneController->GetInput()->GetFriendlyName().AsChar(),
				prevSection->GetName().AsChar(),
				( prevSection->GetFirstDialogsetInstance() != NULL ) ? prevSection->GetFirstDialogsetInstance()->GetName().AsString().AsChar() : TXT( "No dialogset" ),
				nextSection->GetName().AsChar(),
				nextSection->GetDialogsetChange().AsString().AsChar()
			);

		}
	}
	else if ( prevSection->IsGameplay() == true && nextSection->IsGameplay() == true )
	{
		return false;
	}
	else
	{
		shouldFadeOut = ( prevSection->ShouldFadeOutOnEnd() && nextSection->ShouldFadeInOnBeginning() );
		if ( shouldFadeOut == true )
		{
			SCENE_LOG( TXT( "%ls - Fading out between two gameplay sections with explicit settings [%ls:ShouldFadeOutEnd() == true -> %ls:ShouldFadeInOnBeginning() == true]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), prevSection->GetName().AsChar(), nextSection->GetName().AsChar() );
		}
	}

	return shouldFadeOut;
#endif
}

Bool CStoryScenePlayer::ShouldFadeInBetweenSections( const CStorySceneSection* prevSection, const CStorySceneSection* nextSection ) const
{
	Bool ret( false );

	if ( m_blackscreenSet )
	{
		if ( nextSection == nullptr && m_internalState.m_activatedOutput && m_internalState.m_activatedOutput->ShouldEndWithBlackscreen() )
		{
			ret = false;
		}
		else if ( prevSection && prevSection->IsGameplay() )
		{
			ret = false;
		}
		else if ( nextSection && ( nextSection->IsGameplay() || nextSection->ManualFadeIn() ) )
		{
			ret = false;
		}
		else if ( !prevSection && !nextSection )
		{
			ret = false;
		}
		else
		{
			ret = true;
		}
	}

	return ret;

#if 0
	if ( prevSection == NULL && nextSection == NULL )
	{
		return false;
	}

	Bool shouldFadeIn = true;

	if ( prevSection == NULL )
	{
		// First section
		shouldFadeIn = nextSection->IsGameplay() == false && nextSection->HasFadeIn() == false;
		if ( shouldFadeIn == false )
		{
			SCENE_LOG( TXT( "%ls - NOT Fading in with first section [%ls: IsGameplay() == true || HasFadeIn() == true]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), nextSection->GetName().AsChar() );
		}
	}
	else if ( nextSection == NULL )
	{
		// Last section
		Bool sceneEndsWithBlackscreen = ( m_internalState.m_activatedOutput != NULL ) ? m_internalState.m_activatedOutput->ShouldEndWithBlackscreen() : false;
		
		if ( prevSection->HasFadeOut() == true && sceneEndsWithBlackscreen == false )
		{
			return true;
		}
		shouldFadeIn = prevSection->IsGameplay() == false && sceneEndsWithBlackscreen == false ;
		if ( shouldFadeIn == false )
		{
			SCENE_LOG( TXT( "%ls - NOT Fading in after last section because of settings [%ls: IsGameplay() == true || ShouldEndWithBlackscreen() == true]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), prevSection->GetName().AsChar() );
		}
	}
	else if ( nextSection->IsGameplay() == true && prevSection->IsGameplay() == true )
	{
		SCENE_LOG( TXT( "%ls - NOT Fading in between to gameplay sections [%ls -> %ls]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), prevSection->GetName().AsChar(), nextSection->GetName().AsChar() );
		return false;
	}
	else if ( nextSection != NULL )
	{
		shouldFadeIn = nextSection->HasFadeIn() == false;
		if ( shouldFadeIn == false )
		{
			SCENE_LOG( TXT( "%ls - NOT Fading in before next section because of explicit setting [%ls: HasFadeIn() == true]" ), m_sceneController->GetInput()->GetFriendlyName().AsChar(), nextSection->GetName().AsChar() );
		}
	}
		
	return shouldFadeIn;
#endif
}

CWorld* CStoryScenePlayer::GetSceneWorld() const
{
	return m_world;
}

CCamera* CStoryScenePlayer::GetSceneCamera() const
{
	if( !GCommonGame )
	{
		return NULL;
	}

	// this may be null when closing the aplication.
	CStorySceneSystem* sceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();	
	return ( sceneSystem != NULL ) ? sceneSystem->GetSceneCamera() : NULL;
}

Bool CStoryScenePlayer::IsInGameplay() const
{
	return m_internalState.m_currentSection ? m_internalState.m_currentSection->IsGameplay() : false;
}

const CStoryScene* CStoryScenePlayer::GetCurrentStoryScene() const 
{ 
	if ( const CStorySceneSection* s = GetCurrentSection() )
	{
		SCENE_ASSERT( s->GetScene() );
		if ( s->GetScene() )
		{
			return s->GetScene();
		}
	}

	SCENE_ASSERT__FIXME_LATER( 0 );

	return m_storyScene;
}

CStoryScene* CStoryScenePlayer::GetCurrentStoryScene_Unsafe()
{
	return const_cast< CStoryScene* >( static_cast< const CStoryScenePlayer* >( this )->GetCurrentStoryScene() );
}

CStorySceneSectionPlayingPlan* CStoryScenePlayer::GetSectionPlayingPlan( const CStorySceneSection* section )
{
	return m_sectionLoader.GetPlayingPlan( section );
}

const CStorySceneSectionPlayingPlan* CStoryScenePlayer::GetSectionPlayingPlan( const CStorySceneSection* section ) const
{
	return m_sectionLoader.GetPlayingPlan( section );
}

void CStoryScenePlayer::RequestSectionPlayingPlan_StartFlow( const CStorySceneSection* requestedSection, CName& outDialogSet )
{
	SCENE_ASSERT( !IsInGame() );

	outDialogSet = requestedSection->GetDialogsetChange();

	OnRequestSectionPlayingPlan_StartFlow( requestedSection, outDialogSet );

	const CStorySceneDialogsetInstance* dialogset = outDialogSet != CName::NONE ? requestedSection->GetScene()->GetDialogsetByName( outDialogSet ) : nullptr;

	m_sectionLoader.RequestPlayingPlan( requestedSection, this, false, dialogset );

	SCENE_ASSERT( m_sectionLoader.HasPlanRequested( requestedSection ) );
}

void CStoryScenePlayer::RequestSectionPlayingPlan_ContinueFlow( const CStorySceneSection* requestedSection )
{
	const CStorySceneDialogsetInstance* dialogset( nullptr );

	if ( !requestedSection->IsGameplay() )
	{
		dialogset = requestedSection->GetDialogsetChange() != CName::NONE ? requestedSection->GetScene()->GetDialogsetByName( requestedSection->GetDialogsetChange() ) : m_sceneDirector.GetCurrentSettings();
		SCENE_ASSERT__FIXME_LATER( dialogset );
	}

	m_sectionLoader.RequestPlayingPlan( requestedSection, this, false, dialogset );

	SCENE_ASSERT( m_sectionLoader.HasPlanRequested( requestedSection ) );
}

Bool CStoryScenePlayer::HasSceneFinished()
{
	return IsSectionChanging() == false || m_internalState.m_pendingNextSection == NULL;
}

EngineTransform CStoryScenePlayer_GetAbsoluteTransform( CNode* storyScenePlayerAsNode )
{
	CStoryScenePlayer* storyScenePlayer = Cast< CStoryScenePlayer >( storyScenePlayerAsNode );
	ASSERT( storyScenePlayer );
	return storyScenePlayer->GetSceneDirector()->GetCurrentScenePlacement();
}

void CStoryScenePlayer::TogglePlayerMovement( Bool flag )
{ 
	CPlayer* witcher = GCommonGame->GetPlayer();
	if ( witcher != NULL )
	{
		witcher->SetPlayerMovable( flag, true );
		if( flag )
		{
			witcher->CallEvent< THandle< CStorySceneOutput > >( CNAME( OnBlockingSceneEnded ), m_internalState.m_activatedOutput );
			
		}
		else
		{
			witcher->CallEvent( CNAME( OnBlockingScenePrepare ) );
		}
	}
}

void CStoryScenePlayer::SetSceneCameraMovable( Bool flag )
{ 
	GCommonGame->GetSystem< CStorySceneSystem >()->SetSceneCameraMovable( flag ); 
}

void CStoryScenePlayer::OnActorAppearanceChanged( const CActor* actor )
{
	const CName& actorId = actor->GetVoiceTag();
	if ( actorId )
	{
		m_eventsExecutor.OnAppearanceChanged( this, actorId );
	}
}

void CStoryScenePlayer::OnCutscenePlayerStarted( const CCutsceneInstance* cs )
{
	// DIALOG_TOMSIN_TODO - Remove all this crap

	GetSettingController()->ApplyNearAndFarPlanes( cs->GetCamera() );

	// Collect all actors participating in scene, the spawned ones and the already existing
	TDynArray< CEntity* > entitiesInCutScene;
	cs->GetActors( entitiesInCutScene );

	Bool clearActorsHands = true;

	// Drey: Looks like cutscene player can be assigned to the first cutscene section in graph, need to query current section then
	const CStorySceneCutsceneSection* currentSection = Cast< CStorySceneCutsceneSection >( GetCurrentSection() );
	ASSERT( currentSection && "CStorySceneCutscenePlayer::OnStart called while no CStorySceneCutsceneSection is active" );

	if ( currentSection )
	{
		clearActorsHands = currentSection->GetClearActorsHands();
	}

	if ( clearActorsHands )
	{
		for ( Uint32 i=0; i<entitiesInCutScene.Size(); ++i )
		{
			CEntity* entity = entitiesInCutScene[i];
			CActor* actor = Cast< CActor >( entity );
			if ( actor )
			{
				SItemEntityManager::GetInstance().CancelLatentActionsForActor( actor, CNAME( l_weapon ) );
				SItemEntityManager::GetInstance().CancelLatentActionsForActor( actor, CNAME( r_weapon ) );
				actor->IssueRequiredItems( SActorRequiredItems( CName::NONE, CName::NONE ), true );
				ITEM_LOG( TXT("Cutscene: Clearing hands of %s"), actor->GetFriendlyName().AsChar() );
			}
		}
	}
}

void CStoryScenePlayer::OnCutscenePlayerEnded( const CCutsceneInstance* cs )
{
	// DIALOG_TOMSIN_TODO - Remove all this crap

	ClearDeniedAreas();

	const CCamera* cutsceneCamera = cs->GetCsCamera();
	if ( cutsceneCamera )
	{
		GetSceneDirector()->SetCameraData( cutsceneCamera );
	}

	// Because of CExtAnimCutsceneFadeEvent
	if ( GGame && GGame->HasBlackscreenRequested() )
	{
		m_blackscreenSet = true;
	}
}

void CStoryScenePlayer::OnNonGameplaySceneStarted()
{ 
	GGame->NonGameplaySceneStarted(); 


	if(GetCurrentSection()->ShouldBlockMusicTriggers() || GetCurrentStoryScene()->ShouldBlockMusicTriggers())
	{
		GSoundSystem->IncrementMusicBlockedCount();
		m_IsBlockingMusic = true;
	}

	if(!m_sceneIsOverridingListener && !GetCurrentStoryScene()->GetListenerOverride().Empty())
	{
		GSoundSystem->PushListenerOverride(GetCurrentStoryScene()->GetListenerOverride());
		m_sceneIsOverridingListener = true;
	}

	if(!m_sectionIsOverridingListener && !GetCurrentSection()->GetListenerOverride().Empty())
	{
		GSoundSystem->PushListenerOverride(GetCurrentSection()->GetListenerOverride());
		m_sectionIsOverridingListener = true;
	}

#ifdef USE_UMBRA
	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		if ( IRenderScene* renderScene = world->GetRenderSceneEx() )
		{
			( new CRenderCommand_SetCutsceneModeForGates( renderScene, true ) )->Commit();
		}
	}
#endif // USE_UMBRA

	// Inform renderer that we are entering cinematic mode
	(new CRenderCommand_ToggleCinematicMode(true))->Commit();

	// disable the fading of trees for the cutscene
	( new CRenderCommand_SetupTreeFading( false ) )->Commit();

	// enable cachets for the story scene
	( new CRenderCommand_SetupCachets( true ) )->Commit();
}

void CStoryScenePlayer::HandleOnSkipSoundEvents()
{


	//TODO: Scene sounds are always played on Geralt; if this is used for future find a more robust way of handling this
	CEntity * geralt = GetMappedActor(CName(TXT("GERALT")));

	CSoundEmitter * soundEmitter = nullptr;
	if(geralt)
	{
		soundEmitter = geralt->GetSoundEmitterComponent();
	}

	if(soundEmitter && GetCurrentSection())
	{
		for(auto soundEvent : GetCurrentSection()->GetSoundEventsOnSkip())
		{
			soundEmitter->SoundEvent(soundEvent.AsAnsiChar());	
		}

		for(auto soundEvent : GetCurrentStoryScene()->GetSoundEventsOnSkip())
		{
			soundEmitter->SoundEvent(soundEvent.AsAnsiChar());	
		}
	}
}

void CStoryScenePlayer::HandleOnEndSoundEvents(const CStorySceneSection *newSection)
{


	//TODO: Scene sounds are always played on Geralt; if this is used for future find a more robust way of handling this
	CEntity * geralt = GetMappedActor(CName(TXT("GERALT")));
	
	CSoundEmitter * soundEmitter = nullptr;
	if(geralt)
	{
		soundEmitter = geralt->GetSoundEmitterComponent();
	}

	if(soundEmitter && GetCurrentSection())
	{
		for(auto soundEvent : GetCurrentSection()->GetSoundEventsOnEnd())
		{
			soundEmitter->SoundEvent(soundEvent.AsAnsiChar());	
		}

		if(!newSection)
		{
			for(auto soundEvent : GetCurrentStoryScene()->GetSoundEventsOnEnd())
			{
				soundEmitter->SoundEvent(soundEvent.AsAnsiChar());	
			}
		}
	}
}

void CStoryScenePlayer::OnNonGameplaySceneEnded(const CStorySceneSection *newSection)
{ 
	GGame->NonGameplaySceneEnded(); 
	  
	if(m_IsBlockingMusic)
	{
		GSoundSystem->DecrementMusicBlockedCount();
		m_IsBlockingMusic = false;
	}

	if(m_sectionIsOverridingListener && (!newSection || (newSection->GetListenerOverride() != GetCurrentSection()->GetListenerOverride())))
	{
		GSoundSystem->PopListenerOverride();
		m_sectionIsOverridingListener = false;
	}

	if(m_sceneIsOverridingListener && (!newSection || newSection->IsGameplay()))
	{
		GSoundSystem->PopListenerOverride();
		m_sceneIsOverridingListener = false;
	}

	HandleOnEndSoundEvents(newSection);	

#ifdef USE_UMBRA
	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		if ( IRenderScene* renderScene = world->GetRenderSceneEx() )
		{
			( new CRenderCommand_SetCutsceneModeForGates( renderScene, false ) )->Commit();
		}
	}
#endif // USE_UMBRA

	// Inform renderer that we are entering cinematic mode
	(new CRenderCommand_ToggleCinematicMode(false))->Commit();

	// Enable trees fading again
	( new CRenderCommand_SetupTreeFading( true ) )->Commit();

	// disable cachets for the story scene
	( new CRenderCommand_SetupCachets( false ) )->Commit();
}

void CStoryScenePlayer::ToggleHud( Bool flag )					
{
	CStorySceneSystem* storySceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
	if ( storySceneSystem )
	{
		storySceneSystem->ToggleDialogHUD( flag ); 
	}
}

Bool CStoryScenePlayer::IsHudReady() const						
{ 
	CStorySceneSystem* storySceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
	if ( storySceneSystem )
	{
		return storySceneSystem->IsDialogHUDAvailable();
	}
	return false;
}

Bool CStoryScenePlayer::IsFadeInProgress() const				
{ 
	return GGame->IsFadeInProgress(); 
}

void CStoryScenePlayer::CreateNoSaveLock( const String& lockReason ) 
{ 
	SGameSessionManager::GetInstance().CreateNoSaveLock( lockReason, m_saveBlockId, false, false ); 
}

void CStoryScenePlayer::ReleaseNoSaveLock()					
{
	SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveBlockId ); 
	m_saveBlockId = CGameSessionManager::GAMESAVELOCK_INVALID;
}

RED_DEFINE_NAME( VideoClient_StoryScenePlayer );

void CStoryScenePlayer::PlayVideo( const SVideoParams& params ) 
{ 
	if ( ( params.m_videoParamFlags & eVideoParamFlag_Preemptive ) )
	{
		for ( Uint32 i = 0; i < m_renderVideos.Size(); ++i )
		{
			m_renderVideos[ i ]->Cancel();
			m_renderVideos[ i ]->Release();
		}
		m_renderVideos.ClearFast();
	}

	IRenderVideo* video = GCommonGame->GetVideoPlayer()->CreateVideo( CNAME( VideoClient_StoryScenePlayer ), params );
	RED_FATAL_ASSERT( video, "Null IRenderVideo in CStoryScenePlayer::PlayVideo" );

	m_renderVideos.PushBack( video );
}

void CStoryScenePlayer::StopVideo()							
{ 
	for ( Uint32 i = 0; i < m_renderVideos.Size(); ++i )
	{
		RED_FATAL_ASSERT( m_renderVideos[ i ], "Null IRenderVideo in CStoryScenePlayer::StopVideo" );

		m_renderVideos[ i ]->Cancel();
		m_renderVideos[ i ]->Release();
	}
	m_renderVideos.ClearFast();
}

Bool CStoryScenePlayer::IsPlayingVideo() const					
{
	if ( m_renderVideos.Size() > 0 )
	{
		RED_FATAL_ASSERT( m_renderVideos[ 0 ], "Null IRenderVideo in CStoryScenePlayer::IsPlayingVideo" );

		if ( !m_renderVideos[ 0 ]->IsValid() )
		{
			m_renderVideos[ 0 ]->Release();
			m_renderVideos.RemoveAt( 0 );
		}
	}

	return m_renderVideos.Size() > 0;
}

Bool CStoryScenePlayer::HasValidVideo() const					
{ 
	if ( m_renderVideos.Size() > 0 )
	{
		RED_FATAL_ASSERT( m_renderVideos[ 0 ], "Null IRenderVideo in CStoryScenePlayer::HasValidVideo" );

		if ( !m_renderVideos[ 0 ]->IsValid() )
		{
			m_renderVideos[ 0 ]->Release();
			m_renderVideos.RemoveAt( 0 );
		}
	}

	return m_renderVideos.Size() > 0;
}

Bool CStoryScenePlayer::GetVideoSubtitles( String& outSubtitles )
{
	for ( Uint32 i = 0; i < m_renderVideos.Size(); ++i )
	{
		RED_FATAL_ASSERT( m_renderVideos[ i ], "Null IRenderVideo in CStoryScenePlayer::GetVideoSubtitles" );

		if ( m_renderVideos[ i ]->IsValid() )
		{
			return m_renderVideos[ i ]->FlushSubtitles( outSubtitles );
		}
	}

	return false;
}

void CStoryScenePlayer::SoundUpdateAmbientPriorities()					
{ 
	GSoundSystem->GetAmbientManager().UpdateAmbientPriorities(); 
}

void CStoryScenePlayer::SoundForceReverb( const String& reverbDefinition )	
{ 
	GSoundSystem->ForceReverb( reverbDefinition ); 
}

void CStoryScenePlayer::DbFactAdded( const String& factName )
{
	m_internalState.m_factsAdded.PushBackUnique( factName );
}

void CStoryScenePlayer::DbFactRemoved( const String& factName )
{
	m_internalState.m_factsRemoved.PushBackUnique( factName );
}

RED_DEFINE_STATIC_NAME( ScenePlayer )

void CStoryScenePlayer::SetCurrentTimeMultiplier( Float multiplier )
{
	m_internalState.m_timeMultiplier = multiplier;
	GCommonGame->SetTimeScale( 1.f/multiplier, CNAME( ScenePlayer ), 9999 );
	
	for( TPair< CName, THandle< CEntity > >& actorIter : m_sceneActorEntities )
	{
		if( CEntity* ent = actorIter.m_second.Get() )
		{
			for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
			{
				(*it)->SetTimeMultiplier( multiplier );
			}		
		}	
	}
	for( TPair< CName, THandle< CEntity > >& propIter : m_scenePropEntities )
	{
		if( CEntity* ent = propIter.m_second.Get() )
		{
			for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
			{
				(*it)->SetTimeMultiplier( multiplier );
			}
		}
	}
}

void CStoryScenePlayer::RemoveTimeMultiplier()
{
	m_internalState.m_timeMultiplier = 1.f;
	GCommonGame->RemoveTimeScale( CNAME( ScenePlayer ) );

	for( TPair< CName, THandle< CEntity > >& actorIter : m_sceneActorEntities )
	{
		if( CEntity* ent = actorIter.m_second.Get() )
		{
			if( CAnimatedComponent* ac = ent->GetRootAnimatedComponent() )
			{
				ac->SetTimeMultiplier( 1.f );
			}
		}	
	}
	for( TPair< CName, THandle< CEntity > >& propIter : m_scenePropEntities )
	{
		if( CEntity* ent = propIter.m_second.Get() )
		{
			if( CAnimatedComponent* ac = ent->GetRootAnimatedComponent() )
			{
				ac->SetTimeMultiplier( 1.f );
			}
		}
	}
}

void CStoryScenePlayer::RegisterSpawnedItems( TDynArray< TPair< CName, SItemUniqueId > >& spawnedItems )
{
	m_eventsExecutor.RegisterSpawnedItems( spawnedItems );
}

void CStoryScenePlayer::RegisterSpawnedItem( TPair< CName, SItemUniqueId > spawnedItem )
{
	m_eventsExecutor.RegisterSpawnedItem( spawnedItem );
}

void CStoryScenePlayer::HideNonSceneActor( CActor* actorToHide )
{
	for ( THandle< CActor >& handle : m_hiddenNonSceneActors )
	{
		if ( CActor* actor = handle.Get() )
		{
			if ( actor == actorToHide )
			{
				return;
			}
		}
	}

	//Dont hide player horse when he is on the saddle
	const TList< IAttachment* >& attachments = actorToHide->GetChildAttachments();
	for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
	{
		CHardAttachment* ht = ( *it )->ToHardAttachment();
		if ( ht )
		{
			if( Cast< CPlayer >( ht->GetChild()	) )
			{
				return;
			}
		}
	}


	m_hiddenNonSceneActors.PushBack( actorToHide );

	actorToHide->SetHideInGame( true, false, CEntity::HR_SceneArea );
}

void CStoryScenePlayer::UnhideAllNonSceneActors()
{
	for ( THandle< CActor >& handle : m_hiddenNonSceneActors )
	{
		if ( CActor* actor = handle.Get() )
		{
			actor->SetHideInGame( false, false, CEntity::HR_SceneArea );
		}
	}
	m_hiddenNonSceneActors.ClearFast();
}

void CStoryScenePlayer::CollectPropEntitiesForStream(IRenderTextureStreamRequest* textureStreamRequest) const
{
	for ( auto it = m_scenePropEntities.Begin(), end = m_scenePropEntities.End(); it != end; ++it )
	{
		if( CEntity* ent = it->m_second )
		{
			textureStreamRequest->AddEntity( ent );
		}
	}	
}

void CStoryScenePlayer::OnLineSkipped()
{
	HandleOnSkipSoundEvents();
}

void CStoryScenePlayer::OnCameraCut()
{
	if( CWorld* world = GetLayer()->GetWorld() )
	{
		world->GetCameraDirector()->InvalidateLastFrameCamera();
		world->FinishRenderingFades();
	}	
}


// --------------------------------------------------------------
// ---------------------- scripting support ---------------------
// --------------------------------------------------------------

void CStoryScenePlayer::funcRestartSection( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	m_internalState.m_requestSectionRestart = true;
}

void CStoryScenePlayer::funcRestartScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CFactsDB* factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;
	if ( IsSceneInGame() && factsDB )
	{
		for( const String& str : m_internalState.m_factsAdded )
		{
			factsDB->RemoveFact( str );
		}		

		for( const String& str : m_internalState.m_factsRemoved )
		{
			const EngineTime& time = GGame->GetEngineTime();
			factsDB->AddFact( str, 1, time, CFactsDB::EXP_ACT_END );
		}			
	}
	m_internalState.m_factsAdded.ClearFast();
	m_internalState.m_requestSceneRestart = true;
}

void CStoryScenePlayer::funcGetSceneWorldPos( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Vector pos = m_sceneDirector.GetCurrentScenePlacement().GetPosition();
	RETURN_STRUCT( Vector, pos );
}

void CStoryScenePlayer::funcDbFactAdded( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, factName, String::EMPTY );
	FINISH_PARAMETERS;
	
	DbFactAdded( factName );
}

void CStoryScenePlayer::funcDbFactRemoved( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, factName, String::EMPTY );
	FINISH_PARAMETERS;

	DbFactRemoved( factName );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
