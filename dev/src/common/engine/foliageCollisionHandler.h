/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_FOLIAGE_COLLISION_HANDLER_H_
#define _ENGINE_FOLIAGE_COLLISION_HANDLER_H_

#include "../core/uniquePtr.h"
#include "foliageResource.h"

class CWorld;
class IRender;
class CSRTBaseTree;
class CFoliageInstance;
class CPhysicsTileWrapper;

class CFoliageCollisionHandler
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CFoliageCollisionHandler();
	RED_MOCKABLE ~CFoliageCollisionHandler();

	RED_MOCKABLE void AddAllCollision( const CSRTBaseTree* baseTree, const FoliageInstanceContainer& instances, const Box& box);
	void AddCollisionsForInstance(const CSRTBaseTree* baseTree, const SFoliageInstance & newInstance);
	CPhysicsTileWrapper * GetTerrainTileFromPositionOptimized(const Vector & position, CPhysicsTileWrapper * tilesArray[4]);
	CPhysicsTileWrapper * GetTerrainTileFromPosition(CPhysicsWorld * physicsWorld, const Vector & position, CClipMap * clipMap);

	RED_MOCKABLE void RemoveAllCollision( const CSRTBaseTree* baseTree, const Box & box );

	void SetInternalWorld( CWorld * world );
	void SetInternalRender( IRender * renderer );

private:

	CWorld * m_world;
	IRender * m_renderer;
};

Red::TUniquePtr< CFoliageCollisionHandler > CreateFoliageCollisionHandler( CWorld * physicsWorld, IRender * renderer );

#endif
