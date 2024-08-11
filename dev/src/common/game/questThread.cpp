#include "build.h"
#include "questThread.h"
#include "quest.h"
#include "questGraph.h"
#include "questGraphInstance.h"
#include "questStartBlock.h"
#include "questsSystem.h"
#include "questScopeBlock.h"
#include "questDeletionMarkerBlock.h"
#include "../core/memoryFileReader.h"
#include "../core/memoryFileWriter.h"
#include "../engine/gameSaveManager.h"
#include "questLayersHiderBlock.h"
#include "questPhaseBlock.h"
#include "questSceneBlock.h"

IMPLEMENT_ENGINE_CLASS( SQuestThreadSuspensionData )
IMPLEMENT_ENGINE_CLASS( CQuestThread )

CQuestThread::CQuestThread()
	: m_parentBlock( NULL )
	, m_graph( NULL )
	, m_canBlockSaves( true )
{
}

CQuestThread::~CQuestThread()
{
	ASSERT( m_activeBlocks.Empty() );
	ASSERT( m_blocksToActivate.Empty() );
	ASSERT( m_threads.Empty() );
}

void CQuestThread::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( m_graph )
	{
		file << m_graph;
	}

	if ( !file.IsGarbageCollector() )
	{
		ManageThreads();
	}
    else
    {
        Uint32 count = m_threadsToAdd.Size();
        for ( Uint32 i = 0; i < count; ++i )
        {
            file << m_threadsToAdd[ i ];
        }

        count = m_threadsToRemove.Size();
        for ( Uint32 i = 0; i < count; ++i )
        {
            file << m_threadsToRemove[ i ];
        }
    }

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		file << m_threads[ i ];
	}
}

void CQuestThread::Activate( const CQuestGraphBlock* parentBlock, const CQuestGraph& graph )
{
	Kill();
	ASSERT( m_graph == NULL );
	ASSERT( m_activeBlocks.Empty() );
	ASSERT( m_blocksToActivate.Empty() );
	ASSERT( m_threads.Empty() );

	m_parentBlock = parentBlock;
	m_graph = graph.CreateInstance( this );
	m_paused = false;
}

void CQuestThread::ActivateInput( const CName& inputName )
{
	ASSERT( m_graph );

	const CQuestGraphBlock* start = m_graph->GetStartBlock( inputName );
	if ( start )
	{
		ActivateBlock( *start, CName::NONE );
	}
}

void CQuestThread::Kill()
{
	if ( m_graph == NULL )
	{
		ASSERT( m_activeBlocks.Empty() );
		ASSERT( m_blocksToActivate.Empty() );
		ASSERT( m_threads.Empty() );
		ASSERT( m_threadsToAdd.Empty() );
		ASSERT( m_threadsToRemove.Empty() );
		return;
	}

	InstanceBuffer& data = m_graph->GetInstanceData();

	// deactivate all active blocks
	Uint32 count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		NotifyBlockRemoved( m_activeBlocks[ i ].block );
		m_activeBlocks[ i ].block->Deactivate( data );
	}
	m_activeBlocks.ClearFast();
	m_blocksToActivate.ClearFast();

	// dispose of the graph instance
	m_graph->Unbind();
	m_graph->Discard();
	m_graph = NULL;

	KillAllChildren();
	ManageThreads();
}

CQuestThread* CQuestThread::FindThread( const CQuestGraphBlock* parentBlock )
{
	Uint32 count = m_threadsToAdd.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_threadsToAdd[ i ]->GetParentBlock() == parentBlock )
		{
			return m_threadsToAdd[ i ];
		}
	}

	count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_threads[ i ]->GetParentBlock() == parentBlock )
		{
			return m_threads[ i ];
		}
	}

	return NULL;
}

