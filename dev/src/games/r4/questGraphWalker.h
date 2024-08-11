/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_EDITOR

#include "../../common/core/dynarray.h"
#include "../../common/core/sortedmap.h"
#include "../../common/game/questGraphBlock.h"

class CQuest;
class CQuestGraph;

/// helper class to use to collect scenes & community stuff for PlayGO
class CQuestGraphWalker
{
public:
	CQuestGraphWalker();
	~CQuestGraphWalker();

	// extract custom stuff from quest
	void WalkQuest( CQuest* graph );

	// flush the results to output file
	void EmitToSeedFile( class CAnalyzerOutputList& outputList );

protected:
	// manual collection bucket for stuff what we've found in the quests
	struct Bucket
	{
		CName					m_name;
		TDynArray< String >		m_scenes;
		TDynArray< String >		m_communities;
		TDynArray< String >		m_movies;
		TDynArray< String >		m_resources;

		RED_INLINE Bucket( CName name )
			: m_name( name )
		{}
	};

	// buckets for each content package
	typedef TSortedMap< CName, Bucket* >		TBuckets;
	TBuckets					m_buckets;

	// resource consumer
	class ResourceConsumer : public IQuestContentCollector
	{
	public:
		ResourceConsumer( TBuckets* buckets, CName currentBucket );

	private:
		// IQuestContentCollector interface
		virtual void CollectResource( const String& depotPath, const CName contentChunkOverride = CName::NONE ) override;

		// get bucket for given name (even empty name is supported)
		Bucket* GetBucket( CName name );

		CName			m_mainBucket;
		TBuckets*		m_buckets;
	};

	// walk the quest graph
	void WalkQuest( CQuestGraph* graph, CName contentChunk );
};

#endif