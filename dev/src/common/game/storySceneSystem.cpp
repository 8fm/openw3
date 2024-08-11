/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "actorsManager.h"

#include "storySceneInput.h"
#include "storySceneControlPartsUtil.h"
#include "storyScenePlayer.h"
#include "storySceneRandomizer.h"
#include "storySceneSystem.h"
#include "storySceneLine.h"
#include "storySceneItems.h"

#include "actorSpeech.h"
#include "actorSpeechQueue.h"
#include "storySceneFlowCondition.h"
#include "sceneLog.h"

#include "interactionsManager.h"

#include "guiManager.h"
#include "storySceneActorMap.h"
#include "storyScenePlayer.h"
#include "StorySceneVideo.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/loadingJobManager.h"
#include "../engine/camera.h"
#include "../engine/cutsceneInstance.h"
#include "../engine/dynamicLayer.h"
#include "../engine/tagManager.h"
#include "../engine/renderFrame.h"
#include "../engine/fonts.h"
#include "questsSystem.h"
#include "../engine/environmentManager.h"
#include "commonGame.h"
#include "communitySystem.h"
#include "../engine/world.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneSystem );

RED_DEFINE_STATIC_NAME( UserInputWeight );

//IMPLEMENT_ENGINE_CLASS( CStorySceneSpawner );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

const Float CStorySceneSystem::SCENE_UPDATE_INTERVAL( 1.f );
const Uint32 CStorySceneSystem::NUM_SCENE_CAMERAS( 1 );
#ifndef RED_FINAL_BUILD
const Float CStorySceneSystem::DEFAULT_DEBUG_TEXT_FADEOUT_TIME(3.f);
#endif

CGatheredResource resSceneCameraTemplate( TXT("gameplay\\camera\\scene_camera.w2ent"), RGF_Startup );
//CGatheredResource resSceneVideo( TXT("gameplay\\globals\\scene_video.csv"), RGF_Startup );

#ifndef RED_FINAL_BUILD
CGatheredResource resScreenTextFont( TXT("gameplay\\gui\\fonts\\aldo_pc_15.w2fnt"), RGF_Startup );
CGatheredResource resVideoTextFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );
#endif

const Uint8	CStorySceneSystem::RANDOMIZER_COOLDOWN = 10;

using namespace StorySceneControlPartUtils;

