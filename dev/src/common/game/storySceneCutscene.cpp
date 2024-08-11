#include "build.h"
#include "storySceneCutscene.h"
#include "storySceneCutsceneSection.h"
#include "extAnimItemEvents.h"
#include "../engine/extAnimCutsceneSoundEvent.h"
#include "../engine/soundStartData.h"
#include "storyScenePlayer.h"
#include "storySceneVoicetagMapping.h"
#include "sceneLog.h"
#include "../core/jobGenericJobs.h"
#include "../core/loadingJobManager.h"
#include "../../common/core/depot.h"
#include "../engine/soundFileLoader.h"
#include "../engine/tagManager.h"
#include "../engine/pathlibWorld.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

// DIALOG_TOMSIN_TODO - tu jest jakis mega krap!!!

IMPLEMENT_ENGINE_CLASS( CStorySceneCutscenePlayer )

CStorySceneCutscenePlayer::CStorySceneCutscenePlayer()
	: m_displayDesc( true )
{

}

Bool CStorySceneCutscenePlayer::CanBeSkipped() const
{
	return TBaseClass::CanBeSkipped() && GetSection()->CanSectionBeSkipped();
}

IStorySceneElementInstanceData* CStorySceneCutscenePlayer::OnStart( CStoryScenePlayer* player ) const
{
	Bool looped = SafeCast< CStorySceneCutsceneSection >( GetSection() )->IsLooped();
	Bool clearActorsHands = true;

	if ( player->CanUseCutscene() )
	{
		return new StorySceneCutscenePlayerInstanceData( this, player, m_cutscene, looped, clearActorsHands );
	}
	else
	{
		return new StorySceneCutscenePlayerInstanceData_Fake( this, player );
	}
}

void CStorySceneCutscenePlayer::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}

Bool CStorySceneCutscenePlayer::IsPlayable() const
{ 
	CStorySceneSection* s = this->GetSection();
	return s->GetChoice() == nullptr; 
}

Bool CStorySceneCutscenePlayer::GetCsPoint( Matrix& point, const CWorld* world ) const
{
	return GetCsPoint( SafeCast< CStorySceneCutsceneSection >( GetSection() ), point, world );
}

Bool CStorySceneCutscenePlayer::GetCsPoint( const CStorySceneCutsceneSection* section, Matrix& point, const CWorld* world )
{
	// DIALOG_TOMSIN_TODO
	if ( !GGame || !GGame->GetActiveWorld() || world != GGame->GetActiveWorld() )
	{
		point = Matrix::IDENTITY;
		return true;
	}

	const TagList& sectionTags = section->GetTagPoint();
	if ( !sectionTags.Empty() )
	{
		TDynArray< CNode* > nodes;
		world->GetTagManager()->CollectTaggedNodes( sectionTags, nodes );

		if ( !nodes.Empty() )
		{
			point = nodes[0]->GetRotation().ToMatrix();
			point.SetTranslation( nodes[0]->GetPosition() );
			return true;
		}
		else
		{
			SCENE_WARN( TXT("CStorySceneCutscenePlayer: Couldn't find node tag %s"), sectionTags.ToString().AsChar() );
		}
	}

	CCutsceneTemplate* csTemplate = section->GetCsTemplate();
	ASSERT( csTemplate );

	if ( !csTemplate->GetPointTag().Empty() )
	{
		const TagList& tags = csTemplate->GetPointTag();

		TDynArray< CNode* > nodes;
		world->GetTagManager()->CollectTaggedNodes( tags, nodes );

		if ( !nodes.Empty() )
		{
			point = nodes[0]->GetRotation().ToMatrix();
			point.SetTranslation( nodes[0]->GetPosition() );
			return true;
		}
		else
		{
			SCENE_WARN( TXT("CStorySceneCutscenePlayer: Couldn't find node tag %s"), tags.ToString().AsChar() );
		}
	}

	return false;
}

/*
Calculates cutscene duration (in seconds).

\param locale For cutscenes this argument is ignored as cutscene duration is the same in all locales.
\return Cutscene duration in seconds.
*/
Float CStorySceneCutscenePlayer::CalculateDuration( const String& /* locale */ ) const
{
	return ( m_cutscene != NULL ) ? m_cutscene->GetDuration() : StorySceneCutscenePlayerInstanceData::DEFAULT_DURATION;
}

