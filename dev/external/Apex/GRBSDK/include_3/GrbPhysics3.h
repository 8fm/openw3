#ifndef GRBPHYSICS3_H
#define GRBPHYSICS3_H

#include <windows.h>
#undef min
#undef max
#include <delayimp.h>

#include "PxPhysics.h"
#include "common/PxTolerancesScale.h"
#include "foundation/PxFoundation.h"
#include "physxprofilesdk/PxProfileZoneManager.h"
#include "GrbScene3.h"
#include "GrbSceneDesc3.h"

#if (defined(PX_WINDOWS) || defined(PX_WIN8ARM))
	#if defined GRB_CORE_EXPORTS
		#define GRB_CORE_API __declspec(dllexport)
	#else
		#define GRB_CORE_API __declspec(dllimport)
	#endif
 #else
	#define GRB_CORE_API 
#endif

namespace physx
{

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
class PxClassCreationCallback; // forward reference now obsolete class
#endif

//-----------------------------------------------------------------------------
#define PHYSICS_API_UNDEF( x )	PX_ASSERT( 0 && "PxPhysics method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
// PhysX 3.x API for GRB
//-----------------------------------------------------------------------------
class GrbPhysics3 : public PxPhysics
{
public:

	GrbPhysics3(bool /*trackOustandingAllocations*/)	{}
	virtual ~GrbPhysics3()								{}

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual void getMetaData(PxSerialStream& /*stream*/)	const { PHYSICS_API_UNDEF( "getMetaData" ); }

	virtual bool									registerClass(PxType type, PxClassCreationCallback callback) = 0;
	virtual	PxUserReferences*						createUserReferences() { PHYSICS_API_UNDEF( "createUserReferences" ); return 0; }
	virtual	void									releaseUserReferences(PxUserReferences& /*ref*/) { PHYSICS_API_UNDEF( "releaseUserReferences" ); }
#endif

	virtual	PxCollection*							createCollection() { PHYSICS_API_UNDEF( "createCollection" ); return 0; }

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	//virtual PxSerializationManager*					getSerializeManager() { PHYSICS_API_UNDEF( "getSerializeManager" ); return 0; }

	virtual PxCollection*							createCollection(void* /*memBlock*/, const PxCollection* /*externalReferences*/ = NULL) { PHYSICS_API_UNDEF( "createCollection" ); return 0; }
	//virtual bool									serialize(PxOutputStream& /*output*/, PxCollection& /*collection*/, const PxCollection* /*externalReferences*/ = NULL, bool /*exportNames*/ = false) { PHYSICS_API_UNDEF( "serialize" ); return false; }
	//PX_DEPRECATED virtual bool						serialize(PxCollection& /*collection*/, PxSerialStream& /*output*/, const PxCollection* /*externalReferences*/ = NULL, bool /*exportNames*/ = false) { PHYSICS_API_UNDEF( "serialize" ); return false; }
	virtual bool									isSerializable(PxCollection& /*collection*/, const PxCollection* /*externalReferences*/ = NULL) const { PHYSICS_API_UNDEF( "isSerializable" ); return false; }
#endif

	virtual	void									releaseCollection(PxCollection&) { PHYSICS_API_UNDEF( "releaseCollection" ); }
	virtual	void									addCollection(const PxCollection& /*collection*/, PxScene& /*scene*/) { PHYSICS_API_UNDEF( "addCollection" ); }
	virtual	void									releaseCollected(PxCollection&, PxScene&){ PHYSICS_API_UNDEF( "releaseCollected" ); }
	virtual	void									release() = 0;
	virtual PxScene*								createScene(const PxSceneDesc & sceneDesc) = 0;
	virtual GrbScene3*								createScene(const GrbSceneDesc3 & sceneDesc,PxScene * pxScene) = 0;
	virtual PxU32									getNbScenes()			const	{ PHYSICS_API_UNDEF( "getNbScenes" ); return 0;}
	virtual	PxU32									getScenes(PxScene** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { PHYSICS_API_UNDEF( "getScenes" ); return 0;}
	virtual PxRigidStatic*							createRigidStatic(const PxTransform& pose) = 0;
	virtual PxRigidDynamic*							createRigidDynamic(const PxTransform& pose) = 0;

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual PxShape*								createShape(const PxGeometry& /*geometry*/, 
																PxMaterial*const * /*materials*/, 
																PxU16 /*materialCount*/, 
																bool /*isExclusive*/ = false,
																PxShapeFlags /*shapeFlags*/ = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE) = 0;
	virtual PxU32									getNbShapes() const { PHYSICS_API_UNDEF( "getNbShapes" ); return 0;}
	virtual	PxU32									getShapes(PxShape** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { PHYSICS_API_UNDEF( "getShapes" ); return 0;}
#endif

	virtual PxConstraint*							createConstraint(PxRigidActor* /*actor0*/, PxRigidActor* /*actor1*/, PxConstraintConnector& /*connector*/, const PxConstraintShaderTable& /*shaders*/, PxU32 /*dataSize*/)	{ PHYSICS_API_UNDEF( "createConstraint" ); return 0;}
	virtual PxArticulation*							createArticulation() { PHYSICS_API_UNDEF( "createArticulation" ); return 0;}
	virtual	PxAggregate*							createAggregate(PxU32 /*maxSize*/, bool /*enableSelfCollision*/) { PHYSICS_API_UNDEF( "createAggregate" ); return 0;}

#if PX_USE_PARTICLE_SYSTEM_API
	virtual PxParticleSystem*						createParticleSystem(PxU32 /*maxParticles*/, bool /*perParticleRestOffset*/ = false) { PHYSICS_API_UNDEF( "createParticleSystem" ); return 0;}
	virtual PxParticleFluid*						createParticleFluid(PxU32 /*maxParticles*/, bool /*perParticleRestOffset*/ = false) { PHYSICS_API_UNDEF( "createParticleFluid" ); return 0;}
#endif

	//PX_DEPRECATED virtual PxDeformable*			createDeformable(const PxDeformableDesc& deformableDesc) { PHYSICS_API_UNDEF( "createDeformable" ); return 0;}
	//PX_DEPRECATED virtual PxAttachment*			createAttachment(PxDeformable& deformable , PxShape* shape, PxU32 nbVertices, const PxU32* vertexIndices, const PxVec3* positions, const PxU32* flags) { PHYSICS_API_UNDEF( "createAttachment" ); return 0;}
#if PX_USE_CLOTH_API
#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual PxCloth*								createCloth(const PxTransform& /*globalPose*/, PxClothFabric& /*fabric*/, const PxClothParticle* /*particles*/, const PxClothCollisionData& /*collData*/, PxClothFlags /*flags*/) { PHYSICS_API_UNDEF( "createCloth" ); return 0;}
#else
	virtual PxCloth*								createCloth(const PxTransform& /*globalPose*/, PxClothFabric& /*fabric*/, const PxClothParticle* /*particles*/, PxClothFlags /*flags*/) { PHYSICS_API_UNDEF( "createCloth" ); return 0;}
#endif
#endif

	virtual PxMaterial*								createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution) = 0;
	virtual PxU32									getNbMaterials() const = 0;
	virtual	PxU32									getMaterials(PxMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const = 0;
	virtual PxTriangleMesh*							createTriangleMesh(PxInputStream& stream) = 0;
	virtual PxU32									getNbTriangleMeshes() const { PHYSICS_API_UNDEF( "getNbTriangleMeshes" ); return 0;}
	virtual	PxU32									getTriangleMeshes(PxTriangleMesh** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { PHYSICS_API_UNDEF( "getTriangleMeshes" ); return 0;}
	virtual PxHeightField*							createHeightField(const PxHeightFieldDesc& heightFieldDesc) = 0;
#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual PxHeightField*							createHeightField(PxInputStream& /*stream*/) { PHYSICS_API_UNDEF( "createHeightField" ); return NULL;}
#endif
	virtual PxU32									getNbHeightFields() const { PHYSICS_API_UNDEF( "getNbHeightFields" ); return 0;}
	virtual	PxU32									getHeightFields(PxHeightField** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { PHYSICS_API_UNDEF( "getHeightFields" ); return 0;}
	virtual PxConvexMesh*							createConvexMesh(PxInputStream& mesh) = 0;
	virtual PxU32									getNbConvexMeshes() const { PHYSICS_API_UNDEF( "getNbConvexMeshes" ); return 0;}
	virtual	PxU32									getConvexMeshes(PxConvexMesh** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { PHYSICS_API_UNDEF( "getConvexMeshes" ); return 0;}
	//PX_DEPRECATED virtual PxDeformableMesh*		createDeformableMesh(const PxStream& stream) { PHYSICS_API_UNDEF( "createDeformableMesh" ); return 0;}
	//PX_DEPRECATED virtual PxU32					getNbDeformableMeshes() const { PHYSICS_API_UNDEF( "getNbDeformableMeshes" ); return 0;}
	//PX_DEPRECATED virtual	PxU32					getDeformableMeshes(PxDeformableMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const { PHYSICS_API_UNDEF( "getDeformableMeshes" ); return 0;}
#if PX_USE_CLOTH_API
	virtual PxClothFabric*							createClothFabric(PxInputStream& /*stream*/) { PHYSICS_API_UNDEF( "createClothFabric" ); return 0;}
#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual PxClothFabric*							createClothFabric(PxU32 /*nbParticles*/, PxU32 /*nbPhases*/, const PxU32* /*phases*/, 
																		const PxClothFabricPhaseType::Enum* /*phaseTypes*/, PxU32 /*nbRestvalues*/, const PxReal* /*restvalues*/, 
																		PxU32 /*nbSets*/, const PxU32* /*sets*/, const PxU32* /*fibers*/,  const PxU32* /*indices*/)  { PHYSICS_API_UNDEF( "createClothFabric" ); return 0;}
#else
	virtual PxClothFabric*							createClothFabric(const PxClothFabricDesc& /*desc*/) { PHYSICS_API_UNDEF( "createClothFabric" ); return 0;}
#endif
	virtual PxU32									getNbClothFabrics() const { PHYSICS_API_UNDEF( "getNbClothFabrics" ); return 0;}
	virtual	PxU32									getClothFabrics(PxClothFabric** /*userBuffer*/, PxU32 /*bufferSize*/) const { PHYSICS_API_UNDEF( "getClothFabrics" ); return 0;}
#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void									registerDeletionListener(PxDeletionListener& /*observer*/, const PxDeletionEventFlags& /*deletionEvents*/, bool /*restrictedObjectSet*/ = false) { PHYSICS_API_UNDEF( "registerDeletionListener" ); }
	virtual void									unregisterDeletionListener(PxDeletionListener& /*observer*/) { PHYSICS_API_UNDEF( "unregisterDeletionListener" ); }
	virtual void									registerDeletionListenerObjects(PxDeletionListener& /*observer*/, const PxBase* const* /*observables*/, PxU32 /*observableCount*/) { PHYSICS_API_UNDEF( "registerDeletionListenerObjects" ); }
	virtual void									unregisterDeletionListenerObjects(PxDeletionListener& /*observer*/, const PxBase* const* /*observables*/, PxU32 /*observableCount*/) { PHYSICS_API_UNDEF( "unregisterDeletionListenerObjects" ); }
#endif

	virtual const PxTolerancesScale&				getTolerancesScale() const = 0;
	virtual PxFoundation&							getFoundation() = 0;
	virtual PxVisualDebugger*						getVisualDebugger()	{ PHYSICS_API_UNDEF( "getVisualDebugger" ); return 0;}
	virtual debugger::comm::PvdConnectionManager*	getPvdConnectionManager() = 0;
	virtual PxProfileZoneManager*					getProfileZoneManager() = 0;
};

};
//-----------------------------------------------------------------------------
PX_C_EXPORT GRB_CORE_API physx::PxPhysics* PX_CALL_CONV GrbCreatePhysics(
						physx::PxU32 version,
						physx::PxFoundation & foundation,
						const physx::PxTolerancesScale & scale,
						bool trackOutstandingAllocations = false,
						physx::PxProfileZoneManager * profileZoneManager = 0,
						physx::PxU32 cudaDeviceOrdinal = 0
					);

//-----------------------------------------------------------------------------
#endif
