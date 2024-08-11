/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idBasicBlocks.h"
#include "idThread.h"
#include "r6DialogDisplayManager.h"
#include "idTopic.h"
#include "idSystem.h"
#include "idInstance.h"
#include "idThreadActivator.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphSocket.h"
#include "../../common/engine/graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CIDTopic )

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDTopic::CIDTopic()
	: m_graph( nullptr )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopic::SetupTheGraph()
{
	if ( m_graph == nullptr )
	{
		m_graph = CreateObject< CIDGraph > ( this );
		
		#ifndef NO_EDITOR		
		if ( GIsEditor )
		{
			GraphBlockSpawnInfo infoInput( CIDGraphBlockInput::GetStaticClass() );
			CIDGraphBlockInput* createdInput = Cast< CIDGraphBlockInput >( m_graph->GraphCreateBlock( infoInput ) );
			createdInput->OnCreatedInEditor();

			GraphBlockSpawnInfo infoOutput( CIDGraphBlockOutput::GetStaticClass() );
			CIDGraphBlockOutput* createdOutput = Cast< CIDGraphBlockOutput >( m_graph->GraphCreateBlock( infoOutput ) );
			createdOutput->OnCreatedInEditor();

			createdOutput->SetPosition( createdInput->GetPosition() + Vector( 100, 0, 0 ) );

			createdInput->GetSockets()[ 0 ]->ConnectTo( createdOutput->GetSockets()[ 0 ] );
		}
		#endif	

		OnPostChange();
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopic::CompileDataLayout()
{
	InstanceDataLayoutCompiler compiler( m_dataLayout );

	Int32 count = GetGraph()->GraphGetBlocks().SizeInt();
	for ( Int32 i = 0; i < count; ++i )
	{
		CIDGraphBlock* block = Cast< CIDGraphBlock >( GetGraph()->GraphGetBlocks()[ i ] );
		R6_ASSERT( block, TXT("There is a NULL in the graph block array") );

		if ( block )
		{
			block->OnBuildDataLayout( compiler );
		}
		else
		{
			// remove invalid entry from the array to avoid crash
			GetGraph()->GraphGetBlocks().RemoveAt( i );
			count = GetGraph()->GraphGetBlocks().SizeInt();
			--i;
		}
	}

	// Create layout
	m_dataLayout.ChangeLayout( compiler );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopic::OnPostChange()
{
	CompileDataLayout();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopic::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	m_graph->GetLocalizedStrings( localizedStrings );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
#ifndef NO_EDITOR
void CIDTopic::EnsureBlockNameUniqueness()
{
	// Editor only code
	R6_ASSERT( GIsEditor, TXT("CIDTopic::EnsureBlockNameUniqueness() should be called ONLY in editor.") );

	for ( BlockIterator< CIDGraphBlock > it( this ); it; ++it )
	{
		EnsureBlockNameUniqueness( *it );
	}
}

void CIDTopic::EnsureBlockNameUniqueness( CIDGraphBlock* block )
{
	// Editor only code
	R6_ASSERT( GIsEditor, TXT("CIDTopic::EnsureBlockNameUniqueness() should be called ONLY in editor.") );

	struct Local
	{
		static String StripNumPart( const String& string )
		{
			Int32 i = Int32( string.GetLength() ) - 1;
			for ( ; i >= 0; --i )
			{
				if ( string[ i ] < L'0' || string[ i ] > L'9' )
				{
					break;
				}
			}
			return string.LeftString( i + 1 );
		}

		static CName RecomposeName( CName name, Uint32 numPart )
		{
			String nonNumericPart = StripNumPart( name.AsString() );
			return CName( String::Printf( TXT("%s%ld"), nonNumericPart.AsChar(), numPart ).AsChar() );
		}
	};

	Uint32 numPart( 0 );

	Loop:
	for ( BlockIterator< CIDGraphBlock > it( this ); it; ++it )
	{
		if ( *it == block )
		{
			continue;
		}

		if ( ( *it  )->GetName() == block->GetName() )
		{
			// mark this resource as modified
			MarkModified();

			// rename
			block->SetName( Local::RecomposeName( block->GetName(), ++numPart ) );

			// check again
			goto Loop;
		}
	}
}

void CIDTopic::GatherBlocksByGraphOrder( const CIDGraphSocket* outputSocket, TDynArray< const CIDGraphBlock* > &blockArray ) const
{
	for ( Uint32 i = 0; i < outputSocket->GetConnections().Size(); ++i ) 
	{
		CGraphSocket *destSocket = outputSocket->GetConnections()[ i ]->GetDestination();
		if ( destSocket == outputSocket )
		{
			destSocket = outputSocket->GetConnections()[ i ]->GetSource();
		}
		R6_ASSERT( destSocket != nullptr, TXT("Connection to nowhere?" ) );
		R6_ASSERT( destSocket != outputSocket, TXT("Socket plugged into itself?" ) );

		CIDGraphBlock* block = Cast< CIDGraphBlock > ( destSocket->GetBlock() );
		if ( blockArray.Exist( block ) )
		{
			// been there, done that...
			return;
		}

		blockArray.PushBack( block );
		for ( Uint32 k = 0; k < block->GetSockets().Size(); ++k )
		{
			CGraphSocket* socket = block->GetSockets()[ k ];
			if ( socket && socket->GetDirection() == LSD_Output )
			{
				GatherBlocksByGraphOrder( Cast< CIDGraphSocket > ( socket ), blockArray );
			}
		}
	}
}

void CIDTopic::GatherBlocksByGraphOrder( TDynArray< const CIDGraphBlock* > &blockArray ) const
{	
	struct SortPredicateXY
	{
		Bool operator()( const CIDGraphBlock* const &a, const CIDGraphBlock* const &b ) const
		{
			const Float ax( a->GetPosition().X ), bx( b->GetPosition().X );
			return ( ax == bx ) ? ( a->GetPosition().Y < b->GetPosition().Y ) : ( ax < bx );
		}
	} sortPredicateXY;

	struct SortPredicateYX
	{
		Bool operator()( const CIDGraphBlock* const &a, const CIDGraphBlock* const &b ) const
		{
			const Float ay( a->GetPosition().Y ), by( b->GetPosition().Y );
			return ( ay == by ) ? ( a->GetPosition().X < b->GetPosition().X ) : ( ay < by );
		}
	} sortPredicateYX;

	// get array of inputs sorted by Y position
	TDynArray< const CIDGraphBlockInput* > sortedInputs;
	GatherBlocks< CIDGraphBlockInput > ( sortedInputs );
	::Sort( sortedInputs.Begin(), sortedInputs.End(), sortPredicateYX );

	// traverse the graph and add blocks to the array in order of appearance
	for ( Uint32 i = 0; i < sortedInputs.Size(); ++i )
	{
		const CIDGraphBlockInput* input = sortedInputs[ i ];
		for ( Uint32 k = 0; k < input->GetSockets().Size(); ++k )
		{
			CGraphSocket* socket = input->GetSockets()[ k ];
			if ( socket && socket->GetDirection() == LSD_Output )
			{
				GatherBlocksByGraphOrder( Cast< CIDGraphSocket > ( socket ), blockArray );
			}
		}	
	}

	// then add unconnected blocks to another array...
	TDynArray< const CIDGraphBlock* > disconnectedBlockArray;
	for ( BlockConstIterator< CIDGraphBlock > it( this ); it; ++it )
	{
		if ( blockArray.Exist( *it ) )
		{
			continue;
		}
		disconnectedBlockArray.PushBack( *it );
	}

	// ...sort it by X position...
	::Sort( disconnectedBlockArray.Begin(), disconnectedBlockArray.End(), sortPredicateXY );

	// ...to add it to the back of blockArray
	blockArray.PushBack( disconnectedBlockArray );
}

const CIDGraphBlock* CIDTopic::GetLastBlockByGraphXPosition()
{
	return m_graph->GetLastBlockByXPosition();
}

void CIDTopic::OnCreatedInEditor()
{
	SetupTheGraph();
	TBaseClass::OnCreatedInEditor();
}

#endif // ifndef NO_EDIOTR

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
// TOPIC INSTANCE
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDTopicInstance::CIDTopicInstance( CInteractiveDialogInstance* parent, const CIDTopic* topic )
	: m_topic( topic )
	, m_data( topic->GetDataLayout().CreateBuffer( GCommonGame->GetSystem< CInteractiveDialogSystem > (), TXT("CIDTopicInstance") ) )	
	, m_parent( parent )
	, m_currentlyTickingThread( INVALID_THREAD )
{
	RED_ASSERT( topic, TXT("Topic is null") );

	InitBlocks( topic );
	SortThreadStarts();

	if ( m_topic == NULL || m_data == NULL )
	{
		SetPlayState( DIALOG_Error );
	}
	else
	{
		SetPlayState( DIALOG_Ready );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDTopicInstance::~CIDTopicInstance()
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::InitBlocks( const CIDTopic* topic )
{
	// Collect all thread starts
	const TDynArray< CGraphBlock* >& blocks = m_topic->GetGraph()->GraphGetBlocks(); 
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		R6_ASSERT( blocks[ i ], TXT("There is a NULL in the graph block array") );
		const CIDGraphBlock* block = Cast< CIDGraphBlock >( blocks[ i ] );
		if ( block )
		{
			block->OnInitInstance( *m_data );

			// Save starting threads
			TryCollectThreadStart( block );
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::TryCollectThreadStart	( const CIDGraphBlock* block )
{
	const CIDGraphBlockInput* input = Cast< CIDGraphBlockInput  > ( block );
	if ( input )
	{
		m_threadStarts.PushBack( input );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::SortThreadStarts()
{
	SIDGraphBlockInputSortingPredicate	predicate;
	Sort(m_threadStarts.Begin(), m_threadStarts.End(), predicate);
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
EIDPriority	CIDTopicInstance::GetPriority()	const	
{	
	CIDActivator*	activator	= m_topic->GetActivator();
	return ( activator == NULL ) ? IDP_Irrelevant : activator->GetPriority();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CIDThreadInstance* CIDTopicInstance::GetCurrentThread()			
{ 
	R6_ASSERT( m_currentlyTickingThread < m_runningThreads.Size(), TXT("Current thread is invalid! on topic %s"), m_topic->GetName().AsString() ); 
	if( m_currentlyTickingThread < m_runningThreads.Size() )
	{
		return m_runningThreads[ m_currentlyTickingThread ]; 
	}

	return NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CIDTopicInstance::GetWantsToPlay() const
{
	const EIDPlayState state = GetPlayState();
	switch ( state )
	{
	case DIALOG_Finished:
		return false;
	case DIALOG_Playing:
		return true;
	case DIALOG_Interrupted:
		return true;
	case DIALOG_Ready:
		{
			return IsActivatorWantingToPlay();
		}
	default:
		return false;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CIDTopicInstance::IsActivatorWantingToPlay	( )	const
{
	CIDActivator* activator = m_topic->GetActivator();

	if( activator == NULL )
	{
		return true;
	}

	// We don't play invalid priorities
	if ( activator->GetPriority() == IDP_Invalid )
	{
		return false;
	}

	return activator->IsFulfilled( *m_data, m_parent->GetInstanceID() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Uint32 CIDTopicInstance::GetDialogInstanceID() const
{ 
	return m_parent->GetInstanceID(); 
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::Resume( Float& timeDelta )
{
	// ED TODO: handle the end of interruption, find a checkpoint and so on	
	Bool anyPlaying( false );
	Bool anyReady( false );
	Bool allFinished( true );
	for ( Uint32 i = 0; i < m_runningThreads.Size(); ++i )
	{
		m_currentlyTickingThread = i;
		m_runningThreads[ i ]->OnResume( timeDelta );

		EIDPlayState state = m_runningThreads[ i ]->GetPlayState();	 
		if ( state == DIALOG_Playing )
		{
			anyPlaying = true;
		}
		if ( state != DIALOG_Finished )
		{ 
			allFinished = false;
		}
		if ( state == DIALOG_Ready )
		{
			anyReady = true;
		}
	}

	m_currentlyTickingThread = INVALID_THREAD;

	if ( anyPlaying )
	{
		SetPlayState( DIALOG_Playing );
	}
	else if ( anyReady )
	{
		SetPlayState( DIALOG_Ready );
	}
	else if ( allFinished )
	{
		OnEnd();
	}
	else
	{
		R6_ASSERT( false, TXT("Unexpected code path. Please DEBUG.") );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::OnStart( Float& timeDelta )
{
	if ( GetPlayState() == DIALOG_Interrupted )
	{
		Resume( timeDelta );
		return;
	}

	// Activate the first valid thread start (they are sorted by priority )
	for ( Uint32 i = 0; i < m_threadStarts.Size(); ++i )
	{
		const CIDGraphBlockInput* threadStart = m_threadStarts[ i ];

		if ( threadStart->GetWantsToPlay( *m_data, m_parent->GetInstanceID() ) )
		{
			CIDThreadInstance*	threadInstance	= new CIDThreadInstance( this, threadStart );
			m_runningThreads.PushBack( threadInstance );
			m_currentlyTickingThread = m_runningThreads.Size() - 1;

			threadInstance->OnStart( timeDelta );

			SetPlayState( DIALOG_Playing );

			break;
		}
	}

	m_currentlyTickingThread = INVALID_THREAD;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::OnEnd()
{
	if ( m_topic->GetCanBeRestarted() )
	{
		SetPlayState( DIALOG_Ready );
	}
	else
	{
		SetPlayState( DIALOG_Finished );
	}

	for ( m_currentlyTickingThread = 0; m_currentlyTickingThread < m_runningThreads.Size(); ++m_currentlyTickingThread )
	{
		m_runningThreads[ m_currentlyTickingThread ]->OnFinished();
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::OnInterrupt()
{
	if ( GetPlayState() == DIALOG_Playing )
	{
		if ( m_topic->GetEndOnInterrupted() )
		{
			OnEnd();
		}
		else
		{
			SetPlayState( DIALOG_Interrupted );
		}
		
		// interrupt all threads
		for ( Uint32 i = 0; i < m_runningThreads.Size(); ++i )
		{
			m_runningThreads[ i ]->OnInterrupted();
		}

		// Clear the display if it has focus
		if( GetDialogInstance()->GetHasFocus() )
		{
			CR6DialogDisplay* display = GCommonGame->GetSystem< CR6DialogDisplay >();
			display->OnFocusedTopicInterrupted();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::Tick( Float timeDelta )
{
	EIDPlayState	state	= GetPlayState();

	if( state == DIALOG_Finished )
	{
		//OnEnd();
		return;
	}

	// for the moment, do not tick interrupted topics
	if( GetPlayState() != DIALOG_Playing )
	{
		return;
	}

	// tick all threads
	TickThreads( timeDelta );

	// Check Finish
	if ( m_runningThreads.Empty() && GetPlayState() != DIALOG_Error )
	{
		OnEnd();
		//SetPlayState( DIALOG_Finished );
	}

	// Update self interruption
	if( m_topic->GetConditionIsContinous() )
	{
		if( !IsActivatorWantingToPlay() )
		{
			OnEnd();
			return;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::TickThreads( Float timeDelta )
{
	// Tick them
	for ( m_currentlyTickingThread = 0; m_currentlyTickingThread < m_runningThreads.Size(); ++m_currentlyTickingThread )
	{
		CIDThreadInstance* threadInstance = m_runningThreads[ m_currentlyTickingThread ];
		EIDPlayState state = threadInstance->GetPlayState();
		R6_ASSERT( state == DIALOG_Playing || state == DIALOG_Ready );

		threadInstance->Tick( timeDelta );
		state = threadInstance->GetPlayState();

		if ( state == DIALOG_Finished )
		{
			m_finishedThreads.PushBack( threadInstance );
		}
		else if ( state == DIALOG_Playing )
		{
			SetPlayState( DIALOG_Playing );
		}
	}

	m_currentlyTickingThread = INVALID_THREAD;

	// handle finished threads
	for ( Uint32 i = 0; i < m_finishedThreads.Size(); ++i )
	{
		CIDThreadInstance* instance = m_finishedThreads[ i ];
		EIDPlayState state = instance->GetPlayState();
		R6_ASSERT( state == DIALOG_Finished );

		m_runningThreads.Remove( instance );
		delete instance;
	}
	m_finishedThreads.ClearFast();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::Fork( const CIDGraphSocket* forkSocket )
{
	R6_ASSERT( m_currentlyTickingThread < m_runningThreads.Size() );

	CIDThreadInstance* parentThread = m_runningThreads[ m_currentlyTickingThread ];
	CIDThreadInstance* newThread = new CIDThreadInstance( this, parentThread->GetInput() );
	m_runningThreads.PushBack( newThread );	

	newThread->JumpTo( forkSocket );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::OnChoiceSelected( EHudChoicePosition position )
{
	for ( Uint32 i = 0; i < m_runningThreads.Size(); ++i )
	{
		if ( m_runningThreads[ i ]->GetPlayState() == DIALOG_Playing )
		{
			m_currentlyTickingThread = i;
			m_runningThreads[ i ]->OnChoiceSelected( position );
		}
	}
	m_currentlyTickingThread = INVALID_THREAD;
}
/*
void CIDTopicInstance::SetCheckpoint( const CIDGraphBlock*	checkpoint )
{
	for ( Uint32 i = 0; i < m_runningThreads.Size(); ++i )
	{
		m_currentlyTickingThread = i;
		if( i == 0 )
		{
			m_runningThreads[ i ]->SetCheckpoint( checkpoint );
		}
		else
		{
			RED_LOG( Dialog, TXT(" Entering a checkpoint when multiple threads are active, terminating it") );
			m_runningThreads[ i ]->OnFinished();
		}
	}
	m_currentlyTickingThread = INVALID_THREAD;
}
*/

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CIDTopicInstance::Debug_GatherThreadInfos( TDynArray< Debug_SIDThreadInfo >& infos ) const
{
	if ( m_runningThreads.Empty() )
	{
		return;
	}

	Uint32 oldSize = Uint32( infos.Grow( m_runningThreads.Size() ) );
	for ( Uint32 i = 0; i < m_runningThreads.Size(); ++i )
	{
		infos[ oldSize + i ].m_topic = this;
		infos[ oldSize + i ].m_thread = m_runningThreads[ i ];
	}
}
