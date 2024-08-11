/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef NO_EDITOR
	#define NO_EXPLORATION_FINDER
#endif

class CExplorationFinder
{
public:
	struct SFinderParams
	{
		Float m_radius;
		Float m_height;
		
		Float m_stepXY;
		Float m_stepZ;
		Float m_dropOffsetDistClose; // distance to drop (to check if we will climb to this point)
		Float m_dropOffsetDistFar;
		Float m_dropDepth; // how deep we should drop to check if we need to climb
		
		Float m_minSeparation;
		Float m_minHeightSeparation;

		// to existing exploration
		Float m_minExpSeparation;
		Float m_minExpHeightSeparation;

		CPhysicsWorld* m_physicsWorld;
		CPhysicsEngine::CollisionMask m_include;
		CPhysicsEngine::CollisionMask m_exclude;

		SFinderParams();
	};

	struct CAsyncTask : public Red::Threads::CThread
	{
	public:
		CAsyncTask( CExplorationFinder* finder, const AnsiChar* threadName = "exploration-finder" );
		~CAsyncTask();
		
		void ThreadFunc() override;

		void TryToBreak() { m_tryToBreak = true; }
		Float GetPercentageDone() const { return m_percentageDone; }

		void GetReady();

	private:
		SFinderParams m_params;
		CResource* m_markerResource;
		CEntityTemplate* m_markerResourceTemplate;
		CExplorationFinder* m_finder;
		Float m_percentageDone;
		TDynArray<Vector> m_markersAt;
		TDynArray<CJobSpawnEntity*> m_spawnEntityJobs;

		Bool m_tryToBreak;

		Bool LinkSpawnedEntity();

		void RunFor( CLayer* layer, Float percentageWholeStep );
		void Scan( CLayer* layer, Box const & bbox, Float percentageWholeStep );

		Bool CheckIfNeededAt( Vector const & refWS, Vector & outLocation, EulerAngles & outOrientation );
		Bool CheckIfTooClose( CLayer* layer, Vector const & refWS, Float minSeparation, Float minHeightSeparation );
	};

public:
	CExplorationFinder();
	~CExplorationFinder();

public:
	void GenerateEditorFragments( CRenderFrame* frame );

	void FindExplorations( Bool find, CLayer* onlyOnLayer = nullptr );

	void Shutdown();

	void BindToWorld( CWorld* world );

	Bool IsActive() const { return m_active; }

	void RemoveAll();
	void Hide(Bool hide = true);
	Bool IsHidden() const { return ! m_visible; }

	void UpdateCounters();

	static CEntity* FindPrev( CWorld* world, CEntity* startWith, Bool includeIgnored );
	static CEntity* FindNext( CWorld* world, CEntity* startWith, Bool includeIgnored );
	static Bool IsFoundExploration( const CEntity * entity, Bool * outIgnored = nullptr );
	static void ToggleIgnoredTo( const CEntity * entity, Bool ignored );

private:
	Bool m_active;
	CAsyncTask* m_job;

	Bool m_visible;

	THandle< CWorld > m_world; // binded to world
	CLayer* m_onlyOnLayer;

	Int32 m_removedValidMarkers;
	Int32 m_addedInvalidMarkers;
	Int32 m_totalInvalidMarkers;
	Int32 m_ignoredMarkers;

	friend struct CAsyncTask;
};

typedef TSingleton<CExplorationFinder> SExplorationFinder;
