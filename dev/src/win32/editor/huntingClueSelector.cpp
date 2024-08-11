#include "build.h"

#include "huntingClueSelector.h"

#include "../../games/r4/journalQuestHuntingBlock.h"

#include "../../common/game/journalPath.h"
#include "../../games/r4/journalQuest.h"
#include "../../games/r4/journalCreature.h"
#include "../../games/r4/r4GameResource.h"

CEdJournalHuntingClueSelector::CEdJournalHuntingClueSelector( CPropertyItem* propertyItem )
:	ICustomPropertyEditor( propertyItem ),
	m_choice( NULL )
{
}

CEdJournalHuntingClueSelector::~CEdJournalHuntingClueSelector()
{

}

void CEdJournalHuntingClueSelector::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxArrayString choices;

	ASSERT( m_propertyItem->GetRootObject( 0 ).As< CJournalQuestHuntingBlock >(), TXT( "Need to figure out how to specify the creature path generically" ) );
	
	// Grab the game resource with the list of all hunting clues
	CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );

	Uint32 selectedIndex = 0;

	const CJournalCreatureHuntingClueGroup* group = GetHuntingClueGroup();
	if( group != NULL )
	{
		THandle< CJournalPath > path = NULL;
		m_propertyItem->Read( &path );

		CJournalBase* selectedClue = ( path && path->IsValid() )? path->GetTarget() : NULL;

		for( Uint32 iClue = 0; iClue < group->GetNumChildren(); ++iClue )
		{
			ASSERT( group->GetChild( iClue )->IsA< CJournalCreatureHuntingClue >(), TXT( "Only Hunting Clues may be placed in a hunting clue group in the journal editor" ) );

			const CJournalCreatureHuntingClue* clue = static_cast< const CJournalCreatureHuntingClue* >( group->GetChild( iClue ) );

			choices.Add( clue->GetName().AsChar() );

			if( selectedClue && selectedClue->GetGUID() == clue->GetGUID() )
			{
				selectedIndex = iClue;
			}
		}
	}

	if( choices.IsEmpty() )
	{
		// TODO: Better error reporting
		choices.Add( TXT( "Error" ) );
	}

	ASSERT( m_choice == NULL, TXT( "Widgets should have been cleared by OnLostFocus()" ) );

	m_choice = new wxChoice( m_propertyItem->GetPage(), wxID_ANY, propertyRect.GetTopLeft(), propertyRect.GetSize(), choices );
	m_choice->SetSelection( selectedIndex );

	m_choice->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdJournalHuntingClueSelector::OnClueSelected, this );
}

void CEdJournalHuntingClueSelector::CloseControls()
{
	if( m_choice != NULL )
	{
		m_choice->Destroy();
		m_choice = NULL;
	}
}

Bool CEdJournalHuntingClueSelector::GrabValue( String& displayValue )
{
	// Grab the game resource with the list of all hunting clues
	CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );

	THandle< CJournalPath > path = NULL;
	m_propertyItem->Read( &path );

	if( path )
	{
		displayValue = TXT( "Invalid Path" );
		if( path->IsValid() )
		{
			CJournalBase* clue = path->GetTarget();

			if( clue )
			{
				ASSERT( clue->IsA< CJournalCreatureHuntingClue >(), TXT( "only hunting clues may be placed in a hunting clue group in the journal editor" ) );

				displayValue = clue->GetName();
			}
		}

		return true;
	}

	return false;
}

void CEdJournalHuntingClueSelector::OnClueSelected( wxCommandEvent& event )
{
	Uint32 selection = event.GetSelection();

	CJournalCreatureHuntingClueGroup* group = GetHuntingClueGroup();

	if( group != NULL )
	{
		CJournalCreatureHuntingClue* clue = static_cast< CJournalCreatureHuntingClue* >( group->GetChild( selection ) );
		THandle< CJournalPath > path = CJournalPath::ConstructPathFromTargetEntry( clue );
		CJournalQuestHuntingBlock* huntingBlock = m_propertyItem->GetRootObject( 0 ).As< CJournalQuestHuntingBlock >();

		m_propertyItem->Write( &path );
	}
}

const CJournalCreatureHuntingClueGroup* CEdJournalHuntingClueSelector::GetHuntingClueGroup() const
{
	// Grab the game resource with the list of all hunting clues
	CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );

	// Get the quest block we're attached to
	const CJournalQuestHuntingBlock* huntingBlock = m_propertyItem->GetRootObject( 0 ).As< const CJournalQuestHuntingBlock >();
	ASSERT( huntingBlock->IsA< CJournalQuestHuntingBlock >(), TXT( "Not a CJournalQuestHuntingBlock but a '%s'" ), huntingBlock->GetClass()->GetName().AsString().AsChar() );

	// Get the journal path to the hunting tag from the quest block
	THandle< const CJournalPath > pathToHuntingQuest = huntingBlock->GetHuntingQuestPath();

	if( pathToHuntingQuest && pathToHuntingQuest->IsValid() )
	{
		// Resolve the path
		const CJournalQuest* huntingQuest = pathToHuntingQuest->GetTargetAs< CJournalQuest >();

		if( huntingQuest && huntingQuest->IsMonsterHuntQuest() )
		{
			THandle< CJournalPath > pathToCreature = huntingQuest->GetHuntingQuestCreaturePath();
			if( pathToCreature && pathToCreature->IsValid() )
			{
				const CJournalCreature* creature = pathToCreature->GetTargetAs< CJournalCreature >();

				if( creature )
				{
					return creature->GetFirstChildOfType< CJournalCreatureHuntingClueGroup >();
				}
			}
		}
	}

	return NULL;
}
