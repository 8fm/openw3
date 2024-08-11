#pragma once

struct SPhysicsSettings
{
	static Bool m_dontCreateTrees;
	static Bool m_dontCreateRagdolls;
	static Bool m_dontCreateDestruction;
	static Bool m_dontCreateDestructionOnGPU;
	static Bool m_dontCreateCloth;
	static Bool m_dontCreateClothOnGPU;
	static Bool m_dontCreateClothSecondaryWorld;
	static Bool m_dontCreateParticles;
	static Bool m_dontCreateParticlesOnGPU;
	static Bool m_dontCreateCharacterControllers;
	static Bool m_dontCreateHeightmap;
	static Bool m_dontCreateLayerGeometry;
	static Bool m_dontCreateStaticMeshGeometry;
	static Bool m_dontCreateRigidBodies;
	static Bool m_dontCreateTriggers;
	static Bool m_pvdTransmitContacts;
	static Bool m_pvdTransimtScenequeries;
	static Bool m_pvdTransimtConstraints;
	static Bool	m_dynamicStructureIsDynamic;
	static Bool	m_staticsStructureIsDynamic;
	static Int32 m_dynamicRebuildHint;
	static Int32 m_useCpuDefaultDispacherNbCores;
	static Float m_particleSimulationDistanceLimit;
	static Float m_particleCellSize;
	static Float m_simulationDeltaClamp;
	static Float m_ragdollContactMultipler;
	static Float m_ragdollContactClamp;
	static Float m_rigidbodyContactMultipler;
	static Float m_rigidbodyContactClamp;
	static Uint32 m_rigidbodyPositionIters;
	static Uint32 m_rigidbodyVelocityIters;
	static Uint32 m_ragdollMinPositionIters;
	static Uint32 m_ragdollMinVelocityIters;
	static Float m_ragdollInVieportSleepThreshold;
	static Uint32 m_ragdollInVieportMinPositionIters;
	static Uint32 m_ragdollInVieportMinVelocityIters;
	static Float m_actorSleepThreshold;
	static Float m_destructibleUnfracturedSleepThreshold;
	static Float m_actorAngularVelocityLimit;
	static Float m_rigidbodyLinearVelocityClamp;
	static Bool m_onlyPlayerCanPush;
	static Bool m_nonplayersCanPushOnlyNonsleepers;
	static Float m_characterStepBigKinematicClamp;
	static Float m_characterStepBigDynamicsClamp;
	static Float m_characterPushingMultipler;
	static Float m_characterPushingMaxClamp;
	static Float m_characterFootstepWaterLevelLimit;
	static Bool m_characterForcePositionCacheInvalidation;
	static Bool m_characterManualCacheInvalidation;
	static Bool m_ragdollJointsProjection;
	static Float m_ragdollProjectionLinearTolerance;
	static Float m_ragdollProjectionAngularTolerance;
	static Float m_fluidBuoyancyMinimalDepth;
	static Float m_fluidLinearDamping;
	static Float m_fluidLinearForceMultipler;
	static Float m_fluidAngularDamping;
	static Float m_fluidAngularForceMultipler;
	static Float m_fluidAngularForceMaxClamp;
	static Float m_fluidAngularPredefinedRadius;
	static Float m_simpleBodyLinearDamper;
	static Float m_simpleBodyAngularDamper;
	static Float m_simpleBodySimulationDistanceLimit;
	static Float m_staticBodiesDistanceLimit;
	static Float m_tilesDistanceLimit;
	static Float m_ragdollJointedLinearDamper;
	static Float m_ragdollJointedAngularDamper;
	static Float m_ragdollChainedLinearDamper;
	static Float m_ragdollChainedAngularDamper;
	static Float m_ragdollJointedSleepThreshold;
	static Float m_ragdollChainedSleepThreshold;
	static Float m_ragdollMaxLinearDamper;
	static Float m_ragdollMaxAngularDamper;
	static Float m_ragdollMaxSimulationTime;
	static Float m_ragdollSimulationDynamicDistanceLimit;
	static Float m_ragdollSimulationKinematicDistanceLimit;
	static Float m_destructionLinearDamper;
	static Float m_destructionAngularDamper;
	static Float m_destructionSimulationDistanceLimit;
	static Float m_clothSimulationDistanceLimit;
	static Float m_contactOffset;
	static Float m_restOffset;
	static Float m_destructionFadeOutTime;
	static Float m_ragdollGlobalWindScaler;
	static Float m_clothWindScaler;
	static Float m_clothColiderSphereMinimalRadius;
	static Float m_clothColiderCapsuleMinimalRadius;
	static Float m_ragdollSpeedForSleep;
	static Float m_ragdollSleepFallSpeed;
	static Bool m_kinematicsContactsEnabled;
	static Float m_contactReportsThreshold;
	static Bool m_doAsyncTerrainCollisionInEditor;
	static Float m_cameraOccludablesRadiusRatio;
	static Float m_cameraOccludablesStartRatio;
	static Uint32 m_contactBlocksReservation;
	static Uint32 m_maxContactBlocks;
	static Uint32 m_maxActorCount;
	static Uint32 m_maxStaticShapes;
	static Uint32 m_maxDynamicShapes;
	static Uint32 m_maxConstraints;

	static Uint32 m_physxScratchMemory;
};

#ifndef NO_DEBUG_PAGES

struct SPhysicsStatistics
{
	static Float TerrainTilesInstanced;
	static Float StaticBodiesInstanced;
	static Float StaticBodiesSimulated;
	static Float SimpleBodiesInstanced;
	static Float SimpleBodiesSimulated;
	static Float RagdollsInstanced;
	static Float RagdollsSimulated;
	static Float DestructionsInstanced;
	static Float DestructionsSimulated;
	static Float ClothsInstanced;
	static Float ClothsSimulated;
	static Float ClothsRendered;
	static Float WaterCount;
	static Float WindCount;
	static Float CameraCount;
	static Float CharacterCount;
	static Float PhysicalSoundsCount;
};

struct STimeStatisticsScope
{
	Double m_time;
	Float* m_timerAccumulator;
	Float* m_timerCounter;
	STimeStatisticsScope( Float* timerAccumulator, Float* timerCounter ) : m_timerAccumulator( timerAccumulator ), m_timerCounter( timerCounter )
	{
		m_time = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

	~STimeStatisticsScope()
	{
		Double endTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
		m_time = endTime - m_time;

		*m_timerAccumulator += ( Float ) m_time;
		( *m_timerCounter )++;
	}
};

#define PHYSICS_STATISTICS_GET(name) SPhysicsStatistics::name;
#define PHYSICS_STATISTICS_GET_AND_CLEAR(name,val) {val=SPhysicsStatistics::name, SPhysicsStatistics::name=0.0f;}
#define PHYSICS_STATISTICS_ADD(name,val) SPhysicsStatistics::name+=val;
#define PHYSICS_STATISTICS_INC(name) SPhysicsStatistics::name+=1.0f;
#define PHYSICS_STATISTICS_INC_IF(name,expresion) if(expresion)SPhysicsStatistics::name+=1.0f;
#define PHYSICS_STATISTICS_DEC(name) SPhysicsStatistics::name-=1.0f;

#else

#define PHYSICS_STATISTICS_GET(name)
#define PHYSICS_STATISTICS_GET_AND_CLEAR(name,val)
#define PHYSICS_STATISTICS_ADD(name,val)
#define PHYSICS_STATISTICS_INC(name)
#define PHYSICS_STATISTICS_INC_IF(name,expresion)
#define PHYSICS_STATISTICS_DEC(name)

#endif