const Float StorySceneCutscenePlayerInstanceData::DEFAULT_DURATION = 5.f;

StorySceneCutscenePlayerInstanceData::StorySceneCutscenePlayerInstanceData( const CStorySceneCutscenePlayer* csPlayer, CStoryScenePlayer* player, CCutsceneTemplate* csTemplate, Bool looped, Bool clearActorsHands )
	: IStorySceneElementInstanceData( csPlayer, player )
	, m_timer( 0.f )
	, m_showDesc( false )
	, m_cutscene( NULL )
	, m_isGameplay( player->IsGameplay() )
	, m_clearActorsHands( clearActorsHands )
	, m_isLooped( looped )
	, m_csPlayer( csPlayer )
	, m_csTemplate( csTemplate )
{
	m_desc = m_csTemplate && m_csTemplate->GetFile() ? m_csTemplate->GetFriendlyName() + TXT("\n") + m_csPlayer->GetDescriptionText() : m_csPlayer->GetDescriptionText();
	m_asyncTick = m_player->IsCutsceneAsyncTick();
}

StorySceneCutscenePlayerInstanceData::~StorySceneCutscenePlayerInstanceData()
{
	// Destroy cutscene if it was not played
	if ( m_cutscene )
	{
		m_cutscene->DestroyCs();
		m_cutscene = NULL;
	}
}

Bool StorySceneCutscenePlayerInstanceData::OnInit( const String& /* locale */ )
{
	if ( !m_csTemplate )
	{
		m_desc += TXT("\nError(s):\nStorySceneCutscene: cutscene template not specifed");
	}

	return true;
}

void StorySceneCutscenePlayerInstanceData::OnPlay()
{
	if ( m_csTemplate && !m_cutscene )
	{
		SCENE_LOG( TXT("StorySceneCutscene: Create cutscene '%ls'"), m_csTemplate ? m_csTemplate->GetFriendlyName().AsChar() : TXT("<unknown>") );
		String errors;
		Matrix point = Matrix::IDENTITY;

		if ( !m_csPlayer->GetCsPoint( point, m_player->GetLayer()->GetWorld() ) )
		{
			errors = String::Printf( TXT("StorySceneCutscene: Couldn't create cutscene '%ls'. Story scene didn't find tag."), m_csTemplate->GetFriendlyName().AsChar() ); 

			m_desc += TXT("\nError(s):\n") + errors;
			SCENE_ERROR( errors.AsChar() );
		}

		String posErrors;
		if ( !IsCsPositionValid( m_csTemplate, point, posErrors, m_player->GetLayer()->GetWorld() ) )
		{
			errors = String::Printf( TXT("StorySceneCutscene: Couldn't create cutscene '%ls'.\nActors positions are not valid:\n%s\nCheck path engine stuff."), m_csTemplate->GetFriendlyName().AsChar(), posErrors.AsChar() ); 

			m_desc += TXT("\nError(s):\n") + errors;
			SCENE_ERROR( errors.AsChar() );
		}

		// DIALOG_TOMSIN_TODO
		CLayer* layer = m_player->GetLayer();

		m_cutscene = m_csTemplate->CreateInstance( layer, point, errors, this, m_player->GetCsWorldInterface(), false, m_player->GetCsSceneInterface() );

		if ( m_cutscene == NULL )
		{
			SCENE_ERROR( TXT("StorySceneCutscene: Couldn't create cutscene %s.\n%s"), m_csTemplate->GetFriendlyName().AsChar(), errors.AsChar() );
			m_desc += TXT("\nError(s):\n") + errors;
		}
		else
		{
			m_cutscene->SetIsGameplay( GetElement()->GetSection()->IsGameplay() );

			m_cutscene->InitStageOne( point, this, m_player->GetCsWorldInterface(), false, errors );
			RegisterActors();

			m_desc += errors;

			// Start loading item resources that will be used during this cutscene
			PreprocessItems();
		}
	}

	if ( m_cutscene )
	{
		String errors;
		m_cutscene->InitStageTwo( errors );
		
		// DIALOG_TOMSIN_TODO
		m_cutscene->GetLayer()->GetWorld()->DelayedActions();
		

		SCENE_LOG( TXT("StorySceneCutscene: Play cutscene '%ls'"), m_cutscene->GetFriendlyName().AsChar() );

		if( ! m_isGameplay )
		{
			PrepareSoundsForCutscene();
		}

		m_player->CallEvent( CNAME( OnCutsceneStarted ) );

		// Play!
		m_cutscene->Play( false, false, m_isLooped );
		
		m_player->OnCutscenePlayerStarted( m_cutscene );
	}
}

