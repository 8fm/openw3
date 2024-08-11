/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorSpeech.h"
#include "storySceneSystem.h"
#include "storySceneLine.h"
#include "../../common/core/depot.h"
#include "../engine/localizationManager.h"
#include "../engine/soundSettings.h"

const Float CActorSpeech::NO_TEXT_DISTANCE		= 50.0f;
const Float CActorSpeech::NO_MIMIC_DISTANCE		= 15.0f;
const Float CActorSpeech::NO_VOICE_DISTANCE		= 50.0f;
const Float CActorSpeech::NO_GESTURE_DISTANCE	= 70.0f;

const Float CActorSpeech::VO_PLAYER_DISTANCE_UPDATE_FREQUENCY = 0.25f;

const Float	CActorSpeech::WAIT_FOR_MIMIC_LOW	= 0.5f;
const Float	CActorSpeech::WAIT_FOR_MIMIC_HIGH	= 0.5f;

const StringAnsi CActorSpeech::VO_EVENT_DEFAULT( "vo_2d" );
const StringAnsi CActorSpeech::VO_EVENT_DEFAULT_GAMEPLAY( "vo_3d_long" );
const StringAnsi CActorSpeech::VO_EVENT_CINEMATIC( "vo_3d_quest_long" );
const StringAnsi CActorSpeech::VO_EVENT_PARAMETER( "vo_head_mute" );
const StringAnsi CActorSpeech::VO_PLAYER_DISTANCE_PARAMETER( "vo_player_distance" );

ActorSpeechData::ActorSpeechData( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion, Bool alternativeUI )
	: m_speechId( speechId )
	, m_soundEventName( soundEventName )
	, m_sync( sync )
	, m_modeFlags( modeFlags )
	, m_progress( 0.f )
	, m_sceneDisplay( NULL )
	, m_disabledOcclusion( disabledOcclusion )
	, m_alternativeUI( alternativeUI )
	
{
	
}

void ActorSpeechData::Init( Uint32 speechId, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion, Bool alternativeUI )
{
	m_speechId = speechId;
	m_soundEventName = soundEventName;
	m_sync = sync;
	m_modeFlags = modeFlags;
	m_progress = 0;
	m_sceneDisplay = nullptr;
	m_disabledOcclusion = disabledOcclusion;
	m_alternativeUI = alternativeUI;
}

ActorSpeechData::ActorSpeechData( const String& stringKey, const StringAnsi& soundEventName, Bool sync, Int32 modeFlags, Bool disabledOcclusion, Bool alternativeUI )
	: m_speechId( 0 )
	, m_soundEventName( *( const_cast< StringAnsi* >( &soundEventName ) ) )
	, m_sync( sync )
	, m_modeFlags( modeFlags )
	, m_progress( 0.f )
	, m_sceneDisplay( NULL )
	, m_disabledOcclusion( disabledOcclusion )
	, m_alternativeUI( alternativeUI )
{
	if ( !stringKey.Empty() ) 
	{
		m_speechId = SLocalizationManager::GetInstance().GetStringIdByStringKey( stringKey );
	}
}

CActorSpeech::CActorSpeech( ISceneActorInterface* actor, const ActorSpeechData& setup )
	: m_speechId( setup.m_speechId )
	, m_actor( actor )
	, m_loaded( false )
	, m_playing( false )
	, m_finished( false )
	, m_isSync( setup.m_sync )
	, m_modeFlags( setup.m_modeFlags )
	, m_soundEventName( setup.m_soundEventName )
	, m_disabledOcclusion( setup.m_disabledOcclusion )
	, m_alternativeUI( setup.m_alternativeUI )
	, m_duration( 3.0f )
	, m_progress( setup.m_progress )
	, m_distanceParameterUpdateTimer( 0.0f )
	, m_lipsyncAnimationEntry( NULL )
	, m_waitForMimic( 0.f )
	, m_languagePack( NULL )
	, m_soundWasPlayed( false )
	, m_dialogVoicePlayed( false )
	, m_dialogVoicePlayingError( false )
	, m_updateDistanceParameter( false )
	, m_sceneDisplay( setup.m_sceneDisplay )
	, m_dialogVoiceID( 0 )
	, m_isLipsyncPlayed( false )
{
	if ( m_actor != NULL )
	{
		ValidateModes();
	}

	// Default for now
	if ( !m_sceneDisplay )
	{
		CStorySceneSystem* system = GCommonGame->GetSystem< CStorySceneSystem > ();
		if ( system )
		{
			m_sceneDisplay = system->GetSceneDisplay();
		}
	}
	
	Load();
}