CQuestThread* CQuestThread::AddChild( const CQuestGraphBlock* parentBlock, const CQuestGraph& questGraph, Bool canBlockSaves )
{
	CQuestThread* thread = CreateObject< CQuestThread >( this );

	thread->m_canBlockSaves = canBlockSaves;
	thread->Activate( parentBlock, questGraph );

	TDynArray< SQuestThreadSuspensionData >::iterator threadDataIter = Find( 
		m_suspendedScopesData.Begin(), m_suspendedScopesData.End(), SQuestThreadSuspensionData( parentBlock->GetGUID() ) ) ;

	// This will return sth only if the scope was suspended
	if ( threadDataIter != m_suspendedScopesData.End() )
	{
		CGameStorageLoader threadLoader( 
			new CGameStorageReader( 
			new CMemoryFileReader( threadDataIter->m_scopeData, 0 ), SGameSessionManager::GetInstance().GetCNamesRemapper() ), SAVE_VERSION, 0, VER_CURRENT, false);
		thread->LoadGame( &threadLoader );

		m_suspendedScopesData.Erase( threadDataIter );
	}

	m_threadsToAdd.PushBack( thread );

	return thread;
}

void CQuestThread::KillChild( CQuestThread* thread )
{
	if ( thread == NULL ) 
	{
		return;
	}

	thread->Kill();
	RemoveThread( thread );
}

void CQuestThread::KillAllChildren()
{
	ManageThreads();

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		KillChild( m_threads[ i ] );
	}
	
	ManageThreads();
}

namespace Helper
{
	struct SScopedStableUpdate : Red::System::NonCopyable
	{
		SStableTickData& m_tickData;

		SScopedStableUpdate( SStableTickData& tickData )
			: m_tickData( tickData )
		{
			m_tickData.m_latchBlockCountStable = true;
			m_tickData.m_latchChildThreadCountStable = true;
			m_tickData.m_latchChildThreadsStable = true;
			m_tickData.m_latchNoForceKeepLoadingScreen = true;
		}

		~SScopedStableUpdate()
		{
			m_tickData.m_stable = m_tickData.m_latchBlockCountStable &&
								  m_tickData.m_latchChildThreadCountStable && 
								  m_tickData.m_latchChildThreadsStable &&
								  m_tickData.m_latchNoForceKeepLoadingScreen;
		}
	};
}

void CQuestThread::ResetStabilizedThreads()
{
	m_threadsToStabilize = m_threads;
}

void CQuestThread::Tick()
{
	Helper::SScopedStableUpdate scopedStableUpdate( m_stableTickData );

	if ( GCommonGame->GetSystem< CQuestsSystem >()->IsPaused() || m_paused )
	{
		return;
	}
	TickBlocks();

	if ( GCommonGame->GetSystem< CQuestsSystem >()->IsPaused() )
	{
		return;
	}
	TickChildrenThreads();

	// Loop regardless of current m_stable in order to remove threads
	// that are stable now
	for ( Int32 i = m_threadsToStabilize.SizeInt()-1; i >= 0; --i )
	{
		const Bool threadStable = m_threadsToStabilize[ i ]->IsStable();
		m_stableTickData.m_latchChildThreadsStable &= threadStable;
		if ( threadStable )
		{
			m_threadsToStabilize.RemoveAtFast( i );
		}
	}
}

void CQuestThread::TickBlocks()
{
	if ( m_graph == NULL )
	{
		return;
	}

	InstanceBuffer& data = m_graph->GetInstanceData();

	// activate new blocks
	Uint32 count = m_blocksToActivate.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		ActivateBlock( *m_blocksToActivate[ i ].block, m_blocksToActivate[ i ].inputName );
	}
	m_blocksToActivate.ClearFast();

	count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		const CQuestGraphBlock* block = m_activeBlocks[ i ].block;
		if ( block->Execute( data, m_blocksToActivate ) )
		{
			// deactivate the block
			NotifyBlockRemoved( block );
			block->Deactivate( data );

			m_activeBlocks[ i ].block = NULL;
		}
		else if ( GGame->IsLoadingScreenShown() && (block->ForceKeepLoadingScreen() || KeepLoadingScreenForBlock( block, data )) )
		{
			m_stableTickData.m_latchNoForceKeepLoadingScreen = false;
		}
	}

	// remove all unused blocks
	SBlockData emptyBlock;
	for ( Int32 i = m_activeBlocks.SizeInt() - 1; i >= 0; --i )
	{
		if ( m_activeBlocks[ i ] == emptyBlock )
		{
			m_activeBlocks.RemoveAtFast( i );
			continue;
		}
	}
}