void StorySceneCutscenePlayerInstanceData::OnPaused( Bool flag )
{
	// m_cutscene may be nullptr if cutscene section is not yet associated with any cutscene (because it may have been just created)
	if( !m_cutscene )
	{
		return;
	}

	if ( flag )
	{
		m_cutscene->Pause();
	}
	else
	{
		m_cutscene->Unpause();
	}
}

void StorySceneCutscenePlayerInstanceData::OnStop()
{
	SCENE_LOG( TXT("StorySceneCutscene: Cutscene Finished '%ls'"), m_cutscene ? m_cutscene->GetName().AsChar() : TXT("<unknown>") );

	// Destroy cutscene
	if ( m_cutscene )
	{
		if( ! m_isGameplay )
		{
			RestoreSoundsAfterCutscene();
		}
		
		m_player->CallEvent( CNAME( OnCutsceneEnded ) );
		
		m_cutscene->ProcessRemainingEvents();
		
		RestoreItemsState();

		const CCamera* cutsceneCamera = m_cutscene->GetCsCamera();

		m_player->OnCutscenePlayerEnded( m_cutscene );

		UnregisterActors();

		m_cutscene->DestroyCs( !cutsceneCamera );
		m_cutscene = NULL;
	}
	else
	{
		if ( m_showDesc )
		{
			m_player->SceneFadeIn( TXT("Cutscene") );
			m_showDesc = false;
		}

#ifndef RED_FINAL_BUILD
		HideDesc();
#endif
	}

	// Skip all elements in section
	//m_player->SignalForceSkipSection();
}

Bool StorySceneCutscenePlayerInstanceData::IsReady() const
{
	if ( m_cutscene != NULL )
	{
		CCutsceneTemplate* csTemplate = m_cutscene->GetCsTemplate();
		const TDynArray< CName > banksDependency = csTemplate->GetBanksDependency();
		Uint32 banksCount = banksDependency.Size();
		for( Uint32 i = 0; i != banksCount; ++i )
		{
			CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
			if( !soundBank ) continue;

			if( !soundBank->IsLoadingFinished() ) return false;
			RED_ASSERT( soundBank->IsLoaded(), TXT("StorySceneCutscenePlayerInstanceData::IsReady() - sound bank didn't load properly - bank: [%ls] - result [%ls] - sceneDesc [%ls]"), 
				soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar(), m_desc.AsChar() );
		}

		String errors;
		return m_cutscene->AreActorsReady( errors );
	}
	return true;
}

void StorySceneCutscenePlayerInstanceData::PrepareSoundsForCutscene()
{
	CCutsceneTemplate* csTemplate = m_cutscene->GetCsTemplate();
	ASSERT( csTemplate != NULL );

	// Set reverb definition
	if( ! csTemplate->GetReverbName().Empty() )
	{
		m_player->SoundForceReverb( csTemplate->GetReverbName() );
	}

	const TDynArray< CName > banksDependency = csTemplate->GetBanksDependency();
	Uint32 banksCount = banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->QueueLoading(); 
	}
}

void StorySceneCutscenePlayerInstanceData::RestoreSoundsAfterCutscene()
{
	CCutsceneTemplate* csTemplate = m_cutscene->GetCsTemplate();
	ASSERT( csTemplate != NULL );

	// Cancel reverb definition
	if( ! csTemplate->GetReverbName().Empty() )
	{
		m_player->SoundUpdateAmbientPriorities();
	}

	const TDynArray< CName > banksDependency = csTemplate->GetBanksDependency();
	Uint32 banksCount = banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
		if( !soundBank ) continue;

		soundBank->Unload();
	}

}

