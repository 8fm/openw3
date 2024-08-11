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


#include "ScConstraintProjectionTree.h"
#include "ScBodySim.h"
#include "ScConstraintSim.h"
#include "ScConstraintInteraction.h"

#include "PsFoundation.h"
#include "PsBasicTemplates.h"
#include "PsSort.h"
#include "PsArray.h"

using namespace physx;

//------------------------------------------------------------------------------------------
//
// The projection tree related code
// 
// Projection trees are built out of a constraint group/graph. The constraint group just tracks
// the constraint connectivity while the projection trees define the projection root and
// the projection order.
// A constraint group can contain multiple projection trees.
//
//------------------------------------------------------------------------------------------

class Sc::BodyRank		
{
public:
	PX_INLINE bool operator>(const BodyRank & b) const
	{
		return rank > b.rank;
	}

	Sc::ConstraintGroupNode* startingNode;
	Sc::ConstraintSim* anyConstraintToWorld;
	PxU32 rank;
};


PX_INLINE bool isFixedBody(Sc::BodySim* b)
{
	return (!b || (b->isKinematic()));
}


void Sc::ConstraintProjectionTree::rankConstraint(ConstraintSim& c, BodyRank& br)
{
	if (isFixedBody(c.getOtherBody(br.startingNode->body)))	//we care about this for the proj tree
	{
		//joint to world!!
		br.rank += 100000;

		if (c.needsProjection())	//we should prefer picking projected constraints as the root over non-projected ones.
			br.rank += 100;

		br.anyConstraintToWorld = &c;
	}
	else
	{
		br.rank += 1000;	//nonarticulateable constraint -- counts a lot because making this a child link of an articulation is quite unstable.
	}
}


/*
the goal here is to take the constraint group whose root is passed, and create one or more projection trees.

At the moment, the group has to be acyclic and have at most 1 constraint with the ground to be accepted.
*/
void Sc::ConstraintProjectionTree::buildProjectionTrees(ConstraintGroupNode& root)
/*------------------------------------------*\
Here we 'flood fill' the constraint graph several times, starting at 
bodies where articulations can be rooted. Because articulations
linked to the world can't be re-rooted, they have to be built starting at those 
nodes. For this reason we first start articulations at these world
jointed bodies. If there are none of these, and we still have bodies
left over, we need to start articulations also at other bodies.
We now prefer to start at bodies which have some non-articulateable
bodies attached to them, because this makes the simulation more stable.

The algo looks like this:

for all bodies
mark body as undiscovered
rank this body

the rank of a body depends on the constraints its connected to
- is zero if it has no articulateable constraints
- it is very high if the body is jointed to the world
- it is high if it has many non-articulateable constraints
- it is kinda large if it has many articulateable constraints

for all bodies sorted according to rank
if the body still hasn't been discovered
start an articulation there.

we also build the projectionTree here if needed. It is also started
at the best ranked body.
\*------------------------------------------*/
{
	PX_ASSERT(&root == root.parent);
	PX_ASSERT(!root.hasProjectionTreeRoot());

	Ps::InlineArray<BodyRank, 64> bodyRankArray PX_DEBUG_EXP("bodyRankArray");
	BodyRank br;
	ConstraintGroupNode* node = &root;
	while (node)	//for all nodes in group
	{
		node->clearFlag(ConstraintGroupNode::eDISCOVERED);

		//rank 
		br.startingNode = node;
		br.rank = 0;
		br.anyConstraintToWorld = 0;

		//go through all constraints conntected to body
		Cm::Range<Interaction*const> interactions = node->body->getActorInteractions();
		for(; !interactions.empty(); interactions.popFront())
		{
			Interaction *const interaction = interactions.front();
			if (interaction->getType() == PX_INTERACTION_TYPE_CONSTRAINTSHADER)
			{
				ConstraintSim* c = static_cast<ConstraintInteraction*>(interaction)->getConstraint();
				rankConstraint(*c, br);
			}
		}

		PX_ASSERT(br.rank);	//if it has no constraints then why is it in the constraint group?

		bodyRankArray.pushBack(br);

		node = node->next;
	}

	//sort bodyRankArray

	PX_ASSERT(bodyRankArray.size());
	Ps::sort(&bodyRankArray.front(), bodyRankArray.size(), Ps::Greater<BodyRank>());

	//build the projectionTree

	ConstraintGroupNode* firstProjectionTreeRoot = NULL;

	//go through it in sorted order
	for (Ps::Array<BodyRank>::Iterator i = bodyRankArray.begin(); i != bodyRankArray.end(); i++)
	{
		if (!(*i).startingNode->readFlag(ConstraintGroupNode::eDISCOVERED))  // ignore the ones which have been processed by buildOneArticulation() below 
		{
			ConstraintGroupNode* link = buildOneProjectionTree(*((*i).startingNode), (*i).anyConstraintToWorld);	//this should mark all processed bodies as discovered.
			PX_ASSERT(link);

			if (link->projectionFirstChild || !link->body->isKinematic())	// Avoid trees with a single kinematic body (an alternative could be to avoid kinematics as roots completely
																			// and start at the dynamics connected to the kinematics. This would result in more but smaller trees in general
																			// but break the "natural" tree hierarchy. Not clear what's better)
			{
				if (firstProjectionTreeRoot)
				{
					link->projectionNextRoot = firstProjectionTreeRoot;
					firstProjectionTreeRoot = link;
				}
				else
					firstProjectionTreeRoot = link;
			}
		}
	}

	root.setProjectionTreeRoot(firstProjectionTreeRoot);
}


