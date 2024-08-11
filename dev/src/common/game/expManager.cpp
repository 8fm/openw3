/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "expManager.h"
#include "expIntarface.h"
#include "expComponent.h"
#include "expCooked.h"
#include "expCooking.h"

#include "../../common/core/kdTreeSplitter.h"
#include "../../common/core/kdTreeBuilder.h"
#include "../../common/core/kdTreeNNCollector.h"

#include "../../common/engine/dynamicLayer.h"

IMPLEMENT_RTTI_ENUM( EExplorationType )
IMPLEMENT_RTTI_ENUM( ExpZComparision )
IMPLEMENT_RTTI_ENUM( ExpDoubleSided )

ExpManager::ExpManager()
	: m_tree( NULL )
	, m_oracle( new ExpOracleHardCoded )
	, m_isCooked( false )
{
	m_filterOff = true;
}

ExpManager::~ExpManager()
{
	Clear();
}

void ExpManager::OnShutdownAtGameEnd()
{
	Clear();
}

void ExpManager::Clear()
{
	delete m_tree;
	m_tree = nullptr;

	m_exps.Clear();

	m_dynamicExps.Clear();

	m_treeMediator.Clear();

	m_isCooked = false;
}

void ExpManager::AddExplorationByComponent( CExplorationComponent* e )
{
	if ( false == m_isCooked )
	{
		AddExplorationToList( e );
		RebuildTree( nullptr, 0 );
	}
}

void ExpManager::RemoveExplorationByComponent( CExplorationComponent* e )
{
	if ( false == m_isCooked )
	{
		if ( m_exps.Exist( e ) )
		{
			TExpList list = m_exps;

			VERIFY( list.Remove( e ) );

			Clear();

			for ( Uint32 i=0; i<list.Size(); ++i )
			{
				AddExplorationToList( list[ i ] );
			}

			RebuildTree( nullptr, 0 );
		}
	}
}

void ExpManager::AddDynamicExploration( IExploration* e )
{
	m_dynamicExps.PushBackUnique( e );
}

void ExpManager::RemoveDynamicExploration( IExploration* e )
{
	m_dynamicExps.Remove( e );
}

void ExpManager::AddExplorationToList( IExploration* e )
{
	Int32 idx = m_exps.SizeInt();
	m_exps.PushBack( e );

	Vector p1, p2;
	e->GetEdgeWS( p1, p2 );

	m_treeMediator.AddEdge( p1, p2, idx );
}

void ExpManager::RebuildTree( Uint8* cookedTreeData, Uint32 cookedTreeDataSize )
{
	const Int32 maxNodes = MAX_EXPLORATIONS * 2;

	delete m_tree;
	if ( cookedTreeData )
	{
		m_tree = kdTreeBuilder< ExpTree >::BuildCookedTree( cookedTreeData, cookedTreeDataSize, &m_treeMediator, maxNodes );
	}
	else
	{
		m_tree = kdTreeBuilder< ExpTree >::BuildTree( &m_treeMediator, maxNodes ); 
	}
}

void ExpManager::FindNN( Vector const & worldPos, const IExploration* withoutExp, IExplorationList& out ) const
{
	// Static - check tree
	if ( !m_tree )
	{
		return;
	}

	Vector pos = worldPos;

	// TODO we have static array for IExploration list although we would appreciate even more if collector would not allocate dynamic memory
	Float r = 10.f;
	Int32 nn = 10;
	kdTreeNNCollector< ExpTree > collector( nn );

	Int32 num = m_tree->SearchR( pos.A, r, collector, &ExpFilter< MAX_EXPLORATIONS >::FilterFunc, this );

	if ( num == 0 )
	{
		return;
	}
	else if ( num == -1 )
	{
		// Test all
		for ( Int32 i=0; i<NumExps(); ++i )
		{
			const IExploration* e = Exp( i );

			if ( e && e != withoutExp && !out.Exist( e ) && FilterFunc( i ) )
			{
				out.PushBack( e );
			}
		}
	}
	else
	{
		for ( Int32 i=0; i<collector.m_nnNum; ++i )
		{
			Int32 index = collector.m_nnIdx[ i ];
			if ( index != -1 )
			{
				Int32 eId = m_treeMediator.FindEdge( index );

				ASSERT( eId < NumExps() );
				const IExploration* e = eId != -1 ? Exp( eId ) : NULL;

				if ( e && e != withoutExp && !out.Exist( e ) )
				{
					out.PushBack( e );
				}
			}
		}
	} 

	// Dynamic - check list
	const Uint32 dSize = m_dynamicExps.Size();
	if ( dSize > 0 )
	{
		for ( Uint32 i=0; i<dSize; ++i )
		{
			const IExploration* e = m_dynamicExps[ i ];

			if ( e != withoutExp )
			{
				out.PushBack( e );
			}
		}
	}
}

Bool ExpManager::QueryExplorationSync( SExplorationQueryToken & token, const CEntity* entity ) const
{
	IExplorationList ee;

	FindNN( entity->GetWorldPosition(), NULL, ee );

	m_oracle->FilterExplorations( token, ee, ERD_Front, entity );

	return token.IsValid();
}

Bool ExpManager::QueryExplorationFromObjectSync( SExplorationQueryToken & token, const CEntity* entity, const CEntity* object ) const
{
	IExplorationList ee;

	CollectExplorationFromObject( object, ee );

	m_oracle->FilterExplorations( token, ee, ERD_Front, entity );
	
    //Do we really want to play first exploration or the GUI shouldn't display information about available exploration ?
    /*if ( !best && ee.Size() > 0 )
	{
		best = ee[0];
	}*/

	return token.IsValid();
}