void StorySceneCutscenePlayerInstanceData::RestoreItemsState()
{
	m_player->RegisterSpawnedItems( m_spawnedItems );
}

void StorySceneCutscenePlayerInstanceData::PreprocessItems()
{
	TDynArray< const CExtAnimItemEvent* > events;
	CCutsceneTemplate* templ = m_cutscene->GetCsTemplate();
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	templ->GetAnimations( animations );

	for( TDynArray< CSkeletalAnimationSetEntry* >::iterator animIter = animations.Begin(); animIter != animations.End(); ++animIter )
	{
		CSkeletalAnimationSetEntry* animEntry = *animIter;
		ASSERT( animEntry );
		SCutsceneActorDef* def = templ->GetActorDefinition( templ->GetActorName( animEntry->GetName() ) );
		CEntity* ent = m_player->GetSceneActorEntity( def->m_voiceTag );
		if( CActor* actor = Cast<CActor>( ent ) )
		{
			animEntry->GetEventsOfType( events );
			for( TDynArray< const CExtAnimItemEvent* >::const_iterator eventIter = events.Begin(); eventIter != events.End(); ++eventIter )
			{
				const CExtAnimItemEvent* event = *eventIter;				
				SItemUniqueId spawnedItem = SItemUniqueId::INVALID;
				CName ignoreTag = event->GetIgnoreTag();

				if ( event->GetAction() == IA_Unmount )
				{
					continue;
				}
				if ( CName itemName = event->GetItemName() )
				{
					CExtAnimItemEvent::PreprocessItem( actor, spawnedItem, itemName, ignoreTag );										
				}
				else if( CName itemCat = event->GetCategory() )
				{
					CExtAnimItemEvent::PreprocessItem( actor, spawnedItem, itemCat, ignoreTag );										
				}				
				if ( spawnedItem )
				{
					m_spawnedItems.PushBack( MakePair( def->m_voiceTag, spawnedItem ) );
				}
			}
		}		
		events.ClearFast();
	}
}

void StorySceneCutscenePlayerInstanceData::RegisterActors()
{
	if ( m_cutscene )
	{
		TDynArray< TPair< CEntity*, Bool > > spawnEnt;

		m_cutscene->GetNonSceneActors( spawnEnt );

		for ( Uint32 i=0; i<spawnEnt.Size(); ++i )
		{
			RegisterCsActor( spawnEnt[ i ].m_first, spawnEnt[ i ].m_second );
		}
	}
}

void StorySceneCutscenePlayerInstanceData::UnregisterActors()
{
	for ( Uint32 i=0; i<m_regActors.Size(); ++i )
	{
		UnregisterCsActor( m_regActors[i].m_first, m_regActors[i].m_second.Get() );
	}
	m_regActors.Clear();
}

void StorySceneCutscenePlayerInstanceData::RegisterCsActor( CEntity* ent, Bool spawned )
{
	CActor* actor = Cast< CActor >( ent );
	if( actor )
	{
		actor->SetSceneLock( true, m_player->IsGameplay(), Int8( m_player->GetSceneController()->m_priority ) );
		actor->AddStoryScene( m_player->GetSceneController() );
		actor->SetCrossSafeZoneEnabled( true );
		actor->SetForceNoLOD( true );

		CName actorId = actor->GetVoiceTag() ? actor->GetVoiceTag() : CName( m_cutscene->GetActorName( ent ) );

		// Register actor.
		//
		// Skip registration if CStoryScenePlayer already uses the same voicetag - registering it would have
		// no effect. This happens when cutscene spawns some actor using actor definition that doesn't specify
		// voicetag and that spawned actor happens to use voicetag that is already used by CStoryScenePlayer
		// (because other parts of scene use it or because the same cutscene already spawned some other actor
		// with this voicetag).
		//
		// Skipping actor registration doesn't seem ok but it's like this for a long time (see RegisterActor()).
		// If this didn't cause any issues till now then maybe we could just skip registration altogether?
		// There probably was some reason to do this so I wouldn't remove it carelessly. Also, some of our
		// existing bugs may be caused by this but we don't know it yet.
		if( const Bool actorIdAvailable = m_player->GetMappedActor( actorId ) == nullptr )
		{
			m_player->RegisterActor( actorId, actor );
		}
		else
		{
			WARN_GAME( TXT("StorySceneCutscenePlayerInstanceData::RegisterCsActor(): skipping actor registration - actor id is already registered. Actor id: %s."), actorId.AsString().AsChar() );
		}

		m_regActors.PushBack( MakePair( actor->GetVoiceTag(), THandle< CEntity >( actor ) ) );
	}
	else if ( ent )
	{
		CName propId( m_cutscene->GetActorName( ent ) );
		m_player->RegisterProp( propId, ent );
		m_regActors.PushBack( MakePair( propId, THandle< CEntity >( ent ) ) );
	}
}

