/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "pathlibAreaProcessingJob.h"

namespace PathLib
{

class CDetailedSurfaceData;
class CTerrainAreaDescription;

#ifndef NO_EDITOR_PATHLIB_SUPPORT
class CTerrainProcessingThread : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
protected:
	CDetailedSurfaceData*		m_detailedSurfaceData;

	RED_INLINE CTerrainAreaDescription*	GetArea() const;

public:
	CTerrainProcessingThread( CTerrainAreaDescription* area );
};


class CTerrainSurfaceProcessingThread : public CTerrainProcessingThread
{
	typedef CTerrainProcessingThread Super;
protected:
	Bool						m_dataDirty;
	Bool						m_evictTerrainData;								// NOTICE: Its very heavy stuff, that hacks in terrain tile system! It will force all terrain mipmaps to unload on-fly

	
public:
	CTerrainSurfaceProcessingThread( CTerrainAreaDescription* area, Bool evictTerrainData = false );

	Bool						PreProcessingSync() override;
	Bool						ProcessPathLibTask() override;
	IGenerationManagerBase::CAsyncTask*	PostProcessingSync() override;
	void						DescribeTask( String& task ) override;
};

class CTerrainMarkNavmeshesProcessingThread : public CTerrainProcessingThread
{
	typedef CTerrainProcessingThread Super;
protected:
	TDynArray< CNavmeshAreaDescription* >		m_naviAreas;
	Bool										m_markOnSurface;
	Bool										m_markOnObstacles;
public:
	CTerrainMarkNavmeshesProcessingThread( CTerrainAreaDescription* area, Bool markOnSurface = true, Bool markOnObstacles = true );

	Bool						PreProcessingSync() override;
	Bool						ProcessPathLibTask() override;
	IGenerationManagerBase::CAsyncTask*	PostProcessingSync() override;
	void						DescribeTask( String& task ) override;
};

class CTerrainApplySurfaceDataProcessingThread : public CTerrainProcessingThread
{
	typedef CTerrainProcessingThread Super;
public:
	CTerrainApplySurfaceDataProcessingThread( CTerrainAreaDescription* area )
		: Super( area )															{}

	Bool						ProcessPathLibTask() override;
	void						DescribeTask( String& task ) override;
};

class CTerrainHeightComputationThread : public CTerrainProcessingThread
{
	typedef CTerrainProcessingThread Super;
public:
	CTerrainHeightComputationThread( CTerrainAreaDescription* area )
		: Super( area )															{}

	Bool						ProcessPathLibTask() override;
	void						DescribeTask( String& task ) override;
};

#endif

};		// namespace PathLib
