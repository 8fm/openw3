/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cutsceneInstance.h"
#include "behaviorGraphStack.h"
#include "extAnimCutsceneSoundEvent.h"
#include "appearanceComponent.h"
#include "soundStartData.h"
#include "game.h"
#include "extAnimCutsceneEvent.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "cutsceneDebug.h"
#include "extAnimCutsceneActorEffect.h"
#include "extAnimCutsceneBodyPartEvent.h"
#include "extAnimCutsceneDialogEvent.h"
#include "extAnimCutsceneEffectEvent.h"
#include "extAnimCutsceneEnvironmentEvent.h"
#include "extAnimCutsceneFadeEvent.h"
#include "animatedComponent.h"
#include "mimicComponent.h"
#include "soundEmitter.h"
#include "soundSystem.h"
#include "world.h"
#include "entity.h"
#include "dynamicLayer.h"
#include "tagManager.h"
#include "tickManager.h"
#include "actorInterface.h"
#include "camera.h"
#include "skeletalAnimationContainer.h"
#include "renderCommands.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

const CName CCutsceneInstance::CAM_BLEND_IN_SLOT( TXT("CS_BLEND") );
const CName CCutsceneInstance::CAM_BLEND_OUT_SLOT( TXT("EX_BLEND") );
const Float CCutsceneInstance::CS_TIMEOUT = 5.f;

const CName CCutsceneInstance::NPC_CLASS_NAME( TXT("CNewNPC") );
const CName CCutsceneInstance::ACTOR_CLASS_NAME( TXT("CActor") );
const CName CCutsceneInstance::CAMERA_CLASS_NAME( TXT("CCamera") );
const CName CCutsceneInstance::PLAYER_CLASS_NAME( TXT("CPlayer") );

CCutsceneInstance::CCutsceneInstance()
	: m_worldInterface( NULL )
	, m_sceneInterface( nullptr )
	, m_looped( false )
	, m_isSkipped( false )
	, m_isGameplay( false )
	, m_slowMoPrevFactor( 1.f )
	, m_slowMoCurrFactor( 1.f )
	, m_slowMoDestFactor( 1.f )
{

}

CName CCutsceneInstance::FindActorAppearance( CEntity* entity, const CName& voicetag ) const
{
	if ( entity == NULL )
	{
		return CName::NONE;
	}

	CEntityTemplate* actorTemplate = Cast< CEntityTemplate >( entity->GetTemplate() );

	if ( actorTemplate == NULL || voicetag == CName::NONE )
	{
		return CName::NONE;
	}

	TDynArray< const CEntityAppearance* > appearances;
	actorTemplate->GetAllEnabledAppearances( appearances );

	CName actorAppearance = CName::NONE;

	for ( Uint32 j = 0; j < appearances.Size(); ++j )
	{
		if ( actorTemplate->GetApperanceVoicetag( appearances[ j ]->GetName() ) == voicetag )
		{
			actorAppearance = appearances[ j ]->GetName();
			break;
		}
	}

	return actorAppearance;
}


Bool CCutsceneInstance::Init( const Matrix& point, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, String& errorMsg, ICutsceneSceneInterface* sceneInterface )
{
	m_sceneInterface = sceneInterface;

	if ( InitStageOne(point, provider, worldInterface, true, errorMsg) == false )
	{
		Cleanup();
		return false;
	}

	ASSERT( m_worldInterface );
	
	// Delayed actions
	// DIALOG_TOMSIN_TODO
	//m_worldInterface->GetWorld()->DelayedActions();

	if ( !AreActorsReady( errorMsg ) )
	{
		Cleanup();
		return false;
	}

	InitStageTwo( errorMsg );

	return true;
}

Bool CCutsceneInstance::InitStageOne( const Matrix& point, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, Bool sync, String& errorMsg )
{
	ASSERT( m_csTemplate );
	ASSERT( worldInterface );

	m_worldInterface = worldInterface;

	m_offset = point;

	// Variables
	ResetVariables();

	return CreateActors( provider, sync, errorMsg );
}

void CCutsceneInstance::InitStageTwo( String& errorMsg )
{
	// Create effects
	CreateEffects( errorMsg );

	// Collect cameras
	CollectCameras();

	// Fade
	if ( m_csTemplate->IsBlackScreenOnLoading() )
	{
		FadeOn();
	}

	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		m_actorList[i].PrepareForCutscene();

		CEntity* actorEntity = m_actorList[i].m_entity.Get();
		if ( actorEntity )
		{
			actorEntity->ForceUpdateTransformNodeAndCommitChanges();
		}
	}

	if ( m_useStreaming )
	{
		StartStreaming();
	}
}

void CCutsceneInstance::Cleanup()
{
	DestroyModifiers();
	DestroyActors();
	Destroy();
}

void CCutsceneInstance::Play( Bool selfUpdated, 
							  Bool autoDestroy, 
							  Bool looped,
							  Int32 cameraNum,
							  Float timeout )
{
	ASSERT( m_csDuration > 0.f );

	m_selfUpdated = selfUpdated;
	m_autoDestroy = autoDestroy;
	m_looped = looped;

	m_worldInterface->RegisterCutscene( this );

	if ( LockActors() == false )
	{
		if ( m_csTemplate->IsBlackScreenOnLoading() )
		{
			FadeOff();
		}

		RED_LOG( TXT("LockActors failed. Something is wrong with the cutscene template: "), m_csTemplate->GetFile()->GetFileName().AsChar() );

		Finish();
		return;
	}

	const TDynArray<CName>& tags = m_csTemplate->GetEntToHideTags();
	TDynArray< CEntity* > ents;
	for( const CName& tag : tags )
	{
		ents.ClearFast();
		if( CWorld* world = GetLayer()->GetWorld() )
		{
			world->GetTagManager()->CollectTaggedEntities( tag, ents );
			for ( CEntity* ent : ents )
			{
				ent->SetHideInGame( true, false, CEntity::HR_Scene );
				m_hiddenEntities.PushBack( ent );
			}
		}		
	}

	// Set camera
	if ( m_csCameras.Size() > 0 )
	{
		if ( cameraNum >= 0 && cameraNum < (Int32)m_csCameras.Size() )
		{
			// Set first
			SetCamera( m_csCameras[ cameraNum ] );			
		}
		else
		{
			CS_ERROR( TXT("Input camera number is '%d' but cutscene has got only '%d' cameras - cutscene '%ls'"),
				cameraNum, m_csCameras.Size(), GetName().AsChar() );
		}
	}

	m_running = true;

	if ( m_csTemplate->IsBlackScreenOnLoading() && ! m_csTemplate->HasFadeBefore() )
	{
		if ( !m_streaming )
		{
			FadeOff();
		}
		else
		{
#ifdef USE_HAVOK_ANIMATION
			m_streaming->KeepFading();
#endif
		}
	}

	ApplyModifiersPlayCutscene();

	m_lastTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

	ApplyBurnedAudioTrackName();
	Update( 0.f );
}

void CCutsceneInstance::ResetVariables()
{
	m_csCurrCamera = NULL;
	m_fadeInTimer = 0.f;
	m_fadeOutTimer = 0.f;
	m_currFade = 0.f;
	m_timeFactor = 1.f;
	m_slowMoPrevFactor = 1.f;
	m_slowMoCurrFactor = 1.f;
	m_slowMoDestFactor = 1.f;
	m_pauseCount = 0;
	m_running = false;
	m_finished = false;
	m_lastTime = 0.f;
	m_csTime = 0.f;
	m_csDuration = m_csTemplate->GetDuration();
	m_csTimeRemaining = m_csDuration;
	m_ready = false;
	m_timeoutTimer = 0.f;

	m_fadeInSended = false;
	m_fadeOutSended = false;

	m_streaming = NULL;
#if !defined NO_STREAM_CUTSCENES && !defined NO_ANIM_CACHE
	m_useStreaming = m_csTemplate->IsStreamable();
#else
	m_useStreaming = false;
#endif
}

CutsceneActor* CCutsceneInstance::FindActor( const CEntity* entity )
{
	return const_cast< CutsceneActor* >( static_cast< const CCutsceneInstance* >( this )->FindActor( entity ) );
}

const CutsceneActor* CCutsceneInstance::FindActor( const CEntity* entity ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		if ( m_actorList[i].m_entity.Get() == entity )
		{
			return &(m_actorList[i]);
		}
	}

	return nullptr;
}

CutsceneActor* CCutsceneInstance::FindActor( const String& name )
{
	for ( Uint32 i=0; i<m_modifierInstances.Size(); ++i )
	{
		Int32 csActorIndex = m_modifierInstances[i]->FindActor( name, m_actorList );
		if ( csActorIndex != -1 )
		{
			return &(m_actorList[ csActorIndex ]);
		}
	}

	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		if ( m_actorList[i].m_name == name )
		{
			return &(m_actorList[i]);
		}
	}

	return nullptr;
}

Matrix CCutsceneInstance::GetActorFinalPosition( const CEntity* entity ) const
{
	if ( const CutsceneActor* csActor = FindActor( entity ) )
	{
		return csActor->m_endPos;
	}

	return GetCsPosition();
}

Matrix CCutsceneInstance::CalcActorInitialPelvisPosition( const CEntity* entity ) const
{
	if ( const CutsceneActor* csActor = FindActor( entity ) )
	{
		Matrix mat = m_csTemplate->CalcActorCurrentPelvisPositionInTime( csActor->m_name, 0.f, &GetCsPosition() );
		return mat;
	}

	return GetCsPosition();
}

Matrix CCutsceneInstance::CalcActorCurrentPelvisPosition( const CEntity* entity ) const
{
	if ( const CutsceneActor* csActor = FindActor( entity ) )
	{
		Matrix mat = m_csTemplate->CalcActorCurrentPelvisPositionInTime( csActor->m_name, m_csTime, &GetCsPosition() );
		return mat;
	}

	return GetCsPosition();
}