IExpExecutor* ExpManager::CreateTransition( const IExploration* from, const IExploration* to, ExecutorSetup& setup ) const
{
	return m_oracle->CreateTransition( from, to, setup );
}

const IExploration* ExpManager::GetNNForExploration( const IExploration* exploration, ExpRelativeDirection dir, const CEntity* entity ) const
{
	IExplorationList ee;

	FindNN( entity->GetWorldPosition(), exploration, ee );

	SExplorationQueryToken token( entity );
	m_oracle->FilterExplorations( token, ee, dir, entity );

	return token.GetExploration();
}

void ExpManager::CollectExplorationFromObject( const CEntity* object, IExplorationList& out ) const
{
	for ( ComponentIterator< CExplorationComponent > it( object ); it; ++it )
	{
		const CExplorationComponent* comp = *it;

		out.PushBack( comp );
	}
}

Bool ExpManager::LoadCookedData( const DataBuffer* cookedData )
{
	Uint32 realDataSize = cookedData->GetSize();
	if ( realDataSize == 0 )
	{
		RED_LOG( ExpManager, TXT("Error reading explorations from cooked data. Buffer is empty.") );
		return false;
	}

	CMemoryFileReaderExternalBuffer reader( cookedData->GetData(), realDataSize );

	// 1. magic (4-byte)
	Uint32 magic = 0;
	reader << magic;
	if ( magic != EXPLORATION_DATA_MAGIC )
	{
		RED_LOG( ExpManager, TXT("Error reading explorations from cooked data. Wrong magic.") );
		return false;
	}

	// 2. size of all the exploration data [ with magic at start and magic at end ] ( 4-byte )
	Uint32 dataSize = 0;
	reader << dataSize;
	if ( realDataSize != dataSize )
	{
		RED_LOG( ExpManager, TXT("Error reading explorations from cooked data. Wrong data size, expected %ld, found %ld."), realDataSize, dataSize );
		return false;
	}

	// 3. version (4-byte)
	Uint32 version = 0;
	reader << version;
	if ( version != EXPLORATION_DATA_VERSION_CURRENT )
	{
		RED_LOG( ExpManager, TXT("Error reading explorations from cooked data. Wrong data version, current is: %ld, but found %ld."), EXPLORATION_DATA_VERSION_CURRENT, version );
		return false;
	}

	// 4. number of static explorations (4-byte)
	Uint32 numStatic = 0;
	reader << numStatic;

	// 5. mediator data size (4-byte)
	Uint32 mediatorDataSize = 0;
	reader << mediatorDataSize;

	// 6. tree data buffer size (4-byte)  
	Uint32 treeMem = 0;
	reader << treeMem;

	// 7. mediator
	m_treeMediator.RestoreFromCookedFile( reader );

	// 8. cooked list
	m_cookedList.Resize( numStatic );
	for ( Uint32 i = 0; i < numStatic; ++i )
	{
		m_cookedList[ i ].Serialize( reader );
	}

	// 9. layer map
	reader << m_layerMap;

	// 10. tree data buffer ( x-byte )
	if ( numStatic > 0 )
	{
		Uint8* treePtr = ( Uint8* ) cookedData->GetData() + reader.GetOffset();
		RebuildTree( treePtr, treeMem );
		reader.Seek( reader.GetOffset() + treeMem );
	}

	// 11. magic ( 4-byte )
	reader << magic;
	if ( magic != EXPLORATION_DATA_MAGIC )
	{
		RED_LOG( ExpManager, TXT("Error reading explorations from cooked data. Wrong end magic. Things may crash in a moment. Please DEBUG.") );
		return false;
	}

	if ( numStatic > 0 )
	{
		m_isCooked = true;
		m_filterOff = false;
		return true;
	}

	return false;
}

void ExpManager::OnLayerAttached( const CLayer* layer )
{
	if ( m_isCooked )
	{
		SIndexRange range = FindRangeForLayer( layer );
		if ( range.IsValid() )
		{
			RED_LOG( ExpManager, TXT("Enable cooked	explorations: %ld-%ld for layer %ls"), range.m_min, range.m_max, layer->GetFriendlyName().AsChar() );
			
			for ( Int32 i = range.m_min; i <= range.m_max; ++i )
			{
				SetFilterBit( i );
			}

		}
	}
}

void ExpManager::OnLayerDetached( const CLayer* layer )
{
	if ( m_isCooked )
	{
		SIndexRange range = FindRangeForLayer( layer );
		if ( range.IsValid() )
		{
			RED_LOG( ExpManager, TXT("Disable cooked explorations: %ld-%ld for layer %ls"), range.m_min, range.m_max, layer->GetFriendlyName().AsChar() );
			
			for ( Int32 i = range.m_min; i <= range.m_max; ++i )
			{
				ClearFilterBit( i );
			}
		}
	}
}

ExpManager::SIndexRange ExpManager::FindRangeForLayer( const CLayer* layer ) const
{
	ASSERT( false == layer->GetGUID().IsZero() || layer->IsA< CDynamicLayer > () );

	const Uint64* ptr = m_layerMap.FindPtr( layer->GetGUID() );
	if ( ptr )
	{
		return SIndexRange( *ptr );
	}
	
	static const SIndexRange invalid( -1, -1 );
	return invalid;
}
