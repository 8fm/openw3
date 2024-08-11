/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "foliageForward.h"
#include "pathlibAreaProcessingJob.h"

namespace PathLib
{

class CTerrainAreaDescription;

#ifndef NO_EDITOR_PATHLIB_SUPPORT
class CTerrainFoliageProcessingThread : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
protected:
	CellHandleContainer				m_foliageHandle;

	virtual void					GetTreeCollisionShapes( const CSRTBaseTree* tree, TDynArray< Sphere >& outShapes );

	RED_INLINE CTerrainAreaDescription*	GetArea() const;
public:
	CTerrainFoliageProcessingThread( CTerrainAreaDescription* area );

	Bool							PreProcessingSync() override;
	Bool							ProcessPathLibTask() override;
	IGenerationManagerBase::CAsyncTask*	PostProcessingSync() override;
	void							DescribeTask( String& task ) override;
};
#endif

};		// namespace PathLib