Bool CQuestThread::KeepLoadingScreenForBlock( const CQuestGraphBlock* block, const InstanceBuffer& data ) const
{
	if ( block->IsA< CQuestLayersHiderBlock >() )
	{
		const CQuestLayersHiderBlock* hiderBlock = static_cast< const CQuestLayersHiderBlock* >( block );
		const String& worldPath = hiderBlock->GetWorld();
		const TDynArray< String >& layersToShow = hiderBlock->GetLayersToShow();
		const TDynArray< String >& layersToHide = hiderBlock->GetLayersToHide();
		
		const Bool hasLayersToChange = !layersToShow.Empty() || !layersToHide.Empty();
		if ( ! hasLayersToChange )
		{
			return false;
		}

		// Layers are scheduled for the active world. If the layers don't exist, then this will need to be fixed
		// in the data
		if ( worldPath.Empty() )
		{
			return true;
		}

		if ( ! GGame->GetActiveWorld() )
		{
			return false;
		}

		const String activeWorldPath = GGame->GetActiveWorld()->GetDepotPath();
		if ( activeWorldPath.Empty() )
		{
			return false;
		}

		return worldPath.EqualsNC( activeWorldPath );
	}
	else if ( block->IsA< CQuestPhaseBlock >() )
	{
		const CQuestPhaseBlock* phaseBlock = static_cast< const CQuestPhaseBlock* >( block );
		const Bool isChangingLayers = phaseBlock->IsActivelyChangingLayers( data );
	}
	else if ( block->IsA< CQuestSceneBlock >() )
	{
		const CQuestSceneBlock* sceneBlock = static_cast< const CQuestSceneBlock* >( block );
		return !sceneBlock->HasLoadedSection( data );
	}

	return false;
}

void CQuestThread::TickChildrenThreads()
{
	ManageThreads();

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threads[ i ]->Tick();
	}
}

void CQuestThread::ActivateBlock( const CQuestGraphBlock& block, const CName& inputName ) 
{
	if ( m_graph == NULL )
	{
		return;
	}

	InstanceBuffer& data = m_graph->GetInstanceData();
	Bool activated = block.Activate( data, inputName, this );
	if ( activated == false )
	{
		return;
	}

	if ( Find( m_activeBlocks.Begin(), m_activeBlocks.End(), SBlockData( &block ) ) != m_activeBlocks.End() )
	{
		NotifyBlockInputActivated( &block );
	}
	else
	{
		m_activeBlocks.PushBack( SBlockData( &block, inputName ) );

		NotifyBlockAdded( &block );
	}
}

void CQuestThread::DeactivateBlock( const CQuestGraphBlock* block )
{
	if ( ( block == NULL ) || ( m_graph == NULL ) )
	{
		return;
	}

	// remove all used blocks
	if ( m_activeBlocks.Remove( block ) )
	{
		InstanceBuffer& data = m_graph->GetInstanceData();
		NotifyBlockRemoved( block );
		block->Deactivate( data );
	}
}

void CQuestThread::RemoveThread( CQuestThread* thread )
{
	if ( m_threadsToAdd.RemoveFast( thread ) == false )
	{
		m_threadsToRemove.PushBack( thread );
	}
}

void CQuestThread::ManageThreads()
{
	Uint32 count = m_threadsToAdd.Size();

	m_threadsToStabilize.Reserve( count );
	m_threads.Reserve( count );

	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threadsToStabilize.PushBack( m_threadsToAdd[ i ] );
		m_threads.PushBack( m_threadsToAdd[ i ] );
		NotifyThreadAdded( m_threadsToAdd[ i ] );

		// attach listeners to children
		for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
			it != m_listeners.End(); ++it )
		{
			m_threadsToAdd[ i ]->AttachListener( **it );
		}
	}
	m_threadsToAdd.ClearFast();

	count = m_threadsToRemove.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threadsToStabilize.RemoveFast( m_threadsToRemove[ i ] );
		m_threads.RemoveFast( m_threadsToRemove[ i ] );

		// detach listeners from children
		for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
			it != m_listeners.End(); ++it )
		{
			m_threadsToRemove[ i ]->DetachListener( **it );
		}

		NotifyThreadRemoved( m_threadsToRemove[ i ] );

		m_threadsToRemove[ i ]->Discard();
	}
	m_threadsToRemove.ClearFast();
}

