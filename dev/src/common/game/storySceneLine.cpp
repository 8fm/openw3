#include "build.h"

#include "../../common/core/gatheredResource.h"

#include "storySceneLine.h"
#include "actorSpeech.h"
#include "storySceneSection.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "storySceneDisplayInterface.h"
#include "storySceneSystem.h"
#include "storySceneVoiceTagsManager.h"
#include "../core/diskFile.h"
#include "../engine/speechBuffer.h"
#include "../engine/languagePack.h"
#include "../engine/localizationManager.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif


RED_DEFINE_STATIC_NAME( Self );

#define COLUMN_VOICETAG 0
#define COLUMN_SEX 1
#define COLUMN_PITCH 2
#define COLUMN_VOICETAG_ID 4

IMPLEMENT_ENGINE_CLASS( CStorySceneLine );

RED_DEFINE_STATIC_NAME(dialogLine);

CStorySceneLine::CStorySceneLine(void)
	: CAbstractStorySceneLine()
	, m_noBreak( true )
	, m_soundEventName()
	, m_voiceFileName( CLocalizationManager::VOICEFILE_UNDEFINED )
	, m_isBackgroundLine( false )
	, m_disableOcclusion( false )
	, m_alternativeUI ( false )
{}

Bool CStorySceneLine::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == CNAME(dialogLine) )
	{
		CLocalizedContent *locCont = *( CLocalizedContent **) readValue.GetData();

		SCENE_ASSERT( locCont->GetIndex() != 0 && TXT("Localization Content index equals zero!") );
		m_dialogLine.SetIndex( locCont->GetIndex() );

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Float CStorySceneLine::CalcFakeDuration( const String& text )
{
	return 0.6f + Max< Float >( 0.3f, text.GetLength() * 0.08f );
}

/*
Calculates line duration (in seconds).

\param locale Locale for which to calculate line duration.
\return Line duration in seconds.

Locale arg is needed because line duration may be different for each locale.
*/
Float CStorySceneLine::CalculateDuration( const String& locale ) const
{
	Uint32	lineStringId = GetLocalizedContent()->GetIndex();
	Float	duration = 0.0f;
	Bool	calculated = false;

	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

	// Try getting duration from cache (note that cache contains entries for current locale only).
	if ( locale == currentLocale && SLocalizationManager::GetInstance().GetCachedLanguagePackDuration( lineStringId, duration ) )
	{
		calculated = true;
	}

	if( !calculated )
	{
		LanguagePack* pack = SLocalizationManager::GetInstance().GetLanguagePackSync( lineStringId, false, locale );
		if ( pack != NULL && pack->GetVoiceoverSize() != 0 )
		{
			duration = pack->GetSpeechBuffer().GetDuration();
			calculated = true;
		}
		if ( pack != NULL )
		{
			SLocalizationManager::GetInstance().ReleaseLanguagePack( lineStringId );
		}
	}	
	if( !calculated )
	{
		duration = CalcFakeDuration( m_dialogLine.GetString( locale ) );
	}

	return duration;
}

IStorySceneElementInstanceData* CStorySceneLine::OnStart( CStoryScenePlayer* player ) const
{
	CName actorVoicetag = m_voicetag;
	
	// Find the speaker
	THandle< CActor > actorHandle( player->GetMappedActor( m_voicetag ) );
	if ( actorHandle.Get() == NULL )
	{
		SCENE_LOG( TXT("Unable to map voicetag '%ls' for line '%ls'"), GetVoiceTag().AsString().AsChar(), GetFriendlyName().AsChar() );
		// We play line no matter what - line itself will break scene flow if actor is null, but we will have debug info
	}

	// Start the line
	StorySceneLineInstanceData* line = new StorySceneLineInstanceData( this, player, actorHandle );
	return line;
}

void CStorySceneLine::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	if ( !IsBackgroundLine() )
	{
		elements.PushBack( this );
	}
}

void CStorySceneLine::OnVoicetagChanged()
{
	GenerateVoiceFileName();
}

