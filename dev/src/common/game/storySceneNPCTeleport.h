/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storyScene.h"
#include "storySceneDirectorPlacementHelper.h"
#include "storySceneIncludes.h"
#include "..\engine\cutsceneInstance.h"
#include "..\engine\pathlibWorld.h"
#include "..\engine\pathlibWalkableSpotQueryRequest.h"

class CStorySceneCutsceneSection;
class BoxFilter;

// Base filter class used internally in CNPCTeleportPositionRequest
class CNPCTeleportPositionFilter
{
private:
	Red::Threads::CAtomic< Int32 > m_refCount;

public:
	typedef TRefCountPointer< CNPCTeleportPositionFilter > Ptr;

	CNPCTeleportPositionFilter();
	virtual ~CNPCTeleportPositionFilter();
	virtual Bool AcceptPosition( const Vector3& pos ) = 0;

	virtual Bool IsFullyContained( const Box& box ) const = 0;

	// TRefCountPointer implementation

	void AddRef();
	void Release();
};

// Single 'find safe spot' query
class CNPCTeleportPositionRequest : public PathLib::CWalkableSpotQueryRequest
{
public:
	typedef PathLib::CWalkableSpotQueryRequest Super;
	typedef TRefCountPointer< CNPCTeleportPositionRequest > Ptr;

private:
	THandle< CNewNPC >				m_npc;
	Box								m_npcBox;
	Vector							m_npcBoxExtents;
	Bool							m_wasInInterior;
	Float							m_minDistance;
	CNPCTeleportPositionFilter::Ptr	m_filter;

public:
	CNPCTeleportPositionRequest();
	~CNPCTeleportPositionRequest();

	// Overridden from PathLib::CWalkableSpotQueryRequest

	Bool AcceptPosition( const Vector3& pos ) override;

	// Interface

	void Setup( CNewNPC* npc, CNPCTeleportPositionFilter::Ptr& filter, const Box& box, const Vector3& destinationPosition, Float minDistance );
	CNewNPC* GetNPC() const { return m_npc.Get(); }
};

// Batch job to teleport multiple NPCs
class CTeleportNPCsJob
{
private:
	static Float TeleportNPCJobTimeout;

	CStorySceneDirector* m_director;
	EngineTime m_startTime;
	CNPCTeleportPositionFilter::Ptr m_filter;
	Box m_box;
	TDynArray< CNPCTeleportPositionRequest::Ptr > m_requests;

public:
	CTeleportNPCsJob( CStorySceneDirector* director, CNPCTeleportPositionFilter* filter, const Box& box );
	~CTeleportNPCsJob();
	void AddTeleportRequest( CNewNPC* npc, Float minDistance );
	Bool IsAsyncPartDone() const;
	void Cancel();
	void FinalizeSyncPart();
};

// Helper class used to block area (by starting NPCs teleport jobs and creating denied areas)
class CStorySceneNPCTeleportHelper
{
public:
	struct NPCToTeleport
	{
		CNewNPC* m_npc;
		Float m_minDistance;

		NPCToTeleport( CNewNPC* npc = nullptr, Float minDistance = 0.0f )
			: m_npc( npc )
			, m_minDistance( minDistance )
		{}
	};

	static CTeleportNPCsJob* BlockAreaForSection( CStorySceneDirector* director, const CStorySceneSection* section );

private:
	static CTeleportNPCsJob* BlockAreaForNonCutsceneSection( CStorySceneDirector* director, const CStorySceneSection* section );
	static CTeleportNPCsJob* BlockAreaForCutsceneSection( CStorySceneDirector* director, const CStorySceneCutsceneSection* section );
	static CTeleportNPCsJob* TeleportNPCsAway( CStorySceneDirector* director, const TDynArray< NPCToTeleport >& npcsToTeleportAway, const Box& box, CNPCTeleportPositionFilter* filter );
	static void FindAxiiedNPCs( CStorySceneDirector* director, TDynArray< NPCToTeleport >& npcsToTeleportAway, const Vector& center );
	static void FilterOutSceneActors( CStorySceneDirector* director, TDynArray< NPCToTeleport >& npcsToTeleportAway );
	static void AddNearbyNPCsToFilter( BoxFilter* boxFilter, const Vector& searchCenter );
};

