#include "build.h"
#include "questGraph.h"
#include "questGraphInstance.h"
#include "quest.h"
#include "questScopeBlock.h"
#include "questRewardBlock.h"
#include "questPhaseBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "questPhaseInputBlock.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraph );

CQuestGraph::CQuestGraph()
	: m_sourceDataRemoved( false )
	, m_isTest( false )
{
}

CQuestGraph::~CQuestGraph()
{
//	ASSERT( m_dataLayout.GetNumBuffers() == 0 );
}

CQuestGraphInstance* CQuestGraph::CreateInstance( CObject* parent ) const
{
	CQuestGraphInstance* instance = CreateObject< CQuestGraphInstance > ( parent );
	instance->Bind( *this, m_graphBlocks, m_dataLayout );
	return instance;
}

#ifndef NO_EDITOR
void CQuestGraph::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	
	for( Int32 i = m_graphBlocks.SizeInt() - 1; i >= 0; --i )
	{
		if( m_graphBlocks[ i ] == NULL )
		{
			// TODO: Replace NULL pointer with deletion marker, but for now, remove the block entirely
			m_graphBlocks.RemoveAt( i );
		}
	}
}
#endif

void CQuestGraph::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	CompileDataLayout();
}

Vector CQuestGraph::GraphGetBackgroundOffset() const
{
	return m_backgroundOffset;
}

void CQuestGraph::GraphSetBackgroundOffset( const Vector& offset )
{
	m_backgroundOffset = offset;
}

CObject *CQuestGraph::GraphGetOwner()
{
	return this;
}

Bool CQuestGraph::ModifyGraphStructure()
{
	return MarkModified();
}

void CQuestGraph::GraphStructureModified()
{
	#if defined( W2_PLATFORM_WIN32 )
	if ( CQuest *quest = Cast< CQuest >( GetParent() ) )
	{
		quest->MarkModified();
	}
	#else
	CQuest *quest = (CQuest *)GetParent();
	quest->MarkModified();
	#endif
	CompileDataLayout();
}

void CQuestGraph::CompileDataLayout()
{
	InstanceDataLayoutCompiler compiler( m_dataLayout );

	Uint32 count = m_graphBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		ASSERT( m_graphBlocks[ i ], TXT("There is a NULL in the graph block array") );
		CQuestGraphBlock* block = Cast< CQuestGraphBlock >( m_graphBlocks[ i ] );
		if ( block )
		{
			block->OnBuildDataLayout( compiler );
		}
	}

	// Create layout
	m_dataLayout.ChangeLayout( compiler );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestGraph::CleanupSourceData()
{
	// Do not remove data twice
	if ( m_sourceDataRemoved )
	{
		return;
	}

	Uint32 count = m_graphBlocks.Size();

	// cache connections on the graph
	for ( Uint32 i = 0; i < count; ++i )
	{
		ASSERT( m_graphBlocks[ i ], TXT("There is a NULL in the graph block array") );
		CQuestGraphBlock* block = Cast< CQuestGraphBlock >( m_graphBlocks[ i ] );
		if ( !block )
		{
			continue;
		}

		block->CacheConnections();
		CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( block );
		if ( scopeBlock && scopeBlock->GetGraph() )
		{
			scopeBlock->GetGraph()->CleanupSourceData();
		}
	}

	// cleanup the graph
	for ( Uint32 i = 0; i < count; ++i )
	{
		ASSERT( m_graphBlocks[ i ], TXT("There is a NULL in the graph block array") );
		CQuestGraphBlock* block = Cast< CQuestGraphBlock >( m_graphBlocks[ i ] );
		if ( block )
		{
			block->RemoveAllSockets();
			block->BreakAllLinks();
			block->CleanupSourceData();
		}
	}
	m_sourceDataRemoved = true;
}

