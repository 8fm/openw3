#include "build.h"

#include "../../common/game/questsSystem.h"

#include "storySceneChoice.h"
#include "storySceneChoiceLineAction.h"
#include "../engine/localizationManager.h"
#include "storySceneLine.h"
#include "storySceneCutscene.h"
#include "storySceneCutsceneSection.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneChoice )

CStorySceneChoice::CStorySceneChoice(void)
	: m_timeLimit( 0.0f )
	, m_duration( 10.0f )
	, m_isLooped( true )
	, m_questChoice( false )
	, m_showLastLine( true )
	, m_alternativeUI ( false )
{
}

CStorySceneChoiceLine* CStorySceneChoice::GetChoiceLine( Int32 index )
{
	if ( index < 0 || index >= m_choiceLines.SizeInt() )
	{
		return NULL;
	}
	return m_choiceLines[ index ];
}

const CStorySceneChoiceLine* CStorySceneChoice::GetChoiceLine( Int32 index ) const 
{
	if ( index < 0 || index >= m_choiceLines.SizeInt() )
	{
		return NULL;
	}
	return m_choiceLines[ index ];
}


void CStorySceneChoice::AddChoiceLine( CStorySceneChoiceLine* choiceLine, Int32 index /*= -1*/ )
{
	RED_FATAL_ASSERT( choiceLine->GetParent() == this, "CStorySceneChoice::AddChoiceLine(): bad choice line parent." );

	if ( index == -1 )
	{
		m_choiceLines.PushBack( choiceLine );
	}
	else
	{
		m_choiceLines.Insert( index, choiceLine );
	}
	
	NotifyAboutChoiceLineAdded( index );
}

CStorySceneChoiceLine* CStorySceneChoice::AddChoiceLine( Int32 index /*= -1 */ )
{
	CStorySceneChoiceLine* choiceLine = CreateObject< CStorySceneChoiceLine >( this );

	AddChoiceLine( choiceLine, index );

	return choiceLine;
}

void CStorySceneChoice::RemoveChoiceLine( Uint32 index )
{
	if ( index < m_choiceLines.Size() )
	{
		m_choiceLines.Remove( m_choiceLines[ index ] );
	}

	NotifyAboutChoiceLineRemoved( index );
}

Uint32 CStorySceneChoice::GetNumberOfChoiceLines() const
{
	return m_choiceLines.Size();
}

void CStorySceneChoice::NotifyAboutChoiceLinksChanged( CStorySceneChoiceLine* sender )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneChoiceLineLinkChanged ), CreateEventData( sender ) );
}

void CStorySceneChoice::NotifyAboutChoiceLineChanged( CStorySceneChoiceLine* sender )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneChoiceLineChanged ), CreateEventData( sender ) );
}

void CStorySceneChoice::NotifyAboutChoiceLineAdded( Int32 index )
{
	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( GetSection() ) );
}

void CStorySceneChoice::NotifyAboutChoiceLineRemoved( Uint32 index )
{
	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( GetSection() ) );
}

Bool CStorySceneChoice::MakeCopyUniqueImpl()
{
	for ( TDynArray< CStorySceneChoiceLine* >::iterator choiceLine = m_choiceLines.Begin();
		  choiceLine != m_choiceLines.End();
		  ++choiceLine )
	{
		(*choiceLine)->GetLocalizedChoiceLine()->MakeUniqueCopy();
		(*choiceLine)->GetLocalizedComment()->MakeUniqueCopy();
	}

	return true;
}

IStorySceneElementInstanceData* CStorySceneChoice::OnStart( CStoryScenePlayer* player ) const
{
	SCENE_ASSERT( GetSection()->IsGameplay() == false );
	
	CStorySceneChoiceInstanceData* data( nullptr );
	if ( !GetSection()->IsA< CStorySceneCutsceneSection >() )
	{
		data = new CStorySceneChoiceInstanceData( this, player );
	}
	else
	{
		data = new CStorySceneCutsceneChoiceInstanceData( this, player );
	}

	return data;
}

Uint32 CStorySceneChoice::MoveChoiceLine( Uint32 index, Bool moveDown )
{
	if ( m_choiceLines.Empty() == true 
		|| ( index == 0 && moveDown == false ) 
		|| ( index == m_choiceLines.Size() - 1 && moveDown == true ) )
	{
		return index;
	}

	Uint32 newIndex = ( moveDown == true ) ? index + 1 : index - 1;
	m_choiceLines.Swap( index, newIndex );

	NotifyAboutChoiceLineRemoved( index );
	NotifyAboutChoiceLineAdded( newIndex );

	return newIndex;
}

