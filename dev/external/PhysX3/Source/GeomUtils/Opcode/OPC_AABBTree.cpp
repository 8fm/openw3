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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PsIntrinsics.h"
#include "PsVecMath.h"
#include "PsUserAllocated.h"
#include "PsMathUtils.h"
#include "./Ice/IceContainer.h"
#include "OPC_AABBTree.h"
#include "OPC_TreeBuilders.h"
#ifdef SUPPORT_UPDATE_ARRAY
#include "./Ice/IceRevisitedRadix.h"
#endif

using namespace physx;
using namespace physx::shdfnd::aos;
using namespace Gu;

#ifdef PX_WIIU
VecU32V AABBCompressionConstants::ff = VecU32VLoadXYZW(0xFF, 0xFF, 0xFF, 0xFF);
Vec4V   AABBCompressionConstants::scaleMul4 = V4Load(1.0f/10000.0f);
PxF32   AABBCompressionConstants::scaleMul1 = 1.0f/10000.0f;
#endif

#ifdef SUPPORT_PROGRESSIVE_BUILDING
class Gu::FIFOStack2 : public Ps::UserAllocated
{
	public:
									FIFOStack2() : mStack(PX_DEBUG_EXP("SQFIFOStack2")), mCurIndex(0) {}
									~FIFOStack2() {}

	PX_FORCE_INLINE	PxU32			GetNbEntries() const { return mStack.size(); }
	PX_FORCE_INLINE	void			Push(AABBTreeNode* entry, AABBTreeNode* parent)	{ mStack.pushBack(NodeAndParent(entry, parent)); }
					bool			Pop(AABBTreeNode*& entry, AABBTreeNode*& parent);
	private:
		struct NodeAndParent
		{
			NodeAndParent(AABBTreeNode* aNode, AABBTreeNode* aParent) : node(aNode), parent(aParent) {}
			AABBTreeNode* node;
			AABBTreeNode* parent;
		};
		Ps::Array<NodeAndParent>	mStack;
		PxU32						mCurIndex;			//!< Current index within the container
};

bool FIFOStack2::Pop(AABBTreeNode*& entry, AABBTreeNode*& parent)
{
	PxU32 NbEntries = mStack.size(); // Get current number of entries
	if(!NbEntries)
		return false; // Can be NULL when no value has been pushed. This is an invalid pop call.
	entry = mStack[mCurIndex].node; // Get oldest entry, move to next one
	parent = mStack[mCurIndex++].parent; // Get oldest entry, move to next one
	if(mCurIndex==NbEntries)
	{
		// All values have been poped
		mStack.clear();
		mCurIndex=0;
	}
	return true;
}
#endif // #ifdef SUPPORT_PROGRESSIVE_BUILDING

/*static PX_FORCE_INLINE PxU32 BitsToBytes(PxU32 nb_bits)
{
	return (nb_bits>>3) + ((nb_bits&7) ? 1 : 0);
}*/

static PX_FORCE_INLINE PxU32 BitsToDwords(PxU32 nb_bits)
{
	return (nb_bits>>5) + ((nb_bits&31) ? 1 : 0);
}

BitArray::BitArray() : mBits(NULL), mSize(0)
{
}

BitArray::BitArray(PxU32 nb_bits)
{
	Init(nb_bits);
}

bool BitArray::Init(PxU32 nb_bits)
{
	mSize = BitsToDwords(nb_bits);
	// Get ram for n bits
	PX_FREE(mBits);
	mBits = (PxU32*)PX_ALLOC(sizeof(PxU32)*mSize, PX_DEBUG_EXP("BitArray::mBits"));
	// Set all bits to 0
	ClearAll();
	return true;
}

BitArray::~BitArray()
{
	PX_FREE_AND_RESET(mBits);
	mSize = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Splits the node along a given axis.
 *	The list of indices is reorganized according to the split values.
 *	\param		axis		[in] splitting axis index
 *	\param		builder		[in] the tree builder
 *	\return		the number of primitives assigned to the first child
 *	\warning	this method reorganizes the internal list of primitives
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PxU32 AABBTreeNode::Split(const PxBounds3& exactBounds, PxU32 axis, AABBTreeBuilder* builder, PxU32* indices)
{
	PX_ASSERT(builder->mNodeBase);

	// Get node split value
	const float SplitValue = builder->GetSplittingValue(exactBounds, axis);

	PxU32 NbPos = 0;
	// Loop through all node-related primitives. Their indices range from GetNodePrimitives()[0] to GetNodePrimitives()[mNbBuildPrimitives-1].
	// Those indices map the global list in the tree builder.
	for(PxU32 i=0;i<GetNbBuildPrimitives();i++)
	{
		// Get index in global list
		PxU32 Index = indices[GetNodePrimitives()+i];

		// Test against the splitting value. The primitive value is tested against the enclosing-box center.
		// [We only need an approximate partition of the enclosing box here.]
		float PrimitiveValue = builder->GetSplittingValue(Index, axis);

		// Reorganize the list of indices in this order: positive - negative.
		if(PrimitiveValue > SplitValue)
		{
			// Swap entries
			PxU32 Tmp = indices[GetNodePrimitives()+i];
			indices[GetNodePrimitives()+i] = indices[GetNodePrimitives()+NbPos];
			indices[GetNodePrimitives()+NbPos] = Tmp;
			// Count primitives assigned to positive space
			NbPos++;
		}
	}
	return NbPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Subdivides the node.
 *	
 *	          N
 *	        /   \
 *	      /       \
 *	   N/2         N/2
 *	  /   \       /   \
 *	N/4   N/4   N/4   N/4
 *	(etc)
 *
 *	A well-balanced tree should have a O(log n) depth.
 *	A degenerate tree would have a O(n) depth.
 *	Note a perfectly-balanced tree is not well-suited to collision detection anyway.
 *
 *	\param		builder		[in] the tree builder
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBTreeNode::Subdivide(const PxBounds3& exactBounds, AABBTreeBuilder* builder, PxU32* indices)
{
	PX_ASSERT(builder);

	// Stop subdividing if we reach a leaf node. This is always performed here,
	// else we could end in trouble if user overrides this.
	if(GetNbBuildPrimitives()==1)
	{
		setNbRunTimePrimitives(GetNbBuildPrimitives());
		return true;
	}

	// Let the user validate the subdivision
	if(!builder->ValidateSubdivision(GetNbBuildPrimitives()))
	{
		setNbRunTimePrimitives(GetNbBuildPrimitives());
		return true;
	}

	bool ValidSplit = true;	// Optimism...
	PxU32 NbPos=0;
	if(builder->mSettings.mRules & SPLIT_LARGEST_AXIS)
	{
		// Find the largest axis to split along
		PxVec3 Extents = exactBounds.getExtents(); // Box extents
		PxU32 Axis	= Ps::largestAxis(Extents); // Index of largest axis

		// Split along the axis
		NbPos = Split(exactBounds, Axis, builder, indices);

		// Check split validity
		if(!NbPos || NbPos==GetNbBuildPrimitives())	ValidSplit = false;
	}
	else if(builder->mSettings.mRules & SPLIT_SPLATTER_POINTS)
	{
		// Compute the means
		PxU32 numPrims = GetNbBuildPrimitives();
		Vec3V nbBuildSplat = V3Load(float(numPrims));
		Vec3V Means = Vec3V_From_FloatV(FZero());
		const PxU32* Prims = indices+GetNodePrimitives();
		const PxU32* Last = indices+GetNodePrimitives() + numPrims;
		while(Prims<Last)
		{
			PxU32 Index = *Prims++;
			Vec3V Tmp = builder->GetSplittingValues(Index);
			Means = V3Add(Means, Tmp);
		}
		Means = V3Mul(Means, V3Recip(nbBuildSplat)); // Means/=float(GetNbBuildPrimitives());

		// Compute variances
		Vec3V Vars = Vec3V_From_FloatV(FZero());
		Prims = indices+GetNodePrimitives();
		while(Prims!=Last)
		{
			const PxU32 Index = *Prims++;
			Vec3V C = builder->GetSplittingValues(Index);
			Vec3V cm = V3Sub(C, Means);
			Vars = V3Add(Vars, V3Mul(cm, cm)); //Vars.x += (C.x - Means.x)*(C.x - Means.x);
		}
		Vars = V3Mul(Vars, V3Recip(V3Sub(nbBuildSplat, Vec3V_From_FloatV(FOne()))));//Vars/=float(GetNbBuildPrimitives()-1);

		// Choose axis with greatest variance
		PxVec3 Vars3;
		V3StoreU(Vars, Vars3);
		PxU32 Axis = Ps::largestAxis(Vars3);

		// Split along the axis
		NbPos = Split(exactBounds, Axis, builder, indices);

		// Check split validity
		if(!NbPos || NbPos==numPrims)
			ValidSplit = false;
	}
	else if(builder->mSettings.mRules & SPLIT_BALANCED)
	{
		// Test 3 axis, take the best
		float Results[3];
		NbPos = Split(exactBounds, 0, builder, indices);	Results[0] = float(NbPos)/float(GetNbBuildPrimitives());
		NbPos = Split(exactBounds, 1, builder, indices);	Results[1] = float(NbPos)/float(GetNbBuildPrimitives());
		NbPos = Split(exactBounds, 2, builder, indices);	Results[2] = float(NbPos)/float(GetNbBuildPrimitives());
		Results[0]-=0.5f;	Results[0]*=Results[0];
		Results[1]-=0.5f;	Results[1]*=Results[1];
		Results[2]-=0.5f;	Results[2]*=Results[2];
		PxU32 Min=0;
		if(Results[1]<Results[Min])
			Min = 1;
		if(Results[2]<Results[Min])
			Min = 2;
		
		// Split along the axis
		NbPos = Split(exactBounds, Min, builder, indices);

		// Check split validity
		if(!NbPos || NbPos==GetNbBuildPrimitives())	ValidSplit = false;
	}
	else if(builder->mSettings.mRules & SPLIT_BEST_AXIS)
	{
		// Test largest, then middle, then smallest axis...

		// Sort axis
		PxVec3 Extents = exactBounds.getExtents();	// Box extents
		PxU32 SortedAxis[] = { 0, 1, 2 };
		float* Keys = (float*)&Extents.x;
		for(PxU32 j=0;j<3;j++)
		{
			for(PxU32 i=0;i<2;i++)
			{
				if(Keys[SortedAxis[i]]<Keys[SortedAxis[i+1]])
				{
					PxU32 Tmp = SortedAxis[i];
					SortedAxis[i] = SortedAxis[i+1];
					SortedAxis[i+1] = Tmp;
				}
			}
		}

		// Find the largest axis to split along
		PxU32 CurAxis = 0;
		ValidSplit = false;
		while(!ValidSplit && CurAxis!=3)
		{
			NbPos = Split(exactBounds, SortedAxis[CurAxis], builder, indices);
			// Check the subdivision has been successful
			if(!NbPos || NbPos==GetNbBuildPrimitives())
				CurAxis++;
			else
				ValidSplit = true;
		}
	}
	else if(builder->mSettings.mRules & SPLIT_FIFTY)
	{
		// Don't even bother splitting (mainly a performance test)
		NbPos = GetNbBuildPrimitives()>>1;
	}
	else
	{
		PX_ASSERT(0 && "Unknown split rule - number of primitives can be clipped.");
		setNbRunTimePrimitives(GetNbBuildPrimitives());
		return false;
	}

	// Check the subdivision has been successful
	if(!ValidSplit)
	{
		// Here, all boxes lie in the same sub-space. Two strategies:
		// - if we are over the split limit, make an arbitrary 50-50 split
		// - else stop subdividing
		setNbRunTimePrimitives(GetNbBuildPrimitives());
		if(GetNbBuildPrimitives()>builder->mSettings.mLimit)
		{
			builder->IncreaseNbInvalidSplits();
			NbPos = GetNbBuildPrimitives()>>1;
		}
		else
		{
			PX_ASSERT(builder->mSettings.mLimit < PxU32(1<<NBRUNTIME_BITS));
			return true;
		}
	}

	// Now create children and assign their pointers.
	// We use a pre-allocated linear pool for complete trees [Opcode 1.3]
	AABBTreeNode* base = builder->mNodeBase;
	PX_ASSERT(base && "Only node pool-relative AABB trees are supported.");
	PxU32 savePrimitives = GetNodePrimitives(); // save the primitives ptr since it will be overwritten by GetNodePrimitives() (stored in a union)
	ClearLeaf(); // clear leaf flag since it's no longer a leaf
	SetPos(builder->GetCount()); // set pointer to two adjacent nodes

	// Update stats
	builder->IncreaseCount(2);

	// Assign children
	AABBTreeNode* Pos = (AABBTreeNode*)GetPos(base);
	AABBTreeNode* Neg = (AABBTreeNode*)GetNeg(base);
	Pos->setPosOrNodePrimitives(savePrimitives);
	Pos->setNbBuildPrimitivesOrParent(NbPos);
	Pos->setNbRunTimePrimitives(Pos->GetNbBuildPrimitives()); // can be clipped but it's ok here
	Pos->SetLeaf(); // mark as leaf for now, may be cleared during further subdivision
	Neg->setPosOrNodePrimitives(savePrimitives+NbPos);
	Neg->setNbBuildPrimitivesOrParent(GetNbBuildPrimitives()-NbPos);
	Neg->setNbRunTimePrimitives(Neg->GetNbBuildPrimitives()); // can be clipped but it's ok here
	Neg->SetLeaf(); // mark as leaf for now, may be cleared during further subdivision
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Recursive hierarchy building in a top-down fashion.
 *	\param		builder		[in] the tree builder
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static PxU32 Count=0;

// here parentCenter, parentExtents are conservative bounds (decompressed so far)
void AABBTreeNode::_BuildHierarchy(AABBTreeBuilder* builder, PxU32* indices)
{
	Count++;

	// 1) Compute the exact global box for current node
	PxBounds3 exactBounds;
	Vec3V bMin, bMax;
	builder->ComputeGlobalBox(indices+GetNodePrimitives(), GetNbBuildPrimitives(), exactBounds, &bMin, &bMax);

	// 2) Subdivide current node
	SetLeaf();
	Subdivide(exactBounds, builder, indices); // subdivide will clear the leaf flag in case the node was split

	Compress<1>(bMin, bMax);

	AABBTreeNode* base = builder->mNodeBase;
	// 3) Recurse
	if (!IsLeaf())
	{
		AABBTreeNode* Pos = (AABBTreeNode*)GetPos(base);
		AABBTreeNode* Neg = (AABBTreeNode*)GetNeg(base);
		PX_ASSERT(this >= base && (this-base)<1024*1024); // sanity check
		PX_ASSERT(Pos > this && Neg > this);
		if(Pos != base)
		{
			Pos->_BuildHierarchy(builder, indices);
			Pos->SetParent(PxU32(this-base)); // has to be done after _BuildHierarchy since GetNbBuildPrimitives() is in a union with mParent
		}
		if(Neg != base)
		{
			Neg->_BuildHierarchy(builder, indices);
			Neg->SetParent(PxU32(this-base)); // has to be done after _BuildHierarchy since GetNbBuildPrimitives() is in a union with mParent
		}
	}

	builder->mTotalPrims += GetNbBuildPrimitives();
	SetParent(PxU32(this-base)); // overwrites GetNbBuildPrimitives() which we should not be using anymore
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTree::AABBTree() : mIndices(NULL), mPool(NULL), mRefitHighestSetWord(0), mTotalNbNodes(0), mTotalPrims(0)
{
#ifdef SUPPORT_PROGRESSIVE_BUILDING
	mStack = NULL;
#endif
#ifdef SUPPORT_UPDATE_ARRAY
	mNbRefitNodes	= 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AABBTree::~AABBTree()
{
	Release();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Releases the tree.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AABBTree::Release()
{
#ifdef SUPPORT_PROGRESSIVE_BUILDING
	PX_DELETE_AND_RESET(mStack);
#endif
	PX_DELETE_ARRAY(mPool); 
	PX_FREE_AND_RESET(mIndices);
	mRefitBitmask.ClearAll();
	mRefitHighestSetWord = 0;
#ifdef SUPPORT_UPDATE_ARRAY
	mNbRefitNodes	= 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Builds a generic AABB tree from a tree builder.
 *	\param		builder		[in] the tree builder
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBTree::Build(AABBTreeBuilder* builder)
{
	//gBuildCalls++;
	// Checkings
	if(!builder || !builder->mNbPrimitives)
		return false;

	// Release previous tree
	Release();

	// Init stats
	builder->SetCount(1);
	builder->SetNbInvalidSplits(0);

	// Initialize indices. This list will be modified during build.
	mIndices = (PxU32*)PX_ALLOC(sizeof(PxU32)*builder->mNbPrimitives, PX_DEBUG_EXP("AABB tree indices"));
	// Identity permutation
	for(PxU32 i=0;i<builder->mNbPrimitives;i++)
		mIndices[i] = i;

	{
		// Allocate a pool of nodes
		mPool = PX_NEW(AABBTreeNode)[builder->mNbPrimitives*2 - 1];
		builder->mNodeBase = mPool;
	}

	// Setup initial node. Here we have a complete permutation of the app's primitives.
	mPool->setPosOrNodePrimitives(0);
	mPool->setNbBuildPrimitivesOrParent(builder->mNbPrimitives);

	// Build the hierarchy
	builder->mInitNode = true;	// [Opcode 1.4]
	Count = 0;
	mPool->SetLeaf();
	mPool->_BuildHierarchy(builder, mIndices);

	// Get back total number of nodes
	mTotalNbNodes	= builder->GetCount();
	mTotalPrims		= builder->mTotalPrims;

	// For complete trees, check the correct number of nodes has been created [Opcode 1.3]
	if(mPool && builder->mSettings.mLimit==1)
		PX_ASSERT(mTotalNbNodes==builder->mNbPrimitives*2 - 1);

#ifdef PX_DEBUG
	Validate();
#endif

	return true;
}

#ifdef SUPPORT_PROGRESSIVE_BUILDING
static PxU32 IncrementalBuildHierarchy(FIFOStack2& stack, AABBTreeNode* node, AABBTreeNode* parent, AABBTreeBuilder* builder, PxU32* indices)
{
	//gProgressiveBuildCalls ++;
	// 1) Compute the global box for current node. The box is stored in mBV.
	PX_ASSERT(node->IsLeaf());
	PxBounds3 exactBounds;
	Vec3V bMin, bMax;
	builder->ComputeGlobalBox(node->GetPrimitives(indices), node->GetNbBuildPrimitives(), exactBounds, &bMin, &bMax);

	// 2) Subdivide current node
	node->SetLeaf();
	node->Subdivide(exactBounds, builder, indices);

	node->Compress<1>(bMin, bMax);

	AABBTreeNode* base = builder->mNodeBase;
	// 3) Recurse
	if (!node->IsLeaf())
	{
		AABBTreeNode* Pos = (AABBTreeNode*)node->GetPos(base);
		AABBTreeNode* Neg = (AABBTreeNode*)node->GetNeg(base);
		PX_ASSERT(node >= base && (node-base)<1024*1024); // sanity check
		PX_ASSERT(Pos > node && Neg > node);
		// need to push parent along with node ptr
		// since we can't set mParent here since it shares memory with, GetNbBuildPrimitives(), can only patch later
		if(Pos != base)
			stack.Push(Pos, node);
		if(Neg != base)
			stack.Push(Neg, node);
	}

	PxU32 saveBuildPrims = node->GetNbBuildPrimitives();
	builder->mTotalPrims += saveBuildPrims;
	// current node is finalized for build purposes (not on the stack) -> set the parent pointer
	node->SetParent(PxU32(parent-base)); // use parent from the stack, overwrite GetNbBuildPrimitives() which we should not be using anymore
	return saveBuildPrims;
}

PxU32 AABBTree::ProgressiveBuild(AABBTreeBuilder* builder, PxU32 progress, PxU32 limit)
{
	if(progress==0)
	{
		// Checkings
		if(!builder || !builder->mNbPrimitives)
			return PX_INVALID_U32;

		// Release previous tree
		Release();

		// Init stats
		builder->SetCount(1);
		builder->SetNbInvalidSplits(0);

		// Initialize indices. This list will be modified during build.
		mIndices = (PxU32*)PX_ALLOC(sizeof(PxU32)*builder->mNbPrimitives, PX_DEBUG_EXP("AABB tree indices"));
		// Identity permutation
		for(PxU32 i=0;i<builder->mNbPrimitives;i++)
			mIndices[i] = i;

		// Use a linear array for complete trees (since we can predict the final number of nodes) [Opcode 1.3]
		{
			// Allocate a pool of nodes
			mPool = PX_NEW(AABBTreeNode)[builder->mNbPrimitives*2 - 1];

			builder->mNodeBase = mPool;	// ###
		}
		builder->mInitNode = true;	// [Opcode 1.4]

		// Setup initial node. Here we have a complete permutation of the app's primitives.
		mPool->setPosOrNodePrimitives(0);
		mPool->setNbBuildPrimitivesOrParent(builder->mNbPrimitives);
		mPool->SetLeaf();

		mStack = PX_NEW(FIFOStack2);
		AABBTreeNode* FirstNode = mPool;
		mStack->Push(FirstNode, FirstNode);
		return progress++;
	}
	else if(progress==1)
	{
		PxU32 stackCount = mStack->GetNbEntries();
		if(stackCount)
		{
			PxU32 Total = 0;
			const PxU32 Limit = limit;
			while(Total<Limit)
			{
				AABBTreeNode* Entry, *parent;
				if(mStack->Pop(Entry, parent))
					Total += IncrementalBuildHierarchy(*mStack, Entry, parent, builder, mIndices);
				else
					break;
			}
			return progress;
		}

		// Get back total number of nodes
		mTotalNbNodes	= builder->GetCount();
		mTotalPrims		= builder->mTotalPrims;

		// For complete trees, check the correct number of nodes has been created [Opcode 1.3]
		if(mPool && builder->mSettings.mLimit==1)	PX_ASSERT(mTotalNbNodes==builder->mNbPrimitives*2 - 1);

		PX_DELETE_AND_RESET(mStack);

		return 0;	// Done!
	}
	return PX_INVALID_U32;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes the depth of the tree.
 *	A well-balanced tree should have a log(n) depth. A degenerate tree O(n) depth.
 *	\return		depth of the tree
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PxU32 AABBTree::ComputeDepth() const
{
	return Walk(NULL, NULL);	// Use the walking code without callback
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Walks the tree, calling the user back for each node.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PxU32 AABBTree::Walk(WalkingCallback callback, void* user_data) const
{
	// Call it without callback to compute maximum depth
	PxU32 MaxDepth = 0;
	PxU32 CurrentDepth = 0;

	struct Local
	{
		static void _Walk(
			const AABBTreeNode* allNodes, const AABBTreeNode* current_node,
			PxU32& max_depth, PxU32& current_depth, WalkingCallback callback, void* user_data)
		{
			// Entering a new node => increase depth
			current_depth++;

			// Keep track of maximum depth
			if(current_depth>max_depth)
				max_depth = current_depth;

			// Callback
			if(callback && !(callback)(current_node, current_depth, user_data))
				return;

			// Recurse
			if(current_node->GetPos(allNodes) != allNodes)
			{
				_Walk(allNodes, current_node->GetPos(allNodes), max_depth, current_depth, callback, user_data);
				current_depth--;
			}
			if(current_node->GetNeg(allNodes) != allNodes)
			{
				_Walk(allNodes, current_node->GetNeg(allNodes), max_depth, current_depth, callback, user_data);
				current_depth--;
			}
		}
	};

	Local::_Walk(mPool, mPool, MaxDepth, CurrentDepth, callback, user_data);
	return MaxDepth;
}

void AABBTree::Walk2(WalkingCallback callback, void* user_data) const
{
	if(!callback)
		return;
	struct Local
	{
		static void _Walk(const AABBTreeNode* allNodes, const AABBTreeNode* current_node, WalkingCallback callback, void* user_data)
		{
			const AABBTreeNode* P = current_node->GetPos(allNodes);
			const AABBTreeNode* N = current_node->GetNeg(allNodes);

			if(P != allNodes && !(callback)(P, 0, user_data))
				return;
			if(N != allNodes && !(callback)(N, 0, user_data))
				return;

			if(P != allNodes)
				_Walk(allNodes, P, callback, user_data);
			if(N != allNodes)
				_Walk(allNodes, N, callback, user_data);
		}
	};
	if(!(callback)(GetNodes(), 0, user_data))
		return;
	Local::_Walk(GetNodes(), GetNodes(), callback, user_data);
}

/*static PX_INLINE Ps::IntBool SameBoxes(const PxBounds3& a, const PxBounds3& b)
{
	if(a.minimum[0] != b.minimum[0])	return Ps::IntFalse;
	if(a.minimum[1] != b.minimum[1])	return Ps::IntFalse;
	if(a.minimum[2] != b.minimum[2])	return Ps::IntFalse;
	if(a.maximum[0] != b.maximum[0])	return Ps::IntFalse;
	if(a.maximum[1] != b.maximum[1])	return Ps::IntFalse;
	if(a.maximum[2] != b.maximum[2])	return Ps::IntFalse;
	return Ps::IntTrue;
//	return a.IsInside(b);
}*/

// A fast inlined version of this stuff
static PX_FORCE_INLINE void ComputeUnionBox(Vec3V* aResultMin, Vec3V* aResultMax, const PxU32* primitives, PxU32 nb_prims, const PxBounds3* boxes)
{
	Vec3V resultMin, resultMax;
	if(!nb_prims)
	{
		// Might happen after a node has been invalidated
		resultMin = V3LoadU(PxVec3(0.25f * PX_AABB_COMPRESSION_MAX));
		resultMax = V3LoadU(PxVec3(-0.25f * PX_AABB_COMPRESSION_MAX));
	}
	else
	{
		resultMin = V3LoadU(boxes[*primitives].minimum);
		resultMax = V3LoadU(boxes[*primitives].maximum);

		if(nb_prims>1)
		{
			const PxU32* Last = primitives + nb_prims;
			primitives++;

			while(primitives!=Last)
			{
				Vec3V mn = V3LoadU(boxes[*primitives].minimum);
				Vec3V mx = V3LoadU(boxes[*primitives++].maximum);
				resultMin = V3Min(resultMin, mn);
				resultMax = V3Max(resultMax, mx);
			}
		}
	}

	*aResultMin = resultMin;
	*aResultMax = resultMax;
}

bool AABBTree::Refit2(AABBTreeBuilder* builder, PxU32* indices)
{
	if(!builder)
		return false;

	PX_ASSERT(mPool);

	const PxBounds3* Boxes = builder->mAABBArray;

	// Bottom-up update
	PxU32 Index = mTotalNbNodes;
	while(Index--)
	{
		AABBTreeNode& Current = mPool[Index];
		if(Index)
			Ps::prefetch(mPool + Index - 1);

		if(Current.IsLeaf())
		{
			// compute AABB for the bottom node
			Vec3V mn, mx;
			ComputeUnionBox(&mn, &mx, Current.GetPrimitives(indices), Current.GetNbRuntimePrimitives(), Boxes);
			Current.Compress<1>(mn, mx);
		}
		else
		{
			const AABBTreeNode* pos = Current.GetPos(mPool);
			const AABBTreeNode* neg = Current.GetNeg(mPool);
			Vec3V negCenter, negExtents, posCenter, posExtents;
			neg->GetAABBCenterExtentsV(&negCenter, &negExtents);
			pos->GetAABBCenterExtentsV(&posCenter, &posExtents);

			// compute bounds in min/max form around both nodes
			Vec3V mnPN = V3Min(V3Sub(posCenter, posExtents), V3Sub(negCenter, negExtents));
			Vec3V mxPN = V3Max(V3Add(posCenter, posExtents), V3Add(negCenter, negExtents));

			// invoke compression
			Current.Compress<1>(mnPN, mxPN);
		}
	}

	return true;
}

#ifndef SUPPORT_REFIT_BITMASK
bool AABBTree::	Refit3(PxU32 nb_objects, const PxBounds3* boxes, const Container& indices)
{
	PX_ASSERT(mPool);

	if(1)
	{
//		printf("Size: %d\n", sizeof(AABBTreeNode));
		PxU32 Nb = indices.GetNbEntries();
		if(!Nb)	return true;

		PxU32 Index = mTotalNbNodes;

		// ### those flags could be written directly, no need for the indices array
		bool* Flags = (bool*)PxAlloca(Index);
		PxMemZero(Flags, Index);

		const PxU32* in = indices.GetEntries();
		while(Nb--)
		{
			PxU32 Index = *in++;
			PX_ASSERT(Index<mTotalNbNodes);
			const AABBTreeNode* Current = mPool + Index;
			for (;;)
			{
				PxU32 CurrentIndex = PxU32(size_t(Current) - size_t(mPool)) / sizeof(AABBTreeNode);
				if(Flags[CurrentIndex])
				{
					// We can early exit if we already visited the node!
					break;
				}
				else
				{
					Flags[CurrentIndex] = true;
					const AABBTreeNode* parent = Current->GetParent(mPool);
					if (Current == parent)
						break;
					Current = parent;
				}

			}
		}

		while(Index--)
		{
			if(Flags[Index])
			{
				AABBTreeNode* Current = mPool + Index;
				if(Current->IsLeaf())
				{
					// compute AABB for the bottom node
					Vec3V mn, mx;
					ComputeUnionBox(&mn, &mx, Current->GetPrimitives(indices.GetEntries()), Current->GetNbRuntimePrimitives(), boxes);
					PX_ASSERT(FStore(V3GetX(mx)) >= FStore(V3GetX(mn)));
					Current->Compress<1>(mn, mx);
				}
				else
				{
					const AABBTreeNode* pos = Current->GetPos(mPool);
					const AABBTreeNode* neg = Current->GetNeg(mPool);
					Vec3V negCenter, negExtents, posCenter, posExtents;
					neg->GetAABBCenterExtentsV(&negCenter, &negExtents);
					pos->GetAABBCenterExtentsV(&posCenter, &posExtents);

					// compute bounds in min/max form around both nodes
					Vec3V mnPN = V3Min(V3Sub(posCenter, posExtents), V3Sub(negCenter, negExtents));
					Vec3V mxPN = V3Max(V3Add(posCenter, posExtents), V3Add(negCenter, negExtents));
					PX_ASSERT(FStore(V3GetX(mxPN)) >= FStore(V3GetX(mnPN)));

					// invoke compression
					Current->Compress<1>(mnPN, mxPN);
				}
			}
		}
	}
	return true;
}
#endif

void AABBTree::MarkForRefit(PxU32 index)
{
	if(!mRefitBitmask.GetBits())
		mRefitBitmask.Init(mTotalNbNodes);

	PX_ASSERT(index<mTotalNbNodes);

	const AABBTreeNode* Current = mPool + index;
	Ps::prefetch(Current);
	while(1)
	{
		const PxU32 CurrentIndex = PxU32(Current - mPool);
		PX_ASSERT(CurrentIndex<mTotalNbNodes);
		if(mRefitBitmask.IsSet(CurrentIndex))
		{
			// We can early exit if we already visited the node!
			return;
		}
		else
		{
			mRefitBitmask.SetBit(CurrentIndex);
			const PxU32 currentMarkedWord = CurrentIndex>>5;
			mRefitHighestSetWord = PxMax(mRefitHighestSetWord, currentMarkedWord);
#ifdef SUPPORT_UPDATE_ARRAY
			if(mNbRefitNodes<SUPPORT_UPDATE_ARRAY)
				mRefitArray[mNbRefitNodes]=CurrentIndex;
			mNbRefitNodes++;
#endif
			const AABBTreeNode* parent = Current->GetParent(mPool);
			Ps::prefetch(parent);
			PX_ASSERT(parent == mPool || parent < Current);
			if (Current == parent)
				break;
			Current = parent;
		}
	}
}

static PX_FORCE_INLINE void refitNode(	AABBTreeNode* PX_RESTRICT pool, PxU32 index,
										const PxBounds3* PX_RESTRICT boxes, PxU32* PX_RESTRICT indices,
										AABBTreeNode*& todoWriteback, AABBTreeNode*& todoCompress,
										VecU32V& prevScale, Vec3V& prevXYZ,
										Vec3V& bMin, Vec3V& bMax,
										Vec3V& wMin, Vec3V& wMax)
{
	// invoke compression on previous node to avoid LHS
	if(todoWriteback)
	{
		todoWriteback->WriteBack(prevScale, prevXYZ);
		todoWriteback = NULL;
	}
	if(todoCompress)
	{
		todoCompress->Compress<0>(bMin, bMax, &prevScale, &prevXYZ);
		// need to save the bounds because we could be reading from these in the middle of writeback pipeline
		wMin = bMin; wMax = bMax;
		todoWriteback = todoCompress;
		todoCompress = NULL;
	}

	AABBTreeNode* PX_RESTRICT Current = pool + index;
	if(Current->IsLeaf())
	{
		ComputeUnionBox(&bMin, &bMax, Current->GetPrimitives(indices), Current->GetNbRuntimePrimitives(), boxes);
		todoCompress = Current;
	}
	else
	{
		Vec3V negCenter, negExtents, posCenter, posExtents;
		Vec3V negMin, negMax, posMin, posMax;

		const AABBTreeNode* pos = Current->GetPos(pool);
		// since we are manually staggering writebacks in 2 stages to avoid LHS,
		// we need to make sure we read back the most up-to-date data..
		// since todoCompress is always NULL we don't need to check for it, it's left here for clarity
		//if (pos == todoCompress) { posMin = bMin; posMax = bMax; } else
		if(pos == todoWriteback)
		{
			posMin = wMin; posMax = wMax; // reuse from writeback pipeline
		}
		else
		{
			pos->GetAABBCenterExtentsV(&posCenter, &posExtents);
			posMin = V3Sub(posCenter, posExtents);
			posMax = V3Add(posCenter, posExtents);
		}

		// do the same for neg child
		const AABBTreeNode* neg = Current->GetNeg(pool);
		//if (neg == todoCompress) { negMin = bMin; negMax = bMax; } else // reuse bMin, bMax from the pipeline
		if(neg == todoWriteback)
		{
			negMin = wMin; negMax = wMax; // reuse wMin, wMax from the pipeline
		}
		else
		{
			neg->GetAABBCenterExtentsV(&negCenter, &negExtents);
			negMin = V3Sub(negCenter, negExtents);
			negMax = V3Add(negCenter, negExtents);
		}

		// compute bounds in min/max form around both nodes
		bMin = V3Min(negMin, posMin);
		bMax = V3Max(negMax, posMax);

		todoCompress = Current;
	}
}

void AABBTree::RefitMarked(PxU32 nb_objects, const PxBounds3* boxes, PxU32* indices)
{
	PX_UNUSED(nb_objects);

	if(!mRefitBitmask.GetBits())
		return;	// No refit needed

	VecU32V prevScale;
	Vec3V prevXYZ;
	AABBTreeNode* todoWriteback = NULL;
	AABBTreeNode* todoCompress = NULL;
	Vec3V bMin = Vec3V_From_FloatV(FZero()), bMax = Vec3V_From_FloatV(FZero()); // only have to initialize due to compiler warning
	Vec3V wMin = Vec3V_From_FloatV(FZero()), wMax = Vec3V_From_FloatV(FZero()); // only have to initialize due to compiler warning

#ifdef SUPPORT_UPDATE_ARRAY
	PxU32 nbRefitNodes = mNbRefitNodes;
	mNbRefitNodes = 0;
	if(nbRefitNodes<=SUPPORT_UPDATE_ARRAY)
	{
		PxU32* ranks0 = (PxU32*)PxAlloca(nbRefitNodes*sizeof(PxU32));
		PxU32* ranks1 = (PxU32*)PxAlloca(nbRefitNodes*sizeof(PxU32));
		StackRadixSort(rs, ranks0, ranks1);
		const PxU32* sorted = rs.Sort(mRefitArray, nbRefitNodes).GetRanks();

		for(PxU32 i=0;i<nbRefitNodes;i++)
		{
			const PxU32 Index = mRefitArray[sorted[nbRefitNodes-1-i]];

			Ps::prefetch(mPool+Index);

			PX_ASSERT(mRefitBitmask.IsSet(Index));

			mRefitBitmask.ClearBit(Index);

			refitNode(mPool, Index, boxes, indices, todoWriteback, todoCompress, prevScale, prevXYZ, bMin, bMax, wMin, wMax);
		}
	}
	else
#endif
	{
		const PxU32* Bits = mRefitBitmask.GetBits();
//		PxU32 Size = mRefitBitmask.GetSize();
		PxU32 Size = mRefitHighestSetWord+1;
#ifdef _DEBUG
		if(1)
		{
			const PxU32 TotalSize = mRefitBitmask.GetSize();
			for(PxU32 i=Size;i<TotalSize;i++)
			{
				PX_ASSERT(!Bits[i]);
			}
		}
#endif
		while(Size--)
		{
			// Test 32 bits at a time
			if(!Bits[Size])
				continue;

			PxU32 Index = (Size+1)<<5;
			PxU32 Count=32;
			while(Count--)
			{
				Index--;
				Ps::prefetch(mPool+Index);

				if(mRefitBitmask.IsSet(Index))
				{
					mRefitBitmask.ClearBit(Index);

					refitNode(mPool, Index, boxes, indices, todoWriteback, todoCompress, prevScale, prevXYZ, bMin, bMax, wMin, wMax);
				}
			}
		}
		mRefitHighestSetWord = 0;
	}

	if (todoWriteback)
		todoWriteback->WriteBack(prevScale, prevXYZ);
	if (todoCompress)
	{
		todoCompress->Compress<0>(bMin, bMax, &prevScale, &prevXYZ);
		todoCompress->WriteBack(prevScale, prevXYZ);
	}
}

void AABBTree::ShiftOrigin(const PxVec3& shift)
{
	for(PxU32 i=0; i < mTotalNbNodes; i++)
	{
		AABBTreeNode& Current = mPool[i];
		if((i+1) < mTotalNbNodes)
			Ps::prefetch(mPool + i + 1);

		Vec3V centerV, extentsV, shiftV, minV, maxV;
		shiftV = V3LoadU(shift);
		Current.GetAABBCenterExtentsV(&centerV, &extentsV);

		centerV = V3Sub(centerV, shiftV);
		minV = V3Sub(centerV, extentsV);
		maxV = V3Add(centerV, extentsV);

		Current.Compress<1>(minV, maxV);
	}
}

#ifdef PX_DEBUG
void AABBTree::Validate() const
{
	struct Local
	{
		static void _Walk(const AABBTreeNode* parentNode, const AABBTreeNode* currentNode, const AABBTreeNode* root)
		{
			PX_ASSERT(parentNode == currentNode || parentNode < currentNode);
			PX_ASSERT(currentNode->GetParent(root) == parentNode);
			if (!currentNode->IsLeaf())
			{
				_Walk(currentNode, currentNode->GetPos(root), root);
				_Walk(currentNode, currentNode->GetNeg(root), root);
			}
		}
	};
	Local::_Walk(mPool, mPool, mPool);
}
#endif