Sc::ConstraintGroupNode* Sc::ConstraintProjectionTree::buildOneProjectionTree(ConstraintGroupNode& root, ConstraintSim* constraintToWorld)
{
	/*
	depth first traverse the constraint graph starting at this body, and
	stop at unsupported constraints, constraints forced to lagrange, and constraints to bodies that were already processed (cycles).

	optional constraintToWorld must link to this body, and must be of supported type.
	*/
	PX_ASSERT(!root.readFlag(ConstraintGroupNode::eDISCOVERED));

	ConstraintGroupNode* tree = buildBodyTree(0, root, constraintToWorld);

	PX_ASSERT(tree);
	return tree;
}


Sc::ConstraintGroupNode* Sc::ConstraintProjectionTree::buildBodyTree(ConstraintGroupNode* parent, ConstraintGroupNode& node, ConstraintSim* cToParent)
{
	node.raiseFlag(ConstraintGroupNode::eDISCOVERED);	//flag body nodes that we process so we can detect loops

	node.initProjectionData(parent, cToParent);

	//go through all constraints attached to the body.
	BodySim* body = node.body;
	Cm::Range<Interaction*const> interactions = body->getActorInteractions();
	for(; !interactions.empty(); interactions.popFront())
	{
		Interaction *const interaction = interactions.front();
		if (interaction->getType() == PX_INTERACTION_TYPE_CONSTRAINTSHADER)
		{
			ConstraintSim* c = static_cast<ConstraintInteraction*>(interaction)->getConstraint();

			if (c != cToParent)	//don't go back along the edge we came from (not really necessary I guess since the ConstraintGroupNode::eDISCOVERED marker should solve this)
			{
				BodySim* neighbor = c->getOtherBody(body);	
				if(!isFixedBody(neighbor))	//just ignore the eventual constraint with environment over here. This may be an unsupported type or an articulation is jointed to the world twice.
				{
					PX_ASSERT(neighbor->getConstraintGroup());
					ConstraintGroupNode* neighborNode = neighbor->getConstraintGroup();

					if (!neighborNode->readFlag(ConstraintGroupNode::eDISCOVERED))
					{
						// MS: Recursion! But it just does not really seem worth the effort to change this now. And the failure scenario seems rather unlikely (plus it was like that for ages already)

						//traverse subtree of neighbor. Do this as soon as possible to early out quick due to cycles.
						buildBodyTree(&node, *neighborNode, c);
					}
				}
			}
		}
	}

	return &node;
}


void Sc::ConstraintProjectionTree::purgeProjectionTrees(ConstraintGroupNode& root)
{
	PX_ASSERT(&root == root.parent);
	PX_ASSERT(root.hasProjectionTreeRoot());

	// CA: New code (non recursive: recursive calls can cause stack overflow with huge trees)
	ConstraintGroupNode* projRoot = root.projectionFirstRoot;
	do
	{
		ConstraintGroupNode* currentNode = projRoot;
		projRoot = projRoot->projectionNextRoot;  // need to do it here because the info might get cleared below

		do
		{
			// Go down the tree until we find a leaf
			if (currentNode->projectionFirstChild)
			{
				currentNode = currentNode->projectionFirstChild;
				continue;
			}

			// Delete current node and go to next sibling or parent
			ConstraintGroupNode* nodeToDelete = currentNode;
			ConstraintGroupNode* parent = currentNode->projectionParent;
			currentNode = currentNode->projectionNextSibling;

			// Mark parent as leaf
			if (nodeToDelete->projectionParent)
				nodeToDelete->projectionParent->projectionFirstChild = NULL;

			// Clear projection info
			nodeToDelete->clearProjectionData();

			if (currentNode != NULL)
				continue;

			// No more siblings jump back to parent
			currentNode = parent;

		} while (currentNode != NULL);

	} while (projRoot != NULL);

	root.clearProjectionData();  // to make sure the root gets cleared if kinematics are involved
}


void Sc::ConstraintProjectionTree::projectPoseForTree(ConstraintGroupNode& node)
{
	// create a dummy node to keep the loops compact while covering the special case of the first node
	PX_ASSERT(node.body);
	ConstraintGroupNode dummyNode(*node.body);
	dummyNode.projectionNextSibling = &node;
	ConstraintGroupNode* currentNode = &dummyNode;

	// non recursive: recursive calls can cause stack overflow with huge trees
	do
	{
		ConstraintGroupNode* nextSiblingNode = currentNode->projectionNextSibling;

		while (nextSiblingNode)
		{
			currentNode = nextSiblingNode;
			ConstraintGroupNode* nextChildNode = currentNode;

			do
			{
				currentNode = nextChildNode;

				//-----------------------------------------------------------------------------
				ConstraintSim* c = currentNode->projectionConstraint;

				if (c && c->hasDynamicBody() && c->needsProjection())
				{
					c->projectPose(currentNode->body);
				}
				//-----------------------------------------------------------------------------

				nextChildNode = currentNode->projectionFirstChild;

			} while (nextChildNode);

			nextSiblingNode = currentNode->projectionNextSibling;
		}

		currentNode = currentNode->projectionParent;

	} while (currentNode != NULL);
}


void Sc::ConstraintProjectionTree::projectPose(ConstraintGroupNode& root)
{
	PX_ASSERT(&root == root.parent);
	PX_ASSERT(root.hasProjectionTreeRoot());

	ConstraintGroupNode* projRoot = root.projectionFirstRoot;
	do
	{
		projectPoseForTree(*projRoot);
		projRoot = projRoot->projectionNextRoot;

	} while (projRoot != NULL);
}
