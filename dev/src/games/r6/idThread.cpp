/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idThread.h"
#include "idBasicBlocks.h"
#include "idGraphBlockBranch.h"
#include "idGraphBlockChoice.h"
#include "idGraphBlockCheckpoint.h"
#include "idTopic.h"
#include "idInstance.h"


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDThreadInstance::CIDThreadInstance( CIDTopicInstance* parent, const CIDGraphBlockInput* input )
	: m_parent( parent )
	, m_input( input )
	, m_currentBlock( NULL )
	, m_checkpoint( NULL )
	, m_lastSelectedChoiceBlock( NULL )
{	
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		m_choiceOptions->m_active	= false;
	}

	SetPlayState( DIALOG_Ready );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDThreadInstance::~CIDThreadInstance()
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnStart( Float timeDelta )
{
	m_currentBlock	= m_input;
	m_checkpoint	= m_input;
	m_currentBlock->ActivateInput( m_parent, timeDelta, NULL, false );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::Tick( Float timeDelta )
{
	EIDPlayState	state	= GetPlayState();
	R6_ASSERT( state != DIALOG_Finished && state != DIALOG_Error )

	if ( state == DIALOG_Ready || state == DIALOG_Playing )
	{

		// Show the choices or not
		for ( Uint32 i = 0; i < CHOICE_Max; ++i )
		{
			Bool visibleNow	= m_choiceOptions[ i ].m_active && m_choiceOptions[i].m_option->CanOptionBeShown( m_parent );
			if( m_choiceOptions[i].m_visibleLastFrame != visibleNow )
			{
				if( visibleNow )
				{
					m_parent->GetDialogInstance()->AddChoice( *m_choiceOptions[i].m_option );
				}
				else
				{
					m_parent->GetDialogInstance()->RemoveChoice( *m_choiceOptions[i].m_option );
				}
			}
			m_choiceOptions[ i ].m_visibleLastFrame	= m_choiceOptions[ i ].m_visibleNow;
			m_choiceOptions[ i ].m_visibleNow		= visibleNow;

			if( m_choiceOptions[ i ].m_active )
			{
				SetPlayState( DIALOG_Playing );
			}
		}

		// update the branches
		for ( Uint32 i = 0; i < m_currentBranches.Size(); ++i )
		{
			const CIDGraphBlockBranch* block = m_currentBranches[ i ];
			const CIDGraphBlock* nextBlock = block->Update( m_parent );

			// break if branch block changed the execution flow
			if ( nextBlock != block )
			{
				m_currentBlock = nextBlock;
				m_finishedBranches.PushBack( block );
				break;
			}
		}

		// Kill the selected branches
		for ( Uint32 i = 0; i < m_finishedBranches.Size(); ++i )
		{
			const CIDGraphBlockBranch* branch = m_finishedBranches[ i ];

			m_currentBranches.Remove( branch );
		}
		m_finishedBranches.ClearFast();

		// evaluate the current block
		if ( m_currentBlock )
		{
			R6_ASSERT( m_currentBlock->IsActivated( m_parent ) );
			m_currentBlock	= m_currentBlock->Evaluate( m_parent, timeDelta );
			
			if ( m_currentBlock )
			{
				R6_ASSERT( m_currentBlock->IsRegular(), TXT("Non-regular blocks (branch, choice) should never become m_currentBlock") );
				SetPlayState( DIALOG_Playing );
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnInterrupted()
{
	if ( m_currentBlock )
	{
		// deactivate current block
		m_currentBlock->SetActive( m_parent, false ); 
		
		SetPlayState( DIALOG_Interrupted );
	}

	// remove  current choices
	m_parent->GetDialogInstance()->RemoveAllChoices();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnResume( Float& timeDelta )
{
	Bool anychoice( false );

	// re-add current choices
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		if ( m_choiceOptions[ i ].m_active )
		{
			if( m_choiceOptions[ i ].IsVisibleNow() )
			{
				anychoice	= true;
				m_parent->GetDialogInstance()->AddChoice( *m_choiceOptions[ i ].m_option );
			}
		}
	}

	EIDPlayState	state	=	GetPlayState();
	if ( state != DIALOG_Finished || state != DIALOG_Error )
	{
		// Go to the checkpoint
		m_currentBlock	= m_checkpoint;

		// re-activate current block
		if( m_currentBlock )
		{
			InstanceBuffer& data = m_parent->GetInstanceData();
			m_currentBlock->ActivateInput( m_parent, timeDelta, m_currentBlock->GetCurrentInput( data ), false ); 
		}

		// mark playing state
		SetPlayState( DIALOG_Playing );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::JumpTo( const CIDGraphSocket* output )
{
	R6_ASSERT( output && output->GetDirection() == LSD_Output );
	const TDynArray< CGraphConnection* >& connections = output->GetConnections();
	if ( connections.Empty() )
	{
		// treat it as dead end
		m_currentBlock = NULL;
		SetPlayState( DIALOG_Playing );
		OnDeadEndEncountered();
		return;
	}

	const CIDGraphBlock* block = Cast< const CIDGraphBlock > ( output->GetBlock() );
	R6_ASSERT( block );

	Float timeDelta = 0.f;
	const CIDGraphBlock* newBlock = block->ActivateOutput( m_parent, timeDelta, output, false ); 
	if ( newBlock && newBlock->IsRegular() )
	{
		if ( m_currentBlock )
		{
			// m_currentBlock should be active
			R6_ASSERT( m_currentBlock->IsActivated( m_parent ) );

			// deactivate current block
			m_currentBlock->SetActive( m_parent, false );
		}

		// set new block as current
		m_currentBlock = newBlock;
	}
	else if ( newBlock == NULL )
	{
		m_currentBlock = NULL;
	}

	SetPlayState( DIALOG_Playing );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::AddChoiceOption( const SIDOption& option, const CIDGraphSocket* output, Int8 outputIndex )
{
	R6_ASSERT( nullptr != output );

	const EHudChoicePosition pos				= option.m_hudPosition;
	m_choiceOptions[ pos ].m_option				= &option;
	m_choiceOptions[ pos ].m_output				= output;
	m_choiceOptions[ pos ].m_outputIndex		= outputIndex;
	m_choiceOptions[ pos ].m_active				= true;
	m_choiceOptions[ pos ].m_visibleLastFrame	= false;
	m_choiceOptions[ pos ].m_visibleNow			= false;

	RED_LOG( Dialog, TXT("Set choice %s on position: %ld"), option.m_text.GetString(), option.m_hudPosition );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::ClearChoiceOption( EHudChoicePosition position )
{
	m_choiceOptions[ position ].m_active	= false;
	if( m_choiceOptions[ position ].m_active )
	{
		RED_LOG( Dialog, TXT("Cleaning position: %ld that had text: %s"), position, m_choiceOptions[ position ].m_option->m_text.GetString() );
	}
}


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CIDThreadInstance::IsAnyOptionActive()
{
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		if( m_choiceOptions[ i ].m_active )
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnRegularBlockEncountered( const CIDGraphBlock* block )
{
#ifndef NO_DEBUG_PAGES
	if ( m_lastBlocks.Empty() || m_lastBlocks.Back() != block )
	{
		m_lastBlocks.PushBack( block );
	}
#endif

	// AddCheckpoint
	if( block->IsCheckpoint() )
	{
		SetCheckpoint( block );
	}
}


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnBranchBlockEncountered( const CIDGraphBlockBranch* block )
{
	m_currentBranches.PushBackUnique( block );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnChoiceBlockEncountered( const CIDGraphBlockChoice* block )
{
	block->OnEncoutered( m_parent );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::ActivateChoiceOutput( const CIDGraphSocket* output )
{
	R6_ASSERT( output );
	if ( output )
	{
		JumpTo( output );
	}

	m_lastSelectedChoiceBlock = Cast< const CIDGraphBlockChoice > ( output->GetBlock() );
	R6_ASSERT( m_lastSelectedChoiceBlock, TXT("Output does nto correspond to a block ?!") );

	if( m_lastSelectedChoiceBlock )
	{
		// Clear all choices from the block
		if( m_lastSelectedChoiceBlock->FindOption( CHOICE_Down ) )		ClearChoiceOption( CHOICE_Down );
		if( m_lastSelectedChoiceBlock->FindOption( CHOICE_Up ) )		ClearChoiceOption( CHOICE_Up );
		if( m_lastSelectedChoiceBlock->FindOption( CHOICE_Left ) )		ClearChoiceOption( CHOICE_Left );
		if( m_lastSelectedChoiceBlock->FindOption( CHOICE_Right ) )		ClearChoiceOption( CHOICE_Right );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnChoiceSelected( EHudChoicePosition position )
{
	if ( m_choiceOptions[position].IsVisibleNow() )
	{
		ActivateChoiceOutput( m_choiceOptions[position].m_output );
	}

	// TODO: Remember the selected index if needed, m_choiceOptions[position].m_outputIndex
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnFinished()
{
	m_currentBranches.ClearFast();

	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		m_choiceOptions[ i ].m_active = false;
	}
	m_parent->GetDialogInstance()->RemoveAllChoices();
	
	SetPlayState( DIALOG_Finished );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDThreadInstance::OnDeadEndEncountered()
{
	RED_LOG( Dialog, TXT("Dead end encoutered in thread '%ls', topic '%ls'"), m_input->GetName().AsString().AsChar(), m_parent->GetTopic()->GetName().AsString().AsChar() ); 

	if ( m_lastSelectedChoiceBlock )
	{
		// we reached a point of no return in the graph - nothing is connected, signal have nowhere to go
		// by design we should re-display last choice block in such a case
		OnChoiceBlockEncountered( m_lastSelectedChoiceBlock );
	}
}
