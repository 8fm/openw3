#include "build.h"

#include "questCondition.h"
#include "factsDB.h"

#include "storyScene.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "storySceneChoiceLineAction.h"
#include "../engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneChoiceLine );
IMPLEMENT_ENGINE_CLASS( ISceneChoiceMemo );
IMPLEMENT_ENGINE_CLASS( CFactsDBChoiceMemo );

RED_DEFINE_STATIC_NAME(choiceLine);
RED_DEFINE_STATIC_NAME(choiceComment);

CStorySceneChoiceLine::CStorySceneChoiceLine() 
	: m_questCondition( NULL )
	, m_emphasisLine( false )
	, m_singleUseChoice( false )
{
	//m_choiceLine = CreateObject< CLocalizedContent >( this );
	//m_choiceComment = CreateObject< CLocalizedContent >( this );
}

Bool CStorySceneChoiceLine::IsHidden() const
{
	Bool hidden = false;
	if ( m_singleUseChoice )
	{
		hidden = WasChoosen();		
	}
	if ( m_questCondition != NULL && !hidden )
	{
		Bool questConditionResult;
		m_questCondition->Activate();
		questConditionResult = m_questCondition->IsFulfilled();
		m_questCondition->Deactivate();
		hidden = !questConditionResult;
	}
	return hidden;
}

Bool CStorySceneChoiceLine::IsDisabled() const
{
	return m_action && !m_action->CanUseAction();
}

CName CStorySceneChoiceLine::GetPlayGoChunk() const
{
	if (m_action)
	{
		return m_action->GetPlayGoChunk();
	}

	return CName( TXT("content0") );
}

void CStorySceneChoiceLine::OnChoiceSelected( CStoryScenePlayer* player ) const
{
	// Execute persistence
	for ( TDynArray< ISceneChoiceMemo* >::const_iterator it = m_memo.Begin();
		it != m_memo.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Persist();
		}
	}

	// Save choose fact 
	String choiceLineFactId = GetChoiceLineId();	
	if ( choiceLineFactId.Empty() == false )
	{
		const EngineTime& time = GGame->GetEngineTime();
		// DIALOG_TOMSIN_TODO

		player->DbFactAdded( choiceLineFactId );
		GCommonGame->GetSystem< CFactsDB >()->AddFact( choiceLineFactId, 1, time, CFactsDB::EXP_ACT_END );
	}

	// Perform choice line action
	if ( m_action != NULL )
	{
		m_action->PerformAction();
	}
}

CStorySceneChoice* CStorySceneChoiceLine::GetChoice() const
{
	return SafeCast< CStorySceneChoice >( GetParent() );
}

static String StripChoiceLineText( const String& text )
{
	// Strip white spaces at the start and at the end of the choice line
	const Char* ch = text.AsChar();
	String outText;
	while ( *ch >= ' ')
	{
		// Skip initial white spaces
		if ( outText.GetLength() == 0 && *ch <= ' ' )
		{
			ch++;			
		}
		else
		{
			outText += String::Chr( *ch );
			ch++;
		}
	}

	return outText;
}

void CStorySceneChoiceLine::SetChoiceLine( String newValue )
{
	// Update the choice name
	m_choiceLine.SetString( StripChoiceLineText( newValue ) );

	// Notify parent
	CStorySceneChoice *parentChoice = GetChoice();
	parentChoice->NotifyAboutChoiceLineChanged( this );
}

void CStorySceneChoiceLine::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	// Notify parent
	CStorySceneChoice *parentChoice = GetChoice();
	parentChoice->NotifyAboutChoiceLinksChanged( this );
}

void CStorySceneChoiceLine::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	// Notify parent
	CStorySceneChoice *parentChoice = GetChoice();
	parentChoice->NotifyAboutChoiceLinksChanged( this );
}

Bool CStorySceneChoiceLine::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == CNAME(choiceLine) )
	{
		CLocalizedContent *locCont = *( CLocalizedContent **) readValue.GetData();

		ASSERT( locCont->GetIndex() != 0 && TXT("Localization Content index equals zero!") );
		m_choiceLine.SetIndex( locCont->GetIndex() );


		return true;
	}
	else if ( propertyName == CNAME(choiceComment) )
	{
		CLocalizedContent *locCont = *( CLocalizedContent **) readValue.GetData();

		ASSERT( locCont->GetIndex() != 0 && TXT("Localization Content index equals zero!") );
		m_choiceComment.SetIndex( locCont->GetIndex() );


		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

String CStorySceneChoiceLine::GetChoiceLineId() const
{
	CStoryScene* scene = FindParent< CStoryScene >();
	CStorySceneChoice* choice = GetChoice();

	if ( scene != NULL && choice != NULL )
	{
		return String::Printf( TXT( "%u_%u_%d" ), scene->GetSceneIdNumber(), choice->GetSection()->GetSectionId(), choice->GetChoiceLineIndex( this ) );
	}
	return String::EMPTY;
}

Bool CStorySceneChoiceLine::WasChoosen() const
{
	// DIALOG_TOMSIN_TODO
	CFactsDB *factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >(): NULL;
	String choiceLineFactId = GetChoiceLineId();
	if ( factsDB != NULL && choiceLineFactId.Empty() == false )
	{
		return CFactsDBEditorQuery::Evaluate( *factsDB, QF_DoesExist, choiceLineFactId, 1, CF_Equal );
	}
	return false;
}

String CStorySceneChoiceLine::GetChoiceActionText() const
{
	return m_action ? m_action->GetActionText() : String::EMPTY;
}

EDialogActionIcon CStorySceneChoiceLine::GetChoiceActionIcon() const
{
	return m_action ? m_action->GetActionIcon() : DialogAction_NONE;
}

///////////////////////////////////////////////////////////////////////////////

CFactsDBChoiceMemo::CFactsDBChoiceMemo()
	: m_value( 0 )
{
}

void CFactsDBChoiceMemo::Persist() const
{
	if ( m_factID.GetLength() == 0 )
	{
		return;
	}

	const EngineTime& time = GGame->GetEngineTime();
	// DIALOG_TOMSIN_TODO
	GCommonGame->GetSystem< CFactsDB >()->AddFact( m_factID, m_value, time, CFactsDB::EXP_NEVER );

}