Matrix CCutsceneInstance::CalcActorFinalPelvisPosition( const CEntity* entity ) const
{
	if ( const CutsceneActor* csActor = FindActor( entity ) )
	{
		Matrix mat = m_csTemplate->CalcActorCurrentPelvisPositionInTime( csActor->m_name, GetDuration(), &GetCsPosition() );
		return mat;
	}

	return GetCsPosition();
}

void CCutsceneInstance::CsToGameplayRequest( Float blendTime, Bool flag )
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		m_actorList[i].OnSwitchedToGameplay( flag, blendTime );
	}
}

void CCutsceneInstance::CsToGameplayRequestForActor( CEntity* entity, Float blendTime, Bool flag )
{
	if ( CutsceneActor* csActor = FindActor( entity ) )
	{
		csActor->OnSwitchedToGameplay( flag, blendTime );
	}
}

RED_DEFINE_STATIC_NAME( CPlayer );

Bool CCutsceneInstance::CreateActors( ICutsceneProvider* provider, Bool sync, String& errors )
{
	// Get animations
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	// Create tag map
	TCacheTags cacheTagList;

	//CClass* npcClass = SRTTI::GetInstance().FindClass( NPC_CLASS_NAME );
	//ASSERT( npcClass );

	//CEntityTemplate* placeholderTempl = resPlaceholderNpcActor.LoadAndGet< CEntityTemplate >();

	CWorld* world = m_worldInterface->GetWorld();

	// Create actors
	for ( Uint32 i=0; i<animations.Size(); ++i )
	{
		ASSERT( animations[i]->GetAnimation() );
		//ASSERT( animations[i]->GetAnimation()->IsLoaded() );

		if ( !animations[i]->GetAnimation() )
		{
			errors = String::Printf( TXT("Animation %d hasn't got animation data\n"), i );
			return false;
		}

		if ( !m_useStreaming && !animations[i]->GetAnimation()->IsLoaded() )
		{
			errors = String::Printf( TXT("%d-th animation is not loaded\n"), i );
			return false;
		}
		
		// Get animation name, actor name, actor definition
		const CName& animation = animations[i]->GetAnimation()->GetName();
		String actorName = GetActorName( animation );

		SCutsceneActorDef* actorDef = NULL;
		
		if ( provider )
		{
			actorDef = provider->GetActorDefinition( actorName );
		}
		
		if ( !actorDef )
		{
			actorDef = m_csTemplate->GetActorDefinition( actorName );
		}
		
		if ( !actorDef )
		{
			ASSERT( actorDef );
			errors = String::Printf( TXT("Cutscene template hasn't got definition for actor '%ls'\n"), actorName.AsChar() );
			return false;
		}

		// Get entity
		CEntity* entity = NULL;

		// Find in entities list
		CutsceneActor* csActor = FindActor( actorName );
		if ( !csActor )
		{
			// Create new cutscene actor
			CutsceneActor newActor( this );


			//if ( placeholderTempl && actorTemplate->GetEntityObject()->IsA( npcClass ) )
			//{
			//	CS_WARN( TXT("Actor template '%ls' in cutscene '%ls' is CNewNPC - Please check resource! Sorry, actor will be converted to base man!"),
			//		actorName.AsChar(), GetName().AsChar() );
			//	actorTemplate = placeholderTempl;
			//}

			// Get tags and voice tags
			const CName& voiceTag = actorDef->m_voiceTag;
			const TagList& tags = actorDef->m_tag;

			Bool forceSpawn = provider && provider->ForceSpawnActor(actorName);

			if( provider && !forceSpawn )
			{
				if( CEntity* candidateEntity = provider->GetCutsceneActor( voiceTag ) )
				{
					const Bool alreadyUsed = FindActor( candidateEntity ) != nullptr;
					if( !alreadyUsed )
					{
						entity = candidateEntity;
					}
				}

				if( !entity )
				{
					if( CEntity* candidateEntity = provider->GetCutsceneActorExtContext( actorName, voiceTag ) )
					{
						const Bool alreadyUsed = FindActor( candidateEntity ) != nullptr;
						if( !alreadyUsed )
						{
							entity = candidateEntity;
							newActor.m_source = AS_ExtContext;
						}
					}
				}
			}		
			if ( !entity && !forceSpawn )
			{
				if( CEntity* candidateEntity = FindGameplayEntity( tags, voiceTag, cacheTagList ) )
				{
					const Bool alreadyUsed = FindActor( candidateEntity ) != nullptr;
					if( !alreadyUsed )
					{
						entity = candidateEntity;
						newActor.m_source = AS_Gameplay;
					}
				}
			}

			//HACK do not spawn 2nd player
			static CClass* playerClass = SRTTI::GetInstance().FindClass( CNAME( CPlayer ) );
			if( !entity && actorDef->GetEntityTemplate() && actorDef->GetEntityTemplate()->GetEntityObject() && actorDef->GetEntityTemplate()->GetEntityObject()->IsA( playerClass ) )
			{
				TDynArray< CEntity* > entities;
				entity = m_worldInterface->GetWorld()->GetTagManager()->GetTaggedEntity( CNAME(PLAYER) );
			}
			
			if ( !provider || !provider->IsActorOptional( voiceTag ) )
			{
				if ( !entity )
				{
					CEntityTemplate* actorTemplate = actorDef->GetEntityTemplate();
					if ( !actorTemplate )
					{
						errors = String::Printf( TXT("Actor '%ls' hasn't got template\n"), actorName.AsChar() );
						return false;
					}

					// Spawn
					entity = SpawnActorEntity( actorTemplate, m_offset, actorDef->m_appearance, actorDef->m_voiceTag );

					// Set spawned flag
					if ( entity )
					{
						newActor.m_source = AS_Spawned;
					}
				}

				if( !entity )
				{
					errors = String::Printf( TXT("Couldn't find or spawn actor '%ls' \n"), actorName.AsChar() );
					return false;
				}
			}			

			// Set entity
			newActor.m_entity = entity;

			// Name
			newActor.m_name = actorName;

			// App
			//newActor.m_newApp = FindActorAppearance( entity, actorDef->m_voiceTag );
			if ( newActor.m_source != AS_Spawned )
			{
				// If we were spawning actor then he already has proper appearance
				newActor.m_newApp = actorDef->m_appearance;
			}

			// Death state
			newActor.m_setDeathPose = actorDef->m_killMe;

			// Type
			newActor.m_type = actorDef->m_type;

			// Mimic
			newActor.m_useMimic = actorDef->m_useMimic;

			// Init
			newActor.Init( world, sync );

			// Add to list
			m_actorList.PushBack( newActor );

			csActor = &(m_actorList.Back());
		}

		if ( IsMimicComponent( animation ) && csActor->m_useMimic == false )
		{
			CS_WARN( TXT("Cutscene warn: Actor '%ls' has got mimic animation but 'useMimic' flag is false - cutscene '%ls'"), actorName.AsChar(), GetName().AsChar() );
			continue;
		}

		// Find component
		CAnimatedComponent* component = NULL;
		if( CEntity* ent =  csActor->m_entity.Get() )
		{
			if ( IsMimicComponent( animation ) )
			{
				component = GetMimicComponent( animation, ent );
				if ( component == NULL )
				{
					CS_WARN( TXT("Cutscene warn: Couldn't find mimic component '%ls' for actor '%ls'"), animation.AsString().AsChar(), actorName.AsChar() );
					continue;
				}
			}
			else
			{
				component = GetComponent( animation, ent );
				if ( component == NULL )
				{
					errors = String::Printf( TXT("Couldn't find component '%ls' for actor '%ls' \n"), animation.AsString().AsChar(), actorName.AsChar() );
					return false;
				}
			}
			
			ASSERT( component );
			csActor->AddPlayableElement( component, animation );
		}		
	}

	return true;
}

Matrix CCutsceneInstance::GetActorEndPos( const CutsceneActor& actor ) const
{
	if ( actor.m_source == AS_Spawned )
	{
		return GetCsPosition();
	}

	if ( actor.m_type == CAT_Prop )
	{
		const SCutsceneActorDef* def = m_csTemplate->GetActorDefinition( actor.m_name );
		if ( def && def->UseFinalPosition() )
		{
			Matrix finalPos;
			if ( m_csTemplate->GetTagPointMatrix( def->m_finalPosition, finalPos ) )
			{
				return finalPos;
			}
		}

		CEntity* ent = actor.m_entity.Get();
		return ent ? ent->GetLocalToWorld() : Matrix::IDENTITY;
	}
	else
	{
		Matrix pelvis( Matrix::IDENTITY );
		Matrix mat = m_csTemplate->GetActorPositionInTime( actor.m_name, GetDuration(), &GetCsPosition(), &pelvis );

		if ( Vector::Near3( mat.GetTranslation(), GetCsPosition().GetTranslation() ) )
		{
			Matrix pelvisIdle(actor.HACK_GetPelvisIdleOffset().AsFloat());
			Matrix pelvisDelta = Matrix::Mul( pelvis, pelvisIdle.FullInverted() );
			pelvisDelta.SetRotZ33( DEG2RAD( pelvisDelta.GetYaw() ) );
			mat = Matrix::Mul( GetCsPosition(), pelvisDelta );
		}

		return mat;
	}
}

void CCutsceneInstance::DestroyActors()
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		m_actorList[i].Destroy( m_running );
	}
}

Bool CCutsceneInstance::AreActorsReady( String& errors ) const
{
	Bool ret = true;

	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		ret &= m_actorList[i].IsReady( errors );
	}

	return ret;
}

void CCutsceneInstance::UnloadAllTemplates()
{
	m_csTemplate->UnloadActorDefinitiones();
}

void CCutsceneInstance::HideEntityForCsDuration( CEntity* ent )
{
	ent->SetHideInGame( true, false, CEntity::HR_Scene );
	m_hiddenEntities.PushBack( ent );
}


void CCutsceneInstance::CollectCameras()
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		CEntity* entity = m_actorList[i].m_entity.Get();

		if ( entity && entity->IsA< CCamera >() )
		{
			CCamera* cam = static_cast< CCamera* >( entity );
			m_csCameras.PushBack( cam );	
		}
	}
}