CActorSpeech::~CActorSpeech()
{
	if ( m_isLipsyncPlayed )
	{
		RED_ASSERT( HasLipsync() );
		StopLipsync();
	}

	ASSERT( !m_dialogVoicePlayed || m_callbackData.IsEventCompleted() );

	CEntity* entity = m_actor->GetSceneParentEntity();
	if ( entity )
	{
		StringAnsi eventName( m_soundEventName );
		if( eventName.Empty() )
		{
			if ( IsGameplay() == true && IsSubtitle() == false )
			{
				eventName = VO_EVENT_DEFAULT_GAMEPLAY;
			}
			else if ( IsGameplay() == true && IsSubtitle() == true )
			{
				eventName = VO_EVENT_CINEMATIC;
			}
			else
			{
				eventName = VO_EVENT_DEFAULT;
			}
		}

		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
		if ( soundEmitterComponent && soundEmitterComponent->SoundIsActive() )
		{
			soundEmitterComponent->SoundStop( eventName.AsChar() );
		}
	}

	if ( m_languagePack != NULL )
	{
		m_languagePack->Unlock();
	}
	SLocalizationManager::GetInstance().ReleaseLanguagePack( m_speechId );

	RED_ASSERT( !m_isLipsyncPlayed );

	if ( m_lipsyncAnimationEntry )
	{
		// disconnect animation
		m_lipsyncAnimationEntry->SetAnimation( nullptr );
		delete m_lipsyncAnimationEntry;
		m_lipsyncAnimationEntry = nullptr;
	}
}

IFile& operator<<( IFile &file, CActorSpeech &actorSpeech )
{
	file << actorSpeech.m_lipsyncAnimationEntry;

	return file;
}

Bool CActorSpeech::Load()
{
	if( m_isSync )
	{
		m_languagePack = SLocalizationManager::GetInstance().GetLanguagePackSync( m_speechId );
	}
	else
	{
		m_languagePack = SLocalizationManager::GetInstance().GetLanguagePackAsync( m_speechId );
	}

	if ( IsSubtitle() )
	{
		CActor * act = Cast<CActor>( m_actor->GetSceneParentEntity() );
		if( act )
		{
			if( act->GetLocalizedDisplayName().Load() == false)
			{
				return false;
			}
		}
	}

	if ( m_languagePack != NULL )
	{
		m_loaded = true;
		m_languagePack->Lock();

		// Calculate duration
		m_duration =  ( m_languagePack->GetSpeechBuffer().GetSize() > 0 ) 
			? m_languagePack->GetSpeechBuffer().GetDuration() 
			//: 0.8f + Max< Float >( 0.3f, m_languagePack->GetText().GetLength() * 0.075f );
			: CStorySceneLine::CalcFakeDuration( m_languagePack->GetText() );
	}

	return m_loaded;
}

void CActorSpeech::Play( Float progress )
{
	if ( m_languagePack == NULL )
	{
		ASSERT( m_languagePack != NULL );
		m_finished = true;
		return;
	}
	
	if ( HasText() )
	{
		PlayText();
	}

	if ( HasVoice() )
	{
		PlaySound( progress );
	}

	if ( HasLipsync() )
	{
		PlayLipsync( progress );
	}

	if ( HasDialogVoice())
	{
		PlayDialogVoice( progress );
	}

	m_playing = true;
}

void CActorSpeech::Cancel()
{
	if ( HasText() )
	{
		StopText();
	}

	if ( HasVoice() )
	{
		StopSound();
	}

	if ( m_isLipsyncPlayed )
	{
		RED_ASSERT( HasLipsync() );
		StopLipsync();
	}

	if ( HasDialogVoice() )
	{
		StopDialogVoice();
	}

	m_loaded = false;
	m_playing = false;
	m_finished = false;
}