Int32 CStorySceneChoice::GetChoiceLineIndex( const CStorySceneChoiceLine* choiceLine ) const
{
	return static_cast< Int32 >( m_choiceLines.GetIndex( (CStorySceneChoiceLine*) choiceLine ) );
}

void CStorySceneChoice::GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const
{
	for ( TDynArray< CStorySceneChoiceLine* >::const_iterator choiceLineIter = m_choiceLines.Begin();
		choiceLineIter != m_choiceLines.End(); ++choiceLineIter )
	{
		CStorySceneChoiceLine* choiceLine = *choiceLineIter;
		stringIds.PushBackUnique( choiceLine->GetLocalizedChoiceLine()->GetIndex() );
	}
}

/*
Calculates choice duration (in seconds).

\param locale For choices this argument is ignored as choice duration is the same in all locales.
\return Choice duration in seconds.
*/
Float CStorySceneChoice::CalculateDuration( const String& /* locale */) const
{
	return m_duration;
}

CStorySceneChoiceInstanceData::CStorySceneChoiceInstanceData( const CStorySceneChoice* choice, CStoryScenePlayer* player )
	: IStorySceneElementInstanceData( choice, player )
	, m_remainingTime( choice->GetTimeLimit() )
	, m_choice( choice )
	, m_display( player->GetSceneDisplay() )
{
	ASSERT( player );
	ASSERT( choice );
}

CStorySceneChoiceInstanceData::~CStorySceneChoiceInstanceData()
{
	
}

Bool CStorySceneChoiceInstanceData::OnTick( Float timeDelta )
{
	if ( m_avaiableChoiceLines.Empty() == true )
	{
		return false;
	}

	// For time limited choices, show choice timer or make the choice automatically if no more time is left.
	if ( m_remainingTime > 0.0f )
	{
		m_remainingTime -= timeDelta;

		if ( m_remainingTime <= 0.0f )
		{
			// DIALOG_TOMSIN_TODO
			GCommonGame->GetSystem< CStorySceneSystem >()->Signal( SSST_Accept, 0 );
		}
		else if ( m_display )
		{
			SCENE_ASSERT( m_choice->GetTimeLimit() > 0.f );

			// show choice timer
			Float timeLeft = m_remainingTime / m_choice->GetTimeLimit();
			m_display->ShowChoiceTimer( timeLeft );
		}
	}

	return true;
}

RED_DEFINE_STATIC_NAME( OnTutorialMessageForChoiceLines )

void CStorySceneChoiceInstanceData::OnPlay()
{
	m_avaiableChoiceLines.Clear();

	const Bool showAlternativeUI = m_choice->IsUsingAlternativeUI();

	// Collect list of valid choices
	// append choices for players (if there are any)
	// DIALOG_TOMSIN_TODO
	if ( m_choice->IsQuestChoice() )
	{
		TDynArray< IStoryScenePlaybackListener* > listeners;
		GCommonGame->GetSystem< CQuestsSystem >()->GetContextDialogChoices( m_avaiableChoiceLines, m_player->GetStoryScene(), &listeners, m_choice );

		for ( IStoryScenePlaybackListener* listener : listeners )
		{
			m_player->AddPlaybackListener( listener );
		}

	}

	const String& lastLine = m_display->GetLastDialogText();
	if ( m_choice->ShowLastLine() )
	{
		m_display->ShowPreviousDialogText( lastLine );
	}
	
	Uint32 allChoiceActions = 0;
	const Uint32 numChoices = m_choice->GetNumberOfChoiceLines();
	for ( Uint32 i=0; i<numChoices; ++i )
	{
		// Get n-th choice line
		const CStorySceneChoiceLine* line = m_choice->GetChoiceLine( i );
		if ( line )
		{
			SSceneChoice choiceData;
			if ( line->IsHidden() )
			{
				continue;	
			}

			if( line->IsDisabled() )
			{
				choiceData.m_disabled = true;
			}

			choiceData.m_playGoChunk = line->GetPlayGoChunk();

			// Choice should have a valid name
			if ( !line->GetChoiceLine().GetLength() )
			{
				//continue;
			}

			Uint32 orderNumber = m_avaiableChoiceLines.Size() + ( ( line->HasEmphasis() == false ) ? 100 : 0 );
			// A valid choice, remember	
			choiceData.link = line;
			choiceData.m_description = line->GetChoiceActionText() + line->GetChoiceLine();
			choiceData.m_order = orderNumber;
			choiceData.m_emphasised = line->HasEmphasis();
			choiceData.m_previouslyChoosen = line->WasChoosen();
			choiceData.m_dialogAction = line->GetChoiceActionIcon();

			if ( choiceData.m_description.Empty() )
			{
				String sectionName;

				if ( line->GetNextElement() != nullptr )
				{
					if ( line->GetNextElement()->IsA< CStorySceneControlPart >() )
					{
						sectionName = static_cast< const CStorySceneSection* >( line->GetNextElement() )->GetName();
					}
				}

				choiceData.m_description = String::Printf( TXT("[Option %d] '%ls'"), i, sectionName.AsChar() );
			}
			
			allChoiceActions = allChoiceActions | choiceData.m_dialogAction;
			m_avaiableChoiceLines.PushBackUnique( choiceData );
		}
	}

	// Show choices
	if ( !GGame->IsUsingPad() && m_display )
	{
		m_display->SetSceneCameraMovable( false );
	}

	Sort( m_avaiableChoiceLines.Begin(), m_avaiableChoiceLines.End() );

	if ( m_display )
	{
		m_display->SetChoices( m_avaiableChoiceLines, showAlternativeUI );
	}


	if( m_choice->GetTimeLimit() > 0.f )
	{
		allChoiceActions = allChoiceActions | DialogAction_TimedChoice;
	}

	GGame->CallEvent( CNAME( OnTutorialMessageForChoiceLines ), allChoiceActions );
}