// =================================================================================================
namespace {
// =================================================================================================

struct SceneControllerPlayingContext
{
	const CStorySceneInput*				m_sceneInput;
	IControllerSetupActorsProvider*		m_contextActorsProvider;
	Bool								m_mustPlay;
	Int32								m_priority;
	Vector								m_scenePosition;
	TDynArray< THandle< CEntity > >		m_choosenActors;
	CWorld*								m_world;
	Bool								m_spawnAllActors;
};

// functions in this namespace
Bool ValidateSceneActor( const THandle< CEntity >& candidate, const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context );

THandle< CEntity > FindSceneActor( const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context, const TDynArray< CEntity* >& candidates );
THandle< CEntity > FindSceneActor( const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context, const TDynArray< THandle< CEntity > >& candidates );

CStorySceneController* CreateSceneController( const CStorySceneSystem& sceneSystem, SceneControllerPlayingContext& context );

Bool ValidateSceneActor( const THandle< CEntity >& candidate, const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context )
{
	const CEntity* candidatePtr = candidate.Get();

	if ( candidatePtr == NULL )
	{
		SCENE_LOG( TXT( "%s - %s - Found empty handle. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar() );
		return false;
	}

	if ( candidatePtr->IsDetaching() == true )
	{
		SCENE_LOG( TXT( "%s - %s - Entity is being detached. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), candidatePtr->GetName().AsChar() );
		return false;
	}

	if ( candidatePtr->IsSpawned() == false )
	{
		SCENE_LOG( TXT( "%s - %s - Entity is not spawned. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), candidatePtr->GetName().AsChar() );
		return false;
	}

	if ( actorDef.m_actorTags.Empty() == false && TagList::MatchAll( actorDef.m_actorTags, candidatePtr->GetTags() ) == false )
	{
		SCENE_LOG( TXT( "%s - %s - Entity %s does not match required tags (%s != %s). Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar(), candidatePtr->GetName().AsChar(), ToString( candidatePtr->GetTags() ).AsChar(), ToString( actorDef.m_actorTags ).AsChar() );
		return false;
	}

	if ( actorDef.m_dontSearchByVoicetag && actorDef.m_actorTags.Empty() )
	{
		SCENE_LOG( TXT( "%s - %s - Entity %s has no tags and flag dontSearchByVoicetag (%s != %s). Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar(), candidatePtr->GetName().AsChar(), ToString( candidatePtr->GetTags() ).AsChar(), ToString( actorDef.m_actorTags ).AsChar() );
		return false;
	}

	if ( const CActor* actorCandidate = Cast< CActor >( candidatePtr ) )
	{
		if ( actorDef.m_dontSearchByVoicetag == false && actorCandidate->GetVoiceTag() != actorDef.m_id )
		{
			SCENE_LOG( TXT( "%s - %s - Entity %s does not match required voicetags (%s != %s). Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar(), candidatePtr->GetName().AsChar(), actorCandidate->GetVoiceTag().AsString().AsChar(), actorDef.m_id.AsString().AsChar() );
			return false;
		}
		//if ( actorCandidate->IsAlive() == false )
		//{
		//	SCENE_LOG( TXT( "%s - %s - Entity %s is not alive. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar(), candidatePtr->GetName().AsChar() );
		//	return false;
		//}
		String message;
		if ( actorCandidate->IsDoingSomethingMoreImportant( context.m_priority, &message ) == true  )
		{
			SCENE_LOG( TXT( "%s - %s - %s. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef.m_id.AsString().AsChar(), message.AsChar() );
			return false;
		}
	}
	else
	{
		return false;
	}

	if ( context.m_choosenActors.Exist( candidate ) == true )
	{
		SCENE_LOG( TXT( "%s - Entity %s already selected by current scene. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), candidatePtr->GetName().AsChar() );
		return false;
	}

	return true;
}

THandle< CEntity > FindSceneActor( const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context, const TDynArray< CEntity* >& candidates )
{
	THandle< CEntity >	selectedEntity;
	Float				minDistanceToDialog = FLT_MAX;

	for ( TDynArray< CEntity* >::const_iterator candidateIter = candidates.Begin(); candidateIter != candidates.End(); ++candidateIter )
	{
		THandle< CEntity > candidate = *candidateIter;

		if ( ValidateSceneActor( candidate, actorDef, context ) == false )
		{
			continue;
		}

		Float distanceToDialog = candidate.Get()->GetWorldPositionRef().DistanceSquaredTo( context.m_scenePosition );
		if ( distanceToDialog < minDistanceToDialog )
		{
			selectedEntity = candidate;
			minDistanceToDialog = distanceToDialog;
		}
	}

	return selectedEntity;
}

THandle< CEntity > FindSceneActor( const CStorySceneActor& actorDef, const SceneControllerPlayingContext& context, const TDynArray< THandle< CEntity > >& candidates )
{
	THandle< CEntity >	selectedEntity;
	Float				minDistanceToDialog = FLT_MAX;

	for ( TDynArray< THandle< CEntity > >::const_iterator candidateIter = candidates.Begin(); candidateIter != candidates.End(); ++candidateIter )
	{
		THandle< CEntity > candidate = *candidateIter;

		if ( ValidateSceneActor( candidate, actorDef, context ) == false )
		{
			continue;
		}

		Float distanceToDialog = candidate.Get()->GetWorldPositionRef().DistanceSquaredTo( context.m_scenePosition );
		if ( distanceToDialog < minDistanceToDialog )
		{
			selectedEntity = candidate;
			minDistanceToDialog = distanceToDialog;
		}
	}

	return selectedEntity;
}

Bool ValidateSceneProp( const CEntity* candidatePtr, CName id, SceneControllerPlayingContext& context )
{
	if ( candidatePtr == NULL )
	{
		SCENE_LOG( TXT( "%s - %s - Found empty handle. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), id.AsString().AsChar() );
		return false;
	}

	if ( candidatePtr->GetName() != id.AsString() )
	{
		return false;
	}

	if ( candidatePtr->IsDetaching() == true )
	{
		SCENE_LOG( TXT( "%s - %s - Entity is being detached. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), candidatePtr->GetName().AsChar() );
		return false;
	}

	if ( candidatePtr->IsSpawned() == false )
	{
		SCENE_LOG( TXT( "%s - %s - Entity is not spawned. Ignoring" ), context.m_sceneInput->GetFriendlyName().AsChar(), candidatePtr->GetName().AsChar() );
		return false;
	}

	return true;
}

THandle< CEntity > FindSceneProp( CName id, const TDynArray< THandle<CEntity> >* props, SceneControllerPlayingContext& context )
{
	THandle< CEntity >	selectedEntity;
	Float minDistanceToDialog = FLT_MAX;
	if( props )
	{
		Uint32 size = props->Size();
		for ( Uint32 i=0; i < size; ++i )
		{
			THandle< CEntity > candidate( ( *props )[ i ] );

			if ( !ValidateSceneProp( candidate, id, context ) )
			{
				continue;
			}

			Float distanceToDialog = candidate.Get()->GetWorldPositionRef().DistanceSquaredTo( context.m_scenePosition );
			if ( distanceToDialog < minDistanceToDialog )
			{
				selectedEntity = candidate;
				minDistanceToDialog = distanceToDialog;
			}
		}
	}

	return selectedEntity;
}


CStorySceneController* CreateSceneController( const CStorySceneSystem& sceneSystem, SceneControllerPlayingContext& context )
{
	SCENE_ASSERT( context.m_sceneInput );
	SCENE_ASSERT( context.m_world );

	if ( !context.m_sceneInput )
	{
		return NULL;
	}

	CTagManager* tagManager = context.m_world->GetTagManager();
	CStoryScene* scene = context.m_sceneInput->GetScene();

	const CStorySceneDialogsetInstance* dialogset = scene->GetDialogsetByName( context.m_sceneInput->GetDialogsetName() );
	if ( dialogset != NULL && tagManager )
	{
		// Find placement node based on dialogset;
		const TagList& scenePlacementTags = dialogset->GetPlacementTag();

		CNode* scenePlacementNode = tagManager->GetTaggedNodeClosestTo( scenePlacementTags, context.m_scenePosition );
		if ( scenePlacementNode != NULL )
		{
			context.m_scenePosition = scenePlacementNode->GetWorldPositionRef();
		}
	}
	SCENE_LOG( TXT( "%s - Scene position is [%s]" ), context.m_sceneInput->GetFriendlyName().AsChar(), ToString( context.m_scenePosition ).AsChar() );

	TDynArray< CStorySceneController::SceneActorInfo > actorInfos;

	TDynArray< TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* > >	requiredActorsData;

	const TDynArray< CStorySceneActor* >& actorDefinitions = scene->GetSceneActorsDefinitions();
	const TDynArray< CStorySceneVoicetagMapping >& actorMappings = context.m_sceneInput->GetVoicetagMappings();

	// Merge data from actor definitions and mappings - they are both needed, unless we refactor scene files data (which will need converter and resave)
	for ( TDynArray< CStorySceneVoicetagMapping >::const_iterator mappingIter = actorMappings.Begin(); mappingIter != actorMappings.End(); ++mappingIter )
	{
		if ( mappingIter->m_voicetag == CName::NONE )
		{
			continue;
		}

		for ( TDynArray< CStorySceneActor* >::const_iterator definitionIter = actorDefinitions.Begin(); definitionIter != actorDefinitions.End(); ++definitionIter )
		{
			const CStorySceneActor* actorDef = (*definitionIter);
			if ( actorDef && actorDef->m_id == mappingIter->m_voicetag )
			{
				requiredActorsData.PushBack( TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* >( actorDef, &(*mappingIter) ) );
				break;
			}
		}
	}

	GCommonGame->GetSystem< CQuestsSystem >()->GetContextDialogActorData( scene, requiredActorsData );

	Uint32 actorsFound = 0;

	for( auto actorDataIter = requiredActorsData.Begin(); actorDataIter != requiredActorsData.End(); ++actorDataIter )
	{
		const CStorySceneActor* actorDef = actorDataIter->m_first;
		const CStorySceneVoicetagMapping* actorMapping = actorDataIter->m_second;

		THandle< CEntity >	selectedEntity;

		// Find in context actors
		SCENE_LOG( TXT( "%s - %s - Looking for actor in context" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef->m_id.AsString().AsChar() );

		Bool forceSpawn = actorDef->m_forceSpawn;

		if( const TDynArray< THandle<CEntity> >* actors = context.m_contextActorsProvider->GetActors() )
		{
			selectedEntity = FindSceneActor( *actorDef, context, *actors );
		}		
		if ( !forceSpawn && !context.m_spawnAllActors && selectedEntity.Get() == NULL && actorMapping->m_mustUseContextActor == false && tagManager )
		{
			// Find actor entity by tag
			SCENE_LOG( TXT( "%s - %s - Searching actor by tag" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef->m_id.AsString().AsChar() );
			TDynArray< CEntity* > candidateEntities;
			tagManager->CollectTaggedEntities( actorDef->m_actorTags, candidateEntities, BCTO_MatchAll );

			selectedEntity = FindSceneActor( *actorDef, context, candidateEntities );
		}

		if ( !forceSpawn && !context.m_spawnAllActors && selectedEntity.Get() == NULL && actorMapping->m_mustUseContextActor == false && actorDef->m_dontSearchByVoicetag == false && sceneSystem.GetActorMap() )
		{
			// Find actor entity by voicetag
			SCENE_LOG( TXT( "%s - %s - Searching actor by voicetag" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef->m_id.AsString().AsChar() );
			TDynArray< THandle< CEntity > > candidateHandles;
			sceneSystem.GetActorMap()->FindSpeakers( actorDef->m_id, candidateHandles );

			selectedEntity = FindSceneActor( *actorDef, context, candidateHandles );
		}

		if ( !forceSpawn && !selectedEntity && !actorDef->m_entityTemplate.IsEmpty() )
		{
			const CName appearanceName = actorDef->m_appearanceFilter.Size() ? actorDef->m_appearanceFilter[ 0 ] : CName::NONE;

			CNewNPC* communityNpc = nullptr;
			if ( GCommonGame->GetSystem< CCommunitySystem >()->GetSpawnedNPC( actorDef->m_entityTemplate, appearanceName, Vector::ZERO_3D_POINT, false, communityNpc ) && communityNpc && !communityNpc->IsInNonGameplayScene() )
			{
				selectedEntity = communityNpc;
			}
		}

		CStorySceneController::SceneActorInfo actorInfo;
		actorInfo.m_voicetag = actorDef->m_id;
		actorInfo.m_actor = selectedEntity;
		actorInfo.m_invulnerable = actorMapping->m_invulnerable;
		actorInfo.m_destroyOnFinish = false;
		actorInfo.m_actorOptional = actorMapping->m_actorOptional;
		actorInfos.PushBack( actorInfo );

		context.m_choosenActors.PushBack( selectedEntity );

		if ( selectedEntity.Get() != NULL )
		{
			SCENE_LOG( TXT( "%s - %s - Actor entity found (%s)" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef->m_id.AsString().AsChar(), selectedEntity.Get()->GetName().AsChar() );
			++actorsFound;
		}
		else
		{
			SCENE_LOG( TXT( "%s - %s - Actor entity has not been found" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorDef->m_id.AsString().AsChar() );
		}
	}

	if ( actorsFound != requiredActorsData.Size() )
	{
		if ( context.m_mustPlay == false )
		{
			SCENE_LOG( TXT( "%s - Only %d out of %d actors found. Scene is not from quest. Scene cannot be played" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorsFound, requiredActorsData.Size() );
			return NULL;
		}
		else
		{
			SCENE_LOG( TXT( "%s - %d out of %d actors found. Scene is from quest. Missing actors should be spawned" ), context.m_sceneInput->GetFriendlyName().AsChar(), actorsFound, requiredActorsData.Size() );
		}
	}
	else
	{
		SCENE_LOG( TXT( "%s - All actors found" ), context.m_sceneInput->GetFriendlyName().AsChar() );
	}

	const TDynArray< CStorySceneProp* >& propDefinitions = scene->GetScenePropDefinitions();
	TDynArray< CStorySceneController::ScenePropInfo > propsInfos;
	for( Uint32 i = 0; i < propDefinitions.Size(); ++i )
	{
		const CStorySceneProp* prop = propDefinitions[ i ];
		if( !prop )
		{
			continue;
		}

		CStorySceneController::ScenePropInfo propInfo;
		propInfo.m_id = prop->m_id;
		propInfo.m_prop = FindSceneProp( prop->m_id, context.m_contextActorsProvider->GetProps(), context );
		propsInfos.PushBack( propInfo );	
	}

	const TDynArray< CStorySceneEffect* >& effectDefinitions = scene->GetSceneEffectDefinitions();
	TDynArray< CStorySceneController::ScenePropInfo > effectInfos;
	for( Uint32 i = 0; i < effectDefinitions.Size(); ++i )
	{
		const CStorySceneEffect* fxDef = effectDefinitions[ i ];
		if( !fxDef )
		{
			continue;
		}

		CStorySceneController::ScenePropInfo propInfo;
		propInfo.m_id = fxDef->m_id;
		propInfo.m_prop = FindSceneProp( fxDef->m_id, context.m_contextActorsProvider->GetEffects(), context );
		effectInfos.PushBack( propInfo );	
	}

	const TDynArray< CStorySceneLight* >& lightDefinitions = scene->GetSceneLightDefinitions();
	TDynArray< CStorySceneController::ScenePropInfo > lightInfos;
	for( Uint32 i = 0; i < lightDefinitions.Size(); ++i )
	{
		const CStorySceneLight* lightDef = lightDefinitions[ i ];
		if( !lightDef )
		{
			continue;
		}

		CStorySceneController::ScenePropInfo propInfo;
		propInfo.m_id = lightDef->m_id;
		propInfo.m_prop = FindSceneProp( lightDef->m_id, context.m_contextActorsProvider->GetLights(), context );
		lightInfos.PushBack( propInfo );
	}

	CStorySceneController* controller = new CStorySceneController( context.m_sceneInput );

	controller->m_mappedActors = actorInfos;
	controller->m_mappedProps = propsInfos;
	controller->m_mappedEffects = effectInfos;
	controller->m_mappedLights = lightInfos;	
	controller->m_priority = (EArbitratorPriorities) context.m_priority;

	return controller;
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

CStorySceneSystem::CStorySceneSystem()
	: m_dialogHud( NULL )
	, m_sceneUpdateTimeout( 0.f )
	, m_highlightedChoice( -1 )
	, m_lastLineActor( NULL )
	, m_lastSkipLineTime( 0 )
	, m_actorMap( NULL )
	, m_sceneDisplay( NULL )
#ifndef RED_FINAL_BUILD
	,m_debugScreenTextFont( NULL )
	,m_debugVideoTextFont( NULL )
#endif //RED_FINAL_BUILD
{
	
#ifndef RED_FINAL_BUILD
	fadingOutDebugText = false;
	debugTextFadeoutTimer = 0.f;
	m_debugScreenTextFont = resScreenTextFont.LoadAndGet< CFont >();
	m_debugVideoTextFont  = resVideoTextFont.LoadAndGet< CFont >();
#endif
}

CStorySceneSystem::~CStorySceneSystem()
{
	RED_FATAL_ASSERT( m_sceneDisplay == nullptr, "Scene display data is not released. Have you called Shutdown()?" );
}

void  CStorySceneSystem::Initialize()
{
	m_sceneDisplay = new CGlobalStorySceneDisplay( this );

	m_animationList.Load();
	InitSceneVideoMap();
}

void CStorySceneSystem::ReloadAnimationsList()
{
	m_animationList.Destroy();
	m_animationList.Load();
}

void CStorySceneSystem::InitSceneVideoMap()
{
	struct Entry
	{
		const Char* m_fileName;
		SSceneVideo m_sceneVideo;
	};

	static const Entry entries[] = {
		{ TXT("cutscenes\\pre_rendered_cutscenes\\cs502_tower_ciri_walks_in_p2.usm"), { SSceneVideo::eWhiteScreen_Start } },
		{ TXT("cutscenes\\pre_rendered_cutscenes\\cs502_tower_ciri_by_light.usm"), { SSceneVideo::eWhiteScreen_End } },
		{ TXT("dlc\\bob\\data\\movies\\cutscenes\\cs702_vision_1.usm"), { SSceneVideo::eWhiteScreen_Start } },
		{ TXT("dlc\\bob\\data\\movies\\cutscenes\\cs702_vision_4.usm"), { SSceneVideo::eWhiteScreen_End } },
	};

	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(entries); ++i )
	{
		const Char* fileName = entries[i].m_fileName;
		const SSceneVideo& sceneVideo = entries[i].m_sceneVideo;
		m_sceneVideoMap.Insert( fileName, sceneVideo );
		SCENE_LOG(TXT("[resSceneVideo] '%ls'"), fileName);
	}

#if 0
	THandle< C2dArray > ar = resSceneVideo.LoadAndGet<C2dArray>();
	if (!ar)
	{
		SCENE_ERROR(TXT("Failed to load resSceneVideo csv!"));
		return;
	}

	const String videoPathHdr(TXT("VideoPath"));
	Uint32 colVideoPath = 0;
	if (!ar->FindHeader(videoPathHdr, colVideoPath ))
	{
		SCENE_ERROR(TXT("Failed to find header %ls in resSceneVideo csv!"), videoPathHdr.AsChar());
		return;
	}

	const String pauseHdr(TXT("Pause"));
	Uint32 colPause = 0;
	if (!ar->FindHeader(pauseHdr, colPause ))
	{
		SCENE_ERROR(TXT("Failed to find header %ls in resSceneVideo csv!"), pauseHdr.AsChar());
		return;
	}

	for ( Uint32 i = 0; i < ar->GetNumberOfRows(); ++i )
	{
		const String& videoPath = ar->GetValueRef( colVideoPath, i );
		const String& pauseStr = ar->GetValueRef( colPause, i );
		if ( videoPath.Empty() )
		{
			continue;
		}
		Bool pause = false;
		if (::FromString( pauseStr, pause ) )
		{
			m_sceneVideoMap.Insert( videoPath.ToLower(), SSceneVideo(pause) );
			SCENE_LOG(TXT("[resSceneVideo] '%ls': %d"), videoPath.AsChar(), pause );
		}
		else
		{
			SCENE_WARN(TXT("Failed to process resSceneVideo '%ls' pause '%ls'"), videoPath.AsChar(), pauseStr.AsChar());
		}
	}
#endif
}

void CStorySceneSystem::Shutdown()
{
	m_animationList.Destroy();

	delete m_sceneDisplay;
	m_sceneDisplay = NULL;
}

void CStorySceneSystem::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Dump properties
	//file << m_activeScenes;
	file << m_scenesToStart;
	//file << m_scenesToFree;

	for ( CStorySceneController::ScenePropInfo& info : m_finScenesBlendData.m_sceneLightEntities )
	{
		file << info.m_prop;
	}
}

CStoryScenePlayer* CStorySceneSystem::StartScene( CStorySceneController & controller, const ScenePlayerPlayingContext& context )
{
	// No active world to start the scene on
	if ( context.UsesGameWorld() && !GGame->GetActiveWorld() )
	{
		SCENE_WARN( TXT("Active world is needed to start a scene") );
		return NULL;
	}
	
	// Setup entity spawn info
	EntitySpawnInfo spawnInfo;
	spawnInfo.m_entityClass = context.m_playerClass;

	// Create scene player
	SCENE_ASSERT( context.m_world );
	CStoryScenePlayer* storyPlayer = Cast<CStoryScenePlayer>( context.m_world->GetDynamicLayer()->CreateEntitySync( spawnInfo ) );
	if ( !storyPlayer )
	{
		SCENE_ERROR( TXT("Unable to spawn new scene player") );
		return NULL;
	}

	// Stop scenes in which actors of this new scene are playing if new scene is not parallel
	// (check IsParallel only, rest checked in CanStartScene)
	if ( context.m_playerEntity )
	{
		CPlayer* player = context.m_playerEntity;

		TDynArray< CStorySceneController::SceneActorInfo >::const_iterator
			currActor = controller.m_mappedActors.Begin(),
			lastActor = controller.m_mappedActors.End();
		for ( ; currActor != lastActor; ++currActor )
		{
			CActor* actor = Cast< CActor >( currActor->m_actor.Get() );
			if ( ! actor )
				continue;
			
			const TDynArray< CStorySceneController* > & scenes = actor->GetStoryScenes();
			for ( Uint32 i=0; i < scenes.Size(); i++ )
			{
				CStorySceneController * scene = scenes[i];

				if ( scene == & controller )
					continue;
				
				if ( scene->IsPaused() )
					continue;
				
				scene->Stop( SCR_ACTOR_STOLEN );
			}

			if( actor == player )
			{
				//StopForPlayer();
			}
		}
	}

	// Initialize player
	if ( !storyPlayer->Init( controller, context ) )
	{
		DestroyScene( storyPlayer );
		return NULL;
	}

	// Add to list of active scenes
	m_activeScenes.PushBack( THandle< CStoryScenePlayer >( storyPlayer ) );
	return storyPlayer;
}

Bool CStorySceneSystem::CanStopScene( CStorySceneController* c ) const
{
	Bool ret( true );

	if ( m_scenesToFree.Exist( c ) )
	{
		ret = false;
	}
	else if ( m_activeScenes.Size() == 0 )
	{
		ret = false;
	}
	else if ( CStoryScenePlayer* p = c->GetPlayer() )
	{
		if ( !m_activeScenes.Exist( p ) )
		{
			ret = false;
		}
	}

	if ( ret )
	{
		SCENE_ASSERT( c->IsPlaying() );
		SCENE_ASSERT__FIXME_LATER( c->IsStarted() );

		return true;
	}
	else
	{
		return false;
	}
}

void CStorySceneSystem::StopScene( CStorySceneController* c, const SceneCompletionReason& reason )
{
	c->Stop( reason );
}

void CStorySceneSystem::DestroyScene( CStoryScenePlayer* p )
{
	SCENE_ASSERT( p );
	SCENE_ASSERT( p->GetSceneController() );

	CStorySceneController* c = p->GetSceneController();

	SCENE_ASSERT( !m_activeScenes.Exist( p ) );
	SCENE_ASSERT( !m_scenesToStart.Exist( c ) );
	//SCENE_ASSERT( !m_scenesToFree.Exist( c ) ); // The story scene system is so hacky that we cannot do so 'strong' assumptions 

	if ( m_activeScenes.Exist( p ) )
	{
		RED_FATAL_ASSERT( 0, "" );
	}
	if ( m_scenesToStart.Exist( c ) )
	{
		RED_FATAL_ASSERT( 0, "" );
	}
	//if ( m_scenesToFree.Exist( c ) ) // The story scene system is so hacky that we cannot do so 'strong' assumptions 
	//{
	//	RED_FATAL_ASSERT( 0, "" );
	//}

	SCENE_ASSERT( p->HasDestroyFlag() );
	SCENE_ASSERT( p->IsStopped() );

	p->Destroy();
}

void CStorySceneSystem::Stop( CWorld* world )
{
	// Stop all scenes
	for ( Int32 i=m_activeScenes.SizeInt()-1; i>=0; --i )
	{
		CStoryScenePlayer* player = m_activeScenes[i].Get();
		if ( player )
		{
			if ( world && !player->IsInWorld( world ) )
			{
				continue;
			}

			m_activeScenes.RemoveAtFast( i );

			if ( CStorySceneController* c = player->GetSceneController() )
			{
				if ( m_scenesToFree.Exist( c ) )
				{
					SCENE_ASSERT( player->IsStopped() );
					SCENE_ASSERT( player->HasDestroyFlag() );
					
					continue;
				}
			}

			player->Stop();
			SCENE_ASSERT( player->HasDestroyFlag() );

			DestroyScene( player );
		}
	}

	SCENE_ASSERT( m_activeScenes.Size() == 0 );
	
	CleanupFinishedSceneBlend();
	StopForPlayer();
}

void CStorySceneSystem::StopAll()
{
	Stop( nullptr );
}

void CStorySceneSystem::StopForPlayer()
{
	m_lines.Clear();
	m_choices.Clear();

	ToggleDialogHUD( false );
}

void CStorySceneSystem::AttachToWorld()
{
	m_sceneCamera = CreateSceneCamera();
}

void CStorySceneSystem::DetachFromWorld()
{
	// Stop all mappings
	for ( Uint32 i = 0; i < m_scenesToStart.Size(); ++i )
	{
		m_scenesToStart[i]->Stop( SCR_WORLD_UNLOADING );
	}

	FreeFinished();

	m_sceneCamera = NULL;
}

Bool CStorySceneSystem::IsCurrentlyPlayingAnyCinematicScene() const 
{ 
	for ( CStoryScenePlayer* player : m_activeScenes )
	{
		if ( player && !player->IsGameplay() )
		{
			return true;
		}
	}

	return false;
}

void CStorySceneSystem::UnregisterPlaybackListenerFromActiveScenes( IStoryScenePlaybackListener* listener )
{
	for ( Uint32 i=0; i<m_activeScenes.Size(); i++ )
	{
		CStoryScenePlayer* player = m_activeScenes[i].Get();
		if ( player == NULL )
		{
			continue;
		}

		player->RemovePlaybackListener( listener );
	}
}

Bool CStorySceneSystem::HasActorInAnyPlayingScene( const CActor* actor ) const
{
	CName vt = actor->GetVoiceTag();
	if ( vt )
	{
		for ( Uint32 i=0; i<m_activeScenes.Size(); ++i )
		{
			CStoryScenePlayer* p = m_activeScenes[ i ].Get();
			if ( p && !p->IsStopped() )
			{
				if ( p->GetSceneActorEntity( vt ) != nullptr )
				{
					return true;
				}
			}
		}
	}
	return false;
}

void CStorySceneSystem::OnGameStart( const CGameInfo& gameInfo )
{
	m_actorMap = CreateObject< CStorySceneActorMap >( this );
}

void CStorySceneSystem::OnGameEnd( const CGameInfo& gameInfo )
{
	m_actorMap->Discard();
	m_actorMap = NULL;
}

void CStorySceneSystem::OnWorldStart( const CGameInfo& gameInfo )
{
	AttachToWorld();

	RED_ASSERT( GGame );
	RED_ASSERT( GGame->GetInputManager() );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GI_AxisRightY ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GI_AxisRightX ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GI_MouseDampY ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GI_MouseDampX ) );
}

void CStorySceneSystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	SCENE_ASSERT( GGame->GetActiveWorld() );
	SCENE_ASSERT( GGame );
	SCENE_ASSERT( GGame->GetInputManager() );

	GGame->GetInputManager()->UnregisterListener( this );
	CleanupFinishedSceneBlend();
	Stop( GGame->GetActiveWorld() );
	DetachFromWorld();
	CleanupFinishedSceneBlend();
	StopForPlayer();
	m_actorMap->Clear();

	SCENE_ASSERT( m_activeScenes.Size() == 0 );
	SCENE_ASSERT( m_scenesToFree.Size() == 0 );
	SCENE_ASSERT( m_scenesToStart.Size() == 0 );
	SCENE_ASSERT( m_contextsToStart.Size() == 0 );
}

void CStorySceneSystem::CleanupFinishedSceneBlend()
{
	for ( CStorySceneController::ScenePropInfo& info : m_finScenesBlendData.m_sceneLightEntities )
	{
		if ( info.m_destroyAfterScene && info.m_prop )
		{
			info.m_prop->Destroy();
		}		
	}

	m_finScenesBlendData.Clear();
}

namespace StoryScene {

	//Located in "storySceneEventCameraLightInterpolation.h"
	namespace CameraLightBlend
	{
		void BlendCameraLight( SCameraLightsModifiersSetup& outData, const SCameraLightsModifiersSetup& srcData, const  SCameraLightsModifiersSetup& dstData, Float progress, Float dayTime );
	}
}

void CStorySceneSystem::ProcessFinishedSceneBlend( Float timeDelta )
{
	CWorld* world = GGame->GetActiveWorld().Get();
	if ( IsBlendingFinishedScene() && world )
	{			
		if ( m_finScenesBlendData.m_blendTime > 0.f )
		{
			Float prevSceneFactor = Clamp( 1.f - m_finScenesBlendData.m_progress / m_finScenesBlendData.m_blendTime, 0.f, 1.f );
			m_finScenesBlendData.m_progress += timeDelta;
			Float gameplayFactor = Clamp( m_finScenesBlendData.m_progress / m_finScenesBlendData.m_blendTime, 0.f, 1.f );
			Float sceneFactor = 1.f - gameplayFactor;
			Float changeFactor = Clamp( timeDelta > 0.00001 ? sceneFactor/prevSceneFactor : 1.f, 0.f, 1.f );

			if( CEnvironmentManager* envMgr = world->GetEnvironmentManager() )
			{
				//camera lights
				Float dayTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
				SCameraLightsModifiersSetup cameraSetupRes;		
				SCameraLightsModifiersSetup envSetupToBlendTo;	
				cameraSetupRes.SetModifiersIdentity( false );
				envSetupToBlendTo.SetModifiersAllIdentityOneEnabled( ECLT_Gameplay );												
				StoryScene::CameraLightBlend::BlendCameraLight( cameraSetupRes, m_finScenesBlendData.m_envSetupToBlendFrom, envSetupToBlendTo, gameplayFactor, dayTime );
				cameraSetupRes.SetScenesSystemActiveFactor( sceneFactor );
				cameraSetupRes.SetDisableDof( m_finScenesBlendData.m_disableDof );
				envMgr->SetCameraLightsModifiers( cameraSetupRes );
			}

			//scene lights
			for ( CStorySceneController::ScenePropInfo& info : m_finScenesBlendData.m_sceneLightEntities )
			{
				if ( CEntity* light = info.m_prop.Get() )
				{
					if( CLightComponent* lc	= light->FindComponent< CLightComponent >() )
					{					
						lc->SetBrightness( changeFactor * lc->GetBrightness() );
					}
					else if( CDimmerComponent* dc = light->FindComponent< CDimmerComponent >() )
					{
						dc->SetAmbientLevel( changeFactor * dc->GetAmbientLevel() );
					}
				}
			}
		}

		if( m_finScenesBlendData.m_blendTime < m_finScenesBlendData.m_progress )
		{
			CleanupFinishedSceneBlend();			
		}
	}
}

Bool CStorySceneSystem::IsBlendingFinishedScene()
{
	return m_finScenesBlendData.m_blendTime >= 0.f;
}

void CStorySceneSystem::SetFinishedSceneBlendData( CStorySceneSystem::SFinishedSceneBlendData& data )
{
	if( IsBlendingFinishedScene() )
	{
		CleanupFinishedSceneBlend();
	}

	m_finScenesBlendData = data;
}

void CStorySceneSystem::Tick( float timeDelta )
{
	PC_SCOPE_PIX( CStorySceneSystem );

	FreeFinished();

	UpdateScenesToStart( timeDelta );

	// Update scenes that are playing
	UpdatePlayingScenes( timeDelta );

	// Update scene camera
	UpdateSceneCamera( timeDelta );

	ProcessFinishedSceneBlend( timeDelta );

	FreeFinished();

	#ifndef RED_FINAL_BUILD
	if(fadingOutDebugText)
	{
		debugTextFadeoutTimer += timeDelta;
		if(debugTextFadeoutTimer > DEFAULT_DEBUG_TEXT_FADEOUT_TIME)
		{
			debugTextFadeoutTimer = 0.f;
			fadingOutDebugText = false;
			EraseDebugScreenText();
		}		
	}
	#endif

	{
		PC_SCOPE_PIX( LameStoryScenePlayerCleanup );

		if ( GGame && GGame->GetActiveWorld() )
		{
			TDynArray< CEntity *  > tempDestructionList;

			auto& ents = GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities();
			for ( CEntity* ent : ents )
			{
				if( ent && ent->GetClass() == ClassID< CStoryScenePlayer >() )
				{
					CStoryScenePlayer* player = static_cast< CStoryScenePlayer* >( ent );

					if ( player->HasDestroyFlag() && !m_activeScenes.Exist( player ) )
					{
						tempDestructionList.PushBack( player );
					}

				}
			}

			for( auto iter : tempDestructionList )
			{
				iter->Destroy();
			}
		}
	}
}

#ifndef NO_EDITOR

const String& CStorySceneSystem::GetLangRefName() const
{
	static String ref( TXT("EN") );
	return ref;
}

#endif

#ifndef RED_FINAL_BUILD
void CStorySceneSystem::EraseDebugScreenText() const
{
	m_cutsceneDebugText = String::EMPTY;
}

void CStorySceneSystem::SetDebugScreenText(const String & arg) const
{
	fadingOutDebugText = false;
	m_cutsceneDebugText  = arg; 
}

void CStorySceneSystem::AddDebugScreenText(const String & arg) const
{
	fadingOutDebugText = false;	
	m_cutsceneDebugText += arg; 
}

void CStorySceneSystem::ClearDebugScreenText(bool withoutDelay) const
{
	if(withoutDelay)
	{
		EraseDebugScreenText();
	}
	else
	{
		fadingOutDebugText = true;
		debugTextFadeoutTimer = 0.f;
	}
}

const String & CStorySceneSystem::GetDebugScreenText() const
{ 
	return m_cutsceneDebugText;
}
#endif

void CStorySceneSystem::Signal( EStorySceneSignalType inputSignalType, Int32 inputSignalValue )
{
	if ( inputSignalType == SSST_Highlight )
	{
		if( m_choices.Empty() == false && (Uint32) inputSignalValue < m_choices.Size() )
		{
			if ( m_highlightedChoice != inputSignalValue )
			{
				m_highlightedChoice = inputSignalValue;
				if ( GCommonGame && GCommonGame->GetGuiManager() )
				{
					GCommonGame->GetGuiManager()->DialogChoiceSelectionSet( m_highlightedChoice );
				}
			}
		}
	}
	else if ( inputSignalType == SSST_Accept )
	{
		if ( m_choices.Empty() == true )
		{
			for ( Uint32 i = 0; i < m_activeScenes.Size(); ++i )
			{
				CStoryScenePlayer* player = m_activeScenes[i].Get();
				if ( player == NULL )
				{
					continue;
				}
				if ( player->IsPaused() == false && player->IsGameplayNow() == false )
				{
					player->SignalSkipLine();
					m_lastSkipLineTime = (Float)Red::System::Clock::GetInstance().GetTimer().GetSeconds();
					break;
				}
				
			}
			return;
		}

		// skip dialog choice if the human player has just skipped dialog
		// the purpose of this code is to deal with unwanted dialog choices
		const Float DIALOG_OMIT_CHOICE_DELAY = 0.75f;
		if ( m_lastSkipLineTime > 0 )
		{
			Float currentTime = (Float)Red::System::Clock::GetInstance().GetTimer().GetSeconds();
			const Float timeDiff = currentTime - m_lastSkipLineTime;
			if ( timeDiff < DIALOG_OMIT_CHOICE_DELAY )
			{
				return;
			}
		}

		SCENE_ASSERT( m_highlightedChoice >= 0 && (Uint32)m_highlightedChoice < m_choices.Size() );
		// Pass to all non paused scene
		for ( Uint32 i=0; i<m_activeScenes.Size(); i++ )
		{
			CStoryScenePlayer* player = m_activeScenes[i].Get();
			if ( player == NULL )
			{
				continue;
			}
			if ( player->IsPaused() == false && player->HasPendingChoice() == true )
			{
				player->SignalAcceptChoice( m_choices[ m_highlightedChoice ] );
			}
		}
	}
	else if ( inputSignalType == SSST_Skip )
	{
		// Pass to all non paused scene
		for ( Uint32 i = 0; i < m_activeScenes.Size(); ++i )
		{
			CStoryScenePlayer* player = m_activeScenes[i].Get();
			if ( player == NULL )
			{
				continue;
			}
			if ( player->IsPaused() == false && player->IsGameplayNow() == false )
			{

				player->SignalSkipLine();

				break;
			}
		}
	}
}

Bool CStorySceneSystem::OnGameInputEvent( const SInputAction & action )
{
#if 0 // GFx 3
	// Omit input if any panel is loaded (e.g. main menu)
	if ( m_dialogHud == NULL || m_dialogHud->IsAnyPanelVisible() )
	{
		return false;
	}
#endif

	if ( m_dialogHudShown )
	{
		// Input for camera
		ProcessCameraInput( action.m_aName, action.m_value );
	}

	return false;
}

Bool CStorySceneSystem::ProcessCameraInput( const CName& event, Float value )
{
	CCamera* camera = GetActiveSceneCamera();
	if ( camera && m_sceneCameraState.m_enabled == true )
	{
		value /= 4.f;
		
		if ( GGame->IsUsingPad() )
		{
			if ( event == CNAME( GI_AxisRightY ) ) // Up/Down
			{
				m_sceneCameraState.m_cameraUpDown = value;
				return true;
			}
			else if ( event == CNAME( GI_AxisRightX ) ) // Left/Right
			{
				m_sceneCameraState.m_cameraLeftRight = value;
				return true;
			}
		}
		else
		{
			if ( event == CNAME( GI_MouseDampY ) ) // Up/Down
			{
				m_sceneCameraState.m_cameraUpDown = value;
				return true;
			}
			else if ( event == CNAME( GI_MouseDampX ) ) // Left/Right
			{
				m_sceneCameraState.m_cameraLeftRight = value;
				return true;
			}
		}

		return false;
	}

	m_sceneCameraState.Reset();

	return false;
}

CCamera* CStorySceneSystem::GetSceneCamera() const
{
	return m_sceneCamera;
}

CCamera* CStorySceneSystem::GetActiveSceneCamera() const
{
	CCamera* sceneCamera = GetSceneCamera();
	if ( sceneCamera != NULL && sceneCamera->IsActive() == true )
	{
		return sceneCamera;
	}

	return NULL;
}

void CStorySceneSystem::SetSceneCameraMovable( Bool flag )
{
	// Reset
	if ( m_sceneCameraState.m_enabled && !flag )
	{
		m_sceneCameraState.Reset();
	}

	m_sceneCameraState.m_enabled = flag;

	CCamera* camera = GetActiveSceneCamera();
	if ( camera && camera->GetRootAnimatedComponent() && camera->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		Float value = flag ? 1.f : 0.f;
		camera->GetRootAnimatedComponent()->GetBehaviorStack()->SetBehaviorVariable( CNAME( UserInputWeight ), value );
	}
}

Bool CStorySceneSystem::IsSceneCameraMovable() const
{
	CCamera* camera = GetActiveSceneCamera();
	if ( camera && camera->GetRootAnimatedComponent() && camera->GetRootAnimatedComponent()->GetBehaviorStack() )
	{
		return camera->GetRootAnimatedComponent()->GetBehaviorStack()->GetBehaviorFloatVariable( CNAME( UserInputWeight ) ) > 0.f;
	}

	return false;
}

void CStorySceneSystem::AddDialogLine( const CStorySceneLine* line )
{
	SCENE_ASSERT( line );
	//SCENE_ASSERT( !m_lines.Exist( line ) );
	//m_lines.PushBack( line );

	//m_lastLine = line->GetContent();

	//UpdateFlash();
}

void CStorySceneSystem::RemoveDialogLine( const CStorySceneLine* line )
{
	SCENE_ASSERT( line );
	//SCENE_ASSERT( m_lines.Exist( line ) );
	//m_lines.Remove( line );

	//m_lastLine = String::EMPTY;

	//UpdateFlash();
}

void CStorySceneSystem::OnInteractionExecuted( const String& name, CEntity* owner )
{

}

void CStorySceneSystem::ToggleDialogHUD( Bool flag )
{
	// Count hide/show iterations
	const Bool lastState = m_dialogHudShown;
	m_dialogHudShown = flag;

	// Update dialog HUD
	if ( m_dialogHudShown != lastState )
	{
		CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
		SCENE_ASSERT( guiManager );
		if ( guiManager )
		{
			if ( m_dialogHudShown )
			{
				guiManager->DialogHudShow();
			}
			else
			{
				guiManager->DialogHudHide();
			}
		}
	}
}

Bool CStorySceneSystem::IsDialogHUDAvailable() const
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );

	return guiManager ? guiManager->IsDialogHudInitialized() : false;
}

RED_DEFINE_STATIC_NAME( OnCanSkipChanged )

void CStorySceneSystem::OnCanSkipChanged( Bool canSkip )
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );

	if( guiManager )
	{
		guiManager->CallEvent< Bool >( CNAME( OnCanSkipChanged ), canSkip );
	}
}

void CStorySceneSystem::SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI )
{
	if ( choices == m_choices )
	{
		return;
	}

	m_choices = choices;

	if ( m_choices.Empty() == false )
	{
		m_highlightedChoice = 0;
		if ( m_choices.Size() == 1 )
		{
			//return;
		}
	}
	else
	{
		m_highlightedChoice = -1;
	}
	
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	if ( guiManager )
	{
		guiManager->DialogChoicesSet( choices, alternativeUI );
		guiManager->DialogChoiceSelectionSet( m_highlightedChoice );
	}
}

void CStorySceneSystem::ShowChoiceTimer( Float time )
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	if ( guiManager )
	{
		if ( time != 0.f )
		{
			guiManager->DialogChoiceTimeoutSet( time * 100.f );
		}
	}
}

void CStorySceneSystem::HideChoiceTimer()
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	if ( guiManager )
	{
		guiManager->DialogChoiceTimeoutHide();
	}
}

static void W3Hack_SafeFlush( EJobPriority priority )
{
	// Force job manager unlock if we're in StartGame, otherwise we might sit in Flush forever.
	extern Bool GW3Hack_UnlockOverride;
	RED_FATAL_ASSERT( !GW3Hack_UnlockOverride, "GW3Hack_UnlockOverride is already set!" );
	GW3Hack_UnlockOverride = ( !GGame || GGame->CERT_HACK_IsInStartGame() );
	SJobManager::GetInstance().FlushPendingJobs( priority );
	GW3Hack_UnlockOverride = false;
}

void CStorySceneSystem::FlushLoadingVisibleData()
{
	if ( GGame->IsBlackscreen() )
	{
		if ( GGame->GetGameplayConfig().m_enableMeshFlushInScenes == true )
		{
			// CTimeCounter flushTimer;

			W3Hack_SafeFlush( JP_Mesh );

			// LOG_GAME( TXT( "Flushing meshes for scene took %1.2fs" ), flushTimer.GetTimePeriod() );
		}
		
		if ( GGame->GetGameplayConfig().m_enableTextureFlushInScenes == true )
		{
			//CTimeCounter flushTimer;

			W3Hack_SafeFlush( JP_Texture );

			//LOG_GAME( TXT( "Flushing textures for scene took %1.2fs" ), flushTimer.GetTimePeriod() );
		}

		if ( GGame->GetGameplayConfig().m_enableAnimationFlushInScenes == true )
		{
			//CTimeCounter flushTimer;

			W3Hack_SafeFlush( JP_Animation );

			//LOG_GAME( TXT( "Flushing animations for scene took %1.2fs" ), flushTimer.GetTimePeriod() );
		}

	}
}

void CStorySceneSystem::Fade( const String& sceneName, const String& reason, Bool fadeIn, Float duration, const Color& color )
{
	if ( fadeIn )
	{
		GGame->ResetFadeLock( String::Printf( TXT( "%ls - CStorySceneSystem_FadeIn - %ls" ), sceneName.AsChar(), reason.AsChar() ) );
	}

	GGame->StartFade( fadeIn, String::Printf( TXT( "%ls - CStorySceneSystem_Fade - %ls" ), sceneName.AsChar(), reason.AsChar() ), duration, color );

	if ( !fadeIn )
	{
		GGame->SetFadeLock( String::Printf( TXT( "%ls - CStorySceneSystem_FadeOut - %ls" ), sceneName.AsChar(), reason.AsChar() ) );
	}
}

void CStorySceneSystem::Blackscreen( const String& sceneName, Bool enable, const String& reason )
{
	if ( !enable )
	{
		GGame->ResetFadeLock( String::Printf( TXT( "%ls - CStorySceneSystem_ResetBlackscreen, %ls" ), sceneName.AsChar(), reason.AsChar() ) );
	}

	GGame->SetBlackscreen( enable, String::Printf( TXT( "%ls - CStorySceneSystem_Blackscreen, %ls" ), sceneName.AsChar(), reason.AsChar() ) );
	
	if ( enable )
	{
		GGame->ResetFadeLock( String::Printf( TXT( "%ls - CStorySceneSystem_SetBlackscreen, %ls" ), sceneName.AsChar(), reason.AsChar() ) );
	}
}

void CStorySceneSystem::UpdatePlayingScenes( float timeDelta )
{
	// Update active scenes
	for ( Int32 i=m_activeScenes.SizeInt()-1; i>=0; --i )
	{
		CStoryScenePlayer* player = m_activeScenes[i].Get();
		if ( player )
		{
			CStorySceneController* c = player->GetSceneController();
			SCENE_ASSERT__FIXME_LATER( !m_scenesToFree.Exist( c ) );

			if ( !m_scenesToFree.Exist( c ) && !player->Tick( timeDelta ) )
			{
				CStoryScenePlayer* p = m_activeScenes[i].Get(); // OMG - check the scene stop func
				SCENE_ASSERT( p );

				m_activeScenes.RemoveAtFast( i );

				if ( p )
				{
					if ( CStorySceneController* c = p->GetSceneController() )
					{
						SCENE_ASSERT__FIXME_LATER( !m_scenesToFree.Exist( c ) );
						if ( m_scenesToFree.Exist( c ) )
						{
							SCENE_ASSERT( p->IsStopped() );
							SCENE_ASSERT( p->HasDestroyFlag() );
						
							continue;
						}
					}
					else
					{
						SCENE_ASSERT( p->GetSceneController() );
					}

					p->Stop();
					SCENE_ASSERT( p->HasDestroyFlag() );

					//if ( CStorySceneController* c = p->GetSceneController() )
					//{
					//	SCENE_ASSERT( !m_scenesToFree.Exist( c ) ); // The story scene system is so hacky that we cannot do so 'strong' assumptions 
					//}

					DestroyScene( p );
				}
			}
		}
		else 
		{
			m_activeScenes.RemoveAtFast( i );
			SCENE_LOG( TXT("UpdatePlayingScenes, removed null scene from m_activeScenes array") );
		}
	}
}

void CStorySceneSystem::UpdateSceneCamera( Float timeDelta )
{
	CCamera* camera = GetActiveSceneCamera();
	if ( camera )
	{
		Float weight = Clamp( timeDelta, 0.f, 1.f );

		m_sceneCameraState.m_cameraUpDownAcc = Clamp( ( 1.f - weight ) * m_sceneCameraState.m_cameraUpDownAcc + weight * m_sceneCameraState.m_cameraUpDown, -1.f, 1.f );
		m_sceneCameraState.m_cameraLeftRightAcc = Clamp( ( 1.f - weight ) * m_sceneCameraState.m_cameraLeftRightAcc + weight * m_sceneCameraState.m_cameraLeftRight, -1.f, 1.f );

		if ( GGame->IsUsingPad() )
		{
			camera->Rotate( m_sceneCameraState.m_cameraLeftRight, m_sceneCameraState.m_cameraUpDown );	
		}
		else
		{
			camera->Rotate( m_sceneCameraState.m_cameraLeftRightAcc, m_sceneCameraState.m_cameraUpDownAcc );	
		}
	}
}

Bool CStorySceneSystem::ExtractVoicesetDataFromSceneInput( const CStorySceneInput* input, const CName& voicetag, CActorSpeechQueue* speechQueue )
{
	// Returns whether node is a CStorySceneSection that is a valid voiceset section.
	// Multi-path sections, sections with choice, cutscene sections and videosections
	// are not valid voiceset sections.
	auto isVoicesetSection = [] ( const CStorySceneLinkElement* node )
	{
		if( node->IsExactlyA< CStorySceneSection >() )
		{
			const CStorySceneSection* section = static_cast< const CStorySceneSection* >( node );
			if( section->IsGameplay() && section->GetNumberOfInputPaths() == 1 && !section->GetChoice() )
			{
				return true;
			}
		}

		return false;
	};

	// 1. Find voiceset section
	EvalSceneGraphResult evalResult = EvalSceneGraph( input, isVoicesetSection );
	if( evalResult.resolution != EvalSceneGraphResult::EResolution::R_FoundMatch )
	{
		SCENE_WARN( TXT( "Cannot play input '%ls' as simple voiceset for actor '%ls' - no section found" ), input->GetName().AsChar(), voicetag.AsString().AsChar() );
		return false;
	}

	const CStorySceneSection* voicesetSection = static_cast< const CStorySceneSection* >( evalResult.resultNode );

	// 2. Find scene lines		
	TDynArray< CAbstractStorySceneLine* > sectionLines;
	voicesetSection->GetLines( sectionLines );
		
	if ( sectionLines.Size() == 0 )
	{
		SCENE_WARN( TXT( "Cannot play input '%ls' as simple voiceset for actor '%ls' - no lines found in section '%ls'" ), input->GetName().AsChar(), voicetag.AsChar(), voicesetSection->GetName().AsChar() );
		return false;			
	}
		
	Int32 speechFlags = ASM_Text | ASM_Voice | ASM_Lipsync | ASM_Gameplay;
	if ( voicesetSection->HasCinematicOneliners() == true )
	{
		speechFlags |= ASM_Subtitle;
	}

	for( Int32 i=sectionLines.SizeInt()-1; i >= 0; --i )
	{
		CAbstractStorySceneLine* abstractSceneLine = sectionLines[ i ];

		if ( abstractSceneLine->GetVoiceTag() != voicetag )
		{
			SCENE_WARN( TXT( "Cannot play input '%ls' as simple voiceset for actor '%ls' - line in section '%ls' has invalid voicetag (%ls)" ), input->GetName().AsChar(), voicetag.AsChar(), voicesetSection->GetName().AsChar(), abstractSceneLine->GetVoiceTag().AsChar() );
			continue;
		}

		CStorySceneLine* sceneLine = Cast< CStorySceneLine >( abstractSceneLine );			
		speechQueue->AddLine( sceneLine->GetLocalizedContent()->GetIndex(), sceneLine->GetSoundEventName(), false,  speechFlags );
	}
	
	return true;
}

Bool CStorySceneSystem::ExtractChatDataFromSceneInput( const CStorySceneInput* input, TDynArray< SExtractedSceneLineData >& lines )
{
	// Returns whether node is a CStorySceneSection that is a valid chat section.
	// Multi-path sections, sections with choice, cutscene sections and videosections
	// are not valid chat sections.
	auto isChatSection = [] ( const CStorySceneLinkElement* node )
	{
		if( node->IsExactlyA< CStorySceneSection >() )
		{
			const CStorySceneSection* section = static_cast< const CStorySceneSection* >( node );
			if( section->IsGameplay() && section->GetNumberOfInputPaths() == 1 && !section->GetChoice() )
			{
				return true;
			}
		}

		return false;
	};

	// find chat section
	EvalSceneGraphResult evalResult = EvalSceneGraph( input, isChatSection );
	if( evalResult.resolution != EvalSceneGraphResult::EResolution::R_FoundMatch )
	{
		SCENE_WARN( TXT( "Cannot play input '%ls' as chat - no section found" ), input->GetName().AsChar() );
		return false;
	}

	const CStorySceneSection* chatSection = static_cast< const CStorySceneSection* >( evalResult.resultNode );

	// 3. Setup flags
	Int32 speechFlags = ASM_Text | ASM_Voice | ASM_Lipsync | ASM_Gameplay;
	if ( chatSection->HasCinematicOneliners() == true )
	{
		speechFlags |= ASM_Subtitle;
	}

	TDynArray< CAbstractStorySceneLine* > sectionLines;
	chatSection->GetLines( sectionLines );

	// 4. Fill input data
	const CStoryScene* scene = input->GetScene();
	for ( Uint32 i=0; i<sectionLines.Size(); ++i )
	{
		const CStorySceneLine* sceneLine = Cast< const CStorySceneLine >( sectionLines[ i ] );
		if ( sceneLine )
		{
			CName actorVoicetag = sceneLine->GetVoiceTag();
			if( const CStorySceneActor* sceneActor = scene->GetActorDescriptionForVoicetag( actorVoicetag ) )
			{
				SExtractedSceneLineData* elem = new ( lines ) SExtractedSceneLineData();
				elem->m_voicetag = actorVoicetag;
				elem->m_stringIndex = sceneLine->GetLocalizedContent()->GetIndex();
				elem->m_eventName = sceneLine->GetSoundEventName();
				elem->m_modeFlags = speechFlags;
				elem->m_actorTags = sceneActor->m_actorTags;
			}
			else
			{
				SCENE_WARN( TXT( "Cannot play input '%ls' as chat - actor '%ls' is not mapped" ), input->GetName().AsChar(), actorVoicetag.AsString().AsChar() );
				return false;
			}
		}
	}

	return true;
}

Bool CStorySceneSystem::PlayAsVoiceset( const CStorySceneInput* input, CActor* actor )
{
	SCENE_ASSERT( input != NULL );
	SCENE_ASSERT( actor != NULL );
	if ( input == NULL || actor == NULL )
	{
		return false;
	}	

	if ( !ExtractVoicesetDataFromSceneInput( input, actor->GetVoiceTag(), actor->GetSpeechQueue() ) )
	{
		return false;
	}

	if( !actor->GetSpeechQueue()->HasNextLine() )
	{		
		return false;
	}

	actor->ProceedSpeechFromQueue();
	return true;
}

/************************************************************************/
/* Automatic scenes player                                              */
/************************************************************************/

CStorySceneController* CStorySceneSystem::PlayInput( const CStorySceneInput* input, const StorySceneControllerSetup& setup )
{
	SCENE_ASSERT( input );

	SceneControllerPlayingContext context;
	context.m_sceneInput = input;
	context.m_priority = setup.m_priority;
	context.m_mustPlay = setup.m_mustPlay;
	context.m_scenePosition = Vector::ZERO_3D_POINT;
	context.m_world = setup.m_world;
	context.m_spawnAllActors = setup.m_spawnAllActors;
	context.m_contextActorsProvider = setup.m_contextActorsProvider;

	if ( setup.m_suggestedPosition )
	{
		context.m_scenePosition = setup.m_suggestedPosition->GetWorldPositionRef();
	}
	else if ( setup.m_player )
	{
		context.m_scenePosition = setup.m_player->GetWorldPositionRef();
	}

	CStorySceneController* sceneController = CreateSceneController( *this, context );
	if ( !sceneController )
	{
		return NULL;
	}

	if ( setup.m_contextActorsProvider )
	{
		if ( const TDynArray< THandle<CEntity> >* actors = setup.m_contextActorsProvider->GetActors() )
		{
			sceneController->m_externalContextActors = *actors;
		}		
	}

	CPlayer* player = GCommonGame ? GCommonGame->GetPlayer() : nullptr;
	if ( player && context.m_world == player->GetLayer()->GetWorld() && !player->IsAlive() )
	{
		return nullptr;
	}


	ScenePlayerPlayingContext playerContext;
	playerContext.m_world = context.m_world;
	playerContext.m_playerEntity = setup.m_player;
	playerContext.m_playerClass = setup.m_scenePlayerClass;
	playerContext.m_display = setup.m_sceneDisplay;
	playerContext.m_debugger = setup.m_sceneDebugger;
	playerContext.m_csWorldInterface = setup.m_csWorldInterface;
	playerContext.m_spawnAllActors = setup.m_spawnAllActors;
	playerContext.m_asyncLoading = setup.m_asyncLoading;
	playerContext.m_useApprovedVoDurations = setup.m_useApprovedVoDurations;
	playerContext.m_hackFlowCtrl = setup.m_hackFlowCtrl;

	if ( input && !input->IsGameplay() )
	{
		CleanupFinishedSceneBlend();
	}

	if ( ! sceneController->Start() || ! sceneController->Play( playerContext ) )
	{ // mapping has been initialized, schedule mapping to play
		if ( ! sceneController->IsStarted() ) // If mapping has been stopped (sth. went wrong during Start, than do not add it to the pending list, it will be freed soon)
		{
			return NULL;
		}

		m_scenesToStart.PushBack( sceneController );
		m_contextsToStart.PushBack( playerContext );
	}

	return sceneController;
}

CStorySceneController* CStorySceneSystem::PlayInputExt( const CStorySceneInput* input, 
													   EStorySceneForcingMode forceMode /*= SSFM_ForceNothing*/, 
													   EArbitratorPriorities priority /*= 0*/, 
													   const TDynArray< THandle< CEntity > > * contextActors /*= NULL*/,
													   CNode * suggestedPosition /*= NULL*/, 
													   Bool	isQuestScene /*= false*/)
{
	struct SSimpleControllerSetupActorsProvider : public IControllerSetupActorsProvider
	{
		const TDynArray< THandle< CEntity > > * contextActors;
		virtual const TDynArray< THandle<CEntity> >* GetActors() { return contextActors; };
		virtual const TDynArray< THandle<CEntity> >* GetLights() { return nullptr; }
		virtual const TDynArray< THandle<CEntity> >* GetProps()	 { return nullptr; }
		virtual const TDynArray< THandle<CEntity> >* GetEffects(){ return nullptr; }
	} provider;

	provider.contextActors = contextActors;

	StorySceneControllerSetup setup;
	setup.m_forceMode = forceMode;
	setup.m_priority = priority;
	setup.m_contextActorsProvider = &provider;
	setup.m_suggestedPosition = suggestedPosition;
	setup.m_mustPlay = isQuestScene;
	setup.m_player = GCommonGame ? GCommonGame->GetPlayer() : nullptr;
	setup.m_world = GGame ? GGame->GetActiveWorld() : nullptr;
	setup.m_sceneDisplay = GetSceneDisplay();
	setup.m_csWorldInterface = new CCutsceneInGameWorldInterface();
	
	return PlayInput( input, setup );
}

// Fairly ligthweigth method of checking if the scene can play. It is less cumbersome than trying to play scene an checking result
Bool CStorySceneSystem::CanPlaySceneInput( const TSoftHandle< CStoryScene >& sceneHandle, const String& inputName ) const
{
	if ( sceneHandle.IsLoaded() == false )
	{
		return false;
	}

	const CStorySceneInput* input = sceneHandle.Get()->FindInput( inputName );
	if ( input == NULL )
	{
		return false;
	}

	return CanPlaySceneInput( input );
}

// Fairly ligthweigth method of checking if the scene can play. It is less cumbersome than trying to play scene an checking result
Bool CStorySceneSystem::CanPlaySceneInput( const CStorySceneInput* input ) const
{
	if ( input == NULL )
	{
		return false;
	}
	SCENE_ASSERT( input->GetScene() != NULL );

	if ( input->IsGameplay() == false )
	{
		// Non gameplay (story) scenes should be played always
		return true;
	}

	Float actorSearchingDistance ;
	CNode* scenePlacementNode = NULL;
	CActor*	player = GCommonGame->GetPlayer();
	SCENE_ASSERT( player != NULL )

	actorSearchingDistance = input->GetMaxActorsStaryingDistanceFromPlacement();
	const CStorySceneDialogsetInstance* dialogset = input->GetScene()->GetDialogsetByName( input->GetDialogsetName() );


	if ( dialogset != NULL
		&& GGame != NULL
		&& GGame->GetActiveWorld().IsValid()
		&& GGame->GetActiveWorld()->GetTagManager() != NULL )
	{
		// Find placement node based on dialogset;
		const TagList& scenePlacementTags = dialogset->GetPlacementTag();

		scenePlacementNode = GGame->GetActiveWorld()->GetTagManager()
			->GetTaggedNodeClosestTo( scenePlacementTags, player->GetWorldPositionRef() );
	}

	if ( scenePlacementNode == NULL )
	{
		scenePlacementNode = player;
		actorSearchingDistance = input->GetMaxActorsStartingDistanceFormPlayer();
	}

	SCENE_ASSERT( scenePlacementNode != NULL );


	// Find actors close to placement node
	TDynArray< TPointerWrapper< CActor > > outputWrapped;
	STATIC_NODE_FILTER( IsAlive, filterIsAlive );
	static const INodeFilter* filters[] = { &filterIsAlive };
	GCommonGame->GetActorsManager()->GetClosestToNode( *scenePlacementNode, outputWrapped, Box( Vector::ZEROS, actorSearchingDistance ), INT_MAX, filters, 1 );

	// Get voicetags required for scene
	TDynArray< CName > requiredVoicetags;
	const_cast< CStorySceneInput* >( input )->CollectVoiceTags( requiredVoicetags );	// TODO: This is only method preventing scene input object to be const here

	// Iterate through nearby actors and check their voicetags
	for ( TDynArray< TPointerWrapper< CActor > >::const_iterator actorsIter = outputWrapped.Begin();
		actorsIter != outputWrapped.End(); ++actorsIter )
	{
		CActor* actor = actorsIter->Get();
		SCENE_ASSERT( actor != NULL );

		if ( actor->GetStoryScenes().Empty() == true )
		{
			// accept actors who do not have any scene right now
			requiredVoicetags.RemoveFast( actor->GetVoiceTag() );
		}
	}

	// At this point we have a list with voicetags that are not nearby. This should be empty for scene to start
	return requiredVoicetags.Empty();
}

void CStorySceneSystem::OnSceneMappingStopped( CStorySceneController* scene, Bool hasBeenPlaying )
{
	Int32 i = Int32( m_scenesToStart.GetIndex( scene ) );
	if( i > 0 )
	{
		m_scenesToStart.RemoveAtFast( i );
		m_contextsToStart.RemoveAtFast( i );
	}	

	m_scenesToFree.PushBackUnique( scene );
}

void CStorySceneSystem::UpdateScenesToStart( Float timeDelta )
{
	m_sceneUpdateTimeout -= timeDelta;
	if ( m_sceneUpdateTimeout > 0.f )
		return;
	m_sceneUpdateTimeout = SCENE_UPDATE_INTERVAL;

	// Update mappings and play scenes if mappings are satisfied
	for ( Int32 i = (Int32)m_scenesToStart.Size()-1; i >= 0; --i )
	{
		CStorySceneController * scene = m_scenesToStart[i];

		if ( scene->Start() && scene->Play( m_contextsToStart[i] ) )
		{
			m_scenesToStart.EraseFast( m_scenesToStart.Begin() + i );
			m_contextsToStart.EraseFast( m_contextsToStart.Begin() + i );
		}
	}
}

void CStorySceneSystem::FreeFinished()
{
	// Free stopped mappings
	const Uint32 numScenesToFree = m_scenesToFree.Size();
	for ( Uint32 i=0; i<numScenesToFree; ++i )
	{
		CStorySceneController* scene = m_scenesToFree[i];
		SCENE_ASSERT( !scene->IsStarted() );
		SCENE_ASSERT( !scene->IsPlaying() );

		if ( CStoryScenePlayer* p = scene->GetPlayer() )
		{
			if ( !p->IsDestroyed() )
			{
				p->Destroy();
			}
		}

		scene->Free();
	}
	m_scenesToFree.Clear();
}

void CStorySceneSystem::ShowPreviousDialogText( const String& text )
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	guiManager->DialogPreviousSentenceSet( text );
}

void CStorySceneSystem::HidePreviousDialogText()
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	guiManager->DialogPreviousSentenceHide();
}

void CStorySceneSystem::ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI )
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );

	if ( lineType == SSLT_Normal )
	{
		m_lastLine = text;
		m_lastLineActor = actor;
		guiManager->DialogSentenceSet( text, alternativeUI );
	}
	else
	if( actor != NULL )
	{
		if ( lineType == SSLT_Subtitle )
		{
			guiManager->SubtitleShow( actor, text, alternativeUI );
		}
		else
		{
			guiManager->ShowOneliner( text, actor->GetSceneParentEntity() );
		}
	}
}

void CStorySceneSystem::HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType )
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	SCENE_ASSERT( guiManager );
	if ( guiManager )
	{
		if ( lineType == SSLT_Normal )
		{
			if ( m_lastLineActor == actor )
			{			
				guiManager->DialogSentenceHide();
			}
		}
		else if( actor != NULL )
		{
			if ( lineType == SSLT_Subtitle )
			{
				guiManager->SubtitleHide( actor );
			}
			else
			{
				guiManager->HideOneliner( actor->GetSceneParentEntity() );
			}
		}
	}
}

void CStorySceneSystem::HideAllDialogTexts()
{
	
}

const String& CStorySceneSystem::GetLastDialogText()
{
	return m_lastLine;
}

Uint8 CStorySceneSystem::GetLastRandomizerValue( Uint32 randomizerId )
{
	Uint8 lastValue = 255;
	for ( Uint32 i = 0; i < RANDOMIZER_STATES_SIZE; ++i )
	{
		if ( m_randomizerStates[ i ].m_randomizerId == randomizerId )
		{
			lastValue = m_randomizerStates[ i ].m_lastValue;
		}
		else if ( m_randomizerStates[ i ].m_cooldown > 0 )
		{
			m_randomizerStates[ i ].m_cooldown -= 1;
		}
	}
	return lastValue;
}

void CStorySceneSystem::SetLastRandomizerValue( Uint32 randomizerId, Uint8 randomValue )
{
	Uint8 indexForNewState = 0;
	Uint8 lowestCooldown = 255;
	for ( Uint8 i = 0; i < RANDOMIZER_STATES_SIZE; ++i )
	{
		if ( m_randomizerStates[ i ].m_cooldown < lowestCooldown )
		{
			lowestCooldown = m_randomizerStates[ i ].m_cooldown;
			indexForNewState = i;
		}
		if ( m_randomizerStates[ i ].m_randomizerId == randomizerId )
		{
			m_randomizerStates[ i ].m_lastValue = randomValue;
			m_randomizerStates[ i ].m_cooldown = RANDOMIZER_COOLDOWN;
			return;
		}
	}

	m_randomizerStates[ indexForNewState ].m_randomizerId = randomizerId;
	m_randomizerStates[ indexForNewState ].m_lastValue = randomValue;
	m_randomizerStates[ indexForNewState ].m_cooldown = RANDOMIZER_COOLDOWN;
}

CCamera* CStorySceneSystem::CreateSceneCamera( CWorld* world )
{
	// Create scene camera
	SCENE_ASSERT( world->GetDynamicLayer() );

	// TODO: Don't use hardcoded scene camera template. Either extract it to game resource, or simply chcange behavior of gameplay camera
	CEntityTemplate* cameraTemplate = resSceneCameraTemplate.LoadAndGet< CEntityTemplate >();
	if ( cameraTemplate )
	{
		EntitySpawnInfo einfo;
		einfo.m_detachTemplate = false;
		einfo.m_template = cameraTemplate;

		CCamera* camera = Cast< CCamera >( world->GetDynamicLayer()->CreateEntitySync( einfo ) );
		if ( camera )
		{
			//camera->Freeze();

			// Add tag - for behavior debugger
			TagList tags = camera->GetTags();
			tags.AddTag( CNAME( SCENE_CAMERA ) );
			camera->SetTags(tags);
			return camera;
		}
		else
		{
			WARN_GAME( TXT("World: Couldn't create scene camera") );
		}
	}
	else
	{
		WARN_GAME( TXT("Game: Couldn't create scene camera - no resource %s"), resSceneCameraTemplate.GetPath().ToString().AsChar() );
	}

	return NULL;
}

CCamera* CStorySceneSystem::CreateSceneCamera() const
{
	CWorld* activeWorld = GGame->GetActiveWorld();
	return CreateSceneCamera( activeWorld );
}

void CStorySceneSystem::OnGenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD
	const CRenderFrameInfo & frameInfo = frame->GetFrameInfo();

	if ( frameInfo.IsShowFlagOn( SHOW_VisualDebug ) )
	{
		if(GetDebugScreenText().Empty() == false)
		{
			frame->AddDebugScreenText( 50, 250, GetDebugScreenText(), Color::LIGHT_BLUE ,m_debugScreenTextFont ,true);
		}
		if ( GetErrorState().Empty() == false )
		{
			frame->AddDebugScreenText( 50, 50, GetErrorState(), Color::RED ,NULL ,true);
		}
	}
	if ( IsCurrentlyPlayingAnyScene() )
	{
		for ( Uint32 sc = 0; sc < m_activeScenes.Size(); sc++ )
		{
			CStoryScenePlayer* player = m_activeScenes[ sc ].Get();
			if ( player )
			{
				const CStorySceneSection* section = player->GetCurrentSection();
				if ( section && section->IsA< CStorySceneVideoSection >() )
				{
					const CStorySceneVideoSection* videoSection = Cast< CStorySceneVideoSection >( section );
					if ( videoSection )
					{
					    const String& desc = videoSection->GetEventDescription();
						if ( !desc.Empty() )
						{
							frame->AddDebugScreenText( 50, 300, TXT("DESCRIPTION: ") + desc, Color::WHITE, m_debugVideoTextFont, true, Color::BLACK );
						}
					}
				}
			}
		}
	}
#endif//RED_FINAL_BUILD
}

bool CStorySceneSystem::ActiveScenesBlockMusicEvents()
{
	for(CStoryScenePlayer* player : m_activeScenes)
	{
		const CStorySceneSection * section = player->GetCurrentSection();
		const CStoryScene * scene = player->GetStoryScene();

		if((section && section->ShouldBlockMusicTriggers()) || (scene && scene->ShouldBlockMusicTriggers()))
		{
			return true;
		}
	}
	return false;
}


void CStorySceneSystem::funcIsSkippingLineAllowed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool var = !m_activeScenes.Empty();
	for ( CStoryScenePlayer* player : m_activeScenes )
	{
		if (player)
		{
			var &= player->IsSkippingAllowed();
		}
	}

	RETURN_BOOL( var )
}

void CStorySceneSystem::funcSendSignal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EStorySceneSignalType, signal, SSST_Skip );
	GET_PARAMETER( Int32, value, 0 );
	FINISH_PARAMETERS;
	
	Signal( signal, value );
}

void CStorySceneSystem::funcGetChoices( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< String > & choices = * static_cast< TDynArray< String >* >( result );
		for ( Uint32 i = 0; i < m_choices.Size(); ++i )
		{
			choices.PushBack( m_choices[ i ].m_description );
		}
	}
}

void CStorySceneSystem::funcGetHighlightedChoice( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_highlightedChoice );
}

void CStorySceneSystem::funcPlayScene( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CStoryScene >, sceneHandle, NULL );
	GET_PARAMETER( String, inputName, String::EMPTY );
	FINISH_PARAMETERS;

	CStoryScene* scene = sceneHandle.Get();
	if ( scene != NULL )
	{
		const CStorySceneInput* input = scene->FindInput( inputName );
		if ( input != NULL )
		{
			EArbitratorPriorities scenePriority = input->IsGameplay()
				? BTAP_AboveCombat
				: BTAP_FullCutscene;

			PlayInputExt( input, SSFM_SpawnWhenNeeded, scenePriority, NULL, NULL, true );
		}
	}
}

void CStorySceneSystem::funcIsCurrentlyPlayingAnyScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsCurrentlyPlayingAnyScene() );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif

