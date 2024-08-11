#ifndef GRBSCENE3_H
#define GRBSCENE3_H

#include "PxScene.h"
#include "common/PxRenderBuffer.h"
#include "GrbSceneDesc3.h"
#include "GrbSimulationStatistics3.h"
#include "GrbUserContactReport.h"
#include "extensions/PxDefaultSimulationFilterShader.h"

#include "GrbSATModes3.h"
#include "GrbPairReportingModes3.h"

namespace physx
{
class GrbRigidDynamic3;

//-----------------------------------------------------------------------------
#define SCENE_API_UNDEF( x )	PX_ASSERT( 0 && "PxScene method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
class GrbScene3 : public PxScene
{
	public:
												GrbScene3(const GrbSceneDesc3 & /*desc*/) {}
	virtual										~GrbScene3()	{}

	public:

	virtual	void								release() = 0;
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual	bool								saveToDesc(PxSceneDesc& /*desc*/)	const	{ SCENE_API_UNDEF( "saveToDesc" ); return false; }
#endif
	virtual	void								setFlag(PxSceneFlag::Enum /*flag*/, bool /*value*/) { SCENE_API_UNDEF( "setFlag" ); }
	virtual	PxSceneFlags						getFlags() const  = 0;
	virtual void								setGravity(const PxVec3& vec) = 0;
	virtual PxVec3								getGravity() const = 0;
	virtual void								setNbContactDataBlocks(PxU32 /*numBlocks*/)		{ SCENE_API_UNDEF( "setNbContactDataBlocks" ); }
	virtual PxU32								getNbContactDataBlocksUsed() const			{ SCENE_API_UNDEF( "getNbContactDataBlocksUsed" ); return 0; }
	virtual PxU32								getMaxNbContactDataBlocksUsed() const		{ SCENE_API_UNDEF( "getMaxNbContactDataBlocksUsed" ); return 0; }

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual PxU32								getContactReportStreamBufferSize() const { SCENE_API_UNDEF( "getContactReportStreamBufferSize" ); return 0; }
#endif

	virtual	void								addArticulation(PxArticulation& /*articulation*/) { SCENE_API_UNDEF( "addArticulation" ); }
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								removeArticulation(PxArticulation& /*articulation*/, bool /*wakeOnLostTouch*/) { SCENE_API_UNDEF( "removeArticulation" ); }
#else
	virtual	void								removeArticulation(PxArticulation& /*articulation*/) { SCENE_API_UNDEF( "removeArticulation" ); }
#endif
	virtual	void								addActor(PxActor& actor) = 0;

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								addActors(PxActor* const * actors, PxU32 nbActors) = 0;
#else
	virtual	void								addActors(PxU32 nbActors, PxActor** actors) = 0;
#endif

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								addRigidStatics(PxRigidStatic** , PxU32 ) { SCENE_API_UNDEF( "addRigidStatics" ); }
	virtual	void								addRigidDynamics(PxRigidDynamic** , PxU32 ) { SCENE_API_UNDEF( "addRigidDynamics" ); }
#endif

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								removeActor(PxActor& actor, bool wakeOnLostTouch = true) = 0;
	virtual	void								removeActors(PxActor* const * actors, PxU32 nbActors, bool wakeOnLostTouch = true) = 0;
#else
	virtual	void								removeActor(PxActor& actor) = 0;
#endif
	virtual	void								addAggregate(PxAggregate& /*aggregate*/)	{ SCENE_API_UNDEF( "addAggregate" ); }
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								removeAggregate(PxAggregate& /*aggregate*/, bool /*wakeOnLostTouch*/)	{ SCENE_API_UNDEF( "removeAggregate" ); }
#else
	virtual	void								removeAggregate(PxAggregate& /*aggregate*/)	{ SCENE_API_UNDEF( "removeAggregate" ); }
#endif
	virtual	PxU32								getNbAggregates()	const	{ SCENE_API_UNDEF( "getNbAggregates" ); return 0;}
	virtual	PxU32								getAggregates(PxAggregate** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0)	const	{ SCENE_API_UNDEF( "getAggregates" ); return 0;}

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								addCollection(const PxCollection& /*collection*/) { SCENE_API_UNDEF( "addCollection" ); }
#endif

