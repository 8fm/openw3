
#include "build.h"
#include "umbraJobs.h"
#include "umbraIncludes.h"
#include "umbraStructures.h"
#include "umbraTile.h"
#include "renderProxy.h"
#include "../core/events.h"
#include "umbraScene.h"
#include "umbraStructures.h"

#ifdef USE_UMBRA

using Umbra::ComputationParams;

CJobCreateTomeCollection::CJobCreateTomeCollection( const TUmbraTileArray& tiles, IRenderScene* renderScene, const CUmbraTomeCollection* oldTomeCollectionWrapper )
	: m_tiles( tiles )
	, m_renderScene( renderScene )
	, m_oldTomeCollectionWrapper( oldTomeCollectionWrapper )
	, m_tomeCollectionWrapper( nullptr )
	, m_result( false )
#ifndef RED_FINAL_BUILD
	, m_scratchAllocationPeak( 0 )
	, m_tomeCollectionSize( 0 )
#endif // RED_FINAL_BUILD
{
	if ( m_renderScene )
	{
		m_renderScene->AddRef();
	}
}

CJobCreateTomeCollection::~CJobCreateTomeCollection()
{
	SAFE_RELEASE( m_renderScene );
}

void CJobCreateTomeCollection::Run()
{
	PC_SCOPE_PIX( Umbra_CJobCreateTomeCollection );

	CTimeCounter jobTimer;

	TDynArray< const Umbra::Tome* > tomesForCollection;
	for ( auto& tile : m_tiles )
	{
		const Umbra::Tome* tome = tile->GetTome();
		RED_ASSERT( tome );
		if ( tome )
		{
			Bool isValidForTomeCollection = tome->testCapability( Umbra::Tome::CAPABILITY_TOMECOLLECTION_INPUT );
			RED_ASSERT( isValidForTomeCollection );
			if ( isValidForTomeCollection )
			{
				tomesForCollection.PushBack( tome );
			}
		}
	}

	RED_LOG_SPAM( UmbraInfo, TXT("JobCreateTomCollection: loading Tiles took: %1.3fms"), jobTimer.GetTimePeriodMS() );

	if ( IsCancelled() )
	{
		RED_LOG_SPAM( UmbraInfo, TXT("JobCreateTomCollection: Cancelled before building!") );
		return;
	}

	jobTimer.ResetTimer();
	{
		RED_ASSERT( !m_tomeCollectionWrapper );
		m_tomeCollectionWrapper = new CUmbraTomeCollection();
		UmbraTomeCollectionScratchAllocator	scratchAllocator;
		m_result = m_tomeCollectionWrapper->BuildTomeCollection( tomesForCollection, scratchAllocator, m_oldTomeCollectionWrapper );
		if ( !m_result )
		{
			// build failed
			RemoveTomeCollection();
			return;
		}
		RED_ASSERT( m_tomeCollectionWrapper->GetTomeCollection() );
#ifndef RED_FINAL_BUILD
		m_tomeCollectionSize = ( tomesForCollection.Size() == 1 ) ? tomesForCollection[0]->getSize() : m_tomeCollectionWrapper->GetTomeCollection()->getSize();
		m_scratchAllocationPeak = scratchAllocator.GetAllocationPeak();
#endif // RED_FINAL_BUILD
	}
	RED_LOG_SPAM( UmbraInfo, TXT("JobCreateTomCollection: building TomeCollection took: %1.3fms"), jobTimer.GetTimePeriodMS() );

	jobTimer.ResetTimer();
	Umbra::TomeCollection* tomeCollection = m_tomeCollectionWrapper->GetTomeCollection();
	m_remapTable.Resize( tomeCollection->getObjectCount() );
	m_objectIdToIndex.Reserve( tomeCollection->getObjectCount() );
	// build remap table
	for ( Uint32 i = 0; i < m_remapTable.Size(); ++i )
	{
		TObjectIdType objectId = tomeCollection->getObjectUserID( i );
		m_remapTable[ i ] = objectId;
		m_objectIdToIndex.Insert( objectId, i );
	}
	RED_LOG_SPAM( UmbraInfo, TXT("JobCreateTomCollection: building RemapTable took: %1.3fms"), jobTimer.GetTimePeriodMS() );
}

void CJobCreateTomeCollection::RemoveTomeCollection()
{
	if ( m_tomeCollectionWrapper )
	{
		delete m_tomeCollectionWrapper;
		m_tomeCollectionWrapper = nullptr;
	}
}

void CJobCreateTomeCollection::ClearJobInfoFromTomes()
{
	for ( auto& tile : m_tiles )
	{
		if ( tile )
		{
			tile->SetInTomeCollectionJob( false );
		}
	}
}

#endif // USE_UMBRA
