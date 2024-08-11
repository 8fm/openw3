/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef WORLD_PARTITION_QUADTREE_WALKER_H_
#define WORLD_PARTITION_QUADTREE_WALKER_H_
#ifdef USE_RED_RESOURCEMANAGER

class CWorldPartitionNode;

// This interface is used to allow access to the nodes in the world partition, and modify them (add bundle files, etc)
class IWorldPartitionQuadtreeWalker
{
public:
	virtual Bool OnNode( CWorldPartitionNode& node ) = 0;		// Return false = don't process node children
};

#endif // USE_RED_RESOURCEMANAGER
#endif // WORLD_PARTITION_QUADTREE_WALKER_H_