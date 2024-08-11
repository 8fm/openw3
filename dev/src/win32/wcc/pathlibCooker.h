/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/pathlibCookerData.h"

class CPathLibTaskPool;

///////////////////////////////////////////////////////////////////////////////
class CWCCNavigationCookingContext : public CNavigationCookingContext
{
public:
	virtual void				InitializeSystems( CWorld* world, Bool pathlibCook ) override;
};

///////////////////////////////////////////////////////////////////////////////
class CPathLibCooker
{
protected:

	CWorld*						m_world;
	CPathLibWorld*				m_pathlib;
	CPathLibTaskPool*			m_taskPool;
	CDirectory*					m_outputDirectory;
	CDirectory*					m_inputDirectory;

	Float						m_timeStarted;
	Float						m_timeCheckpoint;

	Uint32						m_cookerFlags;

	size_t						m_memoryBudget;

	CNavigationCookingContext*	m_context;							

	void						InitTime();
	void						LogTimeCheckpoint();

	Bool						PrepeareOutputDirectory();
	Bool						PopulateInitialAreaList();
	Bool						InitialWorldProcessing();
	Bool						ProcessTerrainMapSurfaces();
	Bool						SmoothTerrainMapSurfaces();
	Bool						ComputeTerrainHeightStructures();
	Bool						ProcessPathlibComponents();
	Bool						ProcessFoliage();
	Bool						SimplifyObstaclesMap();
	Bool						ComputeNavmeshNeigbours();
	Bool						MarkNavmeshes( Bool onSurface, Bool onObstacles );
	Bool						ApplyDetailedSurfaceData();
	Bool						ComputeNavgraphs();
	Bool						WaterPrecomputation();
	Bool						ConnectAreaDescriptions();
	Bool						ComputeCoherentRegions();
	Bool						ComputeHighLevelGraph();
	Bool						CommitOutput();
	void						Shutdown();
public:
	enum ECookerFlags
	{
		FLAGS_DEFAULT						= 0,
		FLAG_IGNORE_OBSTACLES				= FLAG( 0 ),
		FLAG_NO_PATHLIB						= FLAG( 1 ),
	};


	Bool				Cook();

	// some wrap-up for garbage collector logic
	void				GarbageCollect();
	void				TryGarbageCollect();

	CPathLibCooker( CWorld* world, Uint32 flags )
		: m_world( world )
		, m_taskPool( nullptr )
		, m_outputDirectory( nullptr )
		, m_inputDirectory( nullptr )
		, m_cookerFlags( flags )
		, m_memoryBudget( 0x7fffffff )
		, m_context( nullptr )														{}

	~CPathLibCooker();
};


