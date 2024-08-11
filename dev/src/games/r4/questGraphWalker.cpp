/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questGraphWalker.h"

#ifndef NO_EDITOR

#include "../../common/core/analyzer.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/communityData.h"

#include "../../common/game/quest.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questScopeBlock.h"
#include "../../common/game/questPhaseBlock.h"

//----

CQuestGraphWalker::ResourceConsumer::ResourceConsumer( CQuestGraphWalker::TBuckets* buckets, CName currentBucket )
	: m_buckets( buckets )
	, m_mainBucket( currentBucket )
{
}

void CQuestGraphWalker::ResourceConsumer::CollectResource( const String& depotPath, const CName contentChunkOverride /*= CName::NONE*/ )
{
	String temp1;
	const String& conformedDepothPath = CFilePath::ConformPath( depotPath, temp1 );

	const CName contentChunkName = contentChunkOverride ? contentChunkOverride : m_mainBucket;
	Bucket* bucket = GetBucket( contentChunkName );
	if ( bucket )
	{
		const String ext = StringHelpers::GetFileExtension( conformedDepothPath );
		if ( ext == ResourceExtension< CStoryScene >() )
		{
			bucket->m_scenes.PushBackUnique( conformedDepothPath );
		}
		else if ( ext == ResourceExtension< CCommunity >() )
		{
			bucket->m_communities.PushBackUnique( conformedDepothPath );
		}
		else if ( ext == TXT("usm") )
		{
			bucket->m_movies.PushBackUnique( conformedDepothPath );
		}
		else
		{
			bucket->m_resources.PushBackUnique( conformedDepothPath );
		}
	}
}

CQuestGraphWalker::Bucket* CQuestGraphWalker::ResourceConsumer::GetBucket( CName name )
{
	CQuestGraphWalker::Bucket* ret = nullptr;
	if ( !m_buckets->Find( name, ret ) )
	{
		ret = new CQuestGraphWalker::Bucket( name );
		m_buckets->Insert( name, ret );
	}

	return ret;
}

//----

CQuestGraphWalker::CQuestGraphWalker()
{
}

CQuestGraphWalker::~CQuestGraphWalker()
{
}

void CQuestGraphWalker::WalkQuest( CQuest* quest )
{
	// get graph
	CQuestGraph* graph = quest->GetGraph();
	if ( graph )
	{
		WalkQuest( graph, CName::NONE );
	}
}

void CQuestGraphWalker::WalkQuest( CQuestGraph* graph, CName contentChunk )
{
	// process blocks
	const auto& blocks = graph->GraphGetBlocks();
	for ( CGraphBlock* block : blocks )
	{
		// skip over empty blocks
		if ( !block )
			continue;

		// phase block ?
		if ( block->IsA< CQuestPhaseBlock >() )
		{
			CQuestPhaseBlock* phase = static_cast< CQuestPhaseBlock* >( block );

			// do we have a sub graph ?
			CQuestGraph* phaseGraph = phase->GetGraph();
			if ( phaseGraph )
			{
				const CName contentChunkOverride = phase->GetContentChunk();
				const CName phaseContent = contentChunkOverride ? contentChunkOverride : contentChunk;
				WalkQuest( phaseGraph, phaseContent );
			}				
		}

		// extract resources
		if ( block->IsA< CQuestGraphBlock >() )
		{
			CQuestGraphBlock* questBlock = static_cast< CQuestGraphBlock* >( block );

			ResourceConsumer contentCollector( &m_buckets, contentChunk );
			questBlock->CollectContent( contentCollector );
		}
	}
}

void CQuestGraphWalker::EmitToSeedFile( CAnalyzerOutputList& outputList )
{
	for ( auto it = m_buckets.Begin(); it != m_buckets.End(); ++it )
	{
		const Bucket& bucket = *(*it).m_second;

		LOG_GAME( TXT("Content group '%ls':"), bucket.m_name.AsChar() );
		if ( !bucket.m_scenes.Empty() )
		{
			LOG_GAME( TXT("  %d scenes"), bucket.m_scenes.Size() );
		}
		if ( !bucket.m_communities.Empty() )
		{
			LOG_GAME( TXT("  %d communities"), bucket.m_communities.Size() );
		}
		if ( !bucket.m_movies.Empty() )
		{
			LOG_GAME( TXT("  %d movies"), bucket.m_movies.Size() );
		}
		if ( !bucket.m_resources.Empty() )
		{
			LOG_GAME( TXT("  %d resources"), bucket.m_resources.Size() );
		}

		CAnalyzerOutputList::TChunkIDs chunkIds;
		if ( bucket.m_name )
		{
			chunkIds.PushBack( bucket.m_name );
		}
		outputList.SetContentChunks( &chunkIds );

		// add files
		for ( const String& path : bucket.m_scenes )
			outputList.AddFile( UNICODE_TO_ANSI( path.AsChar() ) );
		for ( const String& path : bucket.m_communities )
			outputList.AddFile( UNICODE_TO_ANSI( path.AsChar() ) );
		for ( const String& path : bucket.m_movies )
			outputList.AddFile( UNICODE_TO_ANSI( path.AsChar() ) );
		for ( const String& path : bucket.m_resources )
			outputList.AddFile( UNICODE_TO_ANSI( path.AsChar() ) );
	}
}

#endif //NO_EDITOR