Bool CActorSpeech::SetTimeState( Float time, Float progress )
{
	ASSERT( IsLoaded() );

	m_finished = false;

	return true;
}

void CActorSpeech::Update( Float timeDelta )
{
	if ( IsLoaded() == false )
	{
		Load();
		return;
	}

	//if ( HasMimic() == true && EnsureMimicOn( m_actor, timeDelta ) == false )
	//{
	//	return;
	//}

	if ( IsLoaded() == true && IsPlaying() == false )
	{
		Play( m_progress );
	}

	if( m_updateDistanceParameter && IsPlaying() && !IsFinished() )
	{
		m_distanceParameterUpdateTimer -= timeDelta;
		if( m_distanceParameterUpdateTimer <= 0.0f )
		{
			m_distanceParameterUpdateTimer += VO_PLAYER_DISTANCE_UPDATE_FREQUENCY;
			UpdatePlayerDistance();
		}
	}

	if ( m_dialogVoicePlayed )
	{
		UpdateDialogVoice( timeDelta );
		return;
	}

	m_progress += timeDelta / m_duration;

	if( m_progress >= 1.0f )
	{
		m_finished = true;
		if ( m_soundWasPlayed == true )
		{
			CEntity* entity = m_actor->GetSceneParentEntity();
			CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
			if ( soundEmitterComponent )
			{			
				soundEmitterComponent->SoundParameter( VO_EVENT_PARAMETER.AsChar(), 0.0f, 0.5f, m_actor->GetSceneHeadBone() );
			}
		}
	}
}

Float CActorSpeech::GetDuration() const
{
	if ( m_loaded == false )
	{
		return -1.0f;
	}
	if ( m_languagePack->GetSpeechBuffer().GetSize() == 0 )
	{
		return m_duration;
		//return 3.0f;
	}

	return m_languagePack->GetSpeechBuffer().GetDuration();
}

void CActorSpeech::ReplayLipsync()
{
	if ( m_isLipsyncPlayed )
	{
		RED_ASSERT( m_lipsyncAnimationEntry );
		if ( m_lipsyncAnimationEntry )
		{
			StopLipsync();

			RED_ASSERT( !m_isLipsyncPlayed );

			m_isLipsyncPlayed = m_actor->PlayLipsyncAnimation( m_lipsyncAnimationEntry, GetProgress() );

			RED_ASSERT( m_isLipsyncPlayed );
		}
	}
}

extern void PlayDebugSound( const String& path );

namespace
{
	String GetWavPath( const String& stringId, const String& languageId )
	{
		String outPath;

		GDepot->GetAbsolutePath( outPath );

		outPath += TXT("speech\\");

		outPath += languageId + TXT("\\audio\\") + stringId + TXT(".wav");

		return outPath;
	}
}

