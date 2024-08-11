/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once 
#ifndef _ENGINE_FOLIAGE_CELL_H_
#define _ENGINE_FOLIAGE_CELL_H_

#include "foliageForward.h"

class CFoliageResource;
class CWorldPartitionNode;

class CFoliageCell
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_Engine, __alignof( CFoliageCell ) );

public:

	CFoliageCell();
	RED_MOCKABLE ~CFoliageCell();

	RED_MOCKABLE void Tick();

	Vector2 GetWorldCoordinate() const;

	void Setup( const Vector2 & position, const IFoliageResourceLoader * loader );

	CFoliageResource * GetFoliageResource() const;
	CFoliageResource * AcquireFoliageResource();

	RED_MOCKABLE bool IsResourceValid() const;
	RED_MOCKABLE bool IsLoading() const;

	RED_MOCKABLE const String & GetPath() const;

	RED_MOCKABLE void Wait();
	
private:
	bool IsResourceLoaded() const;

	Vector2 m_position;
	const IFoliageResourceLoader * m_loader;
	FoliageResourceHandle m_resourceHandle;
	BaseSoftHandle::EAsyncLoadingResult m_loadingResult;
	int test;
};

CellHandle CreateFoliageCell( const Vector2 & postion, const IFoliageResourceLoader * loader );


#endif 