void CCutsceneInstance::CreateEffects( String& errors )
{
	// Get all cutscene effects
	TDynArray< const CExtAnimCutsceneEffectEvent* > events;
	m_csTemplate->GetEventsOfType( events );

	for( auto eventIter = events.Begin(); eventIter != events.End(); ++eventIter )
	{
		const CExtAnimCutsceneEffectEvent* event = *eventIter;
		ASSERT( event != NULL );

		// Spawn
		if ( CEntity* entity = SpawnEffectEntity( *event ) )
		{
			m_csEffects.Insert( event, entity );
		}
		else
		{
			CS_WARN( TXT("Cutscene '%ls' couldn't spawn effect '%ls'"), GetName().AsChar(), event->GetEventName().AsString().AsChar() );
		}
	}
}

void CCutsceneInstance::AddModifier( ICutsceneModifierInstance* m )
{
	ASSERT( m );
	m_modifierInstances.PushBack( m );
}

void CCutsceneInstance::DestroyModifiers()
{
	for ( Uint32 i=0; i<m_modifierInstances.Size(); ++i )
	{
		m_modifierInstances[ i ]->OnDestroyed( this );
	}

	m_modifierInstances.ClearPtr();
}

void CCutsceneInstance::ApplyModifiersUpdate()
{
	if ( m_modifierInstances.Size() > 0 )
	{
		for ( Uint32 i=0; i<m_modifierInstances.Size(); ++i )
		{
			m_modifierInstances[ i ]->Update( this, m_actorList );
		}
	}
}

void CCutsceneInstance::ApplyModifiersPlayCutscene()
{
	for ( Uint32 i=0; i<m_modifierInstances.Size(); ++i )
	{
		ICutsceneModifierInstance* m = m_modifierInstances[ i ];
		m->OnPlayed( this );
	}
}

void CCutsceneInstance::ApplyBurnedAudioTrackName()
{
	const StringAnsi& burnedAudioTrackName = m_csTemplate->GetBurnedAudioTrackName();
	if( burnedAudioTrackName.Size() )
	{
		CSoundEmitter* component = GetSoundEmitterComponent();
		if( component )
		{
			component->SoundFlagSet( SEF_ProcessTiming, true );
			component->SoundEvent( burnedAudioTrackName.AsChar() );
			component->SoundFlagSet( SEF_ProcessTiming, false );
		}
	}
}

void CCutsceneInstance::DestroyEffects()
{
	for ( auto effectIter = m_csEffects.Begin(); effectIter != m_csEffects.End(); ++effectIter )
	{
		if ( auto cutsceneEffect = Cast< const CExtAnimCutsceneEffectEvent >( effectIter->m_first ) )
		{
			cutsceneEffect->UnloadEntityTemplate();
		}

		if ( CEntity* effectEntity = effectIter->m_second.Get() )
		{
			effectEntity->Destroy();
		}

	}

	m_csEffects.ClearFast();
}

const CAnimatedComponent* CCutsceneInstance::GetActorsPlayableElementByAnimation( const CName& animation ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		const TDynArray< CutsceneActorPlayableElement >& elements = m_actorList[i].m_playableElements;
		
		for ( Uint32 j=0; j<elements.Size(); ++j )
		{
			const CutsceneActorPlayableElement& elem = elements[j];

			for ( Uint32 k=0; k<elem.m_animations.Size(); ++k )
			{
				if ( elem.m_animations[k] == animation )
				{
					return elem.Get( m_actorList[i] );
				}
			}
		}
	}

	return NULL;
}

const CEntity* CCutsceneInstance::GetActorsEntityByAnimation( const CName& animation ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		const TDynArray< CutsceneActorPlayableElement >& elements = m_actorList[i].m_playableElements;

		for ( Uint32 j=0; j<elements.Size(); ++j )
		{
			const CutsceneActorPlayableElement& elem = elements[j];

			for ( Uint32 k=0; k<elem.m_animations.Size(); ++k )
			{
				if ( elem.m_animations[k] == animation )
				{
					return m_actorList[i].m_entity.Get();
				}
			}
		}
	}

	return NULL;
}

CEntity* CCutsceneInstance::FindActorByName( const String& actorName )
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		if ( actorName == m_actorList[i].m_name )
		{
			return m_actorList[i].m_entity.Get();
		}
	}

	return nullptr;
}

void CCutsceneInstance::GetActors( TDynArray< CEntity* >& entities ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		CEntity* entity = m_actorList[i].m_entity.Get();
		if ( entity )
		{
			entities.PushBack( entity );
		}
	}
}

void CCutsceneInstance::GetNonSceneActors( TDynArray< TPair< CEntity*, Bool > >& entities ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		CEntity* entity = m_actorList[i].m_entity.Get();
		if ( entity && m_actorList[i].m_source != AS_Scene )
		{
			entities.PushBack( MakePair( entity, m_actorList[i].m_source == AS_Spawned ) );
		}
	}
}

void CCutsceneInstance::GetAllPlayableElements( TDynArray< const CAnimatedComponent* >& elements ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		const CutsceneActor& actor = m_actorList[i];
		
		for ( Uint32 i=0; i<actor.m_playableElements.Size(); ++i )
		{
			const CAnimatedComponent* ac = actor.m_playableElements[i].Get( actor );
			elements.PushBack( ac );
		}
	}
}

void CCutsceneInstance::CollectAllPlayableActorsWithAnims( TDynArray< const CAnimatedComponent* >& actors, TDynArray< CName >& anims ) const
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		const CutsceneActor& actor = m_actorList[i];
		if ( actor.m_type == CAT_Actor )
		{
			if ( actor.m_playableElements.Size() > 0 )
			{
				if ( actor.m_playableElements[0].m_animations.Size() > 0 )
				{
					const CAnimatedComponent* ac = actor.m_playableElements[0].Get( actor );
					CName anim = actor.m_playableElements[0].m_animations[0];

					actors.PushBack( ac );
					anims.PushBack( anim );
				}
			}
		}
	}
}

const CCamera* CCutsceneInstance::GetCsCamera() const 
{
	return m_csCurrCamera; 
}

void CCutsceneInstance::SetIsGameplay( Bool flag )
{
	m_isGameplay = flag;
}

void CCutsceneInstance::SetCamera( CCamera* camera )
{
	Vector prevCamePos;
	EulerAngles prevCameRot;

	if ( m_csCurrCamera )
	{
		prevCamePos = m_csCurrCamera->GetSelectedCameraComponent()->GetWorldPositionRef();
		prevCameRot = m_csCurrCamera->GetSelectedCameraComponent()->GetWorldRotation();
	}

	if ( m_csCurrCamera && m_worldInterface->GetWorld()->GetCameraDirector() )
	{
		m_worldInterface->GetWorld()->GetCameraDirector()->AbandonCamera( m_csCurrCamera );
	}

	m_csCurrCamera = camera;

	if ( m_csCurrCamera )
	{
		m_csCurrCamera->SetActive( m_csTemplate->GetCameraBlendInDuration() );
	}
}

void CCutsceneInstance::ProcessRemainingEvents()
{
	Bool skipping = m_csTime < m_csDuration;

	ProcessCsEvents( m_csTime, m_csDuration + 999.9f, skipping, skipping );

	// You may wonder, why 999.9 is added to cutscene duration?
	// The answer is simple: in this ugly world, there are
	// cases, when duration events don't exactly end on the cutscene end.
	// ( event->start + event->duration > cutscene->duration )
	// Why? Well, nothing is perfect. The difference may be 0.0001 or 1.2,
	// so to be sure, that all events end properly, 999.9 is added to the end.
	// Thank you for your understanding.

	ProcessRemainingActorEvents( m_csTime, m_csDuration + 999.9f, skipping );
}

void CCutsceneInstance::SoundSeekTo( Float newTime )
{
	const StringAnsi& burnedAudioTrackName = m_csTemplate->GetBurnedAudioTrackName();
	if( burnedAudioTrackName.Size() )
	{
		CSoundEmitter* component = GetSoundEmitterComponent();
		if( component && component->SoundIsActive() )
		{
			component->SoundStopAll();
		}		

		if( component )
		{
			component->SoundFlagSet( SEF_ProcessTiming, true );
			component->SoundSeek( burnedAudioTrackName.AsChar(), newTime / m_csDuration );
			component->SoundFlagSet( SEF_ProcessTiming, false );
		}
	}
}



void CCutsceneInstance::StopAllSounds()
{
	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	if( soundEmitterComponent && soundEmitterComponent->SoundIsActive() )
	{
		soundEmitterComponent->SoundStopAll();
	}
}

void CCutsceneInstance::DestroyCs( Bool restoreGameCamera )
{
	CancelStreaming();

	const Bool hadCamera = HasCamera();

	DestroyModifiers();

	StopAllSounds();

	UnlockActors();

	if ( restoreGameCamera )
	{
		m_worldInterface->ActivateGameCamera( m_csTemplate->GetCameraBlendOutDuration() );
	}

	SetCamera( NULL );

	StopAllEvents();

	CancelSlowMo();

	StopAllEnvironments();


	DestroyEffects();

	DestroyActors();

	UnloadAllTemplates();

	m_running = false;

	m_worldInterface->UnregisterCutscene( this );

	if ( m_currFade > 0.f && m_csTemplate->HasFadeAfter() )
	{
		FadeOff();
	}

	for( THandle<CEntity> handle : m_hiddenEntities )
	{
		if( CEntity* ent = handle.Get() )
		{
			ent->SetHideInGame( false, false, CEntity::HR_Scene );
		}
	}

	// Inform the template
	m_csTemplate->UnregisterInstance( this );

	// Destroy instance
	Destroy();

	// Delayed actions
	//m_worldInterface->GetWorld()->DelayedActions();

	// GC
	//if ( hadCamera && GGame->GetGameplayConfig().m_gcAfterCutscenesWithCamera )
	//{
	//	//SGarbageCollector::GetInstance().Collect();
	//}
}