InstanceBuffer& CQuestThread::GetInstanceData() 
{ 
	return m_graph->GetInstanceData(); 
}

void CQuestThread::AttachListener( IQuestSystemListener& listener )
{
	m_listeners.PushBackUnique( &listener );

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		listener.OnAddThread( this, m_threads[ i ] );

		// attach thread to children
		m_threads[ i ]->AttachListener( listener );
	}

	count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		listener.OnAddBlock( this, m_activeBlocks[ i ].block );
	}
}

void CQuestThread::DetachListener( IQuestSystemListener& listener )
{
	m_listeners.Remove( &listener );

	// detach listener from children
	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threads[ i ]->DetachListener( listener );
	}
}

void CQuestThread::NotifyThreadPaused( bool paused )
{
	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnThreadPaused( this, paused );
	}
}

void CQuestThread::NotifyThreadAdded( CQuestThread* thread )
{
	m_stableTickData.m_latchChildThreadCountStable = false;

	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnAddThread( this, thread );
	}
}

void CQuestThread::NotifyThreadRemoved( CQuestThread* thread )
{
	m_stableTickData.m_latchChildThreadCountStable = false;

	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnRemoveThread( this, thread );
	}
}

void CQuestThread::NotifyBlockAdded( const CQuestGraphBlock* block )
{
	m_stableTickData.m_latchBlockCountStable = false;

	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnAddBlock( this, block );
	}
}

void CQuestThread::NotifyBlockRemoved( const CQuestGraphBlock* block )
{
	m_stableTickData.m_latchBlockCountStable = false;

	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnRemoveBlock( this, block );
	}
}

void CQuestThread::NotifyBlockInputActivated( const CQuestGraphBlock* block )
{
	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnBlockInputActivated( this, block );
	}
}

void CQuestThread::GetActiveBlocks( TDynArray< const CQuestGraphBlock* >& blocks ) const
{
	Uint32 count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		blocks.PushBackUnique( m_activeBlocks[ i ].block );
	}
}

const TDynArray< CName >* CQuestThread::GetActivatedInputs( const CQuestGraphBlock* block ) const
{
	Uint32 count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_activeBlocks[ i ].block == block )
		{
			return &m_activeBlocks[ i ].activeInputs;
		}
	}

	return NULL;
}

String CQuestThread::GetName() const
{
//#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_parentBlock )
	{
		return m_parentBlock->GetCaption();
	}
	else
	{
		return TXT( "<<mainQuest>>" );
	}
//#else
//	return TXT("<<unknown>>");
//#endif
}

Bool CQuestThread::IsBlockActive( const CQuestGraphBlock* block ) const
{
	Uint32 count = m_activeBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_activeBlocks[ i ].block == block )
		{
			return true;
		}
	}

	count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_threads[ i ]->IsBlockActive( block ) )
		{
			return true;
		}
	}

	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CQuestThread::IsBlockVisited( const CQuestGraphBlock* block ) const
{
	if ( !m_graph )
	{
		return false;
	}

	CQuestGraph* graph = Cast< CQuestGraph >( block->GetParent() );
	if ( !graph )
	{
		return false;
	}
	if ( !DoesWorkOnGraph( *graph ) )
	{
		return false;
	}

	InstanceBuffer& data = m_graph->GetInstanceData();
	return block->IsBlockVisited( data );
}
#endif //NO_EDITOR_GRAPH_SUPPORT

Bool CQuestThread::DoesWorkOnGraph( const CQuestGraph& graph ) const
{
	if ( !m_graph || !m_graph->GetParentGraph() )
	{
		return false;
	}

	return m_graph->GetParentGraph() == &graph;
}