void CActorSpeech::PlaySound( Float progress )
{
	if( SSoundSettings::m_voiceEventInCutscenesFixed )
	{
		if ( m_languagePack->GetSpeechBuffer().GetSize() > 0 )
		{
			CEntity* entity = m_actor->GetSceneParentEntity();
			CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
			Int32 boneIndex = m_actor->GetSceneHeadBone();
			if ( soundEmitterComponent && boneIndex > -1 )
			{
				if( m_soundEventName.Empty() )
				{
					if ( IsGameplay() == true )
					{
						soundEmitterComponent->SoundParameter( VO_EVENT_PARAMETER.AsChar(), 1.0f, 0.1f, boneIndex );
						soundEmitterComponent->SoundEvent( VO_EVENT_DEFAULT_GAMEPLAY.AsChar(), m_languagePack->GetSpeechBuffer().GetData(), m_languagePack->GetSpeechBuffer().GetCompression(), static_cast< Uint32 >( m_languagePack->GetSpeechBuffer().GetSize() ), boneIndex, progress );
					}
					else
					{
						soundEmitterComponent->SoundEvent( VO_EVENT_DEFAULT.AsChar(), m_languagePack->GetSpeechBuffer().GetData(), m_languagePack->GetSpeechBuffer().GetCompression(), static_cast< Uint32 >( m_languagePack->GetSpeechBuffer().GetSize() ), progress );
					}
				}
				else
				{
					soundEmitterComponent->SoundParameter( VO_EVENT_PARAMETER.AsChar(), 1.0f, 0.1f, boneIndex );
					soundEmitterComponent->SoundEvent( m_soundEventName.AsChar(), m_languagePack->GetSpeechBuffer().GetData(), m_languagePack->GetSpeechBuffer().GetCompression(), static_cast< Uint32 >( m_languagePack->GetSpeechBuffer().GetSize() ), boneIndex, progress );
				}
			}
		}
	}
	else if ( m_languagePack->GetSpeechBuffer().GetSize() > 0 )
	{
		StringAnsi eventName( m_soundEventName );
		if( eventName.Empty() )
		{
			if ( IsGameplay() == true && IsSubtitle() == false )
			{
				eventName = VO_EVENT_DEFAULT_GAMEPLAY;
			}
			else if ( IsGameplay() == true && IsSubtitle() == true )
			{
				// Play 3d...
				eventName = VO_EVENT_CINEMATIC;

				// ... unless it's the player, in which case, 2d.
				if( CActor* actor = Cast< CActor >( GGame->GetPlayerEntity() ) )
				{
					if( m_actor == actor )
					{
						eventName = VO_EVENT_DEFAULT;
					}
				}
			}
			else
			{
				eventName = VO_EVENT_DEFAULT;
			}
		}
		CEntity* entity = m_actor->GetSceneParentEntity();
		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
		Int32 boneIndex = m_actor->GetSceneHeadBone();
		if ( soundEmitterComponent && boneIndex > -1 )
		{			
			soundEmitterComponent->SoundParameter( VO_EVENT_PARAMETER.AsChar(), 1.0f, 0.1f, m_actor->GetSceneHeadBone() );
			if( m_disabledOcclusion || eventName == VO_EVENT_CINEMATIC )
			{
				soundEmitterComponent->SoundEvent( eventName.AsChar(), m_languagePack->GetSpeechBuffer().GetData(), m_languagePack->GetSpeechBuffer().GetCompression(), static_cast< Uint32 >( m_languagePack->GetSpeechBuffer().GetSize() ), progress );
			}
			else
			{
				soundEmitterComponent->SoundEvent( eventName.AsChar(), m_languagePack->GetSpeechBuffer().GetData(), m_languagePack->GetSpeechBuffer().GetCompression(), static_cast< Uint32 >( m_languagePack->GetSpeechBuffer().GetSize() ), boneIndex, progress );

			}
		}

		m_updateDistanceParameter = eventName == VO_EVENT_CINEMATIC;
		if( m_updateDistanceParameter )
		{
			m_distanceParameterUpdateTimer = VO_PLAYER_DISTANCE_UPDATE_FREQUENCY;
			UpdatePlayerDistance();
		}
	}

	m_soundWasPlayed = true;
}

