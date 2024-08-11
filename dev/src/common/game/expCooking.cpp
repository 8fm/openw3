/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "expCooking.h"
#include "expCooked.h"
#include "expComponent.h"

#ifndef NO_EDITOR
CExplorationCookingContext::CExplorationCookingContext( const CWorld& world, CDirectory* cookingDirectory )
	: m_outputDirectory( cookingDirectory )
	, m_worldFileName( world.GetFile()->GetFileName().StringBefore( TXT(".w2w"), true ) )
	, m_resource( nullptr )
{
}

CExplorationCookingContext::~CExplorationCookingContext()
{
	for ( auto exp : m_exps )
	{
		delete exp;
	}
}

void CExplorationCookingContext::OnExploration( CExplorationComponent* e )
{
	m_manager.AddExplorationByComponent( e );
	Int32 expIndex = m_exps.SizeInt();
	m_exps.PushBack( new CCookedExploration( e ) );

	ASSERT( e->GetEntity() && e->GetEntity()->GetLayer() );

	BindExpWithLayer( expIndex, e->GetEntity()->GetLayer() ); 
}

void CExplorationCookingContext::GetStats( CExplorationCookingContext::SStats& stats ) const
{
	stats.m_numExps = m_manager.m_exps.Size();
	stats.m_numDynamic = m_manager.m_dynamicExps.Size();
	stats.m_treeMem = Uint32( m_manager.m_tree ? m_manager.m_tree->GetMemUsage() : 0 );
}

DataBuffer* CExplorationCookingContext::CookToDataBuffer()
{
	const Uint32 dataSize = ComputeCookedDataSize();

	DataBuffer* memory = new DataBuffer( DefaultDataBufferAllocator(), dataSize );

	CMemoryFileWriterExternalBuffer writer( memory->GetData(), dataSize );
	CookToFile( writer );

	return memory;
}

void CExplorationCookingContext::CookToFile( IFile& writer )
{
	SStats stats;
	GetStats( stats );

	// 1. magic (4-byte)
	Uint32 magic = EXPLORATION_DATA_MAGIC;
	writer << magic;

	// 2. size of all the exploration data [ with magic at start and magic at end ] ( 4-byte )
	Uint32 dataSize = ComputeCookedDataSize();
	writer << dataSize;

	// 3. version (4-byte)
	Uint32 version = EXPLORATION_DATA_VERSION_CURRENT;
	writer << version;

	// 4. number of static explorations (4-byte)
	writer << stats.m_numExps;

	// 5. mediator data size (4-byte)
	Uint32 mediatorDataSize = m_manager.m_treeMediator.ComputeCookedDataSize();
	writer << mediatorDataSize;

	// 6. tree data buffer size (4-byte)
	writer << stats.m_treeMem;

	// 7. mediator data	(x-byte)
	m_manager.m_treeMediator.CookToFile( writer );

	// 8. array of exps	( num * x-bute )
	ASSERT( stats.m_numExps == m_exps.Size() );
	for ( Uint32 i = 0; i < m_exps.Size(); ++i )
	{
		m_exps[ i ]->Serialize( writer );
	}

	// 9. layer data ( x-byte )
	writer << m_layerMap;

	// 10. tree data buffer ( x-byte )
	if ( m_manager.m_tree )
	{
		writer.Serialize( m_manager.m_tree->GetData(), stats.m_treeMem );
	}

	// 11. magic ( 4-byte )
	writer << magic;	
}

Uint32 CExplorationCookingContext::ComputeCookedDataSize() const
{
	SStats stats;
	GetStats( stats );

	return 																				// We save:
		Uint32( sizeof( Uint32 )														// 1. magic (4-byte)
		+ sizeof( Uint32 )		 														// 2. size of all the exploration data ( 4-byte )
		+ sizeof( Uint32 )																// 3. version (4-byte)
		+ sizeof( Uint32 )																// 4. number of static explorations (4-byte)
		+ sizeof( Uint32 )																// 5. mediator data size (4-byte)
		+ sizeof( Uint32 )																// 6. tree data buffer size (4-byte)
		+ ( m_exps.Empty() ? 0 : ( m_exps.Size() * m_exps[ 0 ]->ComputeDataSize() ) )	// 7. exps data ( n * x-byte )
		+ m_manager.m_treeMediator.ComputeCookedDataSize()								// 8. mediator data buffer ( x-byte )
		+ ComputeLayerDataSize()														// 9. layer data ( x-byte )
		+ stats.m_treeMem																// 10. tree data buffer ( x-byte )
		+ sizeof( Uint32 )		 														// 11. magic ( 4-byte )	
		);
}

Uint32 CExplorationCookingContext::ComputeLayerDataSize() const
{
	return m_layerMap.Size() * ( sizeof( CGUID ) + sizeof( Uint64 ) ) + sizeof( Uint32 );
}

Bool CExplorationCookingContext::CommitOutput()
{
	m_resource = new CCookedExplorations();	
	m_resource->m_data = CookToDataBuffer();
	return m_resource->SaveAs( m_outputDirectory, m_worldFileName, false );
}

void CExplorationCookingContext::BindExpWithLayer( Int32 expIndex, const CLayer* layer )
{
	ASSERT( expIndex >= 0 && layer != nullptr );

	const CGUID& layerGuid = layer->GetGUID();
	ASSERT( false == layerGuid.IsZero() );

	ExpManager::SIndexRange* ptr = reinterpret_cast< ExpManager::SIndexRange* > ( m_layerMap.FindPtr( layerGuid ) );

	if ( ptr )
	{
		ASSERT( ptr->m_min < expIndex );
		ASSERT( ptr->m_max == expIndex - 1 );
		ptr->m_max = expIndex;
	}
	else
	{
		ExpManager::SIndexRange range;
		range.m_min = expIndex;
		range.m_max = expIndex;
		RED_VERIFY( m_layerMap.Insert( layerGuid, range ) );
	}
}

#endif // NO_EDITOR