void CStorySceneChoiceInstanceData::OnStop()
{
	if ( m_display )
	{
		// Hide choices
		const TDynArray< SSceneChoice > emptyLines;
		m_display->SetChoices( emptyLines, false );
		m_display->HideChoiceTimer();
		m_display->HidePreviousDialogText();
		m_display->SetSceneCameraMovable( true );
	}
	m_avaiableChoiceLines.Clear();
}

Bool CStorySceneChoiceInstanceData::IsLooped() const
{
	return m_choice->IsLooped();
}

//////////////////////////////////////////////////////////////////////////

CStorySceneCutsceneChoiceInstanceData::CStorySceneCutsceneChoiceInstanceData( const CStorySceneChoice* choice, CStoryScenePlayer* player )
	: CStorySceneChoiceInstanceData( choice, player )
	, m_csPlayer( nullptr )
{
	const CStorySceneCutsceneSection* csSection = Cast< const CStorySceneCutsceneSection >( choice->GetSection() );
	SCENE_ASSERT( csSection );

	m_csPlayer = csSection->GetPlayer()->OnStart( player );
	SCENE_ASSERT( m_csPlayer );
}

CStorySceneCutsceneChoiceInstanceData::~CStorySceneCutsceneChoiceInstanceData()
{
	delete m_csPlayer;
}

Bool CStorySceneCutsceneChoiceInstanceData::OnInit( const String& locale )
{
	const Bool ret = TBaseClass::OnInit( locale );

	m_csPlayer->Init( locale, m_startTime );

	m_duration = m_csPlayer->GetDuration();

	return ret;
}

void CStorySceneCutsceneChoiceInstanceData::OnDeinit()
{
	m_csPlayer->Deinit();

	TBaseClass::OnDeinit();
}

void CStorySceneCutsceneChoiceInstanceData::OnPlay()
{
	m_csPlayer->Play();

	TBaseClass::OnPlay();
}

Bool CStorySceneCutsceneChoiceInstanceData::OnTick( Float timeDelta )
{
	Bool ret = false;

	ret |= m_csPlayer->Tick( timeDelta );
	ret |= TBaseClass::OnTick( timeDelta );

	return ret;
}

void CStorySceneCutsceneChoiceInstanceData::OnStop()
{
	m_csPlayer->Stop();

	TBaseClass::OnStop();
}

Float CStorySceneCutsceneChoiceInstanceData::OnSeek( Float newTime )
{
	m_csPlayer->Seek( newTime );

	TBaseClass::OnSeek( newTime );

	return m_csPlayer->GetCurrentTime();
}

void CStorySceneCutsceneChoiceInstanceData::OnPaused( Bool flag )
{
	m_csPlayer->Pause( flag );

	TBaseClass::OnPaused( flag );
}

Bool CStorySceneCutsceneChoiceInstanceData::IsReady() const
{
	Bool ret = false;

	ret |= m_csPlayer->IsReady();
	ret |= TBaseClass::IsReady();

	return ret;
}

void CStorySceneCutsceneChoiceInstanceData::GetUsedEntities( TDynArray< CEntity* >& usedEntities ) const
{
	m_csPlayer->GetUsedEntities( usedEntities );

	TBaseClass::GetUsedEntities( usedEntities );
}

const CCameraComponent* CStorySceneCutsceneChoiceInstanceData::GetCamera() const
{
	return static_cast< const StorySceneCutscenePlayerInstanceData* >( m_csPlayer )->GetCamera();
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