void CActorSpeech::PlayLipsync( Float progress )
{
	RED_ASSERT( !m_isLipsyncPlayed );

#ifndef NO_MIMIC_ANIM
	if ( m_actor != NULL )
	{
		CSkeletalAnimation* animation = m_languagePack->GetLipsync();
		if ( animation != NULL )
		{
			if ( !animation->IsCompressed() && GGame->GetGameplayConfig().m_logMissingAnimations )
			{
				WARN_GAME( TXT("Actor Speech animation '%ls' is not compressed - please reimport it. Speech ID: %d"), 
					animation->GetName().AsString().AsChar(), m_speechId );
			}
			ASSERT( animation->IsCompressed() && TXT("See log file") );

			m_lipsyncAnimationEntry = new CSkeletalAnimationSetEntry;
			m_lipsyncAnimationEntry->SetAnimation( animation );

			m_isLipsyncPlayed = m_actor->PlayLipsyncAnimation( m_lipsyncAnimationEntry, progress );
			if ( !m_isLipsyncPlayed )
			{
				WARN_GAME( TXT("CActorSpeech: Actor '%ls' couldn't play mimic animation '%ls'"), m_actor->GetSceneParentEntity()->GetName().AsChar(), animation->GetName().AsString().AsChar() );
			}
		}
		else
		{
#ifndef NO_EDITOR
			if ( GIsGame )
			{
				ERR_GAME( TXT("CActorSpeech: Couldn't find lipsync animation in localized pack '%ls' for actor '%ls'"), m_languagePack->GetLipsyncFileName().AsChar(), m_actor->GetSceneParentEntity()->GetName().AsChar() );
			}

			m_isLipsyncPlayed = m_actor->PlayMimicAnimation( CName( m_languagePack->GetLipsyncFileName() ), CNAME( MIMIC_SLOT ), 0.0f, progress );
			if ( !m_isLipsyncPlayed )
			{
				WARN_GAME( TXT("CActorSpeech: Fake lipsync for animation '%ls'. Actor '%ls'."), m_languagePack->GetLipsyncFileName().AsChar(), m_actor->GetSceneParentEntity()->GetName().AsChar() );
			}
#endif
		}
	}
#endif

	RED_ASSERT( m_isLipsyncPlayed );
}

void CActorSpeech::StopSound()
{
	CEntity* entity = m_actor->GetSceneParentEntity();
	if( entity )
	{
		StringAnsi eventName( m_soundEventName );
		if( eventName.Empty() )
		{
			if ( IsGameplay() == true && IsSubtitle() == false )
			{
				eventName = VO_EVENT_DEFAULT_GAMEPLAY;
			}
			else if ( IsGameplay() == true && IsSubtitle() == true )
			{
				eventName = VO_EVENT_CINEMATIC;
			}
			else
			{
				eventName = VO_EVENT_DEFAULT;
			}
		}
		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
		if( soundEmitterComponent && soundEmitterComponent->SoundIsActive() )
		{
			soundEmitterComponent->SoundStop( eventName.AsChar() );
		}
	}
}

void CActorSpeech::StopLipsync()
{
	RED_ASSERT( m_isLipsyncPlayed );
	if ( m_isLipsyncPlayed && m_actor->StopLipsyncAnimation() )
	{
		m_isLipsyncPlayed = false;
	}
	RED_ASSERT( !m_isLipsyncPlayed );
}

void CActorSpeech::PlayText()
{
	if( ! GCommonGame->AreSubtitlesEnabled() )
	{
		return;
	}

	EStorySceneLineType lineType = SSLT_Normal;
	if ( IsOneliner() == true )
	{
		lineType = SSLT_Oneliner;
	}
	else if ( IsSubtitle() == true )
	{
		lineType = SSLT_Subtitle;
	}

	if ( m_sceneDisplay )
	{
		//const String& text = m_languagePack->GetText();
		//m_sceneDisplay->ShowDialogText( text, m_actor, lineType );

		// DEMO HACK
		String text = m_languagePack->GetText();
		

		Int32 startInd	= (Int32) text.GetIndex( '{' );
		Int32 endInd	= (Int32) text.GetIndex( '}' );

		if ( startInd >= 0 && endInd > 0 && startInd < endInd )
		{
			text.Erase( text.Begin() + startInd, text.Begin() + endInd + 1 );			
		}
		text.Trim();

		if ( !text.Empty() )
		{
			m_sceneDisplay->ShowDialogText( text, m_actor, lineType, m_alternativeUI );
		}
	}
}

void CActorSpeech::StopText()
{
	EStorySceneLineType lineType = SSLT_Normal;
	if ( IsOneliner() == true )
	{
		lineType = SSLT_Oneliner;
	}
	else if ( IsSubtitle() == true )
	{
		lineType = SSLT_Subtitle;
	}

	if ( m_sceneDisplay )
	{
		m_sceneDisplay->HideDialogText( m_actor, lineType );
	}
}