Bool CQuestThread::CanSaveToSaveGame() const
{
	// Ask parent block if we should save this quest thread
	if ( m_parentBlock ) 
	{
		return m_parentBlock->ShouldSaveThread();
	}

	// Fallback to defualt behavior
	return true;
}

void CQuestThread::SaveGame( IGameSaver* saver )
{
	InstanceBuffer& data = m_graph->GetInstanceData();

	{
		CGameSaverBlock block0( saver, CNAME(questThread) );

		// Save blocks to activate
		{
			saver->WriteValue( CNAME(numBlocksToActivate), m_blocksToActivate.Size() );

			for ( auto it = m_blocksToActivate.Begin(); it != m_blocksToActivate.End(); ++it )
			{
				const SBlockDesc& blockDesc = *it;

				CGameSaverBlock block( saver, CNAME(questBlockToActivate) );

				saver->WriteValue( CNAME(GUID), blockDesc.block->GetGUID() );
				saver->WriteValue< CName >( CNAME( inputName ), blockDesc.inputName );
			}
		}

		// Save active blocks
		Uint32 count = 0;
		{
			// Count active blocks
			for ( Uint32 i=0; i<m_activeBlocks.Size(); ++i )
			{
				if ( m_activeBlocks[ i ].block )
				{
					++count;
				}
			}

			// Save number of active blocks
			saver->WriteValue( CNAME(numBlocks), count );

			// Save blocks
			for ( Uint32 i=0; i<m_activeBlocks.Size(); ++i )
			{
				if ( m_activeBlocks[ i ].block )
				{
					CGameSaverBlock block1( saver, CNAME(questBlock) );

					// Save GUID of the block
					const CGUID& blockGUID = m_activeBlocks[ i ].block->GetGUID();
					saver->WriteValue( CNAME( GUID ), blockGUID );

					// Save the name of the input this block was activated through
					const TDynArray< CName >& inputNames = m_activeBlocks[ i ].block->GetActiveInputs( m_graph->GetInstanceData() );

					saver->WriteValue( CNAME( inputNamesCount ), inputNames.Size() );
					for ( CName inputName : inputNames )
					{
						saver->WriteValue( CNAME( inputName ), inputName );
					}

					// block's activation state
					saver->WriteValue( CNAME( activationState ), (Int32)m_activeBlocks[ i ].block->GetActivationState( m_graph->GetInstanceData() ) );

					// Save block
					m_activeBlocks[ i ].block->SaveGame( data, saver );
				}
			}
		}

		// Update the state of the threads scheduled for removal and 
		// addition so that we can save only the active ones
		ManageThreads();

		// Count number of active threads
		count = 0;
		for ( Uint32 i=0; i<m_threads.Size(); ++i )
		{
			CQuestThread* questThread = m_threads[ i ];
			if ( questThread && questThread->CanSaveToSaveGame() )
			{
				++count;
			}
		}

		// Save number of active blocks
		saver->WriteValue( CNAME(numThreads), count );

		// Save the active threads
		for ( Uint32 i=0; i<m_threads.Size(); ++i )
		{
			CQuestThread* questThread = m_threads[ i ];
			if ( questThread && questThread->CanSaveToSaveGame() )
			{
				CGameSaverBlock block1( saver, CNAME(questThread) );
				saver->WriteValue< CGUID >( CNAME( GUID ), questThread->GetParentBlock()->GetGUID() );
				questThread->SaveGame( saver );
			}
		}

#		ifdef RED_LOGGING_ENABLED
			for ( const auto& data : m_suspendedScopesData )
			{
				Char str[ RED_GUID_STRING_BUFFER_SIZE ];
				data.m_scopeBlockGUID.ToString( str, RED_GUID_STRING_BUFFER_SIZE );
				RED_LOG( Save, TXT("Suspended scope: %ls saving %ld bytes of data."), str, data.m_scopeData.Size() );
			}
#		endif

		// Save suspended threads
		saver->WriteValue( CNAME( suspendedScopesData ), m_suspendedScopesData );
		saver->WriteValue( CNAME( pausedThread ), m_paused );
	}
}

