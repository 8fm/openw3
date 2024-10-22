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
#ifndef APEX_CSG_H
#define APEX_CSG_H


#include "PsShare.h"
#include "NxRenderMeshAsset.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace ApexCSG
{

class UserRandom
{
public:
	virtual	physx::PxU32	getInt() = 0;
	virtual physx::PxF32	getReal(physx::PxF32 min, physx::PxF32 max) = 0;
};


struct BSPBuildParameters
{
	/*
		Used for searching splitting planes.
		If NULL, a default random # generator will be used.
	 */
	UserRandom*		rnd;

	/*
		Mesh pre-processing.  The mesh is initially scaled to fit in a unit cube, then (if gridSize is not
		zero), the vertices of the scaled mesh are snapped to a grid of size 1/gridSize.
		A power of two is recommended.
		Default value = 65536.
	*/
	physx::PxU32	snapGridSize;

	/*
		At each step in the tree building process, the surface with maximum triangle area is compared
		to the other surface triangle areas.  If the maximum area surface is far from the "typical" set of
		surface areas, then that surface is chosen as the next splitting plane.  Otherwise, a random
		test set is chosen and a winner determined based upon the weightings below.
		The value logAreaSigmaThreshold determines how "atypical" the maximum area surface must be to
		be chosen in this manner.
		Default value = 2.0.
	 */
	physx::PxF32	logAreaSigmaThreshold;

	/*
		Larger values of testSetSize may find better BSP trees, but will take more time to create.
		testSetSize = 0 is treated as infinity (all surfaces will be tested for each branch).
	 */
	physx::PxU32	testSetSize;

	/*
		How much to weigh the relative number of triangle splits when searching for a BSP surface.
	 */
	physx::PxF32	splitWeight;

	/*
		How much to weigh the relative triangle imbalance when searching for a BSP surface.
	 */
	physx::PxF32	imbalanceWeight;

	/*
		The BSP representation of the mesh will be transformed from the space of the mesh input into IApexBSP::fromMesh
		using this transform.  By default, this is the identity transformation.  If the user wishes to use a
		different transformation, it may be set using internalTransform.  However, note that when combining
		BSPs using the IApexBSP::combine function, the two BSPs should use the same internal transform.  If they don't,
		the resulting behavior is not specified.  When a mesh is created using IApexBSP::toMesh, the inverse
		of the internal transform is applied to put the mesh back into the original space.

		A special value for internalTransform is the zero 4x4 matrix.  If this is used, an internal transform
		will be calculated in the IApexBSP::fromMesh function.  This may be read using IApexBSP::getInternalTransform(),
		and applied when creating other BSPs which are to be used in combine operations.
	 */
	physx::PxMat44	internalTransform;

	BSPBuildParameters()
	{
		setToDefault();
	}

	void	setToDefault()
	{
		rnd = NULL;
		snapGridSize = 65536;
		logAreaSigmaThreshold = (physx::PxF32)2.0;
		testSetSize = 10;
		splitWeight = (physx::PxF32)0.5;
		imbalanceWeight = 0;
		internalTransform = physx::PxMat44::createIdentity();
	}
};

struct BSPTolerances
{
	/*
		A unitless value (relative to mesh size) used to determine mesh triangle coplanarity during BSP building.
		Default value = 1.0e-6.
	 */
	physx::PxF32	linear;

	/*
		A threshold angle (in radians) used to determine mesh triangle coplanarity during BSP building.
		Default value = 1.0e-5.
	 */
	physx::PxF32	angular;

	/*
		A unitless value (relative to mesh size) used to determine triangle splitting during BSP building.
		Default value = 1.0e-9.
	 */
	physx::PxF32	base;

	/*
		A unitless value (relative to mesh size) used to determine a skin width for mesh clipping against BSP
		nodes during mesh creation from the BSP.
		Default value = 1.0e-13.
	 */
	physx::PxF32	clip;

	/*
		Mesh postprocessing.  A unitless value (relative to mesh size) used to determine merge tolerances for
		mesh clean-up after triangles have been clipped to BSP leaves.  A value of 0.0 disables this feature.
		Default value = 1.0e-6.
	 */
	physx::PxF32	cleaning;

	BSPTolerances()
	{
		setToDefault();
	}

	void	setToDefault()
	{
		linear = (physx::PxF32)1.0e-6;
		angular = (physx::PxF32)1.0e-5;
		base = (physx::PxF32)1.0e-9;
		clip = (physx::PxF32)1.0e-13;
		cleaning = (physx::PxF32)1.0e-6;
	}
};

extern BSPTolerances gDefaultTolerances;

struct Operation
{
	enum Enum
	{
		Empty_Set				= 0x0,	// constant
		All_Space				= 0x1,	// constant
		Set_A					= 0x2,	// unary
		Set_A_Complement		= 0x3,	// unary
		Set_B					= 0x4,	// unary
		Set_B_Complement		= 0x5,	// unary
		Exclusive_Or			= 0x6,
		Equivalent				= 0x7,
		Intersection			= 0x8,
		Intersection_Complement	= 0x9,
		A_Minus_B				= 0xA,
		A_Implies_B				= 0xB,
		B_Minus_A				= 0xC,
		B_Implies_A				= 0xD,
		Union					= 0xE,
		Union_Complement		= 0xF,

		NOP						= 0x80000000	// no op
	};
};


struct BSPVisualizationFlags
{
	enum Enum
	{
		OutsideRegions	= (1 << 0),
		InsideRegions	= (1 << 1),

		SingleRegion	= (1 << 16)
	};
};


struct BSPType
{
	enum Enum
	{
		Empty_Set,	// BSP has a single node, which is an outside leaf.  Therefore the inside region is the empty set.
		All_Space,	// BSP has a single node, which is an inside leaf.  Therefore the inside region is all of space.
		Nontrivial,	// BSP has more than a single node.
		Combined,	// BSP is the combination of two BSPs, ready for a CSG operation to define a single BSP.

		BSPTypeCount
	};
};


/*
	Memory cache for BSP construction.  Not global, so that concurrent calculations can use different pools.
 */
class IApexBSPMemCache
{
public:

	/*
		Deallocate all memory buffers.
	 */
	virtual void	clearAll() = 0;

	/*
		Deallocate only temporary data buffers.
	 */
	virtual void	clearTemp() = 0;

	/*
		Clean up.
	 */
	virtual	void	release() = 0;

protected:

	IApexBSPMemCache()	{}
	virtual			~IApexBSPMemCache()	{}
};


/*
	BSP interface.

	Convert a mesh into a BSP, perform boolean operations between BSPs, and extract the resulting mesh.
 */

class IApexBSP
{
public:
	/*
		Set the tolerances used for various aspects of BSP creation, merging, mesh creation, etc.
		Default values are those in BSPTolerances.
	*/
	virtual void			setTolerances(const BSPTolerances& tolerances) = 0;

	/*
		Construct a BSP from the given mesh, using the given parameters.
	 */
	virtual bool			fromMesh(const physx::Array<physx::NxExplicitRenderTriangle>& mesh, const BSPBuildParameters& params, physx::IProgressListener* progressListener = NULL) = 0;

	/*
		Construct a BSP from a convex polyhedron defined by a list of planes.
		See the definition of internalTransform in BSPBuildParameters.  The same meaning applies here.
	 */
	virtual bool			fromConvexPolyhedron(const physx::Array<physx::PxPlane>& poly, const physx::PxMat44& internalTransform = physx::PxMat44::createIdentity()) = 0;

	/*
		Build a combination of two BSPs (this and the passed-in bsp), upon which boolean operations of the two can be performed.
	 */
	virtual bool			combine(const IApexBSP& bsp) = 0;

	/*
		Build a BSP resulting from a boolean operation upon a combination.
		Note: you may do this "in place," i.e.
			bsp.op( bsp, operation );
		... in this case, bsp will no longer be a combined BSP.
	 */
	virtual bool			op(const IApexBSP& combinedBSP, Operation::Enum operation) = 0;

	/*
		This BSP is changed to its complement (inside <-> outside)
	 */
	virtual bool			complement() = 0;

	/*
		The transform from mesh space to BSP space.  This may be used in the BSPBuildParameters passed into fromMesh,
		in order to match the transform used for a combining mesh.
	 */
	virtual physx::PxMat44	getInternalTransform() const = 0;

	/*
		Returns an enum characterizing the BSP.  See BSPType.
	 */
	virtual BSPType::Enum	getType() const = 0;

	/*
		Returns the total surface area and volume of the regions designated to be on the given side.
		If this is a combined BSP, then you must provide a merge operation.  In this case,
		the BSP will not actually be merged, but the resulting area will be that of the
		merged BSP you would get if you did perform the merge with the op() function.
		If this is not a combined BSP and you provide a merge operation, it will be ignored.
	 */
	virtual void			getSurfaceAreaAndVolume(physx::PxF32& area, physx::PxF32& volume, bool inside, Operation::Enum operation = Operation::NOP) const = 0;

	/*
		Determines if given point is in an outside or inside leaf.
		If this is a combined BSP, then you must provide a merge operation.  In this case,
		the BSP will not actually be merged, but the resulting area will be that of the
		merged BSP you would get if you did perform the merge with the op() function.
		If this is not a combined BSP and you provide a merge operation, it will be ignored.
	*/
	virtual bool			pointInside(const physx::PxVec3& point, Operation::Enum operation = Operation::NOP) const = 0;

	/*
		Construct a mesh from the current BSP.
	 */
	virtual bool			toMesh(physx::Array<physx::NxExplicitRenderTriangle>& mesh) const = 0;

	/*
		Deep copy of given bsp.
		Input bsp may be the same as *this.
		The transform tm will be applied.
		A combined BSP may be copied.
	 */
	virtual	void			copy(const IApexBSP& bsp, const physx::PxMat44& tm = physx::PxMat44::createIdentity()) = 0;

	/*
		Decompose into disjoint islands.
		This BSP is not affected.
		The BSP is split into a set of BSPs, each representing one connected island.
		The set of BSPs is returned as the first BSP in the list, with access
		to the remainder of the list  through the getNext() and getPrev() functions.
		The BSP must be not be a combined BSP (getType() != BSPType::Combined).
		Returns this if the BSP is already an island.
		Returns NULL if the operation fails (e.g. this is a combined BSP).
	*/
	virtual IApexBSP*		decomposeIntoIslands() const = 0;

	/*
		If a BSP has been decomposed into islands, getNext() and getPrev() will iterate through the
		BSPs in the decomposition.  NULL is returned if an attempt is made to iterate past
		the beginning or end of the list.
	*/
	virtual IApexBSP*		getNext() const = 0;
	virtual IApexBSP*		getPrev() const = 0;

	/*
		Serialization.
	 */
	virtual void			serialize(physx::PxFileBuf& stream) const = 0;
	virtual void			deserialize(physx::PxFileBuf& stream) = 0;

	/*
		Visualization.
		Set flags to bits from BSPVisualizationFlags::Enum.
	 */
	virtual void			visualize(physx::NxApexRenderDebug& debugRender, physx::PxU32 flags, physx::PxU32 index = 0) const = 0;

	/*
		Clean up.
	 */
	virtual	void			release() = 0;

protected:

	IApexBSP()	{}
	virtual					~IApexBSP()	{}
};


// CSG Tools API

// Create a BSP memory cache to share among several BSPs
IApexBSPMemCache*
createBSPMemCache();

// Instantiate a BSP.  If cache = NULL, the BSP will create and own its own cache.
IApexBSP*
createBSP(IApexBSPMemCache* memCache = NULL, const physx::PxMat44& internalTransform = physx::PxMat44::createIdentity());

};	// namespace ApexCSG

#endif

#endif // #ifndef APEX_CSG_H