void CActorSpeech::ValidateModes()
{
	CActor* playerActor = GCommonGame->GetPlayer();
	CWorld* activeWorld = GGame->GetActiveWorld();

	if ( activeWorld == NULL || playerActor == NULL || m_actor == NULL || playerActor == m_actor )
	{
		return;
	}

	Float squareDistance = playerActor->GetWorldPositionRef().DistanceSquaredTo( m_actor->GetSceneParentEntity()->GetWorldPositionRef() );
		
	if ( m_modeFlags & ASM_Gameplay)
	{
		if ( squareDistance > NO_VOICE_DISTANCE * NO_VOICE_DISTANCE && IsGameplay() == true && IsSubtitle() == false )
		{
			m_modeFlags &= ~ASM_Voice;
		}
		if ( squareDistance > NO_MIMIC_DISTANCE * NO_MIMIC_DISTANCE )
		{
			m_modeFlags &= ~ASM_Lipsync;
		}
		if ( squareDistance > NO_TEXT_DISTANCE * NO_TEXT_DISTANCE 
			&& !( m_modeFlags & ASM_Subtitle ) )
		{
			m_modeFlags &= ~ASM_Text;
		}
	}
	
	CPhysicsWorld* physicsWorld = nullptr;
	if ( ( m_modeFlags & ASM_Text ) 
		&& ( m_modeFlags & ASM_Gameplay ) && !( m_modeFlags & ASM_Subtitle )
		&& activeWorld->GetPhysicsWorld( physicsWorld ) )
	{
		if ( m_actor->WasSceneActorVisibleLastFrame() == false )
		{
			m_modeFlags &= ~ASM_Text;
		}
	}
}

Vector CActorSpeech::GetHeadPosition( ISceneActorInterface* actor )
{
	return actor->GetSceneHeadPosition();
}

Bool CActorSpeech::EnsureMimicOn( ISceneActorInterface* actor, Float timeDelta )
{
	if ( m_actor == NULL )
	{
		return true;
	}

	if ( m_actor->GetSceneParentEntity()->IsSpawned() == false )
	{
		return false;
	}

	if ( IsGameplay() )
	{
		/*if ( m_actor->IsMimicReady() == false && m_waitForMimic < WAIT_FOR_MIMIC_LOW )
		{
			m_waitForMimic += timeDelta;
			return false;
		}
		else if ( m_actor->IsMimicReady() == false )
		{
			ASSERT( m_waitForMimic <= WAIT_FOR_MIMIC_LOW );
			WARN_GAME( TXT("Actor '%ls' couldn't activate mimic low!"), actor->GetFriendlyName().AsChar() );
		}*/
	}
	else
	{
		if ( m_actor->HasSceneMimic() == false && m_waitForMimic < WAIT_FOR_MIMIC_HIGH )
		{
			m_actor->SceneMimicOn();
			m_waitForMimic += timeDelta;
			return false;
		}
		else if ( m_actor->HasSceneMimic() == false )
		{
			ASSERT( m_waitForMimic <= WAIT_FOR_MIMIC_HIGH );
			WARN_GAME( TXT("Actor '%ls' couldn't activate mimic high!"), actor->GetSceneParentEntity()->GetName().AsChar() );
			ASSERT( !TXT("See log file") && m_actor->HasSceneMimic() );
		}
	}

	return true;
}