void CQuestThread::LoadGame( IGameLoader* loader )
{
	InstanceBuffer& data = m_graph->GetInstanceData();

	{
		CGameSaverBlock block0( loader, CNAME(questThread) );

		// There should be no running blocks
		const Uint32 count = m_activeBlocks.Size();
		for ( Uint32 i=0; i<count; ++i )
		{
			ASSERT( m_activeBlocks[ i ].block->GetActivationState( data ) != CQuestGraphBlock::ST_INACTIVE && "Quest block should not be active during game load" );
			m_activeBlocks[ i ].block->Deactivate( data );
		}
		
		// Cleanup
		m_activeBlocks.Clear();
		m_blocksToActivate.Clear();
		ManageThreads();

		// All should be empty here
		ASSERT( m_threads.Empty() );
		ASSERT( m_activeBlocks.Empty() );

		// Load blocks to activate
		{
			Uint32 numBlocksToActivate = 0;
			loader->ReadValue( CNAME(numBlocksToActivate), numBlocksToActivate );

			for ( Uint32 i = 0; i < numBlocksToActivate; i++ )
			{
				SBlockDesc blockDesc;

				CGameSaverBlock block( loader, CNAME(questBlockToActivate) );

				CGUID blockGUID = CGUID::ZERO;
				loader->ReadValue( CNAME(GUID), blockGUID );
				blockDesc.block = const_cast< CQuestGraphBlock* >( m_graph->FindBlockByGUID( blockGUID ) );
				if ( !blockDesc.block )
				{
					continue;
				}

				loader->ReadValue< CName >( CNAME( inputName ), blockDesc.inputName );

				m_blocksToActivate.PushBack( blockDesc );
			}
		}

		// Load active blocks
		{
			Uint32 numActiveBlocks = 0;
			loader->ReadValue( CNAME(numBlocks), numActiveBlocks );

			// Load blocks
			for ( Uint32 i=0; i<numActiveBlocks; i++ )
			{
				CGameSaverBlock block1( loader, CNAME(questBlock) );

				// Load GUID
				CGUID blockGUID = CGUID::ZERO;
				loader->ReadValue( CNAME(GUID), blockGUID );

				// Load input name
				Uint32 inputNamesCount = loader->ReadValue( CNAME( inputNamesCount ), (Uint32)0 );
				TDynArray< CName > inputNames;
				inputNames.Reserve( inputNamesCount );

				Bool warned = false;
				for ( Uint32 i = 0; i < inputNamesCount; ++i )
				{
					if ( loader->GetSaveVersion() < SAVE_VERSION_STORE_INPUT_NAMES_DIRECTLY )
					{
						String inputName = loader->ReadValue< String >( CNAME( inputName ) );

						// skip duplicated elements to fix the outcome of bug #119512
						if ( false == inputNames.PushBackUnique( CName( inputName ) ) )
						{
							if ( !warned )
							{
								warned = true;
								RED_LOG( Save, TXT("Fixing save data damaged due to bug #119512") );
							}
						}
					}
					else
					{
						CName inputName = loader->ReadValue< CName > ( CNAME( inputName ), CName::NONE );

						// skip duplicated elements to fix the outcome of bug #119512
						if ( false == inputNames.PushBackUnique( inputName ) )
						{
							if ( !warned )
							{
								warned = true;
								RED_LOG( Save, TXT("Fixing save data damaged due to bug #119512") );
							}
						}
					}
				}

				// might have changed due to PushBackUnique() above
				inputNamesCount = inputNames.Size();

				// Load block's activation state
				CQuestGraphBlock::EState activationState = static_cast< CQuestGraphBlock::EState >( loader->ReadValue( CNAME( activationState ), 0 ) );

				// Find block
				const CQuestGraphBlock* block = m_graph->FindBlockByGUID( blockGUID );
				if ( block )
				{
					if ( block->CanActivateInputsOnLoad( activationState ) )
					{
						// Reactivate inputs
						for ( Uint32 i = 0; i < inputNamesCount; ++i )
						{
							ActivateBlock( *block, inputNames[i] );
						}
					}
					else
					{
						// activate a dummy input
						ActivateBlock( *block, CName::NONE );
					}
					block->LoadGame( data, loader );

					if ( activationState != CQuestGraphBlock::ST_INACTIVE )
					{
						block->OnProcessActivation( data );
					}
				}
				else
				{
					ERR_GAME( TXT( "Quest block '%ls' not found during game loading" ), ToString( blockGUID ).AsChar() );
				}
			}
		}	

		// process threads
		ManageThreads();

		// Load active threads
		{
			Uint32 numActiveThreads = 0;
			loader->ReadValue( CNAME(numThreads), numActiveThreads );

			// Load the active threads
			for ( Uint32 i=0; i<numActiveThreads; i++ )
			{
				CGameSaverBlock block1( loader, CNAME(questThread) );

				// Load GUID
				CGUID threadGUID = CGUID::ZERO;
				loader->ReadValue( CNAME(GUID), threadGUID );

				// Find thread owner block 
				const CQuestGraphBlock* block = m_graph->FindBlockByGUID( threadGUID );
				if ( block )
				{
					CQuestThread* blockThread = FindThread( block );
					if ( blockThread )
					{
						blockThread->LoadGame( loader );
					}
					else
					{
						ERR_GAME( TXT( "Embedded thread can't be restored - layers it operates on are not yet loaded" ) );
					}
				}
			}
		}

		// Load suspended threads
		loader->ReadValue( CNAME( suspendedScopesData ), m_suspendedScopesData );
		for ( Int32 i = m_suspendedScopesData.SizeInt() - 1; i >= 0; --i )
		{
			const auto& data = m_suspendedScopesData[ i ];

#			ifdef RED_LOGGING_ENABLED
				Char str[ RED_GUID_STRING_BUFFER_SIZE ];
				data.m_scopeBlockGUID.ToString( str, RED_GUID_STRING_BUFFER_SIZE );
				RED_LOG( Save, TXT("Suspended scope: %ls loaded %ld bytes of data."), str, data.m_scopeData.Size() );
#			endif

			// sanity check: if suspended data size is ridiculously big, it's for sure an error - no way of loading it without risking the OOM
			// it can heppen due to a bug #119512
			// reasons are fixed, but we need to handle the broken data somehow 
			if ( data.m_scopeData.Size() > 100 * 1024 )
			{
				RED_LOG( Save, TXT("Suspended scope: BROKEN (too big) data! REMOVIG!"), str, data.m_scopeData.Size() );
				m_suspendedScopesData.RemoveAt( i );
			}
		}
		loader->ReadValue( CNAME( pausedThread ), m_paused );

		if ( loader->GetSaveVersion() < SAVE_VERSION_DEAD_PHASE_HACKFIX && loader->GetGameVersion() >= GAME_VERSION_WITCHER_3_PATCH_1_20 )
			ReactivateDeadPhasesHack( GGame->GetActiveWorld()->GetDepotPath() );
	}
}