Bool CStorySceneLine::GenerateVoiceFileName()
{
	if ( !GetSection() || !GetSection()->GetScene() )
	{
		return false;
	}

	if ( m_voiceFileName.ContainsSubstring( CLocalizationManager::VOICEFILE_UNDEFINED_GROUP ) )
	{
		// No group defined yet
		String sceneFileName = GetSection()->GetScene()->GetFile()->GetFileName();
		size_t indexOf = 0;
		VERIFY( sceneFileName.FindCharacter( TXT( '_' ), indexOf ), TXT("Index not found in substring") );
		String sceneId = sceneFileName.LeftString( indexOf ).ToUpper();

		m_voiceFileName.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_GROUP, sceneId );
	}

	if ( m_voiceFileName.ContainsSubstring( CLocalizationManager::VOICEFILE_UNDEFINED_ID ) && IsUnstableStringId( m_dialogLine.GetIndex() ) == false )
	{
		m_voiceFileName.Replace( CLocalizationManager::VOICEFILE_UNDEFINED_ID, String::Printf( TXT( "%08d" ), m_dialogLine.GetIndex() ) );
	}

	const C2dArray* voicetagTable = SStorySceneVoiceTagsManager::GetInstance().GetVoiceTags();
	if ( voicetagTable != NULL )
	{
		String voicetagId = voicetagTable->GetValue< String >( TXT( "Voicetag" ), m_voicetag.AsString(), TXT( "Id") );
		if ( voicetagId.Empty() == false )
		{
			m_voiceFileName.Replace( m_voiceFileName.StringBefore( TXT( "_" ) ), voicetagId );
		}
		else
		{
			return false;
		}
	}

	return true;
}

void CStorySceneLine::RefreshVoiceFileName()
{
	m_voiceFileName = SLocalizationManager::GetInstance().GetVoiceoverFilename( m_dialogLine.GetIndex() );
	m_voiceFileName = CLocalizationManager::VOICEFILE_UNDEFINED;
	GenerateVoiceFileName();
}

void CStorySceneLine::OnPostLoad()
{
	TBaseClass::OnPostLoad();

#ifndef NO_EDITOR
	if ( !GIsCooker )
	{
		RefreshVoiceFileName();
	}
#endif
}

void CStorySceneLine::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// protection against changing generated voice file name - regenerate it
	if ( property->GetName() == TXT( "voiceFileName" ) )
	{
		//GenerateVoiceFileName();
	}
}

Bool CStorySceneLine::MakeCopyUniqueImpl()
{
	CAbstractStorySceneLine::MakeCopyUniqueImpl();

	m_dialogLine.MakeUniqueCopy();
	GenerateElementID();
	m_voiceFileName = CLocalizationManager::VOICEFILE_UNDEFINED;
	GenerateVoiceFileName();

	return true;
}

Bool CStorySceneLine::GetVoicetagId( const CName& voicetag, String& voiceTagId )
{
	voiceTagId.Clear();
	TDynArray< String > voicetagParts = voicetag.AsString().Split( TXT( " " ) );
	for ( Uint32 i = 0; i < voicetagParts.Size(); ++i )
	{
		voiceTagId += voicetagParts[ i ].LeftString( 3 );
	}

	return true;
}

void CStorySceneLine::GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const
{
	stringIds.PushBackUnique( m_dialogLine.GetIndex() );
}


void CStorySceneLine::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/
{ 
	TBaseClass::GetLocalizedStrings( localizedStrings );
	localizedStrings.PushBack( LocalizedStringEntry( &m_dialogLine, TXT( "Line text" ), NULL, GetVoiceFileName() ) );
}

void CStorySceneLine::SetContent( String newValue )
{
	m_dialogLine.SetString( newValue );
}

//////////////////////////////////////////////////////////////////////////

StorySceneLineInstanceData::StorySceneLineInstanceData( const CStorySceneLine* line, CStoryScenePlayer* player, THandle< CActor >& actorHandle )
	: IStorySceneElementInstanceData( line, player )
	, m_line( line )
	, m_isVisible( false )
	, m_isOneLiner( line->GetSection()->IsGameplay() )
	, m_actorHandle( actorHandle )
	, m_lineStringId( line->GetLocalizedContent()->GetIndex() )
	, m_display( player->GetSceneDisplay() )
	, m_speechId( TActorSpeechInvalidID )
	, m_speechRunning( false )
	, m_timeOffsetFront( 0.f )
	, m_timeOffsetBack( 0.f )
{
	SCENE_ASSERT( m_line );
}

StorySceneLineInstanceData::~StorySceneLineInstanceData()
{
#ifndef NO_EDITOR
	CActor *pActor = m_actorHandle.Get();
	if ( pActor )
	{
		SCENE_ASSERT( !pActor->IsSpeaking( m_lineStringId ) );
	}
#endif
	if ( m_lineStringId > 0 )
	{
		SLocalizationManager::GetInstance().ReleaseLanguagePack( m_lineStringId );
	}
}

