// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxsIslandManagerAux.h"

using namespace physx;

#include "PxsRigidBody.h"
#include "PxvDynamics.h"
#include "PxsArticulation.h"
#include "CmEventProfiler.h"
#include "PxProfileEventId.h"

static PX_FORCE_INLINE void releaseIsland(const IslandType islandId, IslandManager& islands)
{
	islands.release(islandId);
	PX_ASSERT(islands.getBitmap().test(islandId));
	islands.getBitmap().reset(islandId);
}

static PX_FORCE_INLINE IslandType getNewIsland(IslandManager& islands)
{
	const IslandType newIslandId=islands.getAvailableElemNoResize();
	PX_ASSERT(!islands.getBitmap().test(newIslandId));
	islands.getBitmap().set(newIslandId);
	return newIslandId;
}

static PX_FORCE_INLINE void addEdgeToIsland
(const IslandType islandId, const EdgeType edgeId, 
 EdgeType* PX_RESTRICT nextEdgeIds, const PxU32 allEdgesCapacity, 
 IslandManager& islands)
{
	//Add the edge to the island.
	Island& island=islands.get(islandId);
	PX_ASSERT(edgeId<allEdgesCapacity);
	PX_ASSERT(edgeId!=island.mStartEdgeId);
	PX_UNUSED(allEdgesCapacity);

	nextEdgeIds[edgeId]=island.mStartEdgeId;
	island.mStartEdgeId=edgeId;
	island.mEndEdgeId=(INVALID_EDGE==island.mEndEdgeId ? edgeId : island.mEndEdgeId);
}

static PX_FORCE_INLINE void addNodeToIsland
(const IslandType islandId, const NodeType nodeId,
 Node* PX_RESTRICT allNodes, NodeType* PX_RESTRICT nextNodeIds, const PxU32 allNodesCapacity, 
 IslandManager& islands)
{
	PX_ASSERT(islands.getBitmap().test(islandId));
	PX_ASSERT(nodeId<allNodesCapacity);
	PX_ASSERT(!allNodes[nodeId].getIsDeleted());
	PX_UNUSED(allNodesCapacity);

	//Get the island and the node to be added to the island.
	Node& node=allNodes[nodeId];
	Island& island=islands.get(islandId);

	//Set the island of the node.
	node.setIslandId(islandId);

	//Connect the linked list of nodes
	PX_ASSERT(nodeId!=island.mStartNodeId);
	nextNodeIds[nodeId]=island.mStartNodeId;
	island.mStartNodeId=nodeId;
	island.mEndNodeId=(INVALID_NODE==island.mEndNodeId ? nodeId : island.mEndNodeId);
}

static PX_FORCE_INLINE void joinIslands
(const IslandType islandId1, const IslandType islandId2,
 Node* PX_RESTRICT /*allNodes*/, const PxU32 /*allNodesCapacity*/,
 Edge* PX_RESTRICT /*allEdges*/, const PxU32 /*allEdgesCapacity*/,
 NodeType* nextNodeIds, EdgeType* nextEdgeIds,
 IslandManager& islands)
{
	PX_ASSERT(islandId1!=islandId2);

	Island* PX_RESTRICT allIslands=islands.getAll();

	//Get both islands.
	PX_ASSERT(islands.getBitmap().test(islandId1));
	PX_ASSERT(islandId1<islands.getCapacity());
	Island& island1=allIslands[islandId1];
	PX_ASSERT(islands.getBitmap().test(islandId2));
	PX_ASSERT(islandId2<islands.getCapacity());
	Island& island2=allIslands[islandId2];

	//Join the list of edges together.
	//If Island2 has no edges then the list of edges is just the edges in island1 (so there is nothing to in this case).
	//If island1 has no edges then the list of edges is just the edges in island2.
	if(INVALID_EDGE==island1.mStartEdgeId)
	{
		//Island1 has no edges so the list of edges is just the edges in island 2.
		PX_ASSERT(INVALID_EDGE==island1.mEndEdgeId);
		island1.mStartEdgeId=island2.mStartEdgeId;
		island1.mEndEdgeId=island2.mEndEdgeId;
	}
	else if(INVALID_EDGE!=island2.mStartEdgeId)
	{
		//Both island1 and island2 have edges.
		PX_ASSERT(INVALID_EDGE!=island1.mStartEdgeId);
		PX_ASSERT(INVALID_EDGE!=island1.mEndEdgeId);
		PX_ASSERT(INVALID_EDGE!=island2.mStartEdgeId);
		PX_ASSERT(INVALID_EDGE!=island2.mEndEdgeId);
		PX_ASSERT(island1.mEndEdgeId!=island2.mStartEdgeId);
		nextEdgeIds[island1.mEndEdgeId]=island2.mStartEdgeId;
		island1.mEndEdgeId=island2.mEndEdgeId;
	}

	//Join the list of nodes together.
	//If Island2 has no nodes then the list of nodes is just the nodes in island1 (so there is nothing to in this case).
	//If island1 has no nodes then the list of nodes is just the nodes in island2.
	if(INVALID_NODE==island1.mStartNodeId)
	{
		//Island1 has no nodes so the list of nodes is just the nodes in island 2.
		PX_ASSERT(INVALID_NODE==island1.mEndNodeId);
		island1.mStartNodeId=island2.mStartNodeId;
		island1.mEndNodeId=island2.mEndNodeId;
	}
	else if(INVALID_NODE!=island2.mStartNodeId)
	{
		//Both island1 and island2 have nodes.
		PX_ASSERT(INVALID_NODE!=island1.mStartNodeId);
		PX_ASSERT(INVALID_NODE!=island1.mEndNodeId);
		PX_ASSERT(INVALID_NODE!=island2.mStartNodeId);
		PX_ASSERT(INVALID_NODE!=island2.mEndNodeId);
		PX_ASSERT(island2.mStartNodeId!=island1.mEndNodeId);
		nextNodeIds[island1.mEndNodeId]=island2.mStartNodeId;
		island1.mEndNodeId=island2.mEndNodeId;
	}

	//remove island 2
	releaseIsland(islandId2,islands);
}

static PX_FORCE_INLINE void setNodeIslandIdsAndJoinIslands
(const IslandType islandId1, const IslandType islandId2,
 Node* PX_RESTRICT allNodes, const PxU32 allNodesCapacity,
 Edge* PX_RESTRICT allEdges, const PxU32 allEdgesCapacity,
 NodeType* nextNodeIds, EdgeType* nextEdgeIds,
 IslandManager& islands)
{
	PX_ASSERT(islandId1!=islandId2);

	//Get island 2
	Island* PX_RESTRICT allIslands=islands.getAll();
	PX_ASSERT(islands.getBitmap().test(islandId2));
	PX_ASSERT(islandId2<islands.getCapacity());
	Island& island2=allIslands[islandId2];

	//Set all nodes in island 2 to be in island 1.
	NodeType nextNode=island2.mStartNodeId;
	while(nextNode!=INVALID_NODE)
	{
		PX_ASSERT(nextNode<allNodesCapacity);
		allNodes[nextNode].setIslandId(islandId1);
		nextNode=nextNodeIds[nextNode];
	}

	joinIslands(islandId1,islandId2,allNodes,allNodesCapacity,allEdges,allEdgesCapacity,nextNodeIds,nextEdgeIds,islands);
}

static void removeDeletedNodesFromIsland
(const IslandType islandId, NodeManager& nodeManager,  EdgeManager& /*edgeManager*/, IslandManager& islands)
{
	PX_ASSERT(islands.getBitmap().test(islandId));
	Island& island=islands.get(islandId);

	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();

	PX_ASSERT(INVALID_NODE==island.mStartNodeId || island.mStartNodeId!=nextNodeIds[island.mStartNodeId]);

	//If the start node has been deleted then keep 
	//updating until we find a start node that isn't deleted.
	{
		NodeType nextNode=island.mStartNodeId;
		while(nextNode!=INVALID_NODE && allNodes[nextNode].getIsDeleted())
		{
			const NodeType currNode=nextNode;
			nextNode=nextNodeIds[currNode];
			nextNodeIds[currNode]=INVALID_NODE;
		}
		island.mStartNodeId=nextNode;
	}

	//Remove deleted nodes from the linked list.
	{
		NodeType endNode=island.mStartNodeId;
		NodeType undeletedNode=island.mStartNodeId;
		while(undeletedNode!=INVALID_NODE)
		{
			PX_ASSERT(!allNodes[undeletedNode].getIsDeleted());

			NodeType nextNode=nextNodeIds[undeletedNode];
			while(nextNode!=INVALID_NODE && allNodes[nextNode].getIsDeleted())
			{
				const NodeType currNode=nextNode;
				nextNode=nextNodeIds[nextNode];
				nextNodeIds[currNode]=INVALID_NODE;
			}
			nextNodeIds[undeletedNode]=nextNode;
			endNode=undeletedNode;
			undeletedNode=nextNode;
		}
		island.mEndNodeId=endNode;
	}

	PX_ASSERT(INVALID_NODE==island.mStartNodeId || island.mStartNodeId!=nextNodeIds[island.mStartNodeId]);
}

static void removeDeletedNodesFromIslands2
(const IslandType* islandsToUpdate, const PxU32 numIslandsToUpdate,
 NodeManager& nodeManager,  EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& emptyIslandsBitmap)
{
	//Remove broken edges from affected islands.
	//Remove deleted nodes from affected islands.
	for(PxU32 i=0;i<numIslandsToUpdate;i++)
	{
		//Get the island.
		const IslandType islandId=islandsToUpdate[i];
		removeDeletedNodesFromIsland(islandId,nodeManager,edgeManager,islands);

		PX_ASSERT(islands.getBitmap().test(islandId));
		Island& island=islands.get(islandId);
		if(INVALID_NODE==island.mEndNodeId)
		{
			PX_ASSERT(INVALID_NODE==island.mStartNodeId);
			emptyIslandsBitmap.set(islandId);
		}
	}
}

static void removeDeletedNodesFromIslands
(const NodeType* deletedNodes, const PxU32 numDeletedNodes,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& affectedIslandsBitmap, Cm::BitMap& emptyIslandsBitmap)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();

	//Add to the list any islands that contain a deleted node.
	for(PxU32 i=0;i<numDeletedNodes;i++)
	{
		const NodeType nodeId=deletedNodes[i];
		PX_ASSERT(nodeId<nodeManager.getCapacity());
		const Node& node=allNodes[nodeId];
		//If a body was added after PxScene.simulate and before PxScene.fetchResults then 
		//the addition will be delayed and not processed until the end of fetchResults.
		//If this body is then released straight after PxScene.fetchResults then at the 
		//next PxScene.simulate we will have a body that has been both added and removed.
		//The removal must take precedence over the addition.
		if(node.getIsDeleted() && !node.getIsNew())
		{
			const IslandType islandId=node.getIslandId();
			PX_ASSERT(islandId!=INVALID_ISLAND);
			PX_ASSERT(islands.getBitmap().test(islandId));
			affectedIslandsBitmap.set(islandId);
		}
	}

#define MAX_NUM_ISLANDS_TO_UPDATE 1024

	//Gather a simple list of all islands affected by a deleted node.
	IslandType islandsToUpdate[MAX_NUM_ISLANDS_TO_UPDATE];
	PxU32 numIslandsToUpdate=0;
	const PxU32 lastSetBit = affectedIslandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = affectedIslandsBitmap.getWords()[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
			PX_ASSERT(islands.getBitmap().test(islandId));

			if(numIslandsToUpdate<MAX_NUM_ISLANDS_TO_UPDATE)
			{
				islandsToUpdate[numIslandsToUpdate]=islandId;
				numIslandsToUpdate++;
			}
			else
			{
				removeDeletedNodesFromIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands,emptyIslandsBitmap);
				islandsToUpdate[0]=islandId;
				numIslandsToUpdate=1;
			}
		}
	}

	removeDeletedNodesFromIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands,emptyIslandsBitmap);
}

static void removeBrokenEdgesFromIslands2
(const IslandType* PX_RESTRICT islandsToUpdate, const PxU32 numIslandsToUpdate,
 NodeManager& /*nodeManager*/, EdgeManager& edgeManager, IslandManager& islands)
{
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	//Remove broken edges from affected islands.
	//Remove deleted nodes from affected islands.
	for(PxU32 i=0;i<numIslandsToUpdate;i++)
	{
		//Get the island.
		const IslandType islandId=islandsToUpdate[i];
		PX_ASSERT(islands.getBitmap().test(islandId));
		Island& island=islands.get(islandId);

		PX_ASSERT(INVALID_EDGE==island.mStartEdgeId || island.mStartEdgeId!=nextEdgeIds[island.mStartEdgeId]);

		//If the start edge has been deleted then keep 
		//updating until we find a start edge that isn't deleted.
		{
			EdgeType nextEdge=island.mStartEdgeId;
			while(nextEdge!=INVALID_EDGE && !allEdges[nextEdge].getIsConnected())
			{
				const EdgeType currEdge=nextEdge;
				nextEdge=nextEdgeIds[currEdge];
				nextEdgeIds[currEdge]=INVALID_EDGE;
			}
			island.mStartEdgeId=nextEdge;
		}

		//Remove broken or deleted edges from the linked list.
		{
			EdgeType endEdge=island.mStartEdgeId;
			EdgeType undeletedEdge=island.mStartEdgeId;
			while(undeletedEdge!=INVALID_EDGE)
			{
				PX_ASSERT(allEdges[undeletedEdge].getIsConnected());

				EdgeType nextEdge=nextEdgeIds[undeletedEdge];
				while(nextEdge!=INVALID_EDGE && !allEdges[nextEdge].getIsConnected())
				{
					const EdgeType currEdge=nextEdge;
					nextEdge=nextEdgeIds[nextEdge];
					nextEdgeIds[currEdge]=INVALID_EDGE;
				}
				nextEdgeIds[undeletedEdge]=nextEdge;
				endEdge=undeletedEdge;
				undeletedEdge=nextEdge;
			}
			island.mEndEdgeId=endEdge;
		}

		PX_ASSERT(INVALID_EDGE==island.mStartEdgeId || island.mStartEdgeId!=nextEdgeIds[island.mStartEdgeId]);
	}
}

static void removeBrokenEdgesFromIslands
(const EdgeType* PX_RESTRICT brokenEdgeIds, const PxU32 numBrokenEdges, const EdgeType* PX_RESTRICT deletedEdgeIds, const PxU32 numDeletedEdges, 
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& brokenEdgeIslandsBitmap)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	//const PxU32 allNodesCapacity=nodeManager.getCapacity();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	//const PxU32 allEdgesCapacity=edgeManager.getCapacity();

	//Gather a list of islands that contain a broken edge.
	for(PxU32 i=0;i<numBrokenEdges;i++)
	{
		//Get the edge that has just been broken and add it to the bitmap.
		const EdgeType edgeId=brokenEdgeIds[i];
		PX_ASSERT(edgeId<edgeManager.getCapacity());
		const Edge& edge=allEdges[edgeId];

		//Check that the edge is legal.
		PX_ASSERT(!edge.getIsConnected());

		//Get the two nodes of the edge.
		//Get the two islands of the edge and add them to the bitmap.
		const NodeType nodeId1=edge.getNode1();
		const NodeType nodeId2=edge.getNode2();
		IslandType islandId1=INVALID_ISLAND;
		IslandType islandId2=INVALID_ISLAND;
		if(INVALID_NODE!=nodeId1)
		{
			PX_ASSERT(nodeId1<nodeManager.getCapacity());
			islandId1=allNodes[nodeId1].getIslandId();
			if(INVALID_ISLAND!=islandId1)
			{
				brokenEdgeIslandsBitmap.set(islandId1);
			}
		}
		if(INVALID_NODE!=nodeId2)
		{
			PX_ASSERT(nodeId2<nodeManager.getCapacity());
			islandId2=allNodes[nodeId2].getIslandId();
			if(INVALID_ISLAND!=islandId2)
			{
				brokenEdgeIslandsBitmap.set(islandId2);
			}
		}
	}

	for(PxU32 i=0;i<numDeletedEdges;i++)
	{
		//Get the edge that has just been broken and add it to the bitmap.
		const EdgeType edgeId=deletedEdgeIds[i];
		PX_ASSERT(edgeId<edgeManager.getCapacity());
		Edge& edge=allEdges[edgeId];

		//If the edge was connected and was then deleted without having being disconnected
		//then we have to handle this as a broken edge.
		//The edge still has to be released so it can be reused.  This will be done at a later stage.
		if(edge.getIsConnected())
		{
			edge.setUnconnected();

			//Check that the edge is legal.
			PX_ASSERT(!edge.getIsConnected());

			//Get the two nodes of the edge.
			//Get the two islands of the edge and add them to the bitmap.
			const NodeType nodeId1=edge.getNode1();
			const NodeType nodeId2=edge.getNode2();
			IslandType islandId1=INVALID_ISLAND;
			IslandType islandId2=INVALID_ISLAND;
			if(INVALID_NODE!=nodeId1)
			{
				PX_ASSERT(nodeId1<nodeManager.getCapacity());
				islandId1=allNodes[nodeId1].getIslandId();
				if(INVALID_ISLAND!=islandId1)
				{
					brokenEdgeIslandsBitmap.set(islandId1);
				}
			}
			if(INVALID_NODE!=nodeId2)
			{
				PX_ASSERT(nodeId2<nodeManager.getCapacity());
				islandId2=allNodes[nodeId2].getIslandId();
				if(INVALID_ISLAND!=islandId2)
				{
					brokenEdgeIslandsBitmap.set(islandId2);
				}
			}
		}
	}

#define MAX_NUM_ISLANDS_TO_UPDATE 1024

	//Gather a simple list of all islands affected by broken edge or deleted node.
	IslandType islandsToUpdate[MAX_NUM_ISLANDS_TO_UPDATE];
	PxU32 numIslandsToUpdate=0;
	const PxU32 lastSetBit = brokenEdgeIslandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = brokenEdgeIslandsBitmap.getWords()[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
			PX_ASSERT(islandId!=INVALID_ISLAND);
			PX_ASSERT(islands.getBitmap().test(islandId));

			if(numIslandsToUpdate<MAX_NUM_ISLANDS_TO_UPDATE)
			{
				islandsToUpdate[numIslandsToUpdate]=islandId;
				numIslandsToUpdate++;
			}
			else
			{
				removeBrokenEdgesFromIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands);
				islandsToUpdate[0]=islandId;
				numIslandsToUpdate=1;
			}
		}
	}

	removeBrokenEdgesFromIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands);
}

static void releaseEmptyIslands(const Cm::BitMap& emptyIslandsBitmap, IslandManager& islands, Cm::BitMap& brokenEdgeIslandsBitmap)
{
	const PxU32* PX_RESTRICT emptyIslandsBitmapWords=emptyIslandsBitmap.getWords();
	const PxU32 lastSetBit = emptyIslandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = emptyIslandsBitmapWords[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
			PX_ASSERT(islands.getBitmap().test(islandId));
			PX_ASSERT(INVALID_NODE==islands.get(islandId).mStartNodeId);
			PX_ASSERT(INVALID_NODE==islands.get(islandId).mEndNodeId);
			PX_ASSERT(INVALID_EDGE==islands.get(islandId).mStartEdgeId);
			PX_ASSERT(INVALID_EDGE==islands.get(islandId).mEndEdgeId);
			releaseIsland(islandId,islands);
			brokenEdgeIslandsBitmap.reset(islandId);
			PX_ASSERT(!islands.getBitmap().test(islandId));
		}
	}
}

#if 0

static void processJoinedEdge
(const EdgeType edgeId, 
 Node* PX_RESTRICT allNodes, const PxU32 allNodesCapacity,
 Edge* PX_RESTRICT allEdges, const PxU32 allEdgesCapacity,
 NodeType* PX_RESTRICT nextNodeIds, EdgeType* PX_RESTRICT nextEdgeIds,
 IslandManager& islands,
 Cm::BitMap& brokenEdgeIslandsBitmap)
{
	PX_ASSERT(edgeId<allEdgesCapacity);
	Edge& edge=allEdges[edgeId];

	//Check the edge is legal.
	PX_ASSERT(edge.getIsConnected());
	PX_ASSERT(!edge.getIsRemoved());
	PX_ASSERT(edge.getNode1()!=INVALID_NODE || edge.getNode2()!=INVALID_NODE);

	//Get the two nodes of the edge.
	//Get the two islands of the edge.
	const NodeType nodeId1=edge.getNode1();
	const NodeType nodeId2=edge.getNode2();
	IslandType islandId1=INVALID_ISLAND;
	IslandType islandId2=INVALID_ISLAND;
	if(INVALID_NODE!=nodeId1)
	{
		PX_ASSERT(nodeId1<allNodesCapacity);
		islandId1=allNodes[nodeId1].getIslandId();
	}
	if(INVALID_NODE!=nodeId2)
	{
		PX_ASSERT(nodeId2<allNodesCapacity);
		islandId2=allNodes[nodeId2].getIslandId();
	}

	if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND!=islandId2)
	{
		addEdgeToIsland(islandId1,edgeId,nextEdgeIds,allEdgesCapacity,islands);
		if(islandId1!=islandId2)
		{
			PX_ASSERT(islands.getBitmap().test(islandId1));
			PX_ASSERT(islands.getBitmap().test(islandId2));
			setNodeIslandIdsAndJoinIslands(islandId1,islandId2,allNodes,allNodesCapacity,allEdges,allEdgesCapacity,nextNodeIds,nextEdgeIds,islands);
			PX_ASSERT(islands.getBitmap().test(islandId1));
			PX_ASSERT(!islands.getBitmap().test(islandId2));
			if(brokenEdgeIslandsBitmap.test(islandId2))
			{
				brokenEdgeIslandsBitmap.set(islandId1);
				brokenEdgeIslandsBitmap.reset(islandId2);
			}
		}
	}
	else if(INVALID_ISLAND==islandId1 && INVALID_ISLAND!=islandId2)
	{
		if(INVALID_NODE!=nodeId1)
		{
			addNodeToIsland(islandId2,nodeId1,allNodes,nextNodeIds,allNodesCapacity,islands);
		}
		addEdgeToIsland(islandId2,edgeId,nextEdgeIds,allEdgesCapacity,islands);
	}
	else if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND==islandId2)
	{
		if(INVALID_NODE!=nodeId2)
		{
			addNodeToIsland(islandId1,nodeId2,allNodes,nextNodeIds,allNodesCapacity,islands);
		}
		addEdgeToIsland(islandId1,edgeId,nextEdgeIds,allEdgesCapacity,islands);
	}
	else
	{
		//Make a new island.
		const IslandType newIslandId=getNewIsland(islands);
		PX_ASSERT(islands.getCapacity()<=allNodesCapacity);

		//Add the edge to the island.
		addEdgeToIsland(newIslandId,edgeId,nextEdgeIds,allEdgesCapacity,islands);

		//Add the nodes to the island.
		if(INVALID_NODE!=nodeId1)
		{
			addNodeToIsland(newIslandId,nodeId1,allNodes,nextNodeIds,allNodesCapacity,islands);
		}
		if(INVALID_NODE!=nodeId2)
		{
			addNodeToIsland(newIslandId,nodeId2,allNodes,nextNodeIds,allNodesCapacity,islands);
		}
	}
}

static void processJoinedEdges
(const EdgeType* PX_RESTRICT joinedEdges, const PxU32 numJoinedEdges, 
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& brokenEdgeIslandsBitmap,
 Cm::BitMap& affectedIslandsBitmap,
 NodeType* PX_RESTRICT graphNextNodes, IslandType* PX_RESTRICT graphStartIslands, IslandType* PX_RESTRICT graphNextIslands)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	const PxU32 allEdgesCapacity=edgeManager.getCapacity();

	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	for(PxU32 i=0;i<numJoinedEdges;i++)
	{
		//Get the edge that has just been connected.
		const EdgeType edgeId=joinedEdges[i];
		PX_ASSERT(edgeId<allEdgesCapacity);
		const Edge& edge=allEdges[edgeId];

		//How can an edge be in the list of joined edges but not be joined?
		//1.  It could have been joined and removed.
		//2.  It could have been reported as joined in ccd (that runs post-solver)
		//	  and then reported as unconnected in the next frame after narrowphase
		//    but before the island manager update.  That would mean island manager 
		//    would have accumulated a touch lost and touch found event.  We can 
		//    just filter this out.
		if(!edge.getIsRemoved() && edge.getIsConnected())
		{
			processJoinedEdge(edgeId,allNodes,allNodesCapacity,allEdges,allEdgesCapacity,nextNodeIds,nextEdgeIds,islands,brokenEdgeIslandsBitmap);
		}
	}
}

#else

static void processJoinedEdges
(const EdgeType* PX_RESTRICT joinedEdges, const PxU32 numJoinedEdges, 
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& brokenEdgeIslandsBitmap, 
 Cm::BitMap& affectedIslandsBitmap,
 NodeType* PX_RESTRICT graphNextNodes, IslandType* PX_RESTRICT graphStartIslands, IslandType* PX_RESTRICT graphNextIslands)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	const PxU32 allEdgesCapacity=edgeManager.getCapacity();

	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	PxMemSet(graphNextNodes, 0xff, sizeof(IslandType)*islands.getCapacity());
	PxMemSet(graphStartIslands, 0xff, sizeof(IslandType)*islands.getCapacity());
	PxMemSet(graphNextIslands, 0xff, sizeof(IslandType)*islands.getCapacity());

	//Add new nodes in a joined edge to an island on their own.
	//Record the bitmap of islands with nodes affected by joined edges.
	for(PxU32 i=0;i<numJoinedEdges;i++)
	{
		//Get the edge that has just been connected.
		const EdgeType edgeId=joinedEdges[i];
		PX_ASSERT(edgeId<allEdgesCapacity);
		const Edge& edge=allEdges[edgeId];

		//How can an edge be in the list of joined edges but not be joined?
		//1.  It could have been joined and removed.
		//2.  It could have been reported as joined in ccd (that runs post-solver)
		//	  and then reported as unconnected in the next frame after narrowphase
		//    but before the island manager update.  That would mean island manager 
		//    would have accumulated a touch lost and touch found event.  We can 
		//    just filter this out.
		if(!edge.getIsRemoved() && edge.getIsConnected())
		{
			const NodeType nodeId1=edge.getNode1();
			if(INVALID_NODE!=nodeId1)
			{
				Node&  node=allNodes[nodeId1];
				IslandType islandId=node.getIslandId();
				if(INVALID_ISLAND!=islandId)
				{
					affectedIslandsBitmap.set(islandId);
				}
				else
				{
					PX_ASSERT(node.getIsNew());
					islandId=getNewIsland(islands);
					addNodeToIsland(islandId,nodeId1,allNodes,nextNodeIds,allNodesCapacity,islands);
					affectedIslandsBitmap.set(islandId);
				}
			}

			const NodeType nodeId2=edge.getNode2();
			if(INVALID_NODE!=nodeId2)
			{
				Node&  node=allNodes[nodeId2];
				IslandType islandId=node.getIslandId();
				if(INVALID_ISLAND!=islandId)
				{
					affectedIslandsBitmap.set(islandId);
				}
				else
				{
					PX_ASSERT(node.getIsNew());
					islandId=getNewIsland(islands);
					addNodeToIsland(islandId,nodeId2,allNodes,nextNodeIds,allNodesCapacity,islands);
					affectedIslandsBitmap.set(islandId);
				}
			}
		}
	}

	//Iterate over all islands affected by a node in a joined edge.
	//Record all affected nodes and the start island of all affected nodes. 
	NodeType startNode=INVALID_NODE;
	IslandType prevIslandId=INVALID_ISLAND;
	const PxU32 lastSetBit = affectedIslandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = affectedIslandsBitmap.getWords()[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
			PX_ASSERT(islandId<islands.getCapacity());
			PX_ASSERT(islands.getBitmap().test(islandId));
			const Island& island=islands.get(islandId);

			NodeType nextNode=island.mStartNodeId;
			PX_ASSERT(INVALID_NODE!=nextNode);
			if(INVALID_NODE!=prevIslandId)
			{
				const Island& prevIsland=islands.get(prevIslandId);
				graphNextNodes[prevIsland.mEndNodeId]=island.mStartNodeId;
			}
			else
			{
				startNode=nextNode;
			}
			prevIslandId=islandId;

			while(INVALID_NODE!=nextNode)
			{
				const Node& node=allNodes[nextNode];
				const IslandType islandId=node.getIslandId();
				PX_ASSERT(islandId!=INVALID_ISLAND);
				graphNextNodes[nextNode]=nextNodeIds[nextNode];
				graphStartIslands[nextNode]=islandId;
				graphNextIslands[islandId]=INVALID_ISLAND;
				nextNode=nextNodeIds[nextNode];
			}
		}
	}

	//Join all the edges by merging islands.
	for(PxU32 i=0;i<numJoinedEdges;i++)
	{
		//Get the edge that has just been connected.
		const EdgeType edgeId=joinedEdges[i];
		PX_ASSERT(edgeId<allEdgesCapacity);
		const Edge& edge=allEdges[edgeId];

		//How can an edge be in the list of joined edges but not be joined?
		//1.  It could have been joined and removed.
		//2.  It could have been reported as joined in ccd (that runs post-solver)
		//	  and then reported as unconnected in the next frame after narrowphase
		//    but before the island manager update.  That would mean island manager 
		//    would have accumulated a touch lost and touch found event.  We can 
		//    just filter this out.
		if(!edge.getIsRemoved() && edge.getIsConnected())
		{
			//Get the two islands of the edge nodes.

			IslandType islandId1=INVALID_ISLAND;
			{
				const NodeType nodeId1=edge.getNode1();
				if(INVALID_NODE!=nodeId1)
				{
					PX_ASSERT(nodeId1<allNodesCapacity);
					IslandType nextIsland=graphStartIslands[nodeId1];
					while(INVALID_ISLAND!=nextIsland)
					{
						islandId1=nextIsland;
						nextIsland=graphNextIslands[nextIsland];
					}
				}
			}

			IslandType islandId2=INVALID_ISLAND;
			{
				const NodeType nodeId2=edge.getNode2();
				if(INVALID_NODE!=nodeId2)
				{
					PX_ASSERT(nodeId2<allNodesCapacity);
					IslandType nextIsland=graphStartIslands[nodeId2];
					while(INVALID_ISLAND!=nextIsland)
					{
						islandId2=nextIsland;
						nextIsland=graphNextIslands[nextIsland];
					}
				}
			}

			if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND!=islandId2)
			{
				addEdgeToIsland(islandId1,edgeId,nextEdgeIds,allEdgesCapacity,islands);
				if(islandId1!=islandId2)
				{
					graphNextIslands[islandId2]=islandId1;

					PX_ASSERT(islands.getBitmap().test(islandId1));
					PX_ASSERT(islands.getBitmap().test(islandId2));
					joinIslands(islandId1,islandId2,allNodes,allNodesCapacity,allEdges,allEdgesCapacity,nextNodeIds,nextEdgeIds,islands);
					PX_ASSERT(islands.getBitmap().test(islandId1));
					PX_ASSERT(!islands.getBitmap().test(islandId2));
					if(brokenEdgeIslandsBitmap.test(islandId2))
					{
						brokenEdgeIslandsBitmap.set(islandId1);
						brokenEdgeIslandsBitmap.reset(islandId2);
					}
				}
			}
			else if(INVALID_ISLAND==islandId1 && INVALID_ISLAND!=islandId2)
			{
				addEdgeToIsland(islandId2,edgeId,nextEdgeIds,allEdgesCapacity,islands);
			}
			else if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND==islandId2)
			{
				addEdgeToIsland(islandId1,edgeId,nextEdgeIds,allEdgesCapacity,islands);
			}
			else
			{
				PX_ASSERT(false);
			}
		}
	}

	//Set the island of all nodes in an island affected by a joined edge.
	NodeType nextNode=startNode;
	while(INVALID_NODE!=nextNode)
	{
		IslandType islandId=INVALID_ISLAND;
		IslandType nextIsland=graphStartIslands[nextNode];
		while(INVALID_ISLAND!=nextIsland)
		{
			islandId=nextIsland;
			nextIsland=graphNextIslands[nextIsland];
		}
		Node& node=allNodes[nextNode];
		node.setIslandId(islandId);
		nextNode=graphNextNodes[nextNode];
	}
}

#endif


PX_FORCE_INLINE PxU32 alignSize16(const PxU32 size)
{
	return ((size + 15) & ~15);
}

static void duplicateKinematicNodes
(const Cm::BitMap& kinematicNodesBitmap,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 NodeType* kinematicProxySourceNodes, NodeType* kinematicProxyNextNodes,
 Cm::BitMap& kinematicIslandsBitmap)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();

	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	PxMemSet(kinematicProxySourceNodes, 0xff, sizeof(NodeType)*allNodesCapacity);
	PxMemSet(kinematicProxyNextNodes, 0xff, sizeof(NodeType)*allNodesCapacity);

	//Compute all the islands that need updated.
	//Set all kinematic nodes as deleted so they can be identified as needing removed from their islands.
	{
		const PxU32* PX_RESTRICT kinematicNodesBitmapWords=kinematicNodesBitmap.getWords();
		const PxU32 lastSetBit = kinematicNodesBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicNodesBitmapWords[w]; b; b &= b-1)
			{
				const NodeType kinematicNodeId = (NodeType)(w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(kinematicNodeId<allNodesCapacity);
				Node& node=allNodes[kinematicNodeId];
				PX_ASSERT(node.getIsKinematic());

				//Get the island of the kinematic node and mark it as needing updated.
				const IslandType islandId=node.getIslandId();
				PX_ASSERT(INVALID_ISLAND!=islandId);
				PX_ASSERT(islands.getBitmap().test(islandId));
				kinematicIslandsBitmap.set(islandId);

				//Set the node as deleted so it can be identified as needing to be removed.
				node.setIsDeleted();
			}
		}
	}

	//Remove all kinematics nodes from the islands.
	//Replace kinematics with proxies from each edge referencing each kinematic.
	{
		const PxU32 lastSetBit = kinematicIslandsBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicIslandsBitmap.getWords()[w]; b; b &= b-1)
			{
				const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(islandId<islands.getCapacity());

				//Get the island.
				Island& island=islands.get(islandId);

				//Remove all kinematics from the island so that we can replace them with proxy nodes.
				removeDeletedNodesFromIsland(islandId,nodeManager,edgeManager,islands);
				
				//Create a proxy kinematic node for each edge that references a kinematic.
				EdgeType nextEdge=island.mStartEdgeId;
				while(nextEdge!=INVALID_EDGE)
				{
					//Get the current edge.
					PX_ASSERT(nextEdge<edgeManager.getCapacity());
					Edge& edge=allEdges[nextEdge];

					//Check the edge is legal.
					PX_ASSERT(edge.getIsConnected());
					PX_ASSERT(!edge.getIsRemoved());
					PX_ASSERT(edge.getNode1()!=INVALID_NODE || edge.getNode2()!=INVALID_NODE);

					//Add a proxy node for node1 if it is a kinematic.
					const NodeType nodeId1=edge.getNode1();
					if(INVALID_NODE!=nodeId1)
					{
						PX_ASSERT(nodeId1<allNodesCapacity);
						Node& node1=allNodes[nodeId1];
						if(node1.getIsKinematic())
						{
							//Add a proxy node.
							const NodeType proxyNodeId=nodeManager.getAvailableElemNoResize();
							kinematicProxySourceNodes[proxyNodeId]=nodeId1;

							//Add it to the list of proxies for the source kinematic.
							//The following is a bit of a compromise because we could save ourselves
							//iterating over the list in order to add the end of the list if we also stored either 
							//an array kinematicProxyStartNodes or an array kinematicProxyEndNodes.
							//The assumption is that the list will be small so that the memory saving is 
							//more of a gain than the cost of iterating through the list.
							NodeType mapLast=nodeId1;
							NodeType mapNext=kinematicProxyNextNodes[nodeId1];
							while(mapNext!=INVALID_NODE)
							{
								mapLast=mapNext;
								mapNext=kinematicProxyNextNodes[mapNext];
							}
							kinematicProxyNextNodes[mapLast]=proxyNodeId;
							kinematicProxyNextNodes[proxyNodeId]=INVALID_NODE;

							//Set up the proxy node.
							//Don't copy the deleted flag across because that was artificially added to help remove the node from the island.
							Node& proxyNode=allNodes[proxyNodeId];
							proxyNode.setRigidBodyOwner(node1.getRigidBodyOwner());
							PX_ASSERT(node1.getIsDeleted());
							proxyNode.setFlags(node1.getFlags() & ~Node::eDELETED);
							PX_ASSERT(!proxyNode.getIsDeleted());

							//Add the proxy to the island.
							addNodeToIsland(islandId,proxyNodeId,allNodes,nextNodeIds,allNodesCapacity,islands);
							PX_ASSERT(proxyNode.getIslandId()==islandId);

							//Set the edge to reference the proxy.
							edge.setNode1(proxyNodeId);
						}
					}

					//Add a proxy node for node2 if it is a kinematic.
					const NodeType nodeId2=edge.getNode2();
					if(INVALID_NODE!=nodeId2)
					{
						PX_ASSERT(nodeId2<allNodesCapacity);
						Node& node2=allNodes[nodeId2];
						if(node2.getIsKinematic())
						{
							//Add a proxy node.
							const NodeType proxyNodeId=nodeManager.getAvailableElemNoResize();
							kinematicProxySourceNodes[proxyNodeId]=nodeId2;

							//Add it to the list of proxies for the source kinematic.
							//The following is a bit of a compromise because we could save ourselves
							//iterating over the list in order to add the end of the list if we also stored either 
							//an array kinematicProxyStartNodes or an array kinematicProxyEndNodes.
							//The assumption is that the list will be small so that the memory saving is 
							//more of a gain than the cost of iterating through the list.
							NodeType mapLast=nodeId2;
							NodeType mapNext=kinematicProxyNextNodes[nodeId2];
							while(mapNext!=INVALID_NODE)
							{
								mapLast=mapNext;
								mapNext=kinematicProxyNextNodes[mapNext];
							}
							kinematicProxyNextNodes[mapLast]=proxyNodeId;
							kinematicProxyNextNodes[proxyNodeId]=INVALID_NODE;

							//Set up the proxy node.
							//Don't copy the deleted flag across because that was artificially added to help remove the node from the island.
							Node& proxyNode=allNodes[proxyNodeId];
							proxyNode.setRigidBodyOwner(node2.getRigidBodyOwner());
							PX_ASSERT(node2.getIsDeleted());
							proxyNode.setFlags(node2.getFlags() & ~Node::eDELETED);
							PX_ASSERT(!proxyNode.getIsDeleted());

							//Add the proxy to the island.
							addNodeToIsland(islandId,proxyNodeId,allNodes,nextNodeIds,allNodesCapacity,islands);
							PX_ASSERT(proxyNode.getIslandId()==islandId);

							//Set the edge to reference the proxy.
							edge.setNode2(proxyNodeId);
						}
					}
					
					nextEdge=nextEdgeIds[nextEdge];
				}
			}
		}
	}

	//Kinematics were flagged as deleted to identify them as needing removed from the island
	//so we nos need to clear the kinematic flag.  Take care that if a kinematic was referenced by 
	//no edges then it will have been removed flagged as deleted and then removed above but not 
	//replaced by anything.  Just put lone kinematics back in their original island.
	{
		const PxU32* PX_RESTRICT kinematicNodesBitmapWords=kinematicNodesBitmap.getWords();
		const PxU32 lastSetBit = kinematicNodesBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicNodesBitmapWords[w]; b; b &= b-1)
			{
				const NodeType kinematicNodeId = (NodeType)(w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(kinematicNodeId<allNodesCapacity);
				Node& node=allNodes[kinematicNodeId];
				PX_ASSERT(node.getIsKinematic());
				PX_ASSERT(node.getIsDeleted());
				PX_ASSERT(node.getIslandId()!=INVALID_ISLAND);
				PX_ASSERT(node.getIslandId()<islands.getCapacity());
				node.clearIsDeleted();

				//If the kinematic has no edges referencing it put it back in its own island.
				//If the kinematic has edges referencing it makes sense to flag the kinematic as having been replaced with proxies.
				if(INVALID_NODE==kinematicProxyNextNodes[kinematicNodeId])
				{
					const IslandType islandId=node.getIslandId();
					PX_ASSERT(INVALID_ISLAND!=islandId);
					addNodeToIsland(islandId,kinematicNodeId,allNodes,nextNodeIds,allNodesCapacity,islands);
				}
				else
				{
					//The node has been replaced with proxies so we can reset the node to neutral for now.
					node.setIslandId(INVALID_ISLAND);
				}
			}
		}
	}
}

static void processBrokenEdgeIslands2
(const IslandType* islandsToUpdate, const PxU32 numIslandsToUpdate,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 NodeType* graphNextNodes, IslandType* graphStartIslands, IslandType* graphNextIslands)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	const PxU32 allEdgesCapacity=edgeManager.getCapacity();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	PxMemSet(graphNextNodes, 0xff, sizeof(IslandType)*islands.getCapacity());
	PxMemSet(graphStartIslands, 0xff, sizeof(IslandType)*islands.getCapacity());
	PxMemSet(graphNextIslands, 0xff, sizeof(IslandType)*islands.getCapacity());

	for(PxU32 i=0;i<numIslandsToUpdate;i++)
	{
		//Get the island.
		const IslandType islandId=islandsToUpdate[i];
		PX_ASSERT(islands.getBitmap().test(islandId));
		const Island island=islands.get(islandId);
		const NodeType startNode=island.mStartNodeId;
		const EdgeType startEdge=island.mStartEdgeId;
		releaseIsland(islandId,islands);

		//Create a dummy island for each node with a single node per island.
		NodeType nextNode=startNode;
		while(nextNode!=INVALID_NODE)
		{
			const IslandType newIslandId=getNewIsland(islands);
			graphNextNodes[nextNode]=nextNodeIds[nextNode];
			graphStartIslands[nextNode]=newIslandId;
			graphNextIslands[newIslandId]=INVALID_ISLAND;
			nextNode=nextNodeIds[nextNode];
		}

		//Add all the edges.
		EdgeType nextEdge=startEdge;
		while(nextEdge!=INVALID_EDGE)
		{
			const EdgeType currEdge=nextEdge;
			nextEdge=nextEdgeIds[nextEdge];

			//Get the current edge.
			PX_ASSERT(currEdge<allEdgesCapacity);
			Edge& edge=allEdges[currEdge];

			//Check the edge is legal.
			PX_ASSERT(edge.getIsConnected());
			PX_ASSERT(!edge.getIsRemoved());
			PX_ASSERT(edge.getNode1()!=INVALID_NODE || edge.getNode2()!=INVALID_NODE);

			//Get the two nodes of the edge.
			//Get the two islands of the edge.
			const NodeType nodeId1=edge.getNode1();
			const NodeType nodeId2=edge.getNode2();
			IslandType islandId1=INVALID_ISLAND;
			IslandType islandId2=INVALID_ISLAND;
			PxU32 depth1=0;
			PxU32 depth2=0;
			if(INVALID_NODE!=nodeId1)
			{
				PX_ASSERT(nodeId1<allNodesCapacity);
				IslandType nextIsland=graphStartIslands[nodeId1];
				PX_ASSERT(nextIsland!=INVALID_ISLAND);
				while(nextIsland!=INVALID_ISLAND)
				{
					islandId1=nextIsland;
					nextIsland=graphNextIslands[nextIsland];
					depth1++;
				}
			}
			if(INVALID_NODE!=nodeId2)
			{
				PX_ASSERT(nodeId2<allNodesCapacity);
				IslandType nextIsland=graphStartIslands[nodeId2];
				PX_ASSERT(nextIsland!=INVALID_ISLAND);
				while(nextIsland!=INVALID_ISLAND)
				{
					islandId2=nextIsland;
					nextIsland=graphNextIslands[nextIsland];
					depth2++;
				}
			}

			//Set island2 to be joined to island 1.
			if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND!=islandId2)
			{
				if(islandId1!=islandId2)
				{
					if(depth1<depth2)
					{
						graphNextIslands[islandId1]=islandId2;
					}
					else
					{
						graphNextIslands[islandId2]=islandId1;
					}
				}
			}
			else if(INVALID_ISLAND==islandId1 && INVALID_ISLAND!=islandId2)
			{
				//Node2 is already in island 2.
				//Nothing to do.
			}
			else if(INVALID_ISLAND!=islandId1 && INVALID_ISLAND==islandId2)
			{
				//Node1 is already in island 1.
				//Nothing to do.
			}
			else
			{
				PX_ASSERT(false);
			}
		}

		//Go over all the nodes and add them to their islands.
		nextNode=startNode;
		while(nextNode!=INVALID_NODE)
		{
			IslandType islandId=graphStartIslands[nextNode];
			IslandType nextIsland=graphStartIslands[nextNode];
			PX_ASSERT(nextIsland!=INVALID_ISLAND);
			while(nextIsland!=INVALID_ISLAND)
			{
				islandId=nextIsland;
				nextIsland=graphNextIslands[nextIsland];
			}

			//Add the node to the island.
			//Island& island=islands.get(islandId);
			addNodeToIsland(islandId,nextNode,allNodes,nextNodeIds,allNodesCapacity,islands);

			//Next node.
			nextNode=graphNextNodes[nextNode];
		}

		//Release all empty islands.
		nextNode=startNode;
		while(nextNode!=INVALID_NODE)
		{
			const IslandType islandId=graphStartIslands[nextNode];
			Island& island=islands.get(islandId);
			if(INVALID_NODE==island.mStartNodeId)
			{
				PX_ASSERT(INVALID_NODE==island.mStartNodeId);
				PX_ASSERT(INVALID_NODE==island.mEndNodeId);
				PX_ASSERT(INVALID_EDGE==island.mStartEdgeId);
				PX_ASSERT(INVALID_EDGE==island.mEndEdgeId);
				releaseIsland(islandId,islands);
			}
			//Next node.
			nextNode=graphNextNodes[nextNode];
		}

		//Now add all the edges to the islands.
		nextEdge=island.mStartEdgeId;
		while(nextEdge!=INVALID_EDGE)
		{
			const EdgeType currEdge=nextEdge;
			nextEdge=nextEdgeIds[nextEdge];

			//Get the current edge.
			PX_ASSERT(currEdge<allEdgesCapacity);
			Edge& edge=allEdges[currEdge];

			//Check the edge is legal.
			PX_ASSERT(edge.getIsConnected());
			PX_ASSERT(!edge.getIsRemoved());
			PX_ASSERT(edge.getNode1()!=INVALID_NODE || edge.getNode2()!=INVALID_NODE);

			//Get the two nodes of the edge.
			//Get the two islands of the edge.
			const NodeType nodeId1=edge.getNode1();
			const NodeType nodeId2=edge.getNode2();
			PX_ASSERT(INVALID_NODE!=nodeId1 || INVALID_NODE!=nodeId2);
			if(INVALID_NODE!=nodeId1)
			{
				PX_ASSERT(nodeId1<allNodesCapacity);
				const IslandType islandId1=allNodes[nodeId1].getIslandId();
				PX_ASSERT(INVALID_ISLAND!=islandId1);
				PX_ASSERT(INVALID_NODE==nodeId2 || allNodes[nodeId2].getIslandId()==islandId1);
				addEdgeToIsland(islandId1,currEdge,nextEdgeIds,allEdgesCapacity,islands);
			}
			else if(INVALID_NODE!=nodeId2)
			{
				PX_ASSERT(nodeId2<allNodesCapacity);
				const IslandType islandId2=allNodes[nodeId2].getIslandId();
				PX_ASSERT(INVALID_ISLAND!=islandId2);
				PX_ASSERT(INVALID_NODE==nodeId1 || allNodes[nodeId1].getIslandId()==islandId2);
				addEdgeToIsland(islandId2,currEdge,nextEdgeIds,allEdgesCapacity,islands);
			}
		}
	}
}

static void processBrokenEdgeIslands
(const Cm::BitMap& brokenEdgeIslandsBitmap,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 NodeType* graphNextNodes, IslandType* graphStartIslands, IslandType* graphNextIslands)
{
#define MAX_NUM_ISLANDS_TO_UPDATE 1024

	//Gather a simple list of all islands affected by broken edge or deleted node.
	IslandType islandsToUpdate[MAX_NUM_ISLANDS_TO_UPDATE];
	PxU32 numIslandsToUpdate=0;
	const PxU32 lastSetBit = brokenEdgeIslandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = brokenEdgeIslandsBitmap.getWords()[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));

			if(islands.getBitmap().test(islandId))
			{
				if(numIslandsToUpdate<MAX_NUM_ISLANDS_TO_UPDATE)
				{
					islandsToUpdate[numIslandsToUpdate]=islandId;
					numIslandsToUpdate++;
				}
				else
				{
					processBrokenEdgeIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands,graphNextNodes,graphStartIslands,graphNextIslands);
					islandsToUpdate[0]=islandId;
					numIslandsToUpdate=1;
				}
			}
		}
	}

	processBrokenEdgeIslands2(islandsToUpdate,numIslandsToUpdate,nodeManager,edgeManager,islands,graphNextNodes,graphStartIslands,graphNextIslands);
}

static void releaseDeletedEdges(const EdgeType* PX_RESTRICT deletedEdges, const PxU32 numDeletedEdges, EdgeManager& edgeManager)
{
	//Now release the deleted edges.
	for(PxU32 i=0;i<numDeletedEdges;i++)
	{
		//Get the deleted edge.
		const EdgeType edgeId=deletedEdges[i];

		//Test the edge is legal.
		PX_ASSERT(edgeManager.get(edgeId).getIsRemoved());

		//Release the edge.
		edgeManager.release(edgeId);
	}
}

static void processCreatedNodes
(const NodeType* PX_RESTRICT createdNodes, const PxU32 numCreatedNodes, NodeManager& nodeManager, IslandManager& islands)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();

	for(PxU32 i=0;i<numCreatedNodes;i++)
	{
		const NodeType nodeId=createdNodes[i];
		PX_ASSERT(nodeId<allNodesCapacity);
		Node& node=allNodes[nodeId];
		node.clearIsNew();
		//If a body was added after PxScene.simulate and before PxScene.fetchResults then 
		//the addition will be delayed and not processed until the end of fetchResults.
		//If this body is then released straight after PxScene.fetchResults then at the 
		//next PxScene.simulate we will have a body that has been both added and removed.
		//The removal must take precedence over the addition.
		if(!node.getIsDeleted() && node.getIslandId()==INVALID_ISLAND)
		{
			const IslandType islandId=getNewIsland(islands);
			PX_ASSERT(islands.getCapacity()<=allNodesCapacity);
			addNodeToIsland(islandId,nodeId,allNodes,nextNodeIds,allNodesCapacity,islands);
		}
	}
}

static void releaseDeletedNodes(const NodeType* PX_RESTRICT deletedNodes, const PxU32 numDeletedNodes, NodeManager& nodeManager)
{
	for(PxU32 i=0;i<numDeletedNodes;i++)
	{
		const NodeType nodeId=deletedNodes[i];
		PX_ASSERT(nodeManager.get(nodeId).getIsDeleted());
		nodeManager.release(nodeId);
	}
}	

#ifndef __SPU__
static void processSleepingIslands
(const PxU32 rigidBodyOffset, 
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands, ArticulationRootManager& articulationRootManager, ProcessSleepingIslandsComputeData& psicData)
{
	psicData.mBodiesToWakeSize=0;
	psicData.mBodiesToSleepSize=0;
	psicData.mNarrowPhaseContactManagersSize=0;
	psicData.mSolverKinematicsSize=0;
	psicData.mSolverBodiesSize=0;
	psicData.mSolverArticulationsSize=0;
	psicData.mSolverContactManagersSize=0;
	psicData.mSolverConstraintsSize=0;
	psicData.mIslandIndicesSize=0;

	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();
	//const PxU32 allNodesCapacity=nodeManager.getCapacity();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();
	//const PxU32 allEdgesCapacity=edgeManager.getCapacity();
	ArticulationRoot* PX_RESTRICT allArticRoots=articulationRootManager.getAll();
	//const PxU32 allArtisCootsCapacity=articulationRootManager.getCapacity();
	Island* PX_RESTRICT allIslands=islands.getAll();
	const Cm::BitMap& islandsBitmap=islands.getBitmap();
	const PxU32* PX_RESTRICT bitmapWords=islandsBitmap.getWords();

	//const PxU32 allIslandsCapacity=islands.getCapacity();

	NodeType* PX_RESTRICT solverBodyMap=psicData.mSolverBodyMap;
	//const PxU32 solverBodyMapCapacity=psicData.mSolverBodyMapCapacity;
	PxU8** PX_RESTRICT bodiesToWakeOrSleep=psicData.mBodiesToWakeOrSleep;
	const PxU32 bodiesToWakeOrSleepCapacity=psicData.mBodiesToWakeOrSleepCapacity;
	NarrowPhaseContactManager* PX_RESTRICT npContactManagers=psicData.mNarrowPhaseContactManagers;
	//const PxU32 npContactManagersCapacity=psicData.mNarrowPhaseContactManagersCapacity;
	PxsRigidBody** PX_RESTRICT solverKinematics=psicData.mSolverKinematics;
	//const PxU32 solverKinematicsCapacity=psicData.mSolverKinematicsCapacity;
	PxsRigidBody** PX_RESTRICT solverBodies=psicData.mSolverBodies;
	//const PxU32 solverBodiesCapacity=psicData.mSolverBodiesCapacity;
	PxsArticulation** PX_RESTRICT solverArticulations=psicData.mSolverArticulations;
	void** PX_RESTRICT solverArticulationOwners=psicData.mSolverArticulationOwners;
	//const PxU32 solverArticulationsCapacity=psicData.mSolverArticulationsCapacity;
	PxsIndexedContactManager* PX_RESTRICT solverContactManagers=psicData.mSolverContactManagers;
	EdgeType* PX_RESTRICT solverContactManagerEdges=psicData.mSolverContactManagerEdges; 
	//const PxU32 solverContactManagersCapacity=psicData.mSolverContactManagersCapacity;
	PxsIndexedConstraint* PX_RESTRICT solverConstraints=psicData.mSolverConstraints;
	EdgeType* PX_RESTRICT solverConstraintEdges=psicData.mSolverConstraintEdges;
	//const PxU32 solverConstraintsCapacity=psicData.mSolverConstraintsCapacity;
	PxsIslandIndices* PX_RESTRICT islandIndices=psicData.mIslandIndices;
	//const PxU32 islandIndicesCapacity=psicData.mIslandIndicesCapacity;

	PxU32 bodiesToWakeSize=0;
	PxU32 bodiesToSleepSize=0;
	PxU32 npContactManagersSize=0;
	PxU32 solverKinematicsSize=0;
	PxU32 solverBodiesSize=0;
	PxU32 solverArticulationsSize=0;
	PxU32 solverContactManagersSize=0;
	PxU32 solverConstraintsSize=0;
	PxU32 islandIndicesSize=0;


	const PxU32 lastSetBit = islandsBitmap.findLast();
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = bitmapWords[w]; b; b &= b-1)
		{
			const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
			PX_ASSERT(islandId<islands.getCapacity());
			PX_ASSERT(islands.getBitmap().test(islandId));
			const Island& island=allIslands[islandId];

			//First compute the new state of the island 
			//(determine if any nodes have non-zero wake counter).
			NodeType nextNode=island.mStartNodeId;
			PxU32 islandFlags=0;
			while(INVALID_NODE!=nextNode)
			{
				PX_ASSERT(nextNode<nodeManager.getCapacity());
				const Node& node=allNodes[nextNode];
				islandFlags |= (node.getFlags() & Node::eNOTREADYFORSLEEPING);
				nextNode=nextNodeIds[nextNode];
			}

			if(0 == (islandFlags & Node::eNOTREADYFORSLEEPING))
			{
				//Island is asleep because no nodes have non-zero wake counter.
				NodeType nextNode=island.mStartNodeId;
				while(nextNode!=INVALID_NODE)
				{
					//Get the node.
					PX_ASSERT(nextNode<nodeManager.getCapacity());
					Node& node=allNodes[nextNode];
					Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&allNodes[nextNodeIds[nextNode]]) + 128) & ~127));
					PX_ASSERT(node.getIsReadyForSleeping());

					//Work out if the node was previously in a sleeping island.
					const bool wasInSleepingIsland=node.getIsInSleepingIsland();

					//If the node has changed from not being in a sleeping island to 
					//being in a sleeping island then the node needs put to sleep
					//in the high level.  Store the body pointer for reporting to hl.
					if(!wasInSleepingIsland)
					{
						if(!node.getIsArticulated())
						{
							if(!node.getIsKinematic())
							{
								//Set the node to be in a sleeping island.
								node.setIsInSleepingIsland();

								PX_ASSERT((bodiesToWakeSize+bodiesToSleepSize)<bodiesToWakeOrSleepCapacity);
								PX_ASSERT(0==((size_t)node.getRigidBodyOwner() & 0x0f));
								bodiesToWakeOrSleep[bodiesToWakeOrSleepCapacity-1-bodiesToSleepSize]=(PxU8*)node.getRigidBodyOwner();
								Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&bodiesToWakeOrSleep[bodiesToWakeOrSleepCapacity-1-bodiesToSleepSize-1]) + 128) & ~127));
								bodiesToSleepSize++;
							}
						}
						else if(node.getIsRootArticulationLink())
						{
							//Set the node to be in a sleeping island.
							node.setIsInSleepingIsland();

							const PxU32 rootArticId=(PxU32)node.getRootArticulationId();
							const ArticulationRoot& rootArtic=allArticRoots[rootArticId];
							PxU8* articOwner=(PxU8*)rootArtic.mArticulationOwner;
							PX_ASSERT(0==((size_t)articOwner & 0x0f));
							PX_ASSERT((bodiesToWakeSize+bodiesToSleepSize)<bodiesToWakeOrSleepCapacity);
							bodiesToWakeOrSleep[bodiesToWakeOrSleepCapacity-1-bodiesToSleepSize]=(PxU8*)((size_t)articOwner | 0x01);
							Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&bodiesToWakeOrSleep[bodiesToWakeOrSleepCapacity-1-bodiesToSleepSize-1]) + 128) & ~127));
							bodiesToSleepSize++;
						}
					}

					nextNode=nextNodeIds[nextNode];
				}
			}
			else
			{
				//Island is awake because at least one node has a non-zero wake counter.
				PX_ASSERT(island.mStartNodeId!=INVALID_NODE);

				//Add the indices of the island.
				PX_ASSERT(islandIndicesSize<psicData.mIslandIndicesCapacity);
				islandIndices[islandIndicesSize].articulations=solverArticulationsSize;
				islandIndices[islandIndicesSize].bodies=(NodeType)solverBodiesSize;
				islandIndices[islandIndicesSize].contactManagers=(EdgeType)solverContactManagersSize;
				islandIndices[islandIndicesSize].constraints=(EdgeType)solverConstraintsSize;
				Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&islandIndices[islandIndicesSize+1]) + 128) & ~127));

				//Add nodes that need woken up to an array for reporting to high-level.
				NodeType nextNode=island.mStartNodeId;
				while(nextNode!=INVALID_NODE)
				{
					//Get the node.
					PX_ASSERT(nextNode<nodeManager.getCapacity());
					Node& node=allNodes[nextNode];
					Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&allNodes[nextNodeIds[nextNode]]) + 128) & ~127));

					//Work out if the node was previously in a sleeping island.
					const bool wasInSleepingIsland=node.getIsInSleepingIsland();

					//If the node has changed from being in a sleeping island to 
					//not being in a sleeping island then the node needs to be woken up
					//in the high level.  Store the body pointer for reporting to hl.
					if(wasInSleepingIsland)
					{
						if(!node.getIsArticulated())
						{
							if(!node.getIsKinematic())
							{
								node.clearIsInSleepingIsland();

								PX_ASSERT((bodiesToWakeSize+bodiesToSleepSize)<bodiesToWakeOrSleepCapacity);
								PX_ASSERT(0==((size_t)node.getRigidBodyOwner() & 0x0f));
								bodiesToWakeOrSleep[bodiesToWakeSize]=(PxU8*)node.getRigidBodyOwner();
								Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&bodiesToWakeOrSleep[bodiesToWakeSize+1]) + 128) & ~127));
								bodiesToWakeSize++;
							}
						}
						else if(node.getIsRootArticulationLink())
						{
							node.clearIsInSleepingIsland();

							PX_ASSERT((bodiesToWakeSize+bodiesToSleepSize)<bodiesToWakeOrSleepCapacity);
							const PxU32 rootArticId=(PxU32)node.getRootArticulationId();
							const ArticulationRoot& rootArtic=allArticRoots[rootArticId];
							PxU8* articOwner=(PxU8*)rootArtic.mArticulationOwner;
							PX_ASSERT(0==((size_t)articOwner & 0x0f));
							bodiesToWakeOrSleep[bodiesToWakeSize]=(PxU8*)((size_t)articOwner | 0x01);
							Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&bodiesToWakeOrSleep[bodiesToWakeSize+1]) + 128) & ~127));
							bodiesToWakeSize++;
						}						
					}

					if(!node.getIsKinematic())
					{
						if(!node.getIsArticulated())
						{
							//Create the mapping between the entry id in mNodeManager and the entry id in mSolverBoldies
							PX_ASSERT(nextNode<psicData.mSolverBodyMapCapacity);
							solverBodyMap[nextNode]=(NodeType)solverBodiesSize;

							//Add rigid body to solver island.
							PX_ASSERT(solverBodiesSize<psicData.mSolverBodiesCapacity);
							solverBodies[solverBodiesSize]=node.getRigidBody(rigidBodyOffset);
							Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverBodies[solverBodiesSize+1]) + 128) & ~127));
							solverBodiesSize++;
						}
						else if(node.getIsRootArticulationLink())
						{
							//Add articulation to solver island.
							const PxU32 rootArticId=(PxU32)node.getRootArticulationId();
							const ArticulationRoot& rootArtic=allArticRoots[rootArticId];
							PxsArticulationLinkHandle articLinkHandle=rootArtic.mArticulationLinkHandle;
							void* articOwner=rootArtic.mArticulationOwner;
							PX_ASSERT((solverArticulationsSize)<psicData.mSolverArticulationsCapacity);
							solverArticulations[solverArticulationsSize]=getArticulation(articLinkHandle);
							solverArticulationOwners[solverArticulationsSize]=articOwner;
							Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverArticulations[solverArticulationsSize+1]) + 128) & ~127));
							Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverArticulationOwners[solverArticulationsSize+1]) + 128) & ~127));
							solverArticulationsSize++;
						}
					}
					else
					{
						//Create the mapping between the entry id in mNodeManager and the entry id in mSolverBoldies
						PX_ASSERT(nextNode<psicData.mSolverBodyMapCapacity);
						solverBodyMap[nextNode]=(NodeType)solverKinematicsSize;

						//Add kinematic to array of all kinematics.
						PX_ASSERT(!node.getIsArticulated());
						PX_ASSERT((solverKinematicsSize)<psicData.mSolverKinematicsCapacity);
						solverKinematics[solverKinematicsSize]=node.getRigidBody(rigidBodyOffset);
						Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverKinematics[solverKinematicsSize+1]) + 128) & ~127));
						solverKinematicsSize++;
					}

					nextNode=nextNodeIds[nextNode];
				}

				//Any edges in the island that involve a sleeping pair need to be added to a list
				//for np because we need these edges to have contacts for the the solver.
				bool hasStaticContact=false;
				EdgeType nextEdgeId=island.mStartEdgeId;
				while(nextEdgeId!=INVALID_EDGE)
				{
					PX_ASSERT(nextEdgeId<edgeManager.getCapacity());
					Edge& edge=allEdges[nextEdgeId];
					Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&allEdges[nextEdgeIds[nextEdgeId]]) + 128) & ~127));

					//If the edge involves two bodies that need woken up
					//then we need to process that pair through narrowphase
					//in order for there to be contacts for the solver.
					const NodeType node1Id=edge.getNode1();
					const NodeType node2Id=edge.getNode2();
					PxU8 node1Type=PxsIndexedInteraction::eWORLD;
					PxU8 node2Type=PxsIndexedInteraction::eWORLD;
					PxsArticulationLinkHandle body1=(PxsArticulationLinkHandle)-1;
					PxsArticulationLinkHandle body2=(PxsArticulationLinkHandle)-1;
					bool node1IsKineOrStatic=true;
					bool node2IsKineOrStatic=true;

					if(node1Id!=INVALID_NODE)
					{
						PX_ASSERT(node1Id<nodeManager.getCapacity());
						const Node& node1=allNodes[node1Id];
						node1IsKineOrStatic=false;
						if(!node1.getIsArticulated())
						{
							node1Type=PxsIndexedInteraction::eBODY;
							PX_ASSERT(node1Id<psicData.mSolverBodyMapCapacity);
							body1=solverBodyMap[node1Id];
							PX_ASSERT((node1.getIsKinematic() && body1<psicData.mSolverKinematicsCapacity) || (!node1.getIsKinematic() && body1<psicData.mSolverBodiesCapacity));
							node1IsKineOrStatic=node1.getIsKinematic();
						}
						else if(!node1.getIsRootArticulationLink())
						{
							node1Type=PxsIndexedInteraction::eARTICULATION;
							body1=node1.getArticulationLink();
						}
						else
						{
							node1Type=PxsIndexedInteraction::eARTICULATION;
							const PxU32 articRootId=(PxU32)node1.getRootArticulationId();
							const ArticulationRoot& articRoot=allArticRoots[articRootId];
							body1=articRoot.mArticulationLinkHandle;
						}
					}
					else
					{
						hasStaticContact=true;
					}
					if(node2Id!=INVALID_NODE)
					{
						PX_ASSERT(node2Id<nodeManager.getCapacity());
						const Node& node2=allNodes[node2Id];
						node2IsKineOrStatic=false;
						if(!node2.getIsArticulated())
						{
							node2Type=PxsIndexedInteraction::eBODY;
							PX_ASSERT(node2Id<psicData.mSolverBodyMapCapacity);
							body2=solverBodyMap[node2Id];
							PX_ASSERT((node2.getIsKinematic() && body2<psicData.mSolverKinematicsCapacity) || (!node2.getIsKinematic() && body2<psicData.mSolverBodiesCapacity));
							node2IsKineOrStatic=node2.getIsKinematic();
						}
						else if(!node2.getIsRootArticulationLink())
						{
							node2Type=PxsIndexedInteraction::eARTICULATION;
							body2=node2.getArticulationLink();
						}						
						else
						{
							node2Type=PxsIndexedInteraction::eARTICULATION;
							const PxU32 articRootId=(PxU32)node2.getRootArticulationId();
							const ArticulationRoot& articRoot=allArticRoots[articRootId];
							body2=articRoot.mArticulationLinkHandle;
						}
					}
					else
					{
						hasStaticContact=true;
					}

					//Work out if we have an interaction handled by the solver.
					//The solver doesn't handle kinematic-kinematic or kinematic-static
					const bool isSolverIntearction = (!node1IsKineOrStatic || !node2IsKineOrStatic);

					//If both nodes were asleep at the last island gen then they need to undergo a narrowphase overlap prior to running the solver.
					//It is possible that one or other of the nodes has been woken up since the last island gen.  If either was woken up in this way then 
					//the pair will already have a contact manager provided by high-level. This being the case, the narrowphase overlap of the pair 
					//will already have been performed prior to the current island gen.  If neither has been woken up since the last island gen then 
					//there will be no contact manager and narrowphase needs to be performed on the pair in a second pass after the island gen.  The bodies 
					//will be woken up straight after the current island gen (because they are in the array of bodies to be woken up) and the pair will be 
					//given a contact manager by high-level.  Keep a track of which entries in the solver islands temporarily have a null contact manager 
					//so that we can quickly fill them in with non-null contact managers after they have been created by high-level and passed to the edges.
					if(edge.getIsTypeCM() && !edge.getCM() && isSolverIntearction)
					{
						//Add to list for second np pass.
						PX_ASSERT((npContactManagersSize)<psicData.mNarrowPhaseContactManagersCapacity);
						npContactManagers[npContactManagersSize].mSolverCMId=(EdgeType)solverContactManagersSize;
						npContactManagers[npContactManagersSize].mEdgeId=nextEdgeId;
						npContactManagers[npContactManagersSize].mCM=NULL;
						Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&npContactManagers[npContactManagersSize+1]) + 128) & ~127));
						npContactManagersSize++;
					}

					if(edge.getIsTypeCM() && isSolverIntearction)
					{
						//Add to contact managers for the solver island.
						PX_ASSERT((solverContactManagersSize)<psicData.mSolverContactManagersCapacity);
						PxsIndexedContactManager& interaction=solverContactManagers[solverContactManagersSize];
						interaction.contactManager=edge.getCM();
						interaction.indexType0=node1Type;
						interaction.indexType1=node2Type;
						interaction.articulation0=body1;
						interaction.articulation1=body2;
						solverContactManagerEdges[solverContactManagersSize]=nextEdgeId;
						Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverContactManagers[solverContactManagersSize+1]) + 128) & ~127));
						solverContactManagersSize++;
					}
					else if(edge.getIsTypeConstraint() && isSolverIntearction)
					{
						//Add to constraints for the solver island.
						PX_ASSERT((solverConstraintsSize)<psicData.mSolverConstraintsCapacity);
						PxsIndexedConstraint& interaction=solverConstraints[solverConstraintsSize];
						interaction.constraint=edge.getConstraint();
						interaction.indexType0=node1Type;
						interaction.indexType1=node2Type;
						interaction.articulation0=body1;
						interaction.articulation1=body2;
						solverConstraintEdges[solverConstraintsSize]=nextEdgeId;
						Ps::prefetchLine((PxU8*)(((size_t)((PxU8*)&solverConstraints[solverConstraintsSize+1]) + 128) & ~127));
						solverConstraintsSize++;
					}
					else
					{
						//Already stored in the articulation.
						PX_ASSERT(edge.getIsTypeArticulation() || !isSolverIntearction);
					}

					nextEdgeId=nextEdgeIds[nextEdgeId];
				}

				//Record if the island has static contact and increment the island size.
				islandIndices[islandIndicesSize].setHasStaticContact(hasStaticContact);
				islandIndicesSize++;
			}
		}
	}

	psicData.mBodiesToWakeSize=bodiesToWakeSize;
	psicData.mBodiesToSleepSize=bodiesToSleepSize;
	psicData.mNarrowPhaseContactManagersSize=npContactManagersSize;
	psicData.mSolverKinematicsSize=solverKinematicsSize;
	psicData.mSolverBodiesSize=solverBodiesSize;
	psicData.mSolverArticulationsSize=solverArticulationsSize;
	psicData.mSolverContactManagersSize=solverContactManagersSize;
	psicData.mSolverConstraintsSize=solverConstraintsSize;
	psicData.mIslandIndicesSize=islandIndicesSize;

	PX_ASSERT(islandIndicesSize<=islands.getCapacity());
	psicData.mIslandIndices[islandIndicesSize].bodies=(NodeType)solverBodiesSize;
	psicData.mIslandIndices[islandIndicesSize].articulations=solverArticulationsSize;
	psicData.mIslandIndices[islandIndicesSize].contactManagers=(EdgeType)solverContactManagersSize;
	psicData.mIslandIndices[islandIndicesSize].constraints=(EdgeType)solverConstraintsSize;
}
#endif