void CQuestThread::ForceBlockExit( const CQuestGraphBlock* block, const CName& output )
{
	if ( ( block == NULL ) || ( m_graph == NULL ) || m_activeBlocks.Exist( block ) == false )
	{
		return;
	}

	// remove all used blocks
	InstanceBuffer& data = GetInstanceData();
	block->ThrowErrorNonBlocking( data, output, TXT( "Debug ForceBlockExit" ) );
}

void CQuestThread::OnNewWorldLoading( const String& worldPath )
{
	ReactivateDeadPhasesHack( worldPath );

	for ( TDynArray< SQuestThreadSuspensionData >::iterator suspendedIter = m_suspendedScopesData.Begin();
		suspendedIter != m_suspendedScopesData.End(); ++suspendedIter )
	{
		const CQuestGraphBlock* questBlock = m_graph->FindBlockByGUID( suspendedIter->m_scopeBlockGUID );
		if ( questBlock )
		{
			const CQuestScopeBlock* threadScopeBlock = Cast< CQuestScopeBlock >( questBlock );
			if ( threadScopeBlock != NULL && threadScopeBlock->GetRequiredWorld() == worldPath )
			{
				ActivateBlock( *threadScopeBlock, CName::NONE );
			}
			else if ( questBlock->IsA< CQuestDeletionMarkerBlock >() )
			{
				ActivateBlock( *questBlock, CName::NONE );
			}
		}
	}
	
	for ( TDynArray< CQuestThread* >::iterator threadIter = m_threads.Begin();
		threadIter != m_threads.End(); ++threadIter )
	{
		CQuestThread* thread = *threadIter;

		const CQuestScopeBlock* threadScopeBlock = Cast< CQuestScopeBlock >( thread->GetParentBlock() );

		if ( threadScopeBlock == NULL )
		{
			continue;
		}

		const String& threadRequiredWorld = threadScopeBlock->GetRequiredWorld();

		if ( threadRequiredWorld.Empty() == false && threadRequiredWorld != worldPath )
		{
			CGUID threadGUID = threadScopeBlock->GetGUID();
			m_suspendedScopesData.PushBack( SQuestThreadSuspensionData( threadGUID ) );

			CGameStorageSaver threadSaver( new CGameStorageWriter( new CMemoryFileWriter ( m_suspendedScopesData.Back().m_scopeData ), SGameSessionManager::GetInstance().GetCNamesRemapper() ) );
			thread->SaveGame( &threadSaver );

			DeactivateBlock( threadScopeBlock );
			
			continue;
		}
																									   
		// Pass to child threads
		(*threadIter)->OnNewWorldLoading( worldPath );
	}
}