CAnimatedComponent* CCutsceneInstance::GetComponent( const CName& animation, CEntity* entity ) const
{
	if ( entity == NULL )
	{
		ASSERT( entity );
		return NULL;
	}

	// Get component from entity
	CAnimatedComponent* ac = NULL;

	if ( IsRootComponent( animation ) )
	{
		// Get root
		ac = entity->GetRootAnimatedComponent();
	}
	else
	{
		// Find by name
		ac = entity->FindComponent< CAnimatedComponent >( GetComponentName( animation ) );
	}

	if ( !ac )
	{
		ThrowError( String::Printf( TXT("Unable to find component for animation %s"), animation.AsString().AsChar() ) );
	}

	return ac;
}

CAnimatedComponent* CCutsceneInstance::GetMimicComponent( const CName& animation, CEntity* entity ) const
{
	if ( entity == NULL )
	{
		ASSERT( entity );
		return NULL;
	}

	IActorInterface* actor = entity->QueryActorInterface();
	if ( actor )
	{
		return actor->GetMimicComponent();
	}
	else
	{
		ThrowError( String::Printf( TXT("Unable to find head component for animation %s. Cutscene actor is not CActor"), animation.AsString().AsChar() ) );
		return NULL;
	}
}

CEntity* CCutsceneInstance::SpawnActorEntity( CEntityTemplate* templ, const Matrix& spawnPos, const CName& appearance, const CName& voicetag )
{
	if ( templ == NULL )
	{
		return NULL;
	}

	CWorld* world = m_worldInterface->GetWorld();

	// Spawn entity
	EntitySpawnInfo sinfo;
	sinfo.m_template = templ;
	sinfo.m_spawnPosition = spawnPos.GetTranslation();
	sinfo.m_spawnRotation = spawnPos.ToEulerAngles();
	sinfo.m_previewOnly = world->GetPreviewWorldFlag();

	static CClass* npcClass = SRTTI::GetInstance().FindClass( NPC_CLASS_NAME );
	static CClass* actorClass = SRTTI::GetInstance().FindClass( ACTOR_CLASS_NAME );
	static CClass* cameraClass = SRTTI::GetInstance().FindClass( CAMERA_CLASS_NAME );
	static CClass* playerClass = SRTTI::GetInstance().FindClass( PLAYER_CLASS_NAME );
	
	ASSERT( npcClass );
	ASSERT( actorClass );
	ASSERT( cameraClass );
	ASSERT( playerClass );

	const CEntity* baseEntity = templ ? templ->GetEntityObject() : nullptr;
	if ( baseEntity )
	{
		if ( baseEntity->IsA( cameraClass ) )
		{
			// Nothing
		}
		else if ( baseEntity->IsA( npcClass ) || baseEntity->IsA( playerClass ) )
		{
			sinfo.m_entityClass = actorClass;
		}
		else
		{
			sinfo.m_entityClass = ClassID< CEntity >();
		}
	}
	else
	{
		return nullptr;
	}

	if ( appearance != CName::NONE 
		&& templ->GetEnabledAppearancesNames().Exist( appearance ) == true )
	{
		// If valid appearance is given then we use it
		sinfo.m_appearances.PushBack( appearance );
	}
	else
	{
		// If not we find appearances with valid voicetags
		const TDynArray< VoicetagAppearancePair >& voicetagAppearances = templ->GetVoicetagAppearances();
		for ( TDynArray< VoicetagAppearancePair >::const_iterator voicetagAppearanceIter = voicetagAppearances.Begin();
			voicetagAppearanceIter != voicetagAppearances.End(); ++voicetagAppearanceIter )
		{
			if ( voicetagAppearanceIter->m_voicetag == voicetag )
			{
				sinfo.m_appearances.PushBack( voicetagAppearanceIter->m_appearance );
			}
		}
	}
	
	if ( sinfo.m_appearances.Empty() == true )
	{
		// If we still don't know any appearance then we will use any enabled appearance
		sinfo.m_appearances = templ->GetEnabledAppearancesNames();
	}
	
	CEntity* entity = world->GetDynamicLayer()->CreateEntitySync( sinfo );

	if ( !entity )
	{
		ThrowError( String::Printf( TXT("Unable to spawn entity from template '%ls' in cutscene"), templ->GetDepotPath().AsChar() ) );
		return NULL;
	}

	entity->SetHideInGame( true, false, CEntity::HR_Scene );

	return entity;
}

CEntity* CCutsceneInstance::SpawnEffectEntity( const CExtAnimCutsceneEffectEvent& event )
{
	CEntityTemplate* entTempl = event.LoadAndGetEntityTemplate();
	if ( entTempl )
	{
		entTempl->PreloadAllEffects();
	}

	// Offset effect entity
	Matrix mat = event.GetSpawnRot().ToMatrix();
	mat.SetTranslation( event.GetSpawnPos() );

	Matrix spawnPos = Matrix::Mul( m_offset, mat );

	// Spawn entity
	EntitySpawnInfo sinfo;
	sinfo.m_template = entTempl;
	sinfo.m_spawnPosition = spawnPos.GetTranslation();
	sinfo.m_spawnRotation = spawnPos.ToEulerAngles();
	
	if ( entTempl )
	{
		sinfo.m_appearances = entTempl->GetEnabledAppearancesNames();
	}

	CEntity* entity = m_worldInterface->GetWorld()->GetDynamicLayer()->CreateEntitySync( sinfo );

	if ( !entity )
	{
		if ( entTempl )
		{
			ThrowError( String::Printf( TXT("Unable to spawn entity from template '%ls' in cutscene"), entTempl->GetDepotPath().AsChar() ) );
		}
		else
		{
			ThrowError( String::Printf( TXT("Unable to spawn entity from cutscene effect") ) );
		}

		return nullptr;
	}

	return entity;
}

class CEntityDistanceSortingPredicate
{
private:
	Vector	m_point;
public:
	CEntityDistanceSortingPredicate( const Vector& point ) : m_point( point ) {}

	Bool operator()( const CEntity* a, const CEntity* b ) const
	{
		if ( a == NULL && b == NULL )
		{
			return false;
		}
		else if ( a == NULL )
		{
			return false;
		}
		else if ( b == NULL )
		{
			return true;
		}

		Float distanceA = a->GetWorldPositionRef().DistanceSquaredTo( m_point );
		Float distanceB = b->GetWorldPositionRef().DistanceSquaredTo( m_point );
		return distanceA < distanceB;
	}
};

CEntity* CCutsceneInstance::FindGameplayEntity( const TagList& tags, const CName& voiceTag, TCacheTags& cacheTagList )
{
	CEntity* entity;

	TDynArray< CEntity* > entities;
	m_worldInterface->GetWorld()->GetTagManager()->CollectTaggedEntities( tags, entities );

	if ( voiceTag != CName::NONE )
	{
		// Voice tag filter
		for ( Int32 j=entities.Size()-1; j>=0; --j )
		{
			CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entities[j] );

			const CName& appearance = appearanceComponent ? appearanceComponent->GetAppearance() : CName::NONE;
			CEntityTemplate* entTempl = entities[j]->GetEntityTemplate();

			if( entTempl )
			{
				const CEntityAppearance* entAppearance = entTempl->GetAppearance( appearance, true );
				if ( entAppearance && entTempl->GetApperanceVoicetag( entAppearance->GetName() ) == voiceTag )
				{
					continue;
				} 
			}

			entities.Erase( entities.Begin() + j );
		}
	}

	CEntityDistanceSortingPredicate sortingPredicate( m_offset.GetTranslation() );
	Sort( entities.Begin(), entities.End(), sortingPredicate );

	if ( entities.Size() > 0 )
	{
		Int32 index = -1;

		for ( Uint32 j=0; j<cacheTagList.Size(); ++j )
		{
			if ( cacheTagList[j].m_first == tags )
			{
				if ( cacheTagList[j].m_second + 1 < (Int32)entities.Size() )
				{
					cacheTagList[j].m_second += 1;
				}
				else
				{
					return NULL;
				}

				index = cacheTagList[j].m_second;
				break;
			}
		}

		if ( index == -1 )
		{
			cacheTagList.PushBack( TPair< TagList, Int32 >( tags, 0 ) );
			entity = entities[ 0 ];
			return entity;
		}
		else
		{
			ASSERT( index < (Int32)entities.Size() );
			if ( index < (Int32)entities.Size() )
			{
				entity = entities[ index ];
				return entity;
			}
		}
	}

	return NULL;
}

void CCutsceneInstance::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	AddTimer( CNAME( Update ), 0.0f, true, TICK_PrePhysics );
}

extern Bool GIsGrabbingCutsceneMovie;

void CCutsceneInstance::OnTimer( const CName name, Uint32 id, Float timeDelta )
{
	TBaseClass::OnTimer( name, id, timeDelta );

	if ( m_selfUpdated )
	{
		if ( name == CNAME( Update ) )
		{
			if ( GIsGrabbingCutsceneMovie )
			{
				Float numFramesPerSecond = 0.f;
				if ( !SConfig::GetInstance().GetLegacy().ReadParam< Float >( TXT("user"), TXT("Editor"), TXT("CutsceneGrabbingFPS"), numFramesPerSecond ) ||
					 numFramesPerSecond < 1 )
				{
					numFramesPerSecond = 30;
				}

				const Float frameTimeDelta = 1.0f / numFramesPerSecond;
				Update( frameTimeDelta );
			}
			else
			{
				UpdateAsync();
			}
		}
	}
}

void CCutsceneInstance::OnDetached( CWorld* world )
{
	RemoveTimer( CNAME( Update ), TICK_PrePhysics );
	TBaseClass::OnDetached( world );
}

void CCutsceneInstance::OnTick( Float timeDelta )
{
	ASSERT( 0 );
	if ( m_selfUpdated )
	{
		Update( timeDelta );
	}
}