void StorySceneCutscenePlayerInstanceData::UnregisterCsActor( CName id, CEntity* ent )
{
	CActor* actor = Cast< CActor >( ent );
	if( actor )
	{
		actor->CancelSpeech();
		actor->SetSceneLock( false, m_player->IsGameplay(), Int8( m_player->GetSceneController()->m_priority ) );
		actor->RemoveStoryScene( m_player->GetSceneController() );
		actor->SetCrossSafeZoneEnabled( false );
		actor->SetForceNoLOD( false );
		
		// Unregister actor. Skip it if CStoryScenePlayer already uses the same voicetag for different actor
		// as it would unregister that other actor and not the one we want to unregister. Note that in such
		// case our actor wasn't really registered in the first place - see RegisterCsActor() for more info.
		CActor* mappedActor = m_player->GetMappedActor( id );
		if( actor == mappedActor )
		{
			m_player->UnregisterActor( id );
		}
	}
	else if ( ent )
	{
		m_player->UnregisterProp( id );
	}
}


CEntity* StorySceneCutscenePlayerInstanceData::GetCutsceneActorExtContext( const String& name, CName voiceTag )
{
	if ( ! voiceTag )
	{
		const Uint32 size = m_player->GetSceneController()->m_externalContextActors.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CEntity* ent = m_player->GetSceneController()->m_externalContextActors[ i ].Get();
			if ( ent && ent->GetName() == name )
			{
				return ent;
			}
		}
	}

	return nullptr;
}

CEntity* StorySceneCutscenePlayerInstanceData::GetCutsceneActor( const CName& voiceTag )
{
	// Get voiceTag
	if ( voiceTag == CName::NONE )
	{
		return NULL;
	}

	CActor* actor = m_player->GetMappedActor( voiceTag );

	//HACK do not spawn 2nd player
	Bool isPlayer = false;
	if ( actor == NULL )
	{
		CName actorAppearanceName;
		Bool canSearchInCommunity;
		THandle<CEntityTemplate> templ;
		GetPlayer()->GetCurrentStoryScene()->GetActorSpawnDefinition( voiceTag, templ, actorAppearanceName, canSearchInCommunity );
		isPlayer = templ ? templ->GetEntityObject()->IsA( CPlayer::GetStaticClass() ) : false;
	}

	if ( actor == NULL && !isPlayer && !m_player->IsActorOptional( voiceTag ) )
	{
		actor = Cast< CActor >( m_player->GetMappedActor( voiceTag ) );
		if ( !actor )
		{
			actor = Cast< CActor >( m_player->SpawnSceneActorEntity( voiceTag ) );
		}
		if ( actor != NULL && m_player->IsGameplay() == false )
		{
			actor->SetHideInGame( true, false, CEntity::HR_Scene );
		}
	}
	return actor;
}

SCutsceneActorDef* StorySceneCutscenePlayerInstanceData::GetActorDefinition( const String& actorName ) const
{
	CStorySceneCutsceneSection* cutsceneSection = Cast< CStorySceneCutsceneSection >( m_element->GetSection() );
	if ( cutsceneSection != NULL )
	{
		return cutsceneSection->FindCutsceneActorOverrideDefinition( actorName );
	}
	return NULL;
}