void CQuestThread::ReactivateDeadPhasesHack( const String& worldPath )
{
#ifdef RED_PLATFORM_ORBIS
	// glory, glory, hallelujah...
	const Uint32 NUM_GUIDS = 3;

	if ( GCommonGame->GetSystem< CQuestsSystem > ()->GetDeadPhaseHackfixCounter() == NUM_GUIDS )
		return;

	if ( false == worldPath.EndsWith( TXT("bob.w2w" ) ) )
		return;

	const CGUID guidsToRiseFromDead[] =
	{
		CGUID( TXT("CFF1BE08-4666D676-63CFC797-8FC8D0A7" ) ), // (bob_main_quests)
		CGUID( TXT("1E7F1040-49C690B0-DA39FDAE-4C1E70DD" ) ), // (bob_minor_quests)
		CGUID( TXT("C354EE0F-4548EA55-5EFE4E89-98658FA3" ) ), // (bob_living_world)
	};

	InstanceBuffer& data = m_graph->GetInstanceData();

loop:
	if ( m_suspendedScopesData.Empty() )
		return;

	for ( TDynArray< SQuestThreadSuspensionData >::iterator suspended = m_suspendedScopesData.Begin(); suspended != m_suspendedScopesData.End(); ++suspended )
	{
		for ( Uint32 i = 0; i < NUM_GUIDS; ++i )
		{
			if ( guidsToRiseFromDead[ i ] == suspended->m_scopeBlockGUID )
			{
				if ( const auto questBlock = m_graph->FindBlockByGUID( suspended->m_scopeBlockGUID ) )
				{
					if ( const auto threadScopeBlock = Cast< CQuestScopeBlock >( questBlock ) )
					{
						GCommonGame->GetSystem< CQuestsSystem > ()->BumpDeadPhaseHackfixCounter();
						ActivateBlock( *threadScopeBlock, CName::NONE );
						threadScopeBlock->OnProcessActivation( data );
						ManageThreads();
						goto loop;
					}
				}
			}
		}
	}
#endif // RED_PLATFORM_ORBIS
}

void CQuestThread::SetPaused( Bool paused )
{
	m_paused = paused;

	NotifyThreadPaused( paused );
	
	for ( auto it = m_threads.Begin(); it != m_threads.End(); ++it )
	{
		(*it)->SetPaused(paused);
	}
	for ( auto it = m_threadsToAdd.Begin(); it != m_threadsToAdd.End(); ++it )
	{
		(*it)->SetPaused(paused);
	}
}

