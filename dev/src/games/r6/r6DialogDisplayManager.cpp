
#include "build.h"

#include "idSystem.h"
#include "idInterlocutor.h"
#include "idGraphBlockText.h"
#include "idGraphBlockChoice.h"

#include "r6DialogDisplayManager.h"

#include "r6Hud.h"
#include "r6HudModule.h"
#include "r6GuiManager.h"
#include "../../common/engine/guiGlobals.h"


IMPLEMENT_ENGINE_CLASS( CR6DialogDisplay );


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6DialogDisplay::CR6DialogDisplay()
	: m_active					( true )
	, m_selectedChoices			( 0 )
	, m_clearLineCooldownCur	( 0.0f )
	, m_clearLineCooldownTotal	( 1.0f )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnGameStart( const CGameInfo& gameInfo )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnGameEnd( const CGameInfo& gameInfo )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnWorldStart( const CGameInfo& gameInfo )
{
	TBaseClass::OnWorldStart( gameInfo );

	GCommonGame->GetSystem < CInteractiveDialogSystem > ()->AttachHud( GGame->GetActiveWorld(), this );

	RED_LOG( Dialog, TXT("HUD: world start") );

	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		m_shownChoices[ i ] = nullptr;
	}
	m_selectedChoices = 0;

	GGame->GetInputManager()->RegisterListener( this, CNAME( GC_DialogOption_1 ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GC_DialogOption_2 ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GC_DialogOption_3 ) );
	GGame->GetInputManager()->RegisterListener( this, CNAME( GC_DialogOption_4 ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnWorldEnd( const CGameInfo& gameInfo )
{
	m_playingLines.Clear();
	GCommonGame->GetSystem < CInteractiveDialogSystem > ()->DetachHud( GGame->GetActiveWorld(), this );

	RED_LOG( Dialog, TXT("HUD: world end") );

	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		m_shownChoices[ i ] = nullptr;
	}

	if ( m_hudDialogueModule )
	{
		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueOptionRemovedAll ) );
		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueEnded ) );
		m_hudDialogueModule = NULL;
	}
	
	GGame->GetInputManager()->UnregisterListener( this, CNAME( GC_DialogOption_1 ) );
	GGame->GetInputManager()->UnregisterListener( this, CNAME( GC_DialogOption_2 ) );
	GGame->GetInputManager()->UnregisterListener( this, CNAME( GC_DialogOption_3 ) );
	GGame->GetInputManager()->UnregisterListener( this, CNAME( GC_DialogOption_4 ) );

	m_selectedChoices = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6DialogDisplay::InitHudModule ()
{
	m_selectedChoices = 0;

	CR6GuiManager* guiManager = static_cast< CR6GuiManager* >( GCommonGame->GetGuiManager() );
	RED_ASSERT( guiManager );
	if ( !guiManager )
	{
		GUI_ERROR( TXT("No GUI manager available?") );
		return false;
	}

	CR6Hud* hud = static_cast< CR6Hud* >( guiManager->GetHud() );
	RED_ASSERT( hud );
	if ( !hud )
	{
		GUI_ERROR( TXT("No hud available?") );
		return false;
	}

	CR6HudModule* hudModule = static_cast< CR6HudModule* >( hud->GetChild( TXT("DialogueModule") ) );
	if ( !hudModule )
	{
		GUI_ERROR( TXT("No dialog hud module available?") );
		return false;
	}

	Bool valid	= IsValidObject( hudModule );
	RED_ASSERT( valid );
	if ( !valid )
	{
		GUI_ERROR( TXT("No dialog hud module available?") );
		return false;
	}

	m_hudDialogueModule = hudModule;
	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::Tick( Float timeDelta )
{
	if ( m_hudDialogueModule == NULL || !IsValidObject( m_hudDialogueModule ) )
	{
		if ( InitHudModule () == false )
		{
			return;
		}
	}

	if ( !m_playingLines.Empty() )
	{
		for ( Uint32 i = 0; i < m_playingLines.Size(); ++i )
		{
			SPlayingLine& line = m_playingLines[ i ];

			// show the line?
			if ( line.ShouldPlay() )
			{
				String name								= line.m_speaker->GetLocalizedName();
				String text								= line.m_line->m_text.GetString();
				CIDInterlocutorComponent*	receiver	= line.m_speaker->GetToWhomAmITalking();

				if( receiver )
				{
					String receiverName;
					receiverName	= receiver->GetLocalizedName();
					m_hudDialogueModule->CallEvent(	CNAME( OnDialogueLineChangedWithTarget ), name, receiverName, text );
				}
				else
				{
					m_hudDialogueModule->CallEvent(	CNAME( OnDialogueLineChanged ), name, text );
				}

				// Line needs to be displayed once
				line.m_needsToDisplay	= false;
			}
		}

		m_clearLineCooldownCur	= 0.0f;
	}
	// Delete lines
	else
	{
		// If there are active choices, let the lines live
		for( Uint32 i = 0; i < CHOICE_Max; ++i )
		{
			if( m_shownChoices[i] != nullptr )
			{
				m_clearLineCooldownCur	= 0.0f;
				break; 
			}
		}
		// Check the cooldown
		if( m_clearLineCooldownCur < m_clearLineCooldownTotal )
		{
			m_clearLineCooldownCur	+= timeDelta;
			if( m_clearLineCooldownCur >= m_clearLineCooldownTotal )
			{
				DialogLineDisplayClear();
			}
		}
	}

	UpdateInput();
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnGenerateDebugFragments( CRenderFrame* frame )
{
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::PlayDialogLine( CIDInterlocutorComponent* interlocutor, const SIDBaseLine& line )
{
	SPlayingLine& pl = GetLineByInterlocutor( interlocutor );
	pl.m_line = &line;
	
	pl.m_needsToDisplay = true;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::EndDialogLine( CIDInterlocutorComponent* interlocutor )
{
	DeleteLineByInterlocutor( interlocutor );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::ClearDialogLines()
{
	m_playingLines.Clear();
}

//---------------------------------------------------------------------------------------------------------------------------------
// TODO: We need a better way to clear the line
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::DialogLineDisplayClear()
{
	m_hudDialogueModule->CallEvent(	CNAME( OnClearDialogueLine ) );
	//m_hudDialogueModule->CallEvent(	CNAME( OnDialogueLineChanged ), String::EMPTY, String::EMPTY );
	m_clearLineCooldownCur = m_clearLineCooldownTotal;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnFocusedTopicInterrupted()
{
	DialogLineDisplayClear();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::SetMode( EDialogDisplayMode comunicatorMode )
{
	m_hudDialogueModule->CallEvent( CNAME( OnSetDisplayMode ), comunicatorMode );
	/*
	switch( comunicatorMode )
	{
	case DDM_ActiveDialog:
		m_hudDialogueModule->CallEvent( On)
		break;
	case DDM_IncomingCall:
		break;
	case DDM_ActiveCall:
		break;
	case DDM_SMSReceived:
		break;
	}
	*/
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnInterrupted()
{
	if( m_hudDialogueModule )
	{
		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueInterrupted ) );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnDialogEnded()
{
	if( m_hudDialogueModule )
	{
		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueEnded ) );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::AddChoice( const SIDOption& choice )
{
	if ( RemoveChoiceInternal( choice.m_hudPosition ) )
	{
		ERR_R6( TXT("Dialog: replaced choice on position %s with a new choice %s"), CEnum::ToString( choice.m_hudPosition ).AsChar(), choice.m_text.GetString().AsChar() );
	}

	RED_LOG( Dialog, TXT("HUD: add choice: %ld"), choice.m_hudPosition );

	m_shownChoices[ choice.m_hudPosition ] = &choice;
	EIDChoiceType	type			= choice.m_type;
	const CEntity*	object			= choice.m_relatedObject.Get();
	Bool			hasObject		= object != NULL;
	String			additionalInfo;

	// Additional info
	if( hasObject )
	{
		additionalInfo	= object->GetDisplayName();
	}

	// Correct the type
	switch( type )
	{
		case  IDCT_Default :
			type	= hasObject ? IDCT_Object : IDCT_Default;
		break;
		case IDCT_Quest :
			type	= hasObject ? IDCT_QuestObject : IDCT_Quest;
			break;
		case IDCT_Object :
			type	= hasObject ? IDCT_Object : IDCT_Default;
			break;
		case IDCT_QuestObject :
			type	= hasObject ? IDCT_QuestObject : IDCT_Quest;
			break;
	}

	// Send the choice
	m_hudDialogueModule->CallEvent(	CNAME( OnDialogueOptionAdded ), String::EMPTY, choice.m_hudPosition + 1, choice.m_text.GetString(), type, additionalInfo );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::RemoveChoice( const SIDOption& choice )
{
	RemoveChoiceInternal( choice.m_hudPosition );

	Uint32 numChoices( 0 );
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		numChoices += m_shownChoices[ i ] ? 1 : 0;
	}

	if ( numChoices == 0 )
	{
		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueOptionRemovedAll ) );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CR6DialogDisplay::RemoveChoiceInternal( EHudChoicePosition position )
{
	if ( m_shownChoices[ position ] )
	{
		RED_LOG( Dialog, TXT("HUD: removed choice: %ld"), position );

		m_hudDialogueModule->CallEvent(	CNAME( OnDialogueOptionRemoved ), String::EMPTY, position + 1 );	
		m_shownChoices[ position ] = nullptr;
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::RemoveAllChoices()
{
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		RemoveChoiceInternal( EHudChoicePosition( i ) );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::OnChoiceSelected( EHudChoicePosition pos ) const
{
	if ( m_shownChoices[ pos ] )
	{
		RED_LOG( Dialog, TXT("HUD: OnChoiceSelected(): %ld"), pos );
		GCommonGame->GetSystem< CInteractiveDialogSystem > ()->OnChoiceSelected( pos );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::UpdateInput() 
{
	// We will select one only if the player is choosing a single option (thus avoiding diagonals)
	if ( m_selectedChoices == 1 )		OnChoiceSelected( CHOICE_Left );
	else if ( m_selectedChoices == 2 )	OnChoiceSelected( CHOICE_Right );
	else if ( m_selectedChoices == 4 )	OnChoiceSelected( CHOICE_Up );
	else if ( m_selectedChoices == 8 )	OnChoiceSelected( CHOICE_Down );

	m_selectedChoices = 0;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
CR6DialogDisplay::SPlayingLine& CR6DialogDisplay::GetLineByInterlocutor( CIDInterlocutorComponent* speaker )
{
	for ( Uint32 i = 0; i < m_playingLines.Size(); ++i )
	{
		if ( speaker == m_playingLines[ i ].m_speaker )
		{
			return m_playingLines[ i ];
		}
	}

	// not found, add one
	Uint32 idx = Uint32( m_playingLines.Grow() );
	m_playingLines[ idx ].m_speaker = speaker;
	return m_playingLines[ idx ];
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::DeleteLineByInterlocutor( CIDInterlocutorComponent* speaker )
{
	for ( Uint32 i = 0; i < m_playingLines.Size(); ++i )
	{
		if ( speaker == m_playingLines[ i ].m_speaker )
		{
			m_playingLines.RemoveAt( i );
			return;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Bool CR6DialogDisplay::OnGameInputEvent( const SInputAction & action )
{
	const CName actions[] = { CNAME( GC_DialogOption_1 ), CNAME( GC_DialogOption_2 ), CNAME( GC_DialogOption_3 ), CNAME( GC_DialogOption_4 ) };
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		if ( action.m_aName == actions[ i ] && action.m_value > 0.f )
		{
			m_selectedChoices |= ( 1 << i );
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CR6DialogDisplay::funcSetActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, _ActiveB, false );

	m_active = _ActiveB;
}