Bool StorySceneLineInstanceData::OnTick( Float timeDelta )
{
	//CActor *pActor = m_actorHandle.Get();
	
	// DIALOG_TOMSIN_TODO
	const Bool timeOut = GetCurrentTime() >= GetDuration();
	
	//if ( !pActor || !pActor->IsSpeaking( m_lineStringId ) )
	//{
	//	return false;
	//}

	if ( !m_player->IsPaused() && !m_speechRunning )
	{
		const Float lineStartTime = GetDuration() - m_timeOffsetFront - m_timeOffsetBack;
		Float progress = lineStartTime > 0.f ? (GetCurrentTime()-m_timeOffsetFront) / lineStartTime : 0.f;
		if ( progress >= 0.f && progress <= 1.f )
		{
			//SCENE_ASSERT( progress >= 0.f && progress <= 1.f );
			//progress = Clamp( progress, 0.f, 1.f );

			{
				Bool subtitle = false;

				Int32 speechFlags = ASM_Text | ASM_Lipsync;

				if ( m_player->ShouldPlaySpeechSound() && m_player->ShouldPlaySounds() )
				{
					speechFlags |= ASM_Voice;
				}

				if ( m_line != NULL )
				{
					subtitle = m_line->GetSection()->HasCinematicOneliners();

					if ( m_line->GetSection()->IsGameplay() == true )
					{
						speechFlags |= ASM_Gameplay;
					}

					if ( m_line->GetSection()->HasCinematicOneliners() == true )
					{
						speechFlags |= ASM_Subtitle;
					}
				}

				CActor *pActor = m_actorHandle.Get();

				if ( pActor == NULL )
				{
					// Hack to ensure fallback use of actors spawned by cutscene after section preload
					// Needs to be here until preloading refactor and optimization 
					m_actorHandle = THandle< CActor >( m_player->GetMappedActor( m_line->GetVoiceTag() ) );
					pActor = m_actorHandle.Get();
				}

				if ( pActor ) 
				{
					ActorSpeechData speechData( m_lineStringId, m_line->GetSoundEventName(), false, speechFlags, m_line->GetDisableOclusionFlag(), m_line->IsAlternativeUI() );
					speechData.m_sceneDisplay = m_display;
					speechData.m_progress = progress;

					m_speechId = pActor->SpeakLine( speechData );
				}
			}

			m_speechRunning = true;
		}
	}

	if ( timeOut )
	{
		return false;
	}

	return true;

	/*const Float time = GetCurrentTime();
	const Float progress = time/m_lineDuration;

	if ( pActor && m_speechId != TActorSpeechInvalidID && pActor->UpdateSpeaking( m_speechId, time, progress ) && progress < 1.f )
	{
		return true;
	}

	return false;*/
}

String StorySceneLineInstanceData::GetName() const 
{ 
	CActor* a = m_actorHandle.Get();
	if ( a )
	{
		return String::Printf( TXT("Line [%ls]"), a->GetVoiceTag().AsString().AsChar() );
	}
	else
	{
		return TXT("Line"); 
	}
}

void StorySceneLineInstanceData::OnPlay()
{
	SCENE_ASSERT( !m_speechRunning );
}

void StorySceneLineInstanceData::OnStop()
{
	CActor *pActor = m_actorHandle.Get();
	if ( pActor && pActor->IsSpeaking( m_lineStringId ) )
	{
		pActor->CancelSpeech();
	}

	// DIALOG_TOMSIN_TODO - poniewaz nie ma zarzadzania speechem tu tylko w CActor jest problem z tickami i zarzadzaniem obiektami wiec tu na razie hack
	if ( m_display )
	{
		m_display->HideAllDialogTexts();
	}

	m_speechRunning = false;
}

void StorySceneLineInstanceData::OnPaused( Bool flag )
{
	OnStop();
}

Bool StorySceneLineInstanceData::IsReady() const
{
	CActor *pActor = m_actorHandle.Get();
	if ( pActor == NULL )
	{
		return true;
	}
	return pActor->IsSpawned() == true;// && SLocalizationManager::GetInstance().GetLanguagePackAsync( m_lineStringId ) != NULL;
}

/*
Sets amount of leading silence.

\param leadingSilence Amount of leading silence in seconds.

This function changes duration reported by GetDuration().
*/
void StorySceneLineInstanceData::SetLeadingSilence( Float leadingSilence )
{
	m_duration = m_duration - m_timeOffsetFront + leadingSilence;
	m_timeOffsetFront = leadingSilence;
}

/*
Sets amount of trailing silence.

\param leadingSilence Amount of trailing silence in seconds.

This function changes duration reported by GetDuration().
*/
void StorySceneLineInstanceData::SetTrailingSilence( Float trailingSilence )
{
	m_duration = m_duration - m_timeOffsetBack + trailingSilence;
	m_timeOffsetBack = trailingSilence;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
