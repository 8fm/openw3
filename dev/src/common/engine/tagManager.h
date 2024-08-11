/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "globalEventsManager.h"
#include "node.h"

/// Math operation
enum ECompareTagsOperation
{
	BCTO_MatchAll,
	BCTO_MatchAny,
};

BEGIN_ENUM_RTTI( ECompareTagsOperation );
	ENUM_OPTION( BCTO_MatchAll );
	ENUM_OPTION( BCTO_MatchAny );
END_ENUM_RTTI();

/// Global tag manager
class CTagManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Tags );

public:
	CTagManager( Bool createGlobalEventsReporter = true );
	~CTagManager();

	//! Clear all tag mapping
	void Clear();

	//! Add tagged node
	void AddNode( CNode* node, const CName& tag );

	//! Add tagged node
	void AddNode( CNode* node, const TagList& tags );

	//! Remove tagged node
	void RemoveNode( CNode* node, const CName& tag );

	//! Add tagged node
	void RemoveNode( CNode* node, const TagList& tags );

	//! Per-frame update
	void Update();

	//! Get any node that have given tag 
	CNode* GetTaggedNode( const CName& tag );

	//! Get node that have given tag and is closest to given point in space
	CNode* GetTaggedNodeClosestTo( const TagList& tags, const Vector& point );

	//! Get any entity that have given tag 
	CEntity* GetTaggedEntity( const CName& tag );

	//! Get all used tags
	void GetAllTags( TDynArray< CName >& outTags ) const;

	//! Get nodes that have given tag 
	void CollectTaggedNodes( const CName& tag, TDynArray< CNode* >& nodes );

	//! Get entities that have given tag set
	void CollectTaggedEntities( const CName& tags, TDynArray< CEntity* >& entities );
	
	//! Get nodes that have given tag set
	void CollectTaggedNodes( const TagList& tags, TDynArray< CNode* >& nodes, ECompareTagsOperation matchOption = BCTO_MatchAny, const TDynArray< CLayer* >* layersList = NULL );

	//! Get entities that have given tag set
	void CollectTaggedEntities( const TagList& tags, TDynArray< CEntity* >& entities, ECompareTagsOperation matchOption = BCTO_MatchAny, const TDynArray< CLayer* >* layersList = NULL );

	//! Reserve memory for bucket hashmap
	void ReserveBucket( Uint32 count );

	//! Iterate nodes that have given tag set and class
	struct DefaultNodeIterator
	{
		RED_INLINE Bool EarlyTest( CNode* node ) const				{ return true; }
		RED_INLINE void Process( CNode* node, Bool isInitialList )	{}
	};

	template< class Functor >
	RED_INLINE void IterateTaggedNodes( const TagList& tagList, Functor& functor, ECompareTagsOperation matchOption = BCTO_MatchAny );

	template< class Functor >
	RED_INLINE void IterateTaggedNodes( CName tag, Functor& functor );

	//! Get nodes that have given tag set and class - msl> as it promote collecting output I personally would scrap this function
	template< class T >
	RED_INLINE void CollectTaggedNodesOfClass( const TagList& tagList, TDynArray< T* >& nodes, ECompareTagsOperation matchOption = BCTO_MatchAny );

private:

	typedef TDynArray< CNode *, MC_Tags > Bucket;
	typedef THashMap< CName, Bucket, DefaultHashFunc< CName >, DefaultEqualFunc< CName >, MC_Tags, MemoryPool_Default > BucketContainer;

	BucketContainer m_bucketContainer;

	Red::TScopedPtr< TGlobalEventsReporterImpl< CName > > m_globalEventsReporter;	// For reporting added/removed tags
};

template< class Functor >
RED_INLINE void CTagManager::IterateTaggedNodes( const TagList& tagList, Functor& functor, ECompareTagsOperation matchOption )
{
	// No tags to match
	if ( tagList.Empty() )
	{
		return;
	}

	// Match single tag
	if ( tagList.GetTags().Size() == 1 || matchOption == BCTO_MatchAny )
	{
		Bool isInitialList = true;
		for ( CName tag : tagList.GetTags() )
		{
			auto bucketIter = m_bucketContainer.Find( tag );
			if( bucketIter != m_bucketContainer.End() )
			{
				Bucket & bucket = bucketIter->m_second;
				for( CNode * node : bucket )
				{
					if ( functor.EarlyTest( node ) )
					{
						functor.Process( node, isInitialList );
					}
				}

				isInitialList = false;
			}
		}
	}

	// Match many tags
	else if ( matchOption == BCTO_MatchAll )
	{
		// We just need to iterate over first tag bucket, cause if some node is not in this first one - it won't match all for sure :)
		CName firstTag = tagList.GetTags()[ 0 ];
		auto bucketIter = m_bucketContainer.Find( firstTag );
		if( bucketIter != m_bucketContainer.End() )
		{
			Bucket & bucket = bucketIter->m_second;
			for( CNode * node : bucket )
			{
				if ( functor.EarlyTest( node ) && TagList::MatchAll( tagList, node->GetTags() ) )
				{
					functor.Process( node, true );
				}
			}
		}
	}
}

template< class Functor >
RED_INLINE void CTagManager::IterateTaggedNodes( CName tag, Functor& functor )
{
	auto bucketIter = m_bucketContainer.Find( tag );
	if( bucketIter != m_bucketContainer.End() )
	{
		Bucket & bucket = bucketIter->m_second;
		for ( CNode* node : bucket )
		{
			if ( functor.EarlyTest( node ) )
			{
				functor.Process( node, true );
			}
		}
	}
}


template< class T >
RED_INLINE void CTagManager::CollectTaggedNodesOfClass( const TagList& tagList, TDynArray< T* >& nodes, ECompareTagsOperation matchOption )
{
	struct Functor : public Red::System::NonCopyable
	{
		Functor( TDynArray< T* >& output )
			: m_output( output ) {}
		RED_INLINE Bool EarlyTest( CNode* node )
		{
			return node->IsA< T >();
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			if ( isGuaranteedUnique )
			{
				this->m_output.PushBack( static_cast< T* >( node ) );
			}
			else
			{
				this->m_output.PushBackUnique( static_cast< T* >( node ) );
			}
		}

		TDynArray< T* >& m_output;
	} functor( nodes );

	IterateTaggedNodes( tagList, functor, matchOption );
}