	virtual void								setDominanceGroupPair(PxDominanceGroup /*group1*/, PxDominanceGroup /*group2*/, const PxConstraintDominance& /*dominance*/) { SCENE_API_UNDEF( "setDominanceGroupPair" ); }
	virtual PxConstraintDominance				getDominanceGroupPair(PxDominanceGroup /*group1*/, PxDominanceGroup /*group2*/) const { SCENE_API_UNDEF( "getDominanceGroupPair" ); static char dummy[sizeof(PxConstraintDominance)]; return *(reinterpret_cast<PxConstraintDominance *>(dummy)); }
	virtual	PxU32								getNbActors(PxActorTypeSelectionFlags /*types*/) const { SCENE_API_UNDEF( "" ); return 0;}
	virtual	PxU32								getActors(PxActorTypeSelectionFlags /*types*/, PxActor** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const	{ SCENE_API_UNDEF( "getActors" ); return 0;}
	virtual PxActiveTransform*					getActiveTransforms(PxU32& nbTransformsOut, PxClientID client = PX_DEFAULT_CLIENT) = 0;
	virtual PxU32								getNbArticulations() const { SCENE_API_UNDEF( "getNbArticulations" ); return 0;}
	virtual	PxU32								getArticulations(PxArticulation** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { SCENE_API_UNDEF( "getArticulations" ); return 0;}
	virtual PxU32								getNbConstraints()	const	{ SCENE_API_UNDEF( "getNbConstraints" ); return 0;}
	virtual	PxU32								getConstraints(PxConstraint** /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { SCENE_API_UNDEF( "getConstraints" ); return 0;}
	virtual PxU32								getNbAttachments()	const	{ SCENE_API_UNDEF( "getNbAttachments" ); return 0;}
//	virtual	PxU32								getAttachments(PxAttachment** userBuffer, PxU32 bufferSize, PxU32 startIndex=0) const { SCENE_API_UNDEF( "getAttachments" ); return 0;}
	virtual const PxRenderBuffer&				getRenderBuffer() = 0;
	virtual	PxPhysics&							getPhysics() = 0;
	virtual	void								getSimulationStatistics(PxSimulationStatistics& /*stats*/) const { SCENE_API_UNDEF( "getSimulationStatistics" ); }
	virtual	const GrbSimulationStatistics3 *	getSimulationStatistics() const = 0;

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual PxCpuDispatcher*				getCpuDispatcher() const { SCENE_API_UNDEF( "getCpuDispatcher" ); return 0;}
	virtual PxGpuDispatcher*				getGpuDispatcher() const { SCENE_API_UNDEF( "getGpuDispatcher" ); return 0;}
	virtual PxSpuDispatcher*				getSpuDispatcher() const { SCENE_API_UNDEF( "getSpuDispatcher" ); return 0;}
#endif

	virtual PxClientID							createClient() { SCENE_API_UNDEF( "createClient" ); return PxClientID(); }
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
	virtual void								setClientBehaviorBits(PxClientID /*client*/, PxU32 /*clientBehaviorBits*/) { SCENE_API_UNDEF( "setClientBehaviorBits" ); } 
	virtual PxU32								getClientBehaviorBits(PxClientID /*client*/) const { SCENE_API_UNDEF( "getClientBehaviorBits" ); return 0;}
#else
	virtual	void								setClientBehaviorFlags(PxClientID, PxClientBehaviorFlags) { SCENE_API_UNDEF("setClientBehaviorFlags"); }
	virtual	PxClientBehaviorFlags				getClientBehaviorFlags(PxClientID) const { SCENE_API_UNDEF("getClientBehaviorFlags"); return PxClientBehaviorFlags(); }
#endif	

#if PX_USE_CLOTH_API
#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void								setClothInterCollisionDistance(PxF32 /*distance*/)			{ SCENE_API_UNDEF( "setClothInterCollisionDistance" ); } 
	virtual PxF32								getClothInterCollisionDistance() const					{ SCENE_API_UNDEF( "getClothInterCollisionDistance" ); return 0.0f; }
	virtual void								setClothInterCollisionStiffness(PxF32 /*stiffness*/)		{ SCENE_API_UNDEF( "setClothInterCollisionStiffness" ); } 
	virtual PxF32								getClothInterCollisionStiffness() const					{ SCENE_API_UNDEF( "getClothInterCollisionStiffness" ); return 0.0f; } 
	virtual void								setClothInterCollisionNbIterations(PxU32 /*nbIterations*/)	{ SCENE_API_UNDEF( "setClothInterCollisionNbIterations" ); } 	
	virtual PxU32								getClothInterCollisionNbIterations() const				{ SCENE_API_UNDEF( "getClothInterCollisionNbIterations" ); return 0; }; 
#endif
#endif

	virtual void								setSimulationEventCallback(PxSimulationEventCallback* /*callback*/, PxClientID /*client*/ = PX_DEFAULT_CLIENT) { SCENE_API_UNDEF( "setSimulationEventCallback" ); }
	virtual PxSimulationEventCallback*			getSimulationEventCallback(PxClientID /*client*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "getSimulationEventCallback" ); return 0;}
	virtual void								setContactModifyCallback(PxContactModifyCallback* /*callback*/) { SCENE_API_UNDEF( "setContactModifyCallback" ); }
	virtual PxContactModifyCallback*			getContactModifyCallback() const { SCENE_API_UNDEF( "getContactModifyCallback" ); return 0;}

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void								setCCDContactModifyCallback(PxCCDContactModifyCallback* /*callback*/) { SCENE_API_UNDEF( "setCCDContactModifyCallback" ); } 
	virtual PxCCDContactModifyCallback*			getCCDContactModifyCallback() const { SCENE_API_UNDEF( "getCCDContactModifyCallback" ); return 0; } 
	virtual void								setBroadPhaseCallback(PxBroadPhaseCallback* /*callback*/, PxClientID /*client*/ = PX_DEFAULT_CLIENT){ SCENE_API_UNDEF( "setBroadPhaseCallback" ); } 
	virtual PxBroadPhaseCallback*				getBroadPhaseCallback(PxClientID /*client*/ = PX_DEFAULT_CLIENT)	const { SCENE_API_UNDEF( "getBroadPhaseCallback" ); return 0; }
#endif

	virtual	const void*							getFilterShaderData() const { SCENE_API_UNDEF( "getFilterShaderData" ); return 0;}
	virtual	PxU32								getFilterShaderDataSize() const { SCENE_API_UNDEF( "getFilterShaderDataSize" ); return 0;}
	virtual	PxSimulationFilterShader			getFilterShader() const { SCENE_API_UNDEF( "getFilterShader" ); return 0;}
	virtual	PxSimulationFilterCallback*			getFilterCallback() const { SCENE_API_UNDEF( "getFilterCallback" ); return 0;}

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void								resetFiltering(PxActor& /*actor*/) { SCENE_API_UNDEF( "resetFiltering" ); }
	virtual void								resetFiltering(PxRigidActor& /*actor*/, PxShape*const* /*shapes*/, PxU32 /*shapeCount*/) { SCENE_API_UNDEF( "resetFiltering" ); }
#endif


#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual void								simulate(PxReal elapsedTime, pxtask::BaseTask* completionTask = NULL, void* scratchMemBlock = 0, PxU32 scratchMemBlockSize = 0, bool controlSimulation = true) = 0;
#else
	virtual void								simulate(PxReal elapsedTime, PxBaseTask* completionTask = NULL, void* scratchMemBlock = 0, PxU32 scratchMemBlockSize = 0, bool controlSimulation = true) = 0;
#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								solve(PxReal /*elapsedTime*/, physx::PxBaseTask* /*completionTask*/, void* /*scratchMemBlock*/ = 0, PxU32 /*scratchMemBlockSize*/ = 0, bool /*controlSimulation*/ = true) { SCENE_API_UNDEF("solve"); }
	virtual	void								collide(PxReal /*elapsedTime*/, physx::PxBaseTask* /*completionTask*/, void* /*scratchMemBlock*/ = 0, PxU32 /*scratchMemBlockSize*/ = 0) { SCENE_API_UNDEF("collide"); }
#endif

	virtual	bool								checkResults(bool /*block*/ = false)	{ SCENE_API_UNDEF( "checkResults" ); return false;}
	virtual	bool								fetchResults(bool block = false, PxU32* errorState = 0) = 0;
	virtual	void								flush(bool /*sendPendingReports*/ = false) { SCENE_API_UNDEF( "flush" ); }

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	void								flushSimulation(bool /*sendPendingReports*/ = false) { SCENE_API_UNDEF( "flushSimulation" ); }
	virtual	void								flushQueryUpdates() { SCENE_API_UNDEF( "flushQueryUpdates" ); }

	virtual	PxPruningStructure::Enum			getStaticStructure() const { SCENE_API_UNDEF( "getStaticStructure" ); return PxPruningStructure::eLAST;}
	virtual PxPruningStructure::Enum			getDynamicStructure() const { SCENE_API_UNDEF( "getDynamicStructure" ); return PxPruningStructure::eLAST;}
#endif

	virtual	PxBatchQuery*						createBatchQuery(const PxBatchQueryDesc& /*desc*/) { SCENE_API_UNDEF( "createBatchQuery" ); return 0;}
#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	PxVolumeCache*						createVolumeCache(PxU32 /*maxStaticShapes*/ = 32, PxU32 /*maxDynamicShapes*/ = 8) { SCENE_API_UNDEF( "createVolumeCache" ); return 0;}
#else
	virtual	PxSweepCache*						createSweepCache(PxReal /*dimensions*/ = 5.0f) { SCENE_API_UNDEF( "createSweepCache" ); return 0;}
#endif
	virtual	void								setDynamicTreeRebuildRateHint(PxU32 /*dynamicTreeRebuildRateHint*/) { SCENE_API_UNDEF( "setDynamicTreeRebuildRateHint" ); }
	virtual PxU32								getDynamicTreeRebuildRateHint() const { SCENE_API_UNDEF( "getDynamicTreeRebuildRateHint" ); return 0;}

#if (PX_PHYSICS_VERSION_MAJOR >= 3) && ((PX_PHYSICS_VERSION_MINOR == 2) && (PX_PHYSICS_VERSION_BUGFIX >= 4) || (PX_PHYSICS_VERSION_MINOR >= 3))
	virtual void								forceDynamicTreeRebuild(bool /*rebuilStaticStructure*/, bool /*rebuildDynamicStructure*/) { SCENE_API_UNDEF( "forceDynamicTreeRebuild" ); }
#endif

	virtual	void								setSolverBatchSize(PxU32 /*solverBatchSize*/) { SCENE_API_UNDEF( "setSolverBatchSize" ); }
	virtual PxU32								getSolverBatchSize() const { SCENE_API_UNDEF( "getSolverBatchSize" ); return 0;}

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void								setCCDMaxPasses(PxU32 /*ccdMaxPasses*/) { SCENE_API_UNDEF( "setCCDMaxPasses" ); }
	virtual PxU32								getCCDMaxPasses() const { SCENE_API_UNDEF( "getCCDMaxPasses" ); return 0; }

	virtual PxReal								getContactCorrelationDistance() const { SCENE_API_UNDEF( "getContactCorrelationDistance" ); return 0.0f;}
	virtual PxReal								getFrictionOffsetThreshold() const { SCENE_API_UNDEF( "getFrictionOffsetThreshold" ); return 0.0f;}
	PX_DEPRECATED virtual PxReal				getMeshContactMargin() const { SCENE_API_UNDEF( "getMeshContactMargin" ); return 0.0f;}
#endif

	virtual bool								setVisualizationParameter(PxVisualizationParameter::Enum param, PxReal value) = 0;
	virtual PxReal								getVisualizationParameter(PxVisualizationParameter::Enum paramEnum) const = 0;
	virtual void								setVisualizationCullingBox(const PxBounds3& /*box*/) { SCENE_API_UNDEF( "setVisualizationCullingBox" ); }
	virtual const PxBounds3&					getVisualizationCullingBox() const { static PxBounds3 bounds; SCENE_API_UNDEF( "getVisualizationCullingBox" ); return bounds; }
	virtual void								setCacheBlocks(PxU8** /*blocks*/, PxU32 /*numBlocks*/) { SCENE_API_UNDEF( "setCacheBlocks" ); }
	virtual	PxU32								retrieveAvailableCacheBlocks(PxU8** /*blocks*/) { SCENE_API_UNDEF( "retrieveAvailableCacheBlocks" ); return 0; }

	virtual	void								setFilterOps(const PxFilterOp::Enum & op0, const PxFilterOp::Enum & op1, const PxFilterOp::Enum & op2) = 0;
	virtual	void								setFilterBool(bool flag) = 0;
	virtual	void								setFilterConstants(const PxGroupsMask & const0, const PxGroupsMask & const1) = 0;
	virtual	void								getFilterOps(PxFilterOp::Enum & op0, PxFilterOp::Enum & op1, PxFilterOp::Enum & op2) const = 0;
	virtual	bool 								getFilterBool() const = 0;
	virtual	void								getFilterConstants(PxGroupsMask & const0, PxGroupsMask & const1) const = 0;


#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual bool raycastAny(const PxVec3& /*origin*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							PxSceneQueryHit& /*hit*/,
							const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							const PxSceneQueryCache* /*cache*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "raycastAny" ); return false;}
	virtual bool raycastSingle(const PxVec3& /*origin*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							   PxSceneQueryFlags /*outputFlags*/,
							   PxRaycastHit& /*hit*/,
							   const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
							   PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							   const PxSceneQueryCache* /*cache*/ = NULL,
							   PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "raycastSingle" ); return false;}
	virtual PxI32 raycastMultiple(const PxVec3& /*origin*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								  PxSceneQueryFlags /*outputFlags*/,
								  PxRaycastHit* /*hitBuffer*/,
								  PxU32 /*hitBufferSize*/,
								  bool& /*blockingHit*/,
								  const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
								  PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								  const PxSceneQueryCache* /*cache*/ = NULL,
								  PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "raycastMultiple" ); return false;}
#else
	// 3.3 raycast
	virtual bool raycast(const PxVec3& /*origin*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
						 PxRaycastCallback& /*hitCall*/, PxHitFlags /*hitFlags*/ = PxHitFlags(PxHitFlag::eDEFAULT),
						 const PxQueryFilterData& /*filterData*/ = PxQueryFilterData(), PxQueryFilterCallback* /*filterCall*/ = NULL,
						 const PxQueryCache* /*cache*/ = NULL) const { SCENE_API_UNDEF( "raycast" ); return false;}

#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
#if (PX_PHYSICS_VERSION_BUGFIX < 2)
	virtual bool sweepAny(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							PxSceneQueryFlags /*queryFlags*/,
							PxSceneQueryHit& /*hit*/,
							const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							const PxSceneQueryCache* /*cache*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepAny" ); return false;}
	virtual bool sweepAny(	const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
							const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							PxSceneQueryFlags /*queryFlags*/,
							PxSceneQueryHit& /*hit*/,
							PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							const PxSceneQueryCache* /*cache*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepAny" ); return false;}
	virtual bool sweepSingle(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit& /*hit*/,
								const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepSingle" ); return false;}
	virtual bool sweepSingle(	const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
								const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit& /*hit*/,
								PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepSingle" ); return false;}
	virtual PxI32 sweepMultiple(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
									PxSceneQueryFlags /*outputFlags*/,
									PxSweepHit* /*hitBuffer*/,
									PxU32 /*hitBufferSize*/,
									bool& /*blockingHit*/,
									const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
									PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
									const PxSceneQueryCache* /*cache*/ = NULL,
									PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepMultiple" ); return false;}
	virtual PxI32 sweepMultiple(const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
								const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit* /*hitBuffer*/,
								PxU32 /*hitBufferSize*/,
								bool& /*blockingHit*/,
								PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "sweepMultiple" ); return false;}
#else // PX_PHYSICS_VERSION_BUGFIX >= 2
	virtual bool sweepAny(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							PxSceneQueryFlags /*queryFlags*/,
							PxSceneQueryHit& /*hit*/,
							const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							const PxSceneQueryCache* /*cache*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
							const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepAny" ); return false;}
	virtual bool sweepAny(	const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
							const PxVec3& /*unitDir*/, const PxReal /*distance*/,
							PxSceneQueryFlags /*queryFlags*/,
							PxSceneQueryHit& /*hit*/,
							PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							const PxSceneQueryCache* /*cache*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
							const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepAny" ); return false;}
	virtual bool sweepSingle(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit& /*hit*/,
								const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
								const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepSingle" ); return false;}
	virtual bool sweepSingle(	const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
								const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit& /*hit*/,
								PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
								const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepSingle" ); return false;}
	virtual PxI32 sweepMultiple(	const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
									PxSceneQueryFlags /*outputFlags*/,
									PxSweepHit* /*hitBuffer*/,
									PxU32 /*hitBufferSize*/,
									bool& /*blockingHit*/,
									const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
									PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
									const PxSceneQueryCache* /*cache*/ = NULL,
									PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
									const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepMultiple" ); return false;}
	virtual PxI32 sweepMultiple(const PxGeometry** /*geometryList*/, const PxTransform* /*poseList*/, const PxFilterData* /*filterDataList*/, PxU32 /*geometryCount*/, 
								const PxVec3& /*unitDir*/, const PxReal /*distance*/,
								PxSceneQueryFlags /*outputFlags*/,
								PxSweepHit* /*hitBuffer*/,
								PxU32 /*hitBufferSize*/,
								bool& /*blockingHit*/,
								PxSceneQueryFilterFlags /*filterFlags*/ = PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::eSTATIC,
								PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
								const PxSceneQueryCache* /*cache*/ = NULL,
								PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT,
								const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweepMultiple" ); return false;}

	virtual void lockRead(const char* /*file*/=NULL, PxU32 /*line*/=0) { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void unlockRead() { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void lockWrite(const char* /*file*/=NULL, PxU32 /*line*/=0) { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void unlockWrite() { /* No assert, although no lock will be implemented by Richard's request. */ }
#endif // PX_PHYSICS_VERSION_BUGFIX >= 2
#else
	// 3.3 sweep
	virtual bool sweep(const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, const PxVec3& /*unitDir*/, const PxReal /*distance*/,
					   PxSweepCallback& /*hitCall*/, PxHitFlags /*hitFlags*/ = PxHitFlags(PxHitFlag::eDEFAULT),
					   const PxQueryFilterData& /*filterData*/ = PxQueryFilterData(), PxQueryFilterCallback* /*filterCall*/ = NULL,
					   const PxQueryCache* /*cache*/ = NULL, const PxReal /*inflation*/ = 0.f) const { SCENE_API_UNDEF( "sweep" ); return false;}
#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual bool overlapAny(const PxGeometry& /*geometry*/,
							const PxTransform& /*pose*/,
							PxShape*& /*hit*/,
							const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
							PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
							PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "overlapAny" ); return false;}

	virtual PxI32 overlapMultiple(	const PxGeometry& /*geometry*/,
									const PxTransform& /*pose*/,
									PxShape** /*hitBuffer*/,
									PxU32 /*hitBufferSize*/,
									const PxSceneQueryFilterData& /*filterData*/ = PxSceneQueryFilterData(),
									PxSceneQueryFilterCallback* /*filterCall*/ = NULL,
									PxClientID /*queryClient*/ = PX_DEFAULT_CLIENT) const { SCENE_API_UNDEF( "overlapMultiple" ); return 0; }
#else
	//3.3 overlap
	virtual bool			overlap(const PxGeometry& /*geometry*/, const PxTransform& /*pose*/, PxOverlapCallback& /*hitCall*/,
								const PxQueryFilterData& /*filterData*/ = PxQueryFilterData(), PxQueryFilterCallback* /*filterCall*/ = NULL
								) const { SCENE_API_UNDEF( "overlap" ); return false; }
#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR < 3)
	virtual pxtask::TaskManager *		getTaskManager() const = 0;
#else
	virtual PxTaskManager *		getTaskManager() const = 0;
#endif
	virtual	PxU32						getTimestamp()	const { SCENE_API_UNDEF( "getTimestamp" ); return 0;}
	virtual	PxU32						getSceneQueryStaticTimestamp()	const { SCENE_API_UNDEF( "getSceneQueryStaticTimestamp" ); return 0;}

// API supported in both 3.2.2 and 3.3
#if (PX_PHYSICS_VERSION_MAJOR >= 3) && ((PX_PHYSICS_VERSION_MINOR == 2) && (PX_PHYSICS_VERSION_BUGFIX >= 2) || (PX_PHYSICS_VERSION_MINOR >= 3))
	virtual void						setBounceThresholdVelocity(const PxReal /*t*/) { SCENE_API_UNDEF( "getSceneQueryStaticTimestamp" ); }
	virtual PxReal						getBounceThresholdVelocity() const { SCENE_API_UNDEF( "getSceneQueryStaticTimestamp" ); return 0.0f;}
#endif

#if (PX_PHYSICS_VERSION_MAJOR > 3) || (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual void					setLimits(const PxSceneLimits& /*limits*/) { SCENE_API_UNDEF( "setLimits" ); }
	virtual PxSceneLimits			getLimits() const { SCENE_API_UNDEF( "getLimits" ); return PxSceneLimits();}
	virtual	PxBroadPhaseType::Enum	getBroadPhaseType() const { SCENE_API_UNDEF( "getBroadPhaseType" ); return PxBroadPhaseType::eLAST; }
	virtual	bool					getBroadPhaseCaps(PxBroadPhaseCaps& /*caps*/) const { SCENE_API_UNDEF( "getBroadPhaseCaps" ); return false; }
	virtual	PxU32					getNbBroadPhaseRegions() const { SCENE_API_UNDEF( "getNbBroadPhaseRegions" ); return 0; }
	virtual	PxU32					getBroadPhaseRegions(PxBroadPhaseRegionInfo* /*userBuffer*/, PxU32 /*bufferSize*/, PxU32 /*startIndex*/=0) const { SCENE_API_UNDEF( "getBroadPhaseRegions" ); return 0; }
	virtual	PxU32					addBroadPhaseRegion(const PxBroadPhaseRegion& /*region*/, bool /*populateRegion*/=false) { SCENE_API_UNDEF( "addBroadPhaseRegion" ); return 0; }
	virtual	bool					removeBroadPhaseRegion(PxU32 /*handle*/) { SCENE_API_UNDEF( "removeBroadPhaseRegion" ); return false; }
	virtual void					lockRead(const char* /*file*/=NULL, PxU32 /*line*/=0) { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void					unlockRead() { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void					lockWrite(const char* /*file*/=NULL, PxU32 /*line*/=0) { /* No assert, although no lock will be implemented by Richard's request. */ }
	virtual void					unlockWrite() { /* No assert, although no lock will be implemented by Richard's request. */ }
#endif

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	virtual	PxReal						getWakeCounterResetValue() const { SCENE_API_UNDEF( "getWakeCounterResetValue" ); return 0.0f; }
	virtual	void					shiftOrigin(const PxVec3& /*shift*/) { SCENE_API_UNDEF( "shiftOrigin" ); };
#endif

	//GRB specific methods
	virtual void						syncPhysXsdk(bool discardUpdates) = 0;
	virtual	void						setGrbUserContactReport(GrbUserContactReport * callback) = 0;
	virtual	GrbUserContactReport *		getGrbUserContactReport() const = 0;
	virtual	void						setGrbSATMode(PxSATMode::Enum satMode) = 0;
	virtual	PxSATMode::Enum				getGrbSATMode() const = 0;
	virtual	void						setGrbSATFixedDirectionsLevel(PxU32 satFixedDirectionsLevel) = 0;
	virtual	PxU32						getGrbSATFixedDirectionsLevel() const = 0;
	virtual	void						setGrbPairReportingMode(PxPairReportingMode::Enum pairReportingMode) = 0;
	virtual	PxPairReportingMode::Enum	getGrbPairReportingMode() const = 0;
	virtual void						setPxScene(PxScene * scene)=0;

	virtual void						setGrbSphereSlices(PxU32 slices) = 0;
	virtual PxU32						getGrbSphereSlices() const = 0;
	virtual void						setGrbSphereStacks(PxU32 stacks) = 0;
	virtual PxU32						getGrbSphereStacks() const = 0;

	virtual void						setGrbCapsuleSlices(PxU32 slices) = 0;
	virtual PxU32						getGrbCapsuleSlices() const = 0;
	virtual void						setGrbCapsuleStacks(PxU32 stacks) = 0;
	virtual PxU32						getGrbCapsuleStacks() const = 0;

	void *	userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.
};
//-----------------------------------------------------------------------------
}
#endif