void CActorSpeech::PlayDialogVoice( Float progress )
{
	if ( false == m_dialogVoicePlayingError && false == m_dialogVoicePlayed )
	{
		if ( m_languagePack->GetVoiceoverFileName().Empty() )
		{
			m_dialogVoicePlayingError = true;
			return;
		}

		if ( nullptr == m_languagePack )
		{
			m_dialogVoicePlayingError = true;
			return;
		}

		if ( nullptr == m_actor )
		{
			m_dialogVoicePlayingError = true;
			return;
		}

		String voiceTag = m_actor->GetSceneActorVoiceTag().AsString();
		String voiceFile = m_languagePack->GetVoiceoverFileName();
		
		if ( voiceTag.Empty() )
		{
			m_dialogVoicePlayingError = true;
			return;
		}

		if ( voiceFile.Empty() )
		{
			m_dialogVoicePlayingError = true;
			return;
		}

		CEntity* entity = m_actor->GetSceneParentEntity();
		CSoundEmitter* soundEmitterComponent = entity->GetSoundEmitterComponent();
		if ( nullptr == soundEmitterComponent )
		{
			return;
		}

		RED_LOG( Dialog, TXT("Trying to play voice: %s (previously: %s), voicetag: %s (previously: %s), text: %s"), 
			voiceFile.AsChar(), m_languagePack->GetVoiceoverFileName().AsChar(),
			voiceTag.AsChar(), m_actor->GetSceneActorVoiceTag().AsString().AsChar(),
			m_languagePack->GetText().AsChar() );

		const Char* args[ 3 ] =	
		{
			voiceTag.AsChar(),
			TXT("neutral"), // temp hardcoded for the milestone
			voiceFile.AsChar()
		};

		m_dialogVoiceID = soundEmitterComponent->PlaySoundSequence( TXT( "scene_1_prototype_dx" ), args, 3, &m_callbackData );  // name temp hardcoded for the milestone
		if ( m_dialogVoiceID )
		{
			m_dialogVoicePlayed = true;
			m_duration = 0.f; // will be returned by callback when known
			m_progress = 0.f;
		}
		else
		{
			m_dialogVoicePlayingError = true;
		}
	}

}

void CActorSpeech::StopDialogVoice()
{
	if ( m_dialogVoiceID )
	{
		CEntity* entity = m_actor->GetSceneParentEntity();
		CSoundEmitter* soundEmitterComponent = entity->GetSoundEmitterComponent();
		if ( soundEmitterComponent )
		{
			soundEmitterComponent->StopSoundSequence( m_dialogVoiceID );
		}

		m_dialogVoiceID = 0;
	}
}

void CActorSpeech::UpdateDialogVoice( Float timeDelta )
{
	if ( m_dialogVoicePlayingError )
	{
		// not really playing, just update the fake progress
		ASSERT( m_duration > 0.f );
		m_progress += timeDelta / m_duration;
		return;
	}

	Float duration;
	if ( m_duration == 0.f )
	{
		Int32 val = m_callbackData.ReadDuration();
		duration = *reinterpret_cast< Float* > ( &val );

		if ( duration == 0.f )
		{
			// duration yet unknown, check for playing error
			if ( m_callbackData.IsEventCompleted() )
			{
				m_dialogVoicePlayed = false;
				m_dialogVoicePlayingError = true;
				m_duration = CStorySceneLine::CalcFakeDuration( m_languagePack->GetText() );
				m_progress = timeDelta / m_duration;

				RED_LOG( Dialog, TXT("voice playing error") );
			}
			return;
		}
		ASSERT( duration > 0.f );
		m_duration = duration / 1000.f; // it comes in ms
	}

	m_progress += ( timeDelta / m_duration );

	// don't allow m_progress to reach 1.0 before callback event fires
	m_progress = Clamp( m_progress, 0.f, 0.999f );

	Int32 itemsCompleted = m_callbackData.ReadItemsCompleted();
	if ( itemsCompleted )
	{
		ASSERT( itemsCompleted == 1 );
		
		if ( m_callbackData.IsEventCompleted() )
		{
			// done playing voice
			m_progress = 1.f;
			m_finished = true;
			m_dialogVoiceID = 0;
		
			if ( HasText() == true )
			{
				StopText();
			}
			if ( m_isLipsyncPlayed )
			{
				RED_ASSERT( HasLipsync() );
				StopLipsync();
			}
		}
	}
}

void CActorSpeech::UpdatePlayerDistance()
{
	if( CEntity* playerEntity = GGame->GetPlayerEntity() )
	{
		if( CActor* playerActor = Cast< CActor >( playerEntity ) )
		{
			if( CEntity* entity = m_actor->GetSceneParentEntity() )
			{
				if( CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent() )
				{
					Vector3 diff = playerActor->GetSceneHeadPosition() - m_actor->GetSceneHeadPosition();
					soundEmitterComponent->SoundParameter( VO_PLAYER_DISTANCE_PARAMETER.AsChar(), diff.Mag() );
				}
			}
		}
	}
}

