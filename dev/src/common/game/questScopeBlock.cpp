#include "build.h"
#include "questGraphSocket.h"
#include "questScopeBlock.h"
#include "questPhaseInputBlock.h"
#include "questPhaseOutputBlock.h"
#include "questsSystem.h"
#include "questThread.h"
#include "questGraph.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestScopeBlock )

CQuestScopeBlock::CQuestScopeBlock()
: m_phase( NULL )
, m_embeddedGraph( NULL )
#ifdef LATENT_QUEST_SCOPE_ENABLED
, m_phaseRefs( 0 )
#endif
{
#ifndef NO_EDITOR
	m_embeddedGraph = CreateObject< CQuestGraph >( this );
#endif
}

void CQuestScopeBlock::UpdateGraph() const
{

}



void CQuestScopeBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();
#if ( !defined( NO_EDITOR ) || !defined( LATENT_QUEST_SCOPE_ENABLED ) ) && !defined( DEBUGGING_LATENT_QUEST_SCOPE )
	UpdateGraph();
#endif

#ifdef DEBUGGING_LATENT_QUEST_SCOPE
	if ( m_phase )
	{
		m_phaseHandle = m_phase;
		m_phaseHandle.Release();
		m_phase->SetParent( NULL );
		//m_phase->Discard();
		m_phase = NULL;
	}
#endif
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestScopeBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("phase") )
	{
		if ( CanConvertToResource() )
		{
			// Make sure the user doesn't link a quest to another quest phase.
			// Quests should be the topmost entities there are.
			if ( m_phase )
			{
				if ( !m_phase->IsExactlyA<CQuestPhase>() )
				{
					if ( !m_phase->IsInRootSet() )
					{
						m_phase->Discard();
					}
					m_phase = NULL;
				}
			}

			UpdateGraph();
			OnRebuildSockets();
			InvalidateLayout();
		}
		else
		{
			m_phase = NULL;
		}
	}
}

void CQuestScopeBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CQuestGraph *activeGraph = GetGraph();
	ASSERT( activeGraph );
	if ( activeGraph )
	{
		TDynArray<CGraphBlock *> &blocks = activeGraph->GraphGetBlocks();
		Uint32 count = blocks.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( blocks[i]->IsExactlyA<CQuestPhaseInputBlock>() )
			{
				CQuestPhaseInputBlock* inputBlock = SafeCast< CQuestPhaseInputBlock >( blocks[i] );
				CreateSocket( CQuestGraphSocketSpawnInfo( inputBlock->GetSocketID(), LSD_Input, LSP_Left ) );
			}
			else if ( blocks[i]->IsExactlyA<CQuestPhaseOutputBlock>() )
			{
				CQuestPhaseOutputBlock* outputBlock = SafeCast< CQuestPhaseOutputBlock >( blocks[i] );
				CreateSocket( CQuestGraphSocketSpawnInfo( outputBlock->GetSocketID(), LSD_Output, LSP_Right ) );
			}
		}

		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
	}
}

class CQuestScopeBlockPredicate
{
public:
	Bool operator()( const CGraphBlock* block1, const CGraphBlock* block2 ) const
	{
		if ( block1 == block2 )
		{
			return false;
		}

		const CQuestPhaseOutputBlock* outputBlock1	= Cast< CQuestPhaseOutputBlock >( block1 );
		const CQuestPhaseOutputBlock* outputBlock2	= Cast< CQuestPhaseOutputBlock >( block2 );
		if ( !outputBlock1 && !outputBlock2 )
		{
			return block1 < block2;
		}
		if ( !outputBlock1 && outputBlock2 )
		{
			return true;
		}
		if ( outputBlock1 && !outputBlock2 )
		{
			return false;
		}

		const String& socketName1	= outputBlock1->GetSocketID().AsString();
		const String& socketName2	= outputBlock2->GetSocketID().AsString();
		if ( socketName1 == socketName2 )
		{
			return outputBlock1 < outputBlock2;
		}
		return socketName1 < socketName2;
	}
};

void CQuestScopeBlock::SortOutputBlocks()
{
	CQuestGraph *activeGraph = GetGraph();
	ASSERT( activeGraph );
	if ( activeGraph )
	{
		TDynArray< CGraphBlock* >& blocks = activeGraph->GraphGetBlocks();
		if ( !blocks.Empty() )
		{
			if ( MarkModified() && activeGraph->ModifyGraphStructure() )
			{
				Sort( blocks.Begin(), blocks.End(), CQuestScopeBlockPredicate() );
				activeGraph->GraphStructureModified();
			}
		}
	}
}

void CQuestScopeBlock::OnPasted( Bool wasCopied )
{
	TBaseClass::OnPasted( wasCopied );

	if ( GetPhase() != NULL )
	{
		// If this block contains a resource - don't propagate the event.
		return;
	}

	// go through all the blocks and pass the event to them
	CQuestGraph *activeGraph = GetGraph();
	ASSERT( activeGraph );
	if ( activeGraph )
	{
		TDynArray<CGraphBlock *> &blocks = activeGraph->GraphGetBlocks();
		Uint32 count = blocks.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			blocks[i]->OnPasted( wasCopied );
		}
	}
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestScopeBlock::NeedsUpdate() const
{
	const CQuestGraph* graph = GetGraph();
	if ( !graph )
	{
		return false;
	}

	// gather names of inputs and outputs of the underlying phase graph
	TDynArray< CName > inputNames;
	TDynArray< CName > outputNames;
	const TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
	Uint32 count = blocks.Size();
	for ( Uint32 i = 0; i < blocks.Size(); ++i )
	{
		if ( blocks[i]->IsExactlyA<CQuestPhaseInputBlock>() )
		{
			CQuestPhaseInputBlock* inputBlock = SafeCast< CQuestPhaseInputBlock >( blocks[i] );
			inputNames.PushBack( inputBlock->GetSocketID() );
		}
		else if ( blocks[i]->IsExactlyA<CQuestPhaseOutputBlock>() )
		{
			CQuestPhaseOutputBlock* outputBlock = SafeCast< CQuestPhaseOutputBlock >( blocks[i] );
			outputNames.PushBack( outputBlock->GetSocketID() );
		}
	}

	// analyze this block's sockets in search for the matching input/output sockets
	const TDynArray< CGraphSocket* >& sockets = GetSockets();
	Uint32 inputsCount = 0;
	Uint32 outputsCount = 0;
	count = sockets.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( sockets[ i ]->GetDirection() == LSD_Input )
		{
			if ( sockets[ i ]->GetName() == CNAME( Cut ) )
			{
				continue;
			}

			++inputsCount;
			if ( Find( inputNames.Begin(), inputNames.End(), sockets[ i ]->GetName() ) == inputNames.End() )
			{
				return true;
			}

		}
		else if ( sockets[ i ]->GetDirection() == LSD_Output )
		{
			++outputsCount;
			if ( Find( outputNames.Begin(), outputNames.End(), sockets[ i ]->GetName() ) == outputNames.End() )
			{
				return true;
			}
		}
	}

	if ( inputNames.Size() != inputsCount )
	{
		return true;
	}

	if ( outputNames.Size() != outputsCount )
	{
		return true;
	}

	return false;
}

void CQuestScopeBlock::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	CQuestGraph *graph = GetGraph();
	ASSERT( graph );
	const Int32 defaultBlockSize = 50;

	GraphBlockSpawnInfo inputInfo( CQuestPhaseInputBlock::GetStaticClass() );
	inputInfo.m_position = Vector( 10, 400, 0, 1 );
	graph->GraphCreateBlock( inputInfo );

	GraphBlockSpawnInfo outputInfo( CQuestPhaseOutputBlock::GetStaticClass() );
	outputInfo.m_position = Vector( 600, 400, 0, 1 );
	graph->GraphCreateBlock( outputInfo );
}

#endif

CQuestGraph *CQuestScopeBlock::GetGraph()
{
	if( m_phase.IsValid() )
	{
		return m_phase->GetGraph();
	}
	else if( m_embeddedGraph != nullptr )
	{
		return m_embeddedGraph;
	}

	return nullptr;
}

const CQuestGraph *CQuestScopeBlock::GetGraph() const
{
	if( m_phase.IsValid() )
	{
		return m_phase->GetGraph();
	}
	else if( m_embeddedGraph != nullptr )
	{
		return m_embeddedGraph;
	}

	return nullptr;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestScopeBlock::ConvertToResource()
{	
	if ( !m_phase.IsValid() )
	{
		m_phase = CreateObject<CQuestPhase>();
		m_phase->SetGraph( m_embeddedGraph );

		m_embeddedGraph->Discard();
		m_embeddedGraph = CreateObject< CQuestGraph >( this );

		UpdateGraph();
		OnRebuildSockets();
		InvalidateLayout();
	}
	else
	{
		WARN_GAME( TXT("The resource '%ls' already exists."), m_phase->GetFriendlyName().AsChar() );
	}
}

void CQuestScopeBlock::EmbedGraphFromResource()
{
	if ( m_phase.IsValid() )
	{
		m_embeddedGraph->Discard();

		CQuestGraph* phaseGraph = m_phase->GetGraph();
		#if defined(W2_PLATFORM_WIN32)
		m_embeddedGraph = Cast< CQuestGraph >( phaseGraph->Clone( this ) );
		#else
		m_embeddedGraph = (CQuestGraph*)( phaseGraph->Clone( this ) );
		#endif
		m_phase->Discard();
		m_phase = NULL;

		UpdateGraph();
		MarkModified();
		OnRebuildSockets();
		InvalidateLayout();
	}
	else
	{
		WARN_GAME( TXT("The embedded graph '%ls' already exists."), m_embeddedGraph->GetFriendlyName().AsChar() );
	}
}

#endif

CQuestScopeBlock * CQuestScopeBlock::GetParentScopeBlock( const CQuestGraphBlock *block )
{
	if ( block )
	{
		#if defined( W2_PLATFORM_WIN32 )
		if ( CQuestGraph *parentGraph = Cast< CQuestGraph >( block->GetParent() ) )
		{
			if ( CQuestScopeBlock *phaseBlock = Cast< CQuestScopeBlock >( parentGraph->GetParent() ) )
			{
				return phaseBlock;
			}
		}
		#else
		CQuestGraph *parentGraph = (CQuestGraph *) block->GetParent();
		CQuestScopeBlock *phaseBlock = (CQuestScopeBlock *)parentGraph->GetParent();
		return phaseBlock;
		#endif
	}

	return NULL;
}


void CQuestScopeBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_thread;
	compiler << i_parentThread;
	compiler << i_inputNames;
	compiler << i_activated;
	compiler << i_phaseRefAdded;
}

void CQuestScopeBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_thread ] = NULL;
	instanceData[ i_parentThread ] = NULL;
	instanceData[ i_activated ] = false;
	instanceData[ i_phaseRefAdded] = false;
}

void CQuestScopeBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	data[ i_parentThread ] = parentThread;
	data[ i_inputNames ].PushBack( inputName );
}

void CQuestScopeBlock::OnExecute( InstanceBuffer& data ) const
{
	if ( !data[ i_inputNames ].Empty() )
	{
		TDynArray< CName >& inputs = data[ i_inputNames ];
		Uint32 count = inputs.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			data[ i_thread ]->ActivateInput( inputs[ i ] );
		}
		inputs.Clear();
	}
}

void CQuestScopeBlock::OnDeactivate( InstanceBuffer& data ) const
{
	// deactivate the thread
	if ( data[ i_parentThread ] && data[ i_thread ] )
	{
		data[ i_parentThread ]->KillChild( data[ i_thread ] );

		data[ i_thread ] = NULL;
		data[ i_parentThread ] = NULL;
		data[ i_activated ] = false;
	}

	TBaseClass::OnDeactivate( data );
}

void CQuestScopeBlock::OnScopeEndReached( InstanceBuffer& data, const CName& outSocket ) const
{
	ActivateOutput( data, outSocket );
}

void CQuestScopeBlock::ActivateSubgraph( InstanceBuffer& data ) const
{
	// activate the subgraph
	if ( data[ i_activated ] == false )
	{
		data[ i_activated ] = true;

		CQuestThread* parentThread = data[ i_parentThread ];
		const Bool canBlockSaves = CanSpawnedThreadBlockSaves() && parentThread->CanBlockSaves();
		CQuestThread* thread = parentThread->AddChild( this, *GetGraph(), canBlockSaves );
		ASSERT( thread );
		data[ i_thread ] = thread;

		// Find all graph outputs and register self as their listener.
		// Once the control reaches them, they will send a notification
		// and we'll be able to push the control forward
		CQuestGraph* graph = const_cast< CQuestGraph* >( GetGraph() );
		if ( !graph )
		{
			return;
		}
		TDynArray< CGraphBlock* >& graphBlocks = graph->GraphGetBlocks();
		CQuestPhaseOutputBlock* outputBlock;
		Uint32 count = graphBlocks.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if( graphBlocks[ i ] == NULL )
				continue;
			if ( graphBlocks[ i ]->IsExactlyA< CQuestPhaseOutputBlock >() )
			{
				outputBlock = SafeCast< CQuestPhaseOutputBlock >( graphBlocks[ i ] );
				outputBlock->AttachListener( thread->GetInstanceData(), *this, data );
			}
		}
	}
}

Bool CQuestScopeBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( m_requiredWorld.Empty() == false && GGame->GetActiveWorld()->DepotPath() != m_requiredWorld )
	{
		return false;
	}

#if ( defined( NO_EDITOR ) && defined( LATENT_QUEST_SCOPE_ENABLED ) ) || defined( DEBUGGING_LATENT_QUEST_SCOPE )
	if ( m_embeddedGraph )
	{
		UpdateGraph();
	}
	else
	{
		// Start loading the phase resource
		LoadAsync();
		{
			++m_phaseRefs;
			data[ i_phaseRefAdded ] = true;
		}
	}

	if ( !m_embeddedGraph && !LoadAsync() )
	{
		// Keep waiting
		return false;
	}
#endif
	ActivateSubgraph( data );

	return true;
}

CQuestThread* CQuestScopeBlock::GetThread( InstanceBuffer& data ) const
{
	return data[ i_thread ];
}

#ifdef LATENT_QUEST_SCOPE_ENABLED

Bool CQuestScopeBlock::LoadAsync() const
{
	BaseSoftHandle::EAsyncLoadingResult result = m_phaseHandle.GetAsync();
	if ( result == BaseSoftHandle::ALR_Loaded )
	{
		UpdateGraph();
		return true;
	}
	return false;
}

#ifndef NO_RESOURCE_COOKING
void CQuestScopeBlock::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	if ( m_embeddedGraph && m_phase )
	{
		// No dangling graphs in cook
		m_embeddedGraph->Discard();
		m_embeddedGraph = NULL;
	}
}
#endif // NO_RESOURCE_COOKING

#endif // LATENT_QUEST_SCOPE_ENABLED

void CQuestScopeBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	// Pass to base class
	TBaseClass::LoadGame( data, loader );
}