#ifdef __SPU__

void physx::accountForKinematicCount
(ProcessSleepingIslandsComputeData& psicData, NodeManager& nodeManager, EdgeManager& edgeManager)
{
}

#else

static void accountForKinematicCount2
(PxsIndexedInteraction& c, const EdgeType edgeId,
 ProcessSleepingIslandsComputeData& psicData, NodeManager& nodeManager, EdgeManager& edgeManager)
{
	//When the body ids for each PxsIndexedInteraction were recorded they didn't take 
	//into account the number of kinematics (because the kinematic count was being updated
	//at the same time).  The solver code builds an array of bodies with all kinematics at the 
	//beginning, followed by non-kinematics.  Non-kinematics need their array index to be offset
	//by the kinematic count, while kinematics need to be given their corresponding position 
	//in the kinematic array. Articulations have a different indexing scheme so they can be ignored.

	const Edge& edge=edgeManager.get(edgeId);

	const NodeType nodeId1=edge.getNode1();
	if(INVALID_NODE!=nodeId1)
	{
		Node& node=nodeManager.get(nodeId1);
		if(!node.getIsKinematic())
		{
			if(!node.getIsArticulated())
			{
				c.solverBody0+=psicData.mSolverKinematicsSize;
			}
		}
		else
		{
			PX_ASSERT(INVALID_NODE!=c.solverBody0);
			PX_ASSERT(nodeId1<psicData.mSolverBodyMapCapacity);
			c.solverBody0=psicData.mSolverBodyMap[nodeId1];
		}
	}

	const NodeType nodeId2=edge.getNode2();
	if(INVALID_NODE!=nodeId2)
	{
		Node& node=nodeManager.get(nodeId2);
		if(!node.getIsKinematic())
		{
			if(!node.getIsArticulated())
			{
				c.solverBody1+=psicData.mSolverKinematicsSize;
			}
		}
		else
		{
			PX_ASSERT(INVALID_NODE!=c.solverBody1);
			PX_ASSERT(nodeId2<psicData.mSolverBodyMapCapacity);
			c.solverBody1=psicData.mSolverBodyMap[nodeId2];
		}
	}

	PX_ASSERT(c.solverBody0!=c.solverBody1);
}

void physx::accountForKinematicCount
(ProcessSleepingIslandsComputeData& psicData, NodeManager& nodeManager, EdgeManager& edgeManager)
{
	if(psicData.mSolverKinematicsSize>0)
	{
		for(PxU32 i=0;i<psicData.mSolverContactManagersSize;i++)
		{
			PxsIndexedContactManager& cm=psicData.mSolverContactManagers[i];
			const EdgeType edgeId=psicData.mSolverContactManagerEdges[i];
			accountForKinematicCount2(cm,edgeId,psicData,nodeManager,edgeManager);
		}

		for(PxU32 i=0;i<psicData.mSolverConstraintsSize;i++)
		{
			PxsIndexedConstraint& constraint=psicData.mSolverConstraints[i];
			const EdgeType edgeId=psicData.mSolverConstraintEdges[i];
			accountForKinematicCount2(constraint,edgeId,psicData,nodeManager,edgeManager);
		}
	}
}

#endif


/*
void computeBrokenEdgeIslandsBitmap
(const EdgeType* PX_RESTRICT brokenEdgeIds, const PxU32 numBrokenEdges, const EdgeType* PX_RESTRICT deletedEdgeIds, const PxU32 numDeletedEdges, 
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& affectedIslandsBitmap)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	Edge* PX_RESTRICT allEdges=edgeManager.getAll();

	affectedIslandsBitmap.clearFast();

	//Gather a list of islands that contain a broken edge.
	for(PxU32 i=0;i<numBrokenEdges;i++)
	{
		//Get the edge that has just been broken and add it to the bitmap.
		const EdgeType edgeId=brokenEdgeIds[i];
		PX_ASSERT(edgeId<edgeManager.getCapacity());
		const Edge& edge=allEdges[edgeId];

		//Check that the edge is legal.
		PX_ASSERT(!edge.getIsConnected());

		//Get the two nodes of the edge.
		//Get the two islands of the edge and add them to the bitmap.
		const NodeType nodeId1=edge.getNode1();
		const NodeType nodeId2=edge.getNode2();
		IslandType islandId1=INVALID_ISLAND;
		IslandType islandId2=INVALID_ISLAND;
		if(INVALID_NODE!=nodeId1)
		{
			PX_ASSERT(nodeId1<nodeManager.getCapacity());
			islandId1=allNodes[nodeId1].getIslandId();
			if(INVALID_ISLAND!=islandId1)
			{
				affectedIslandsBitmap.set(islandId1);
			}
		}
		if(INVALID_NODE!=nodeId2)
		{
			PX_ASSERT(nodeId2<nodeManager.getCapacity());
			islandId2=allNodes[nodeId2].getIslandId();
			if(INVALID_ISLAND!=islandId2)
			{
				affectedIslandsBitmap.set(islandId2);
			}
		}
	}

	for(PxU32 i=0;i<numDeletedEdges;i++)
	{
		//Get the edge that has just been broken and add it to the bitmap.
		const EdgeType edgeId=deletedEdgeIds[i];
		PX_ASSERT(edgeId<edgeManager.getCapacity());
		Edge& edge=allEdges[edgeId];

		//Get the two nodes of the edge.
		//Get the two islands of the edge and add them to the bitmap.
		const NodeType nodeId1=edge.getNode1();
		const NodeType nodeId2=edge.getNode2();
		IslandType islandId1=INVALID_ISLAND;
		IslandType islandId2=INVALID_ISLAND;
		if(INVALID_NODE!=nodeId1)
		{
			PX_ASSERT(nodeId1<nodeManager.getCapacity());
			islandId1=allNodes[nodeId1].getIslandId();
			if(INVALID_ISLAND!=islandId1)
			{
				affectedIslandsBitmap.set(islandId1);
			}
		}
		if(INVALID_NODE!=nodeId2)
		{
			PX_ASSERT(nodeId2<nodeManager.getCapacity());
			islandId2=allNodes[nodeId2].getIslandId();
			if(INVALID_ISLAND!=islandId2)
			{
				affectedIslandsBitmap.set(islandId2);
			}
		}
	}
}
*/