void CCutsceneInstance::UpdateAsync()
{
	Double currTime;
	#ifdef CONTINUOUS_SCREENSHOT_HACK
		if ( Red::System::Clock::GetInstance().GetTimer().IsTimeHackEnabled() )
			currTime = Red::System::Clock::GetInstance().GetTimer().GetTimeHackSeconds();
		else
			currTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	#else
		currTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	#endif

	if ( m_lastTime > 0.f )
	{
		Float dt = (Float)Max( currTime - m_lastTime, 0.0 );
		m_lastTime = currTime;

#ifdef CONTINUOUS_SCREENSHOT_HACK
		// IsTimeHackEnabled() is a temporary solution which replaces GIsEnabledTimeHack
		// Which was defined in systemWin32.cpp to force rendering to a fixed step (30fps)
		// for recording.  We use this here in CCutsceneInstance::Update to avoid modifying
		// the playback time from sound emitter components
		CSoundEmitterComponent* component = ( Red::System::Clock::GetInstance().GetTimer().IsTimeHackEnabled() )? NULL : GetSoundEmitterComponent();
#else
		CSoundEmitterComponent* component = GetSoundEmitterComponent();
#endif
		if( component )
		{
			Float position = component->GetHighestTime();
			if( position >= 0.0f )
			{				
				dt = position - m_csTime;
			}
		}		
		///////////////////////////////

		dt = Max( dt, 0.f );

		Update( dt );
	}
	else
	{
		m_lastTime = currTime;

		Update( 0.f );
	}
}

void CCutsceneInstance::Update( Float dt, Bool force /* = false  */)
{
	PC_SCOPE_PIX( CutsceneInstanceTick );

	const Float slowMoInv = GetSlowMo() > 0.f ? 1.f / GetSlowMo() : 1.f;
	dt *= m_timeFactor * slowMoInv;

	if ( force || ( m_running && ! IsPause() && !m_finished ) )
	{
		Float newTime = Clamp( m_csTime + dt, 0.f, m_csDuration );

		if ( m_useStreaming && UpdateStreaming( newTime ) )
		{
			return;
		}

		m_csTimeRemaining = Max( 0.0f, m_csDuration - ( m_csTime + dt ) );

		ApplyModifiersUpdate();

		ProcessCsEvents( m_csTime, newTime, m_isSkipped, false );

		if ( m_csTemplate->HasCameraCut( m_csTime, newTime ) )
		{
			if( m_sceneInterface )
			{
				m_sceneInterface->OnCameraCut();
			}
			else // TODO for non scene cutscenes
			{
				if( CWorld* world = GetLayer()->GetWorld() )
				{
					world->GetCameraDirector()->InvalidateLastFrameCamera();
					world->FinishRenderingFades();
				}	
			}
			
#ifndef NO_EDITOR
			if( GetCamera() )
			{
				GetCamera()->InvalidateLastFrameCamera();
			}
#endif
		}

		UpdateFade();

		CSyncInfo info;
		info.m_prevTime = m_csTime;
		info.m_currTime = newTime;

		m_csTime = newTime;

		// Update all actors
		UpdateActors( info );

		// Check time
		if ( m_csTime >= m_csDuration )
		{
			ASSERT( m_csTimeRemaining >= 0.f );

			if ( m_looped )
			{
				// Loop
				m_csTime = 0.f;
				m_fadeInSended = false;
				m_fadeOutSended = false;
				ApplyBurnedAudioTrackName();
			}
			else
			{
				Finish();
				return;
			}
		}
	}
}

void CCutsceneInstance::SkipToEnd()
{
	ASSERT( !m_isSkipped );
	m_isSkipped = true;

	const Float prevTime = m_csTime;

	const Float timeDelta = Clamp( GetDuration() - GetTime(), 0.f, GetDuration() );
	Update( timeDelta );

	ProcessRemainingActorEvents( prevTime, m_csTime, true );
}

#undef PlaySound
void CCutsceneInstance::UpdateActors( const CSyncInfo& info )
{
	static THashMap< const CExtAnimCutsceneSoundEvent*, CAnimatedComponent* > soundEvents;

	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		soundEvents.ClearFast();

		m_actorList[i].Update( info, &soundEvents );

		// Fire sound events
		for( THashMap< const CExtAnimCutsceneSoundEvent*, CAnimatedComponent* >::const_iterator 
			eventIter = soundEvents.Begin(); eventIter != soundEvents.End(); ++eventIter )
		{
			eventIter->m_first->PlaySound( eventIter->m_second );
		}
	}
}

Bool CCutsceneInstance::IsFinished() const
{
	return m_finished;
}

void CCutsceneInstance::Finish()
{
	m_finished = true;

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().QueueEvent( CNAME( SectionFinished ), EventDataCreator< CCutsceneTemplate* >().Create( m_csTemplate ) );
#endif

	if ( m_autoDestroy )
	{
		DestroyCs();
	}
}

Bool CCutsceneInstance::LockActors()
{
	Bool ret = true;

	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		// End pos
		m_actorList[i].m_endPos = GetActorEndPos( m_actorList[i] );

		ret &= m_actorList[i].Lock();
	}

	return ret;
}

void CCutsceneInstance::UnlockActors()
{
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		if ( m_actorList[i].IsLocked() )
		{
			m_actorList[i].Unlock( m_isSkipped );
		}
	}
}

void CCutsceneInstance::StartStreaming()
{
	if ( m_streaming )
	{
		HALT( "Error in CCutsceneInstance::StartStreaming 1");
	}
	
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	// Do we need streaming?
	Int32 allAnimsLoaded = -1;
	const Uint32 size = animations.Size();
	if ( size > 0 )
	{
		for ( Uint32 i=0; i<size; ++i )
		{
			CSkeletalAnimation* anim = animations[ i ] ? animations[ i ]->GetAnimation() : NULL;
			if ( anim )
			{
				if ( allAnimsLoaded == -1 )
				{
					allAnimsLoaded = anim->IsLoaded() ? 1 : 0;
				}
				else if ( allAnimsLoaded == 0 )
				{
					if ( anim->IsLoaded() )
					{
						HALT( "Error in CCutsceneInstance::StartStreaming 2" );
					}
				}
				else if ( allAnimsLoaded == 1 )
				{
					if ( !anim->IsLoaded() )
					{
						HALT( "Error in CCutsceneInstance::StartStreaming 3" );
					}
				}
			}
		}

		if ( allAnimsLoaded == 0 )
		{
#ifdef USE_HAVOK_ANIMATION
			m_streaming = new CCutsceneStreamer( animations );
#endif
		}
	}
}

Bool CCutsceneInstance::UpdateStreaming( Float time )
{
#ifdef USE_HAVOK_ANIMATION
	return m_streaming ? m_streaming->Update( time ) : false;
#else
	return false;
#endif
}

void CCutsceneInstance::CancelStreaming()
{
#ifdef USE_HAVOK_ANIMATION
	if ( m_streaming )
	{
		delete m_streaming;
		m_streaming = NULL;
	}
#endif
}

void CCutsceneInstance::ProcessCsEvents( Float prevTime, Float currTime, Bool skippingFades, Bool skippingSoundEvt )
{
	// Get all events in current time range
	TDynArray< CAnimationEventFired > events;
	m_csTemplate->GetEventsByTime( prevTime, currTime, events );

	for( TDynArray< CAnimationEventFired >::iterator firedIter = events.Begin();
		firedIter != events.End(); ++firedIter )
	{
		CAnimationEventFired& firedEvent = *firedIter;
		const CExtAnimEvent* event = firedEvent.m_extEvent;
		ASSERT( event != NULL );

		const TDynArray< CFXDefinition* >& csEffects = m_csTemplate->GetEffects();

		if ( const CExtAnimCutsceneEffectEvent* effectEvent = Cast< CExtAnimCutsceneEffectEvent >( event ) )
		{
			if ( THandle< CEntity >* entHandle = m_csEffects.FindPtr( effectEvent ) )
			{
				if ( CEntity* entity = entHandle->Get() )
				{
					const CName& effectName = effectEvent->GetEffectName();

					if ( effectEvent->GetEntityTemplateHandle().IsEmpty() )
					{
						// NOTE: Search only among locally-defined effects to avoid name collisions (if the cutscene event has the same name as a template event)
						auto efIt =	FindIf( csEffects.Begin(), csEffects.End(), 
							[ effectName ]( CFXDefinition* e ) { return e->GetName() == effectName; } 
							);

						if ( efIt != csEffects.End() )
						{
							// locally-defined effect found
							if( firedEvent.m_type == AET_DurationStart )
							{
								entity->PlayEffect( *efIt );
							}
							else if ( firedEvent.m_type == AET_DurationEnd )
							{
								entity->StopEffect( *efIt );
							}
						}
					}
					else
					{
						if( firedEvent.m_type == AET_DurationStart )
						{
							entity->PlayEffect( effectName );
						}
						else if ( firedEvent.m_type == AET_DurationEnd )
						{
							entity->StopEffect( effectName );
						}
					}
				}
				else
				{
					CS_WARN( TXT("Couldn't play effect. Cutscene effect '%ls' was not spawned - check resources"), effectEvent->GetEffectName().AsString().AsChar() );
					ASSERT( entity != NULL );
				}
			}
		}
		// Dialogs
		else if( IsType< CExtAnimCutsceneDialogEvent >( event ) )
		{
			// nothing
		}
		// Sound events
		else if( IsType< CExtAnimCutsceneSoundEvent >( event ) )
		{
			// Do not play sounds, when skipping cutscene
			if( ! skippingSoundEvt )
			{
				static_cast< const CExtAnimCutsceneSoundEvent* >( event )->PlaySound();
			}
		}
		else if( IsType< CExtAnimCutsceneFadeEvent >( event ) )
		{
			// Do apply fades when skipping
			if( ! skippingFades )
			{
				event->Process( firedEvent, NULL );
			}
		}		
		else if( IsType< CExtAnimCutsceneBreakEvent >( event ) )
		{
			Finish();
		}
		else if( IsType< CExtAnimCutsceneBokehDofEvent >( event ) )
		{
			CCameraComponent* cam = GetCamera();
			static_cast< const CExtAnimCutsceneBokehDofEvent* >( event )->ProcessEx( firedEvent, cam, currTime );
		}
		else if( IsType< CExtAnimCutsceneBokehDofBlendEvent >( event ) )
		{
			CCameraComponent* cam = GetCamera();
			static_cast< const CExtAnimCutsceneBokehDofBlendEvent* >( event )->ProcessEx( firedEvent, cam, currTime );
		}
		else if( IsType< CExtAnimCutsceneSetClippingPlanesEvent >( event ) )
		{
			CCameraComponent* cam = GetCamera();
			static_cast< const CExtAnimCutsceneSetClippingPlanesEvent* >( event )->ProcessEx( firedEvent, cam, currTime );
		}
		// Anim events
		else if( event->GetClass() == CExtAnimEvent::GetStaticClass() )
		{
			m_worldInterface->OnCutsceneEvent( GetName(), event->GetEventName() );
		}
		else if( IsType< CExtAnimCutsceneEvent > ( event ) )
		{
			static_cast< const CExtAnimCutsceneEvent* >( event )->Process( firedEvent, NULL, this );
		}
		else if ( IsType< CExtAnimCutsceneDurationEvent >( event ) )
		{
			const CExtAnimCutsceneDurationEvent* e = static_cast< const CExtAnimCutsceneDurationEvent* >( event );

			if ( firedEvent.m_type == AET_DurationStart )
			{
				e->StartEx( firedEvent, this );

				//ASSERT( !m_runningEvts.Exist( e ) );
				m_runningEvts.PushBack( e );
			}

			e->ProcessEx( firedEvent, this );

			if ( firedEvent.m_type == AET_DurationEnd )
			{
				e->StopEx( firedEvent, this );

				//ASSERT( m_runningEvts.Exist( e ) );
				m_runningEvts.Remove( e );
			}
		}
		// other type of events, process normal way
		else
		{
			switch( firedEvent.m_type )
			{
			case AET_DurationStart:
				static_cast< const CExtAnimDurationEvent* >( event )->Start( firedEvent, NULL );
				break;
			case AET_DurationEnd:
				static_cast< const CExtAnimDurationEvent* >( event )->Stop( firedEvent, NULL );
				break;
			}

			event->Process( firedEvent, NULL );
		}
	}
}

