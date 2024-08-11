/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "tagManager.h"
#include "entity.h"
#include "node.h"

IMPLEMENT_RTTI_ENUM( ECompareTagsOperation );

CTagManager::CTagManager( Bool createGlobalEventsReporter )
	: m_globalEventsReporter( nullptr )
{
	if ( createGlobalEventsReporter )
	{
		m_globalEventsReporter.Reset( new TGlobalEventsReporterImpl< CName >( GEC_Tag ) );
	}
}

CTagManager::~CTagManager()
{}

void CTagManager::ReserveBucket( Uint32 count )
{
	m_bucketContainer.Reserve( count );
}

void CTagManager::Clear()
{
	m_bucketContainer.ClearFast();
}

void CTagManager::AddNode( CNode* node, const CName& tag )
{
	if( tag == CName::NONE )
	{
		return;
	}

	Bucket & bucket = m_bucketContainer[ tag ];
	auto iter = Find( bucket.Begin(), bucket.End(), node );
	
	if( iter == bucket.End() )
	{
		bucket.PushBack( node );
		if ( m_globalEventsReporter )
		{
			m_globalEventsReporter->AddEvent( GET_TagAdded, tag );
		}
	}
}

void CTagManager::AddNode( CNode* node, const TagList& tags )
{
	for ( CName tag : tags.GetTags() )
	{
		AddNode( node, tag );
	}
}

void CTagManager::RemoveNode( CNode* node, const CName& tag )
{
	auto iter = m_bucketContainer.Find( tag );
	if( iter != m_bucketContainer.End() )
	{
		Bucket & bucket = iter->m_second;
		auto nodeIter = Find( bucket.Begin(), bucket.End(), node );
		if( nodeIter != bucket.End() )
		{
			bucket.EraseFast( nodeIter );
			if( bucket.Empty() )
			{
				m_bucketContainer.Erase( iter );
			}
		}
	}

	if ( m_globalEventsReporter )
	{
		m_globalEventsReporter->AddEvent( GET_TagRemoved, tag );
	}
}

void CTagManager::RemoveNode( CNode* node, const TagList& tags )
{
	for ( CName tag : tags.GetTags() )
	{
		RemoveNode( node, tag );
	}
}

void CTagManager::Update()
{
	if ( m_globalEventsReporter )
	{
		m_globalEventsReporter->ReportEvents();
	}
}

CNode* CTagManager::GetTaggedNode( const CName& tag )
{
	auto iter = m_bucketContainer.Find( tag );
	if( iter != m_bucketContainer.End() )
	{
		Bucket & bucket = iter->m_second;
		return bucket.Front();
	}

	return nullptr;
}

CNode* CTagManager::GetTaggedNodeClosestTo( const TagList& tags, const Vector& point )
{
	TDynArray< CNode* > nodes;
	CollectTaggedNodes( tags, nodes );

	CNode* bestFoundNode = NULL;

	if ( nodes.Empty() == false )
	{
		auto nodeIter = nodes.Begin();

		bestFoundNode = *nodeIter;
		ASSERT( bestFoundNode != NULL );
		Float bestPlacementDistanceSqr = point.DistanceSquaredTo( bestFoundNode->GetWorldPositionRef() );

		++nodeIter;

		for( ; nodeIter != nodes.End(); ++nodeIter )
		{
			CNode* candidatePlacementNode = *nodeIter;
			ASSERT( candidatePlacementNode != NULL );
			Float candidatePlacementDistanceSqr = point.DistanceSquaredTo( candidatePlacementNode->GetWorldPositionRef() );
			if ( candidatePlacementDistanceSqr < bestPlacementDistanceSqr )
			{
				bestPlacementDistanceSqr = candidatePlacementDistanceSqr;
				bestFoundNode = candidatePlacementNode;
			}
		}
	}
	return bestFoundNode;
}

