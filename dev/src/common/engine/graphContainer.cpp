/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "graphContainer.h"
#include "../core/dependencyLoader.h"
#include "../core/memoryFileReader.h"
#include "graphBlock.h"

IMPLEMENT_RTTI_ENUM( EGraphLayerState );
IMPLEMENT_ENGINE_CLASS( SGraphLayer );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void IGraphContainer::GraphStructureModified()
{

}

Bool IGraphContainer::ModifyGraphStructure()
{
	return true;
}

Bool IGraphContainer::GraphSupportsBlockClass( CClass *blockClass ) const
{
	ASSERT( blockClass );
	return blockClass->IsBasedOn( ClassID< CGraphBlock >() ) && !blockClass->IsAbstract();
}

Vector IGraphContainer::GraphGetBackgroundOffset() const
{
	return Vector::ZEROS;
}

void IGraphContainer::GraphSetBackgroundOffset( const Vector& offset )
{
}

CGraphBlock* IGraphContainer::GraphCreateBlock( const GraphBlockSpawnInfo& info )
{
	ASSERT( info.GetClass() );
	ASSERT( GraphSupportsBlockClass( info.GetClass() ) );
	ASSERT( GraphGetOwner() );

	if ( ModifyGraphStructure() )
	{
		CGraphBlock *block = ::CreateObject< CGraphBlock >( info.GetClass(), GraphGetOwner() );
		if ( block )
		{
			GraphGetBlocks().PushBack( block );
			block->OnSpawned( info );
			block->OnRebuildSockets();

			GraphStructureModified();
		}
		return block;
	}
	return NULL;
}

Bool IGraphContainer::GraphCanRemoveBlock( CGraphBlock *block ) const
{
	return true;
}

Bool IGraphContainer::GraphRemoveBlock( CGraphBlock *block )
{
	TDynArray< CGraphBlock* >& blocks = GraphGetBlocks();
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		if ( blocks[i] == block )
		{
			if ( ModifyGraphStructure() )
			{
				blocks.Remove( block );
				block->BreakAllLinks();
				block->OnDestroyed();

				GraphStructureModified();

				return true;
			}
		}
	}
	return false;
}

Bool IGraphContainer::GraphPasteBlocks( const TDynArray< Uint8 >& data, TDynArray< CGraphBlock* >& pastedBlocks, Bool relativeSpawn, Bool atLeftUpper, const Vector& spawnPosition )
{
	// Setup data buffer reader
	CMemoryFileReader reader( data, 0 );
	CDependencyLoader loader( reader, NULL );

	// Deserialize
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = GraphGetOwner();
	if ( !loader.LoadObjects( loadingContext ) )
	{
		// No objects loaded
		return false;
	}

	if ( ModifyGraphStructure() )
	{
		// Get spawned entities
		TDynArray< CGraphBlock* > blocks;
		for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
		{
			CGraphBlock* block = SafeCast< CGraphBlock >( loadingContext.m_loadedRootObjects[i] );
			if ( block )
			{
				blocks.PushBack( block );
			}
		}

		// Finalize load
		loader.PostLoad();

		// Offset root components by specified spawn point
		if ( relativeSpawn )
		{
			if ( atLeftUpper )
			{
				if ( blocks.Size() )
				{
					Vector minPos( blocks[0]->GetPosition().X, blocks[0]->GetPosition().Y, 0 );
					for ( Uint32 i = 1; i < blocks.Size(); i++ )
					{
						Vector pos = blocks[i]->GetPosition();
						if ( minPos.X > pos.X )
						{
						    minPos.X = pos.X;
						}
						if ( minPos.Y > pos.Y )
						{
						    minPos.Y = pos.Y;
						}
					}
					for ( Uint32 i=0; i<blocks.Size(); i++ )
					{
						CGraphBlock* block = blocks[i];
						Vector delta = block->GetPosition() - minPos;
						block->SetPosition( spawnPosition + delta );
					}
				}
			}
			else
			{
				Vector rootPosition = blocks[0]->GetPosition();
				for ( Uint32 i=0; i<blocks.Size(); i++ )
				{
					CGraphBlock* block = blocks[i];
					Vector delta = block->GetPosition() - rootPosition;
					block->SetPosition( spawnPosition + delta );
				}
			}
		}

		// Add to list of blocks
		for ( Uint32 i=0; i<blocks.Size(); i++ )
		{
			CGraphBlock* block = blocks[i];
			if ( GraphSupportsBlockClass( block->GetClass() ) )
			{
				block->InvalidateLayout();
				GraphGetBlocks().PushBack( block );
				pastedBlocks.PushBack( block );
			}
			else
			{
				// Delete block
				block->BreakAllLinks();
				block->Discard();
			}
		}

		GraphStructureModified();

		// Done
		return true;
	}
	return false;
}

Uint32 IGraphContainer::GetLayersInStateFlag( EGraphLayerState state )
{
	if ( GetLayerNum() > 0 )
	{
		// Reset flag
		Uint32 flag = 0;

		Uint32 layerNum = GetLayerNum();

		for ( Uint32 i=0; i<layerNum; i++ )
		{
			SGraphLayer* layer = GetLayer( i );

			if ( layer->m_state == state )
			{
				// Set flag
				flag |= 1<<i;
			}
		}

		return flag;
	}
	else
	{
		return 0xFFFFFFFF;
	}
}

#endif