void CCutsceneInstance::UpdateFade()
{
	const Float currTime = m_csTime;
	const Float duration = m_csDuration;

	Float fadeBeforeDuration = m_csTemplate->GetFadeBeforeDuration();
	Float fadeAfterDuration = m_csTemplate->GetFadeAfterDuration();

	// Temp - set 
	m_currFade = 0.f;

	if ( ! m_fadeInSended && ( fadeBeforeDuration > 0.0f ) )//|| m_looped ) ) // mcinek: always fade in when looped cutscene
	{
		// Send fade in
		m_currFade = 1.f;
		if ( !m_isGameplay )
		{
			m_worldInterface->Fade( true, TXT( "CCutsceneInstance_UpdateFade" ), fadeBeforeDuration );
		}
		m_fadeInSended = true;
	}
	if ( ! m_fadeOutSended && fadeAfterDuration > 0.0f && duration - currTime <= fadeAfterDuration )
	{
		// Send fade out
		if ( !m_isGameplay )
		{
			m_worldInterface->Fade( false, TXT( "CCutsceneInstance_UpdateFade" ), duration - currTime );
		}
		m_fadeOutSended = true;
	}
}

void CCutsceneInstance::FadeOn()
{
	// Send fade out
	m_currFade = 1.f;
	if ( !m_isGameplay )
	{
		m_worldInterface->SetBlackscreen( true, TXT( "CCutsceneInstance_FadeOn" ) );
	}
}

void CCutsceneInstance::FadeOff()
{
	// Send fade in
	m_currFade = 0.f;
	if ( !m_isGameplay )
	{
		m_worldInterface->SetBlackscreen( false, TXT( "CCutsceneInstance_FadeOff" ) );
	}
}

void CCutsceneInstance::PauseAllEffect( Bool isPaused )
{
	// Actors
	for ( auto aIt = m_actorList.Begin(); aIt != m_actorList.End(); ++aIt )
	{
		if ( CEntity* entity = aIt->m_entity.Get() )
		{
			entity->PauseAllEffects( isPaused );
		}
	}

	// Effects
	for ( auto eIt = m_csEffects.Begin(); eIt != m_csEffects.End(); ++eIt )
	{
		if ( CEntity* entity = (*eIt).m_second.Get() )
		{
			entity->PauseAllEffects( isPaused );
		}
	}
}

void CCutsceneInstance::StopAllEffects()
{
	// Actors
	for ( auto aIt = m_actorList.Begin(); aIt != m_actorList.End(); ++aIt )
	{
		if ( CEntity* entity = aIt->m_entity.Get() )
		{
			entity->DestroyAllEffects();
		}
	}

	// Effects
	for ( auto eIt = m_csEffects.Begin(); eIt != m_csEffects.End(); ++eIt )
	{
		if ( CEntity* entity = (*eIt).m_second.Get() )
		{
			entity->DestroyAllEffects();
		}
	}
}

void CCutsceneInstance::StopAllEnvironments()
{
	TDynArray< const CExtAnimCutsceneEnvironmentEvent* > events;
	m_csTemplate->GetEventsOfType( events );

	for ( Uint32 i = 0; i < events.Size(); ++i )
	{
		events[ i ]->OnCutsceneFinish();
	}
}

void CCutsceneInstance::StopAllEvents()
{
	for ( const CExtAnimCutsceneDurationEvent* e : m_runningEvts )
	{
		e->OnCutsceneFinish( this );
	}
	m_runningEvts.Clear();
}

void CCutsceneInstance::CancelSlowMo()
{
	m_slowMoPrevFactor = 1.f;
	m_slowMoCurrFactor = 1.f;
	m_slowMoDestFactor = 1.f;

	if ( GGame )
	{
		GGame->RemoveTimeScale( CNAME( Cutscene ) );
	}
}

void CCutsceneInstance::ProcessSlowMoBlending( Float factor, Float weight )
{
	m_slowMoDestFactor = factor;
	m_slowMoCurrFactor = Lerp( weight, m_slowMoPrevFactor, m_slowMoDestFactor );

	RefreshSlowMo();
}

void CCutsceneInstance::StartSlowMoBlending( Float factor )
{
	m_slowMoPrevFactor = m_slowMoDestFactor;
	m_slowMoCurrFactor = m_slowMoDestFactor;
	m_slowMoDestFactor = factor;

	RefreshSlowMo();
}

void CCutsceneInstance::FinishSlowMoBlending( Float factor )
{
	m_slowMoCurrFactor = factor;
	m_slowMoDestFactor = factor;

	RefreshSlowMo();
}

void CCutsceneInstance::RefreshSlowMo()
{
	if ( GGame )
	{
		GGame->SetTimeScale( m_slowMoCurrFactor, CNAME( Cutscene ), 10000 );
	}
}

Float CCutsceneInstance::GetSlowMo() const
{
	return m_slowMoCurrFactor;
}

Bool CCutsceneInstance::HasBoundingBoxes() const 
{ 
	return m_csTemplate->HasBoundingBoxes(); 
}

void CCutsceneInstance::GetBoundingBoxes( TDynArray< Box >& boxes, Matrix& offset ) const 
{ 
	m_csTemplate->GetBoundingBoxes( boxes ); 
	offset = m_offset;
}

const Matrix& CCutsceneInstance::GetCsPosition() const 
{ 
	return m_offset; 
}

void CCutsceneInstance::SetTime( Float time )
{
	CancelSlowMo();

	Float newTime = Clamp( time, 0.f, m_csDuration );
	SoundSeekTo( newTime );
	if ( newTime > m_csTime )
	{
		Update( newTime - m_csTime, true );
	}
	else
	{
		m_csTime = newTime;
		Update( 0.f, true );
	}
}

Float CCutsceneInstance::GetTime() const 
{ 
	return m_csTime; 
}

Float CCutsceneInstance::GetDuration() const 
{ 
	return m_csDuration; 
}

Float CCutsceneInstance::GetTimeRemaining() const 
{ 
	return m_csTimeRemaining;
}

CCameraComponent* CCutsceneInstance::GetCamera() const
{
	return m_csCurrCamera ? m_csCurrCamera->GetSelectedCameraComponent() : NULL;
}

Bool CCutsceneInstance::HasCamera() const 
{ 
	return m_csCameras.Size() > 0; 
}

Bool CCutsceneInstance::HasActiveCamera() const
{
	return m_csCurrCamera && m_csCurrCamera->IsActive();
}

void CCutsceneInstance::GetAvailableCameras( TDynArray< CCameraComponent* >& cameras ) const
{
	for ( Uint32 i=0; i<m_csCameras.Size(); i++ )
	{
		if ( m_csCameras[i] && m_csCameras[i]->GetSelectedCameraComponent() )
		{
			cameras.PushBack( m_csCameras[i]->GetSelectedCameraComponent() );
		}
	}
}

void CCutsceneInstance::Pause()
{
	ASSERT( m_pauseCount >= 0 );
	
	m_pauseCount++;

	if ( m_pauseCount == 1 )
	{
		PauseAllEffect( true );
	}	

	CSoundEmitter* component = GetSoundEmitterComponent( false );
	if( component )	component->SoundPause();

	for ( auto aIt = m_actorList.Begin(); aIt != m_actorList.End(); ++aIt )
	{
		if ( CEntity* entity = aIt->m_entity.Get() )
		{
			CSoundEmitterComponent* component = entity->GetSoundEmitterComponent( false );
			if( component )	component->SoundPause();
		}
	}

}