void physx::mergeKinematicProxiesBackToSource
(const Cm::BitMap& kinematicNodesBitmap,
 const NodeType* PX_RESTRICT kinematicProxySourceNodes, const NodeType* PX_RESTRICT kinematicProxyNextNodes,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands,
 Cm::BitMap& kinematicIslandsBitmap,
 IslandType* graphStartIslands, IslandType* graphNextIslands)
{
	Node* PX_RESTRICT allNodes=nodeManager.getAll();
	const PxU32 allNodesCapacity=nodeManager.getCapacity();
	NodeType* PX_RESTRICT nextNodeIds=nodeManager.getNextNodeIds();

	Edge* PX_RESTRICT allEdges=edgeManager.getAll();
	const PxU32 allEdgesCapacity=edgeManager.getCapacity();
	EdgeType* PX_RESTRICT nextEdgeIds=edgeManager.getNextEdgeIds();

	PxMemSet(graphStartIslands, 0xff, sizeof(IslandType)*islands.getCapacity());
	PxMemSet(graphNextIslands, 0xff, sizeof(IslandType)*islands.getCapacity());

	//Remove all the proxies from all islands.

	//Compute the bitmap of islands affected by kinematic proxy nodes.
	//Work out if the node was in a non-sleeping island.
	{
		const PxU32* PX_RESTRICT kinematicNodesBitmapWords=kinematicNodesBitmap.getWords();
		const PxU32 lastSetBit = kinematicNodesBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicNodesBitmapWords[w]; b; b &= b-1)
			{
				const NodeType kinematicNodeId = (NodeType)(w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(kinematicNodeId<allNodesCapacity);
				PX_ASSERT(allNodes[kinematicNodeId].getIsKinematic());
				NodeType nextProxyNodeId=kinematicProxyNextNodes[kinematicNodeId];
				while(nextProxyNodeId!=INVALID_NODE)
				{
					PX_ASSERT(nextProxyNodeId<allNodesCapacity);
					Node& node=allNodes[nextProxyNodeId];
					node.setIsDeleted();
					const IslandType islandId=node.getIslandId();
					PX_ASSERT(islandId<islands.getCapacity());
					graphStartIslands[nextProxyNodeId]=islandId;
					graphNextIslands[nextProxyNodeId]=INVALID_ISLAND;
					kinematicIslandsBitmap.set(islandId);
					nextProxyNodeId=kinematicProxyNextNodes[nextProxyNodeId];
				}
			}//b
		}//w
	}

	//Iterate over all islands with a kinematic proxy and remove them from the node list and from the edges.
	{
		const PxU32 lastSetBit = kinematicIslandsBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicIslandsBitmap.getWords()[w]; b; b &= b-1)
			{
				const IslandType islandId = (IslandType)(w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(islandId<islands.getCapacity());

				//Remove all kinematic proxy nodes from all affected islands.
				removeDeletedNodesFromIsland
					(islandId,
					nodeManager,edgeManager,islands);

				//Make the edges reference the original node.
				const Island& island=islands.get(islandId);
				EdgeType nextEdge=island.mStartEdgeId;
				while(INVALID_EDGE!=nextEdge)
				{
					//Get the edge.
					PX_ASSERT(nextEdge<allEdgesCapacity);
					Edge& edge=allEdges[nextEdge];

					const NodeType node1=edge.getNode1();
					if(INVALID_NODE!=node1)
					{
						PX_ASSERT(node1<allNodesCapacity);
						if(kinematicProxySourceNodes[node1]!=INVALID_NODE)
						{
							PX_ASSERT(allNodes[node1].getIsKinematic());
							PX_ASSERT(allNodes[node1].getIsDeleted());
							edge.setNode1(kinematicProxySourceNodes[node1]);
						}
					}

					const NodeType node2=edge.getNode2();
					if(INVALID_NODE!=node2)
					{
						PX_ASSERT(node2<allNodesCapacity);
						if(kinematicProxySourceNodes[node2]!=INVALID_NODE)
						{
							PX_ASSERT(allNodes[node2].getIsKinematic());
							PX_ASSERT(allNodes[node2].getIsDeleted());
							edge.setNode2(kinematicProxySourceNodes[node2]);
						}
					}

					nextEdge=nextEdgeIds[nextEdge];			
				}//nextEdge
			}//b
		}//w
	}

	//Merge all the islands.
	{
		const PxU32* PX_RESTRICT kinematicNodesBitmapWords=kinematicNodesBitmap.getWords();
		const PxU32 lastSetBit = kinematicNodesBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicNodesBitmapWords[w]; b; b &= b-1)
			{
				const NodeType kinematicNodeId = (NodeType)(w<<5|Ps::lowestSetBit(b));

				//Get the first proxy and its island.
				const NodeType firstProxyNodeId=kinematicProxyNextNodes[kinematicNodeId];

				if(INVALID_NODE!=firstProxyNodeId)
				{
					//The kinematic is referenced by at least one edge.
					//Safe to get the first proxy node.
					PX_ASSERT(firstProxyNodeId<allNodesCapacity);
					IslandType firstIslandId=INVALID_ISLAND;
					IslandType nextIslandId=graphStartIslands[firstProxyNodeId];
					PX_ASSERT(nextIslandId!=INVALID_ISLAND);
					while(nextIslandId!=INVALID_ISLAND)
					{
						firstIslandId=nextIslandId;
						nextIslandId=graphNextIslands[nextIslandId];
					}
					PX_ASSERT(firstIslandId<islands.getCapacity());

					//First set the node to be in the correct island.
					const NodeType kinematicSourceId=kinematicProxySourceNodes[firstProxyNodeId];
					PX_ASSERT(kinematicSourceId==kinematicNodeId);
					PX_ASSERT(kinematicSourceId<allNodesCapacity);
					Node& kinematicSourceNode=allNodes[kinematicSourceId];
					kinematicSourceNode.setIslandId(firstIslandId);

					//Now add the node to the island.
					PX_ASSERT(kinematicSourceId<allNodesCapacity);
					addNodeToIsland(firstIslandId,kinematicSourceId,allNodes,nextNodeIds,allNodesCapacity,islands);

					//Now merge the other islands containing a kinematic proxy with the first island.
					NodeType nextProxyNodeId=kinematicProxyNextNodes[firstProxyNodeId];
					while(INVALID_NODE!=nextProxyNodeId)
					{
						PX_ASSERT(nextProxyNodeId<allNodesCapacity);

						//Get the island.
						IslandType islandId=INVALID_ISLAND;
						IslandType nextIslandId=graphStartIslands[nextProxyNodeId];
						PX_ASSERT(nextIslandId!=INVALID_ISLAND);
						while(nextIslandId!=INVALID_ISLAND)
						{
							islandId=nextIslandId;
							nextIslandId=graphNextIslands[nextIslandId];
						}
						PX_ASSERT(islandId<islands.getCapacity());

						//Merge the two islands provided they are 
						//(i)  different
						//(ii) islandId hasn't already been merged into another island and released

						if(firstIslandId!=islandId && islands.getBitmap().test(islandId))
						{
							setNodeIslandIdsAndJoinIslands(firstIslandId,islandId,allNodes,allNodesCapacity,allEdges,allEdgesCapacity,nextNodeIds,nextEdgeIds,islands);
							graphNextIslands[islandId]=firstIslandId;
						}

						//Get the next proxy node.			
						nextProxyNodeId=kinematicProxyNextNodes[nextProxyNodeId];
					}
				}//INVALID_NODE!=firstProxyNodeId

				//Nodes with no edges require no treatment.
				//They were already put in their own island when during node duplication.
			}//b
		}//w
	}

	//Release the proxies.
	{
		const PxU32* PX_RESTRICT kinematicNodesBitmapWords=kinematicNodesBitmap.getWords();
		const PxU32 lastSetBit = kinematicNodesBitmap.findLast();
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = kinematicNodesBitmapWords[w]; b; b &= b-1)
			{
				const NodeType kinematicNodeId = (NodeType)(w<<5|Ps::lowestSetBit(b));
				NodeType nextProxyNode=kinematicProxyNextNodes[kinematicNodeId];
				while(nextProxyNode!=INVALID_NODE)
				{
					PX_ASSERT(allNodes[nextProxyNode].getIsKinematic());
					PX_ASSERT(allNodes[nextProxyNode].getIsDeleted());
					nodeManager.release(nextProxyNode);
					nextProxyNode=kinematicProxyNextNodes[nextProxyNode];
				}
			}
		}
	}
}

#ifndef __SPU__
#define ISLANDGEN_PROFILE 1
#else
#define ISLANDGEN_PROFILE 0
#endif

void physx::updateIslandsMain
(const PxU32 rigidBodyOffset,
 const NodeType* PX_RESTRICT deletedNodes, const PxU32 numDeletedNodes,
 const NodeType* PX_RESTRICT createdNodes, const PxU32 numCreatedNodes,
 const EdgeType* PX_RESTRICT deletedEdges, const PxU32 numDeletedEdges,
 const EdgeType* PX_RESTRICT /*createdEdges*/, const PxU32 /*numCreatedEdges*/,
 const EdgeType* PX_RESTRICT brokenEdges, const PxU32 numBrokenEdges,
 const EdgeType* PX_RESTRICT joinedEdges, const PxU32 numJoinedEdges,
 const Cm::BitMap& kinematicNodesBitmap, const PxU32 numAddedKinematics,
 NodeManager& nodeManager, EdgeManager& edgeManager, IslandManager& islands, ArticulationRootManager& articulationRootManager,
 ProcessSleepingIslandsComputeData& psicData,
 IslandManagerUpdateWorkBuffers& workBuffers,
 Cm::EventProfiler* profiler)
{
	//Bitmaps of islands affected by joined/broken edges.
	Cm::BitMap& brokenEdgeIslandsBitmap=*workBuffers.mIslandBitmap1;
	brokenEdgeIslandsBitmap.clearFast();

	//Remove deleted nodes from islands.
	//Remove deleted/broken edges from islands.
	//Release empty islands.
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetemptyIslands());
#endif

		//When removing deleted nodes from islands some islands end up empty.
		//Don't immediately release these islands because we also want to 
		//remove edges from islands too and it makes it harder to manage
		//the book-keeping if some islands have already been released.
		//Store a list of empty islands and release after removing 
		//deleted edges from islands.

		Cm::BitMap& emptyIslandsBitmap=*workBuffers.mIslandBitmap2;
		emptyIslandsBitmap.clearFast();

		//Remove deleted nodes from islands with the help of a bitmap to record
		//all islands affected by a deleted node.
		removeDeletedNodesFromIslands(
			deletedNodes,numDeletedNodes,
			nodeManager,edgeManager,islands,
			brokenEdgeIslandsBitmap,emptyIslandsBitmap);
		brokenEdgeIslandsBitmap.clearFast();

		//Remove broken/deleted edges from islands.
		removeBrokenEdgesFromIslands(
			brokenEdges,numBrokenEdges,
			deletedEdges,numDeletedEdges,
			nodeManager,edgeManager,islands,
			brokenEdgeIslandsBitmap);

		//Now release all islands.
		releaseEmptyIslands(
			emptyIslandsBitmap,
			islands,
			brokenEdgeIslandsBitmap);
		emptyIslandsBitmap.clearFast();

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetemptyIslands());
#endif
	}

	//Process all edges that are flagged as connected by joining islands together.
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetjoinedEdges());
#endif

		Cm::BitMap& affectedIslandsBitmap=*workBuffers.mIslandBitmap2;
		affectedIslandsBitmap.clearFast();

		NodeType* graphNextNodes=workBuffers.mGraphNextNodes;
		IslandType* graphStartIslands=workBuffers.mGraphStartIslands;
		IslandType* graphNextIslands=workBuffers.mGraphNextIslands;

		processJoinedEdges(
			joinedEdges,numJoinedEdges,
			nodeManager,edgeManager,islands,
			brokenEdgeIslandsBitmap,
			affectedIslandsBitmap,
			graphNextNodes,graphStartIslands,graphNextIslands);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetjoinedEdges());
#endif
	}

	//Any new nodes not involved in a joined edge need to be placed in their own island.
	//Release deleted edges/nodes so they are available for reuse.
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetcreatedNodes());
#endif

		processCreatedNodes(
			createdNodes,numCreatedNodes,
			nodeManager,islands);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetcreatedNodes());
#endif
	}

	//Release deleted edges/nodes so they are available for reuse.
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetdeletedNodesEdges());
#endif

		releaseDeletedNodes(
			deletedNodes, numDeletedNodes, 
			nodeManager);

		releaseDeletedEdges(
			deletedEdges,numDeletedEdges,
			edgeManager);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetdeletedNodesEdges());
#endif
	}

	//Duplicate all kinematics with temporary proxy nodes to ensure that kinematics don't
	//act as a bridge between islands.
	if(numAddedKinematics>0)
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetduplicateKinematicNodes());
#endif

		NodeType* kinematicProxySourceNodeIds=workBuffers.mKinematicProxySourceNodeIds;
		NodeType* kinematicProxyNextNodeIds=workBuffers.mKinematicProxyNextNodeIds;

		duplicateKinematicNodes(
			kinematicNodesBitmap,
			nodeManager,edgeManager,islands,
			kinematicProxySourceNodeIds,kinematicProxyNextNodeIds,
			brokenEdgeIslandsBitmap);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetduplicateKinematicNodes());
#endif
	}

	//Any islands with a broken edge or an edge referencing a kinematic need to be rebuilt into their sub-islands.
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetbrokenEdgeIslands());
#endif

		NodeType* graphNextNodes=workBuffers.mGraphNextNodes;
		IslandType* graphStartIslands=workBuffers.mGraphStartIslands;
		IslandType* graphNextIslands=workBuffers.mGraphNextIslands;

		processBrokenEdgeIslands(
			brokenEdgeIslandsBitmap,
			nodeManager,edgeManager,islands,
			graphNextNodes,graphStartIslands,graphNextIslands);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetbrokenEdgeIslands());
#endif
	}

	//Process all the sleeping and awake islands to compute the solver islands data.
	//Offset all solver body list positions with the number of kinematics that are passed to the solver.
	//(Note that the number of kinematic solver bodies is not the number of kinematics because some are asleep.)
	{
#if ISLANDGEN_PROFILE
		CM_PROFILE_START(profiler, Cm::ProfileEventId::IslandGen::GetprocessSleepingIslands());
#endif
		
		processSleepingIslands(
			rigidBodyOffset,
			nodeManager,edgeManager,islands,articulationRootManager,
			psicData);

		accountForKinematicCount(
			psicData,
			nodeManager,edgeManager);

#if ISLANDGEN_PROFILE
		CM_PROFILE_STOP(profiler, Cm::ProfileEventId::IslandGen::GetprocessSleepingIslands());
#endif
	}
}