CEntity* CTagManager::GetTaggedEntity( const CName& tag )
{
	auto iter = m_bucketContainer.Find( tag );
	if( iter != m_bucketContainer.End() )
	{
		Bucket & bucket = iter->m_second;
		for( CNode * node : bucket )
		{
			CEntity* entity = node->AsEntity();
			if( entity )
			{
				return entity;
			}
		}
	}

	return nullptr;
}

void CTagManager::GetAllTags( TDynArray< CName >& outTags ) const
{
	outTags.Reserve( m_bucketContainer.Size() );
	for( auto iter = m_bucketContainer.Begin(), end = m_bucketContainer.End(); iter != end; ++iter )
	{
		outTags.PushBack( iter->m_first );
	}
}

void CTagManager::CollectTaggedNodes( const CName& tag, TDynArray< CNode* >& nodes )
{
	auto iter = m_bucketContainer.Find( tag );
	if( iter != m_bucketContainer.End() )
	{
		nodes = iter->m_second;
	}
}

void CTagManager::CollectTaggedEntities( const CName& tag, TDynArray< CEntity* >& entities )
{
	auto iter = m_bucketContainer.Find( tag );
	if( iter != m_bucketContainer.End() )
	{
		Bucket & bucket = iter->m_second;
		for( CNode * node : bucket  )
		{
			CEntity* entity = node->AsEntity();
			if( entity )
			{
				entities.PushBack( entity );
			}
		}
	}
}

static RED_INLINE Bool LayerFilter( CNode* node, const TDynArray< CLayer* >* layersList )
{
	if ( layersList )
	{
		return layersList->Exist( node->GetLayer() );
	}

	return true;
}

void CTagManager::CollectTaggedNodes( const TagList& tagList, TDynArray< CNode* >& nodes, ECompareTagsOperation matchOption /*= BCTO_MatchAny*/, const TDynArray< CLayer* >* layersList /*=NULL*/ )
{
	struct Functor : public Red::System::NonCopyable
	{
		Functor( const TDynArray< CLayer* >* layersList, TDynArray< CNode* >& output )
			: m_layersList( layersList )
			, m_output( output )
		{}
		RED_INLINE Bool EarlyTest( CNode* node )
		{
			return LayerFilter( node, m_layersList );
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			if ( isGuaranteedUnique )
			{
				ASSERT( ::Find( m_output.Begin(), m_output.End(), node ) == m_output.End(), TXT("Since we are iterating over one hash bucket, we should only access unique nodes!") );
				m_output.PushBack( node );
			}
			else
			{
				m_output.PushBackUnique( node );
			}
		}
		const TDynArray< CLayer* >*		m_layersList;
		TDynArray< CNode* >&			m_output;
	} functor( layersList, nodes );

	IterateTaggedNodes( tagList, functor, matchOption );
}

void CTagManager::CollectTaggedEntities( const TagList& tagList, TDynArray< CEntity* >& entities, ECompareTagsOperation matchOption /*= BCTO_MatchAny*/, const TDynArray< CLayer* >* layersList /*=NULL*/ )
{
	struct Functor : public Red::System::NonCopyable
	{
		Functor( const TDynArray< CLayer* >* layersList, TDynArray< CEntity* >& output )
			: m_layersList( layersList )
			, m_output( output )
		{}
		RED_INLINE Bool EarlyTest( CNode* node )
		{
			if ( node->AsEntity() == NULL )
			{
				return false;
			}
			return LayerFilter( node, m_layersList );
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			if ( isGuaranteedUnique )
			{
				m_output.PushBack( static_cast< CEntity* >( node ) );
			}
			else
			{
				m_output.PushBackUnique( static_cast< CEntity* >( node ) );
			}
		}
		const TDynArray< CLayer* >*		m_layersList;
		TDynArray< CEntity* >&			m_output;
	} functor( layersList, entities );

	IterateTaggedNodes( tagList, functor, matchOption );
}