void CCutsceneInstance::Unpause()
{
	if ( m_pauseCount > 0 )
	{
		m_pauseCount--;
	}

	if ( m_pauseCount == 0 )
	{
		PauseAllEffect( false );
		m_lastTime= Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().QueueEvent( CNAME( AudioTrackStarted ), EventDataCreator< CCutsceneTemplate* >().Create( m_csTemplate ) );
#endif

	CSoundEmitter* component = GetSoundEmitterComponent( false );
	if( component )	component->SoundResume();

	for ( auto aIt = m_actorList.Begin(); aIt != m_actorList.End(); ++aIt )
	{
		if ( CEntity* entity = aIt->m_entity.Get() )
		{
			CSoundEmitterComponent* component = entity->GetSoundEmitterComponent( false );
			if( component )	component->SoundResume();
		}
	}
}

Bool CCutsceneInstance::IsPause() const 
{ 
	return m_pauseCount > 0; 
}

void CCutsceneInstance::SetTimeMul( Float factor ) 
{ 
	m_timeFactor = factor; 
}

Float CCutsceneInstance::GetTimeMul() const 
{ 
	return m_timeFactor;
}

Bool CCutsceneInstance::CheckActors( const TDynArray< CEntity*>& actors )
{
	Bool ret = true;

	for ( Int32 i=(Int32)m_actorList.Size()-1; i>=0; --i )
	{
		CEntity* entity = m_actorList[i].m_entity.Get();
		if ( entity )
		{
			for ( Uint32 j=0; j<actors.Size(); ++j )
			{
				if ( entity == actors[j] )
				{
					CS_ERROR( TXT("Cutscene: Actor '%ls' is used in two cutscene"), m_actorList[i].m_name.AsChar() );
					m_actorList.Erase( m_actorList.Begin() + i );
					ret = false;
				}
			}
		}
	}

	return ret;
}

CCutsceneTemplate* CCutsceneInstance::GetCsTemplate() const
{
	return m_csTemplate;
}

void CCutsceneInstance::ThrowError( const String& errors ) const
{
	CS_ERROR( TXT("Cutscene error [%s] : %s"), GetName().AsChar(), errors.AsChar() );
}

void CCutsceneInstance::ProcessRemainingActorEvents( Float prevTime, Float currTime, Bool skipping )
{
	// Get all animations
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	// Get all items events from animations
	for( TDynArray< CSkeletalAnimationSetEntry* >::iterator animIter = animations.Begin();
		animIter != animations.End(); ++animIter )
	{
		CSkeletalAnimationSetEntry* animEntry = *animIter;
		ASSERT( animEntry != NULL );

		// Extract actor name
		String actorName = CCutsceneTemplate::GetActorName( animEntry->GetName() );

		// Find actor by name
		for( TDynArray< CutsceneActor >::iterator actorIter = m_actorList.Begin();
			actorIter != m_actorList.End(); ++actorIter	)
		{
			if( actorIter->m_name == actorName )
			{
				const CEntity* actorEntity = actorIter->m_entity.Get();

				if ( !actorEntity )
				{
					continue;
				}

				// Find playable element
				for( TDynArray< CutsceneActorPlayableElement >::iterator elementIter = actorIter->m_playableElements.Begin();
					elementIter != actorIter->m_playableElements.End(); ++elementIter )
				{
					// Find animation
					for ( TDynArray< CName >::iterator animIter = (*elementIter).m_animations.Begin();
						animIter != (*elementIter).m_animations.End(); ++animIter )
					{
						if( *animIter == animEntry->GetName() )
						{
							TDynArray< CAnimationEventFired > events;

							animEntry->GetEventsByTime( prevTime, currTime, 0, 1.0f, &events, NULL );

							CAnimatedComponent* ac = elementIter->Get( *actorIter );
							if ( ac == NULL )
							{
								ThrowError( String::Printf( TXT( "Cutscene actors '%ls' playable element has been destroyed before stopping the cutscene" ), actorName.AsChar() ) );
								continue;
							}


							// Process events
							for( TDynArray< CAnimationEventFired >::const_iterator eventIter = events.Begin();
								eventIter != events.End(); ++eventIter )
							{
								const CAnimationEventFired& eventFired = *eventIter;
								ASSERT( eventFired.m_extEvent != NULL );

								if( skipping && IsType< CExtAnimCutsceneSoundEvent >( eventFired.m_extEvent ) )
								{
									// Do not play sounds, when skipping cutscene
									continue;
								}

								switch( eventFired.m_type )
								{
								case AET_DurationStart:
									static_cast< const CExtAnimDurationEvent* >( eventFired.m_extEvent )->Start( eventFired, ac );
									break;
								case AET_DurationEnd:
									static_cast< const CExtAnimDurationEvent* >( eventFired.m_extEvent )->Stop( eventFired, ac );
									break;
								case AET_Tick:
									eventFired.m_extEvent->Process( eventFired, ac );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

CCutsceneInstance::CameraState CCutsceneInstance::GetCameraState()
{
	CameraState state;
	const CutsceneActor* cameraActor = nullptr;
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		if ( m_actorList[i].m_type == CAT_Camera )
		{
			cameraActor = &m_actorList[i];
		}
	}
	
	if ( cameraActor && !cameraActor->m_playableElements.Empty() &&  !cameraActor->m_playableElements[0].m_animations.Empty() )
	{
		if ( const CAnimatedComponent* ac = cameraActor->m_playableElements[0].Get( *cameraActor ) )
		{		
			SBehaviorGraphOutput pose;	
			CSkeletalAnimationSetEntry* animEntry = ac->GetAnimationContainer()->FindAnimation( cameraActor->m_playableElements[0].m_animations[0] );
			CSkeletalAnimation* anim = animEntry ? animEntry->GetAnimation() : nullptr;
			
			if ( anim && ac->GetBonesNum() > 0 && anim->GetTracksNum() > SBehaviorGraphOutput::FTT_FOV )
			{
				Int32 boneIndex = ac->GetBonesNum() - 1;
				pose.Init( anim->GetBonesNum(), anim->GetTracksNum() );
				anim->Sample( m_csTime, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks  );

				Matrix eyeBone = pose.GetBoneModelMatrix( ac, boneIndex );
				Matrix res = eyeBone * GetCsPosition();

				state.m_position = res.GetTranslation();
				state.m_rotation = res.ToEulerAngles();		
				state.m_fov = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_FOV ];
			}			
		}
	}

	return state;
}

const String& CCutsceneInstance::GetActorName( const CEntity* ent ) const
{	
	for ( Uint32 i=0; i<m_actorList.Size(); ++i )
	{
		CEntity* entity = m_actorList[i].m_entity.Get();
		if ( entity && entity == ent )
		{
			return m_actorList[i].m_name;
		}
	}
	return String::EMPTY;
}

CWorld* CCutsceneInGameWorldInterface::GetWorld() const
{
	return GGame->GetActiveWorld();
}

void CCutsceneInGameWorldInterface::ActivateGameCamera( Float blendTime ) 
{ 
	GGame->ActivateGameCamera( blendTime ); 
}

void CCutsceneInGameWorldInterface::SetBlackscreen( Bool enable, const String& reason, const Color & color ) 
{ 
	if ( !enable )
	{
		GGame->ResetFadeLock( TXT( "CCutsceneInGameWorldInterface_ResetBlackscreen" ) );
	}

	GGame->SetBlackscreen( enable, reason, color ); 

	if ( enable )
	{
		GGame->SetFadeLock( TXT( "CCutsceneInGameWorldInterface_SetBlackscreen" ) );
	}
}

void CCutsceneInGameWorldInterface::Fade( Bool fadeIn, const String& reason, Float fadeDuration, const Color & color  ) 
{ 
	if ( fadeIn )
	{
		GGame->ResetFadeLock( TXT( "CCutsceneInGameWorldInterface_FadeIn" ) );
	}

	GGame->StartFade( fadeIn, reason, fadeDuration ); 

	if ( !fadeIn )
	{
		GGame->SetFadeLock( TXT( "CCutsceneInGameWorldInterface_FadeOut" ) );
	}
}

void CCutsceneInGameWorldInterface::OnCutsceneEvent( const String& csName, const CName& csEvent ) 
{ 
	GGame->OnCutsceneEvent( csName, csEvent ); 
}

void CCutsceneInGameWorldInterface::RegisterCutscene( CCutsceneInstance* cs )
{
	GGame->RegisterCutscene( cs );
}

void CCutsceneInGameWorldInterface::UnregisterCutscene( CCutsceneInstance* cs )
{
	GGame->UnregisterCutscene( cs );
}

//////////////////////////////////////////////////////////////////////////

#ifdef USE_HAVOK_ANIMATION

CCutsceneStreamer::CCutsceneStreamer( TDynArray< CSkeletalAnimationSetEntry* >& anims )
	: m_prevPart( 0 )
	, m_fadeOwner( false )
{
	LOG_ENGINE( TXT("### Start cs streaming") );

	const Uint32 animSize = anims.Size();
	ASSERT( animSize > 0 );

	// 1. Animations
	m_animationsToStream.Resize( animSize );

	TDynArray< Uint32 > animBuffPos;
	animBuffPos.Resize( animSize );

	Uint32 accSize = 0;

	for ( Uint32 i=0; i<animSize; ++i )
	{
		CSkeletalAnimation* skAnim = anims[ i ] ? anims[ i ]->GetAnimation() : NULL;

		m_animationsToStream[ i ] = skAnim;

		if ( !skAnim || !skAnim->HasValidId() )
		{
			HALT( "Error in cs streaming 4" );
		}

		animBuffPos[ i ] = accSize;

		if ( skAnim->GetSizeOfAnimBuffer() == 0 )
		{
			HALT( "Error in cs streaming 5" );
		}

		if ( skAnim->m_animBuffer )
		{
			HALT( "Error in cs streaming 6" );
		}

		accSize += skAnim->GetSizeOfAnimBuffer();
	}

	// 2. Time
	m_time = 0.f;

	// 3. Jobs
	CSkeletalAnimation* anim = m_animationsToStream[ 0 ];
	if ( anim )
	{
		m_streamingJob = new CCutsceneAnimationStreamingJob( anim->GetId(), animBuffPos, animSize );
		SJobManager::GetInstance().Issue( m_streamingJob );
	}
}

CCutsceneStreamer::~CCutsceneStreamer()
{
	if ( m_streamingJob )
	{
		m_streamingJob->Cancel();
		m_streamingJob->Release();
		m_streamingJob = NULL;
	}

	if ( m_fadeOwner )
	{
		FadeOff();
	}
}

void CCutsceneStreamer::KeepFading()
{
	m_fadeOwner = true;
}

void CCutsceneStreamer::FadeOff()
{
	ASSERT( m_fadeOwner );

	//GGame->SetBlackscreen( false, TXT( "CCutsceneInstance_FadeOff" ) );

	m_fadeOwner = false;
}

Bool CCutsceneStreamer::Update( Float time )
{
	if ( m_streamingJob )
	{
		Uint32 currPart = m_streamingJob->GetLastLoadedPartNum();

		while ( m_prevPart < currPart )
		{
			// Next part is ready

			if ( m_prevPart == 0 )
			{
				// Prepare buffers
				PrepareAnimBuffers();

				if ( m_fadeOwner )
				{
					FadeOff();
				}
			}

			const Uint32 size = m_animationsToStream.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				CSkeletalAnimation* anim = m_animationsToStream[ i ];
				const AnimBuffer* animBuff = anim->GetAnimBuffer();

				MoveDataBufferPart( const_cast<AnimBuffer*>(animBuff), i, m_prevPart );

				anim->FillAnimTimes();
			}

			m_time = m_streamingJob->GetAccTime( m_prevPart );

			m_prevPart += 1;
		}

		if ( m_streamingJob->HasEnded() && m_streamingJob->GetAnimPartNum() == m_prevPart )
		{
			m_streamingJob->Release();
			m_streamingJob = NULL;

			DebugFinalCheck();
		}

		// True == waiting
		Bool waiting = time > m_time;
		if ( waiting )
		{
			LOG_ENGINE( TXT("### Waiting for cs streaming") );
		}
		else
		{
			DebugCheck( time );
		}

		return waiting;
	}
	else
	{
		return false;
	}
}

void CCutsceneStreamer::PrepareAnimBuffers()
{
	const Uint32 parts = m_streamingJob->GetAnimPartNum();
	if ( parts == 0 )
	{
		HALT( TXT("Error in cs streaming 7") );
	}

	const Uint32 size = m_animationsToStream.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimation* anim = m_animationsToStream[ i ];

		if ( anim->GetAnimBuffer() )
		{
			HALT( TXT("Error in cs streaming 8") );
		}

		anim->CreateAnimations( parts );
	}
}

void CCutsceneStreamer::MoveDataBufferPart( AnimBuffer* buff, Uint32 anim, Uint32 part )
{
	if ( !buff )
	{
		HALT( TXT("Error in cs streaming 9") );
	}

	if ( part >= buff->m_animNum )
	{
		HALT( TXT("Error in cs streaming 10") );
	}
#ifdef USE_HAVOK_ANIMATION
	tHavokAnimationBuffer& dest = buff->m_animations[ part ];

	m_streamingJob->MoveLoadedBuffer( dest, anim, part );
#else
	HALT( TXT( "Needs to be implemented" ) );
#endif
}

void CCutsceneStreamer::DebugCheck( Float time ) const
{
	// Are all animations loaded?
	const Uint32 size = m_animationsToStream.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimation* anim = m_animationsToStream[ i ];
		
		if ( anim->IsLoaded() && !anim->HasLoadedBuffAtTime( time ) )
		{
			HALT( TXT("Error in cs streaming 11") );
		}
	}
}

void CCutsceneStreamer::DebugFinalCheck() const
{
	const Uint32 size = m_animationsToStream.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimation* anim = m_animationsToStream[ i ];
		const AnimBuffer* animBuff = anim->GetAnimBuffer();

		if ( animBuff )
		{
			for ( Uint32 i=0; i<animBuff->m_animNum; ++i )
			{
#ifdef USE_HAVOK_ANIMATION
				if ( !animBuff->m_animations[ i ].GetHavokObject() )
				{
					HALT( TXT("Error in cs streaming 12") );
				}
#endif
			}
		}
	}
}

CCutsceneAnimationStreamingJob::CCutsceneAnimationStreamingJob( Uint32 firstAnimId, const TDynArray< Uint32 >& animBuffPos, Uint32 animNums )
	: ILoadJob( JP_Cutscene )
	, m_firstAnimId( firstAnimId )
	, m_animBuffPos( animBuffPos )
	, m_animNums( animNums )
	, m_animParts( 0 )
	, m_lastLoadedPart( 0 )
{
	
}

CCutsceneAnimationStreamingJob::~CCutsceneAnimationStreamingJob()
{
	m_buffers.ClearPtr();
}

EJobResult CCutsceneAnimationStreamingJob::Process()
{
	IFileLatentLoadingToken* loadingToken = SAnimationsCache::GetInstance().CreateLoadingToken( m_firstAnimId );
	if ( !loadingToken )
	{
		WARN_ENGINE( TXT("CSstremaing: no source loading token. Failed.") );
		return JR_Failed;
	}

	// Resume file reading
	IFile* sourceFile = loadingToken->Resume(0);
	if ( !sourceFile )
	{
		delete loadingToken;
		WARN_ENGINE( TXT("CSstremaing: unable to resume loading from '%ls'."), loadingToken->GetDebugInfo().AsChar() );
		return JR_Failed;
	}

	// Delete loading token, it's not needed any more
	delete loadingToken;

	// Create buffers
	m_buffers.Resize( m_animNums );
	m_animOffsets.Resize( m_animNums );

	for ( Uint32 i=0; i<m_animNums; ++i )
	{
		m_buffers[ i ] = new AnimBuffer();
		m_animOffsets[ i ] = 0;
	}

	if ( m_buffers[ 0 ]->m_animNum != 0 )
	{
		HALT( TXT("Error in cs streaming 13") );
	}

	const Uint32 startingPos = static_cast< Uint32 >( sourceFile->GetOffset() );

	// First loop - get anim sizes and first buffers part
	for ( Uint32 i=0; i<m_animNums; ++i )
	{
		sourceFile->Seek( startingPos + m_animBuffPos[ i ] );

		Uint32 posA = static_cast< Uint32 >( sourceFile->GetOffset() );

		Uint32 num = 0;
		*sourceFile << num;

		if ( m_animParts == 0 )
		{
			m_animParts = num;

			m_accTimes.Resize( m_animParts );
			for ( Uint32 i=0; i<m_animParts; ++i )
			{
				m_accTimes[ i ] = 0;
			}

		}

		if ( m_animParts != num || num == 0 )
		{
			HALT( TXT("Error in cs streaming 14") );
		}

		AnimBuffer* buff = m_buffers[ i ];

		buff->m_animNum = num;
#ifdef USE_HAVOK_ANIMATION
		buff->m_animations = new tHavokAnimationBuffer[ buff->m_animNum ];

		buff->m_animations[ 0 ].Serialize( *sourceFile );
#else
		HALT( TXT( "Needs to be implemented" ) );
#endif
		Uint32 posB = static_cast< Uint32 >( sourceFile->GetOffset() );

		Uint32 dataSize = posB - posA;

		m_animOffsets[ i ] += dataSize;
	}

	{
#ifdef USE_HAVOK_ANIMATION
		tHavokAnimationBuffer* data = const_cast< tHavokAnimationBuffer* >( &(m_buffers[ 0 ]->m_animations[ 0 ]) );
		hkaAnimation* animation = data->GetHavokObject();
		m_accTimes[ 0 ] = animation->m_duration;
#else
		HALT( TXT( "Needs to be implemented" ) );
#endif
	}

	m_lastLoadedPart += 1;

	// Load all parts one by one
	while ( m_lastLoadedPart < m_animParts )
	{
		const Uint32 partNum = m_lastLoadedPart;

		for ( Uint32 i=0; i<m_animNums; ++i )
		{
			AnimBuffer* buff = m_buffers[ i ];


			Uint32 posA = startingPos + m_animBuffPos[ i ] + m_animOffsets[ i ];
			
			sourceFile->Seek( posA );
#ifdef USE_HAVOK_ANIMATION
			buff->m_animations[ partNum ].Serialize( *sourceFile );
#else
			HALT( TXT( "Needs to be implemented" ) );
#endif
			Uint32 posB = static_cast< Uint32 >( sourceFile->GetOffset() );


			Uint32 dataSize = posB - posA;

			m_animOffsets[ i ] += dataSize;
		}

		{
			if ( partNum - 1 < 0 )
			{
				HALT( TXT("Error in cs streaming 15") );
			}
#ifdef USE_HAVOK_ANIMATION
			tHavokAnimationBuffer* data = const_cast< tHavokAnimationBuffer* >( &(m_buffers[ 0 ]->m_animations[ partNum ]) );
			hkaAnimation* animation = data->GetHavokObject();
			m_accTimes[ partNum ] = m_accTimes[ partNum - 1 ] + animation->m_duration;
#else
			HALT( TXT( "Needs to be implemented" ) );
#endif
		}

		m_lastLoadedPart += 1;
	}


	// Check
	for ( Uint32 i=0; i<m_animNums-1; ++i )
	{
		Uint32 posA = m_animBuffPos[ i ] + m_animOffsets[ i ];
		Uint32 posB = m_animBuffPos[ i+1 ];

		if ( posA != posB )
		{
			LOG_ENGINE( TXT("Error in cs streaming 16") );
		}
	}

	// Close streaming file
	delete sourceFile;

	// Finished
	return JR_Finished;
}

Uint32 CCutsceneAnimationStreamingJob::GetLastLoadedPartNum() const
{
	return m_lastLoadedPart;
}

Uint32 CCutsceneAnimationStreamingJob::GetAnimPartNum() const
{
	return m_animParts;
}

Float CCutsceneAnimationStreamingJob::GetAccTime( Uint32 part ) const
{
	if ( part >= m_accTimes.Size() )
	{
		HALT( TXT("Error in cs streaming -1") );
	}
	return m_accTimes[ part ];
}
#ifdef USE_HAVOK_ANIMATION
void CCutsceneAnimationStreamingJob::MoveLoadedBuffer( tHavokAnimationBuffer& dest, Uint32 anim, Uint32 part )
{
	if ( m_buffers.Empty() || anim >= m_buffers.Size() || part >= m_buffers[ anim ]->m_animNum )
	{
		HALT( TXT("Error in cs streaming 17") );
	}

	tHavokAnimationBuffer& src = m_buffers[ anim ]->m_animations[ part ];

	if ( !src.GetHavokObjectData() )
	{
		HALT( TXT("Error in cs streaming 18") );
	}

	dest.MoveHandleWithObject( src );
}
#endif

#endif

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