void CQuestGraph::CheckGUIDs( DuplicateMap& duplicatedGUIDs, const ScopeStack& scope ) const
{
	for ( CGraphBlock* block : GraphGetBlocks() )
	{
		if ( CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block ) )
		{
			// add block to map for given guid
			const CGUID& guid = questBlock->GetGUID();
			if ( TDynArray< GUIDCheckEntry >* foundEntry = duplicatedGUIDs.FindPtr( guid ) )
			{
				// if it's a the same _exact_ block, it means that this is propbably an included graph entry 
				// - do not treat this as a GUID duplicate
				auto it = FindIf( foundEntry->Begin(), foundEntry->End(), 
					[questBlock]( const GUIDCheckEntry& e ){ return e.m_block == questBlock; } 
					);

				if ( it == foundEntry->End() ) 
				{	
					foundEntry->PushBack( GUIDCheckEntry( questBlock, scope ) );
				}
			}
			else
			{
				TDynArray< GUIDCheckEntry > sameGuidBlocks;
				sameGuidBlocks.PushBack( GUIDCheckEntry( questBlock, scope ) );
				duplicatedGUIDs.Insert( guid, sameGuidBlocks );
			}

			if ( CQuestScopeBlock* questScopeBlock = Cast< CQuestScopeBlock >( questBlock ) )
			{
				// recursively search the graph
				CQuestGraph* graph = questScopeBlock->GetGraph();
				if ( graph )
				{
					ScopeStack newScope = scope;
					newScope.PushBack( questScopeBlock );
					graph->CheckGUIDs( duplicatedGUIDs, newScope );
				}
			}
		}
	}
}

CQuestGraphBlock* CQuestGraph::FindGuid( const CGUID& guid ) const
{
	for ( CGraphBlock* block : GraphGetBlocks() )
	{
		if ( CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block ) )
		{
			const CGUID& questBlockGUID = questBlock->GetGUID();
			
			if ( questBlockGUID == guid )
			{
				return questBlock;
			}

			if ( CQuestScopeBlock* questScopeBlock = Cast< CQuestScopeBlock >( questBlock ) )
			{
				if ( CQuestGraph* graph = questScopeBlock->GetGraph() )
				{
					if ( CQuestGraphBlock* foundBlock = graph->FindGuid( guid ) )
					{
						return foundBlock;
					}
				}
			}
		}
	}

	return nullptr; // nothing found
}

void CQuestGraph::GetAllGUIDs( TDynArray< CGUID >& allGUIDs ) const
{
	for ( CGraphBlock* block : GraphGetBlocks() )
	{
		if ( CQuestGraphBlock* questBlock = Cast< CQuestGraphBlock >( block ) )
		{
			const CGUID& questBlockGUID = questBlock->GetGUID();

			allGUIDs.PushBack( questBlockGUID );

			if ( CQuestScopeBlock* questScopeBlock = Cast< CQuestScopeBlock >( questBlock ) )
			{
				if ( CQuestGraph* graph = questScopeBlock->GetGraph() )
				{
					graph->GetAllGUIDs( allGUIDs );
				}
			}
		}
	}
}

const CQuestScopeBlock* CQuestGraph::GetScopeBlockWithEmbeddedPhase( const CQuestPhase* phase ) const
{
	for ( CGraphBlock* block : GraphGetBlocks() )
	{
		if ( CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( block ) )
		{
			if ( scopeBlock->GetPhase() == phase )
			{
				return scopeBlock;
			}
		}
	}
	return nullptr;
}

#endif

void CQuestGraph::GetRewards( TDynArray< CName >& rewards ) const
{
	// TODO some graph traversing algorithm...
	for ( Uint32 i = 0; i < m_graphBlocks.Size(); i++ )
	{
		if ( CQuestPhaseBlock* phaseBlock = Cast< CQuestPhaseBlock >( m_graphBlocks[ i ] ) )
		{
			CQuestGraph* graph = phaseBlock->GetGraph();
			if ( graph )
			{
				graph->GetRewards( rewards );
			}
		}
		else if ( CQuestRewardBlock* rewardBlock = Cast< CQuestRewardBlock >( m_graphBlocks[ i ] ) )
		{
			rewards.PushBackUnique( rewardBlock->GetRewardName() );
		}
	}
}

#ifndef NO_EDITOR
void CQuestGraph::RepairDuplicateInputs( TDynArray< CQuestPhaseIOBlock* >& changed )
{
	THashSet< String > usedNames;
	for ( CGraphBlock* block : m_graphBlocks )
	{
		if ( block->IsA< CQuestPhaseIOBlock >() )
		{
			CQuestPhaseIOBlock* ioBlock = Cast< CQuestPhaseIOBlock >( block );
			CName socketID = ioBlock->GetSocketID();
			if ( usedNames.Exist( socketID.AsString() ) )
			{
				Uint32 i = 0;
				while ( usedNames.Exist( String::Printf( TXT("%s%d"), socketID.AsString().AsChar(), i ) ) )
				{
					++i;
				}
				ioBlock->SetSocketID( CName( String::Printf( TXT("%s%d"), socketID.AsString().AsChar(), i ) ) );
				changed.PushBack( ioBlock );
			}
			usedNames.Insert( ioBlock->GetSocketID().AsString() );
		}
	}
}
#endif