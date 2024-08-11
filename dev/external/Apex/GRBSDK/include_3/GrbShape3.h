#ifndef GRBSHAPE3_H
#define GRBSHAPE3_H

#include "PxShape.h"
#include "GrbScene3.h"

namespace physx
{

//-----------------------------------------------------------------------------
#define SHAPE_API_UNDEF( x )	PX_ASSERT( 0 && "PxShape method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
class GrbShape3 : public PxShape //, public Ps::UserAllocated
{
public:
// PX_SERIALIZATION
	virtual	PxU32								getObjectSize()	const	{ return sizeof(*this);		}
//~PX_SERIALIZATION

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	GrbShape3(PxRigidActor& /*nxActor*/) : PxShape(PxConcreteType::eSHAPE, PxBaseFlag::eIS_RELEASABLE | PxBaseFlag::eOWNS_MEMORY) {}
#else
	GrbShape3(PxRigidActor& /*nxActor*/)		{}
#endif
	~GrbShape3()								{}

	//---------------------------------------------------------------------------------
	// PxShape implementation
	//---------------------------------------------------------------------------------
	virtual			void						release() = 0;

	virtual			PxGeometryType::Enum		getGeometryType() const = 0;

	virtual			void						setGeometry(const PxGeometry &) = 0;
	virtual			bool						getBoxGeometry(PxBoxGeometry&) const = 0;
	virtual			bool						getSphereGeometry(PxSphereGeometry&) const = 0;
	virtual			bool						getCapsuleGeometry(PxCapsuleGeometry&) const = 0;
	virtual			bool						getPlaneGeometry(PxPlaneGeometry&) const = 0;
	virtual			bool						getConvexMeshGeometry(PxConvexMeshGeometry& g) const = 0;
	virtual			bool						getTriangleMeshGeometry(PxTriangleMeshGeometry& g) const = 0;
	virtual			bool						getHeightFieldGeometry(PxHeightFieldGeometry& g) const = 0;

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual			PxRigidActor&				getActor() const = 0;
#else
	virtual			PxRigidActor*				getActor() const = 0;
#endif

	virtual			PxBounds3					getWorldBounds() const { SHAPE_API_UNDEF("getWorldBounds"); return PxBounds3(); }

	virtual			void						setLocalPose(const PxTransform& pose) = 0;
	virtual			PxTransform					getLocalPose() const = 0;

	virtual			void						setSimulationFilterData(const PxFilterData& data) = 0;
	virtual			PxFilterData				getSimulationFilterData() const = 0;
	virtual			void						resetFiltering() { SHAPE_API_UNDEF("resetFiltering"); }
	virtual			void						setQueryFilterData(const PxFilterData& data) = 0;
	virtual			PxFilterData				getQueryFilterData() const { SHAPE_API_UNDEF("getQueryFilterData"); return PxFilterData(); }

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual			void						setMaterials(PxMaterial*const* materials, PxU32 materialCount) = 0;
	virtual			PxU32						getNbMaterials()										const  = 0;
#else
	virtual			void						setMaterials(PxMaterial*const* materials, PxU16 materialCount) = 0;
	virtual			PxU16						getNbMaterials()										const  = 0;
#endif
	virtual			PxU32						getMaterials(PxMaterial** /*userBuffer*/, PxU32 /*bufferSize*/)	const	{ SHAPE_API_UNDEF("getMaterials"); return 0; }
	virtual			PxMaterial*					getMaterialFromInternalFaceIndex(PxU32 /*faceIndex*/)		const		{ SHAPE_API_UNDEF("getMaterialFromInternalFaceIndex"); return 0; }

	virtual			void						setContactOffset(PxReal) = 0;
	virtual			PxReal						getContactOffset() const = 0;

	virtual			void						setRestOffset(PxReal) = 0;
	virtual			PxReal						getRestOffset() const = 0;

	virtual			void						setFlag(PxShapeFlag::Enum flag, bool value) = 0;
	virtual			void						setFlags( PxShapeFlags inFlags ) = 0;
	virtual			PxShapeFlags				getFlags() const = 0;

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual			bool						isExclusive() const														{ SHAPE_API_UNDEF("isExclusive"); return false; }
#endif

	virtual			void					    setName(const char* /*debugName*/) { SHAPE_API_UNDEF("setName"); }
	virtual			const char*					getName() const { SHAPE_API_UNDEF("getName"); return 0; }
	
	//---------------------------------------------------------------------------------
	// Collision queries
	//---------------------------------------------------------------------------------

	virtual			PxU32						raycast(const PxVec3& /*rayOrigin*/, const PxVec3& /*rayDir*/, PxReal /*maxDist*/, PxSceneQueryFlags /*hintFlags*/,
														PxU32 /*maxHits*/, PxRaycastHit* /*rayHits*/, bool /*firstHit*/, const PxTransform* /*shapePose*/)	const { SHAPE_API_UNDEF("raycast"); return 0; }

	virtual			bool						overlap(const PxGeometry& /*otherGeom*/, const PxTransform& /*otherGeomPose*/, const PxTransform* /*shapePose*/)	const { SHAPE_API_UNDEF("overlap"); return false; }

	virtual			bool						sweep(	const PxVec3& /*unitDir*/, const PxReal /*distance*/, const PxGeometry& /*otherGeom*/, const PxTransform& /*otherGeomPose*/,
														PxSweepHit& /*sweepHit*/, PxSceneQueryFlags /*hintFlags*/, const PxTransform* /*shapePose*/)	const { SHAPE_API_UNDEF("sweep"); return false; }
};
//-----------------------------------------------------------------------------
}
#endif