Bool StorySceneCutscenePlayerInstanceData::OnTick( Float timeDelta )
{
	if ( m_cutscene )
	{
		if ( IsSkipped() )
		{
			m_cutscene->SkipToEnd();
		}
		else
		{
			if ( m_asyncTick )
			{
				m_cutscene->UpdateAsync();
				m_currentTime = m_cutscene->GetTime();
			}
			else
			{
				m_cutscene->Update( timeDelta, true );
			}
		}

		CCutsceneInstance::CameraState camState = m_cutscene->GetCameraState();
		m_player->GetSceneDirector()->SynchronizeCameraWithCS( camState );

		if ( m_cutscene->IsFinished() )
		{
			//Float time = m_cutscene->GetTimeRemaining();
			return false;
		}
	}
	else
	{

		if ( !m_showDesc && m_timer < DEFAULT_DURATION )
		{
			m_player->SceneFadeOut( TXT("Cutscene") );
			m_showDesc = true;
			
#ifndef RED_FINAL_BUILD
			ShowDesc( m_desc );
#endif
		}

		m_timer += timeDelta;

		if ( m_timer > DEFAULT_DURATION )
		{
			if ( m_showDesc )
			{
				m_player->SceneFadeIn( TXT("Cutscene") );
				m_showDesc = false;
			}

#ifndef RED_FINAL_BUILD
			HideDesc();
#endif

			//m_player->OnSceneElementFinished( this );
			return false;
		}
	}

	return true;
}

Float StorySceneCutscenePlayerInstanceData::OnSeek( Float newTime )
{
	Float ret = newTime;

	if ( m_cutscene )
	{
		m_cutscene->SetTime( newTime );

		m_cutscene->CsToGameplayRequest( 0.f,  false );

		ret = m_cutscene->GetTime();
	}

	return ret;
}

#ifndef RED_FINAL_BUILD
void StorySceneCutscenePlayerInstanceData::ShowDesc( const String& text )
{

	String textProcessed;
	TDynArray<String> lines = text.Split( TXT("\n") , false);
	for( unsigned int i = 0 ; i < lines.Size() ; ++i)
	{
		while( lines[i].GetLength() > 200 )
		{
			textProcessed += lines[i].LeftString(200) ;
			lines[i] = lines[i].MidString(200);
			textProcessed += TXT("\n"); 
		}
		textProcessed += lines[i];
		textProcessed += TXT("\n");
	}
	m_player->SetDebugScreenText(textProcessed);
}

void StorySceneCutscenePlayerInstanceData::HideDesc()
{
	m_player->ClearDebugScreenText();

}
#endif

Bool StorySceneCutscenePlayerInstanceData::IsCsPositionValid( CCutsceneTemplate* csTemplate, const Matrix& offset, String& errorMsg, const CWorld* world ) const
{
	Bool ret = true;

	if ( csTemplate->CheckActorsPosition() )
	{
		TDynArray< Matrix > endingPos;
		TDynArray< String > endingPosActorName;
		csTemplate->GetActorsFinalPositions( endingPos, &offset, true, &endingPosActorName );

		//Vector rootPos = offset.GetTranslation();

		for ( Uint32 i=0; i<endingPos.Size(); ++i )
		{
			const Vector actorPos = endingPos[i].GetTranslation();

			if ( !world->GetPathLibWorld()->TestLocation( actorPos.AsVector3(), PathLib::CT_DEFAULT ) )
			{
				errorMsg += String::Printf( TXT("Final point for '%ls'\n"), endingPosActorName[i].AsChar() );
				ret = false;
			}
		}
	}

	return ret;
}

void StorySceneCutscenePlayerInstanceData::MarkSkipped()
{
	if ( IsSkipped() )
	{
		return;
	}

	IStorySceneElementInstanceData::MarkSkipped();
}

Bool StorySceneCutscenePlayerInstanceData::ShouldConfirmSkip() const
{
	return true;
}

void StorySceneCutscenePlayerInstanceData::GetUsedEntities( TDynArray< CEntity* >& usedEntities ) const
{
	if ( m_cutscene != NULL )
	{
		m_cutscene->GetActors( usedEntities );
	}
}

Bool StorySceneCutscenePlayerInstanceData::IsActorOptional( CName id ) const 
{
	return m_player->IsActorOptional( id );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
