#include "build.h"
#include "../physics/physicsSettings.h"

Bool SPhysicsSettings::m_dontCreateTrees = false;
Bool SPhysicsSettings::m_dontCreateRagdolls = false;
Bool SPhysicsSettings::m_dontCreateDestruction = false;
Bool SPhysicsSettings::m_dontCreateDestructionOnGPU = true;
Bool SPhysicsSettings::m_dontCreateCloth = false;
Bool SPhysicsSettings::m_dontCreateClothOnGPU = false;
Bool SPhysicsSettings::m_dontCreateClothSecondaryWorld = false;
Bool SPhysicsSettings::m_dontCreateParticles = false;
Bool SPhysicsSettings::m_dontCreateParticlesOnGPU = false;
Bool SPhysicsSettings::m_dontCreateCharacterControllers = false;
Bool SPhysicsSettings::m_dontCreateHeightmap = false;
Bool SPhysicsSettings::m_dontCreateLayerGeometry = false;
Bool SPhysicsSettings::m_dontCreateStaticMeshGeometry = false;
Bool SPhysicsSettings::m_dontCreateRigidBodies = false;
Bool SPhysicsSettings::m_dontCreateTriggers = false;
Bool SPhysicsSettings::m_pvdTransmitContacts = false;
Bool SPhysicsSettings::m_pvdTransimtScenequeries = false;
Bool SPhysicsSettings::m_pvdTransimtConstraints = false;
Bool SPhysicsSettings::m_dynamicStructureIsDynamic = true;
Bool SPhysicsSettings::m_staticsStructureIsDynamic = true;
Int32 SPhysicsSettings::m_dynamicRebuildHint = 100;
Int32 SPhysicsSettings::m_useCpuDefaultDispacherNbCores = 0;
Float SPhysicsSettings::m_particleSimulationDistanceLimit = 50.0f;
Float SPhysicsSettings::m_particleCellSize = 4.0f;
Float SPhysicsSettings::m_contactOffset = 0.05f;
Float SPhysicsSettings::m_restOffset = 0.0f;
Float SPhysicsSettings::m_destructionFadeOutTime = 0.5f;
Float SPhysicsSettings::m_simulationDeltaClamp = 0.033f;
Float SPhysicsSettings::m_ragdollContactMultipler = 10.0f;
Float SPhysicsSettings::m_ragdollContactClamp = 15.0f;
Float SPhysicsSettings::m_rigidbodyContactMultipler = 1.0f;
Float SPhysicsSettings::m_rigidbodyContactClamp = 5.0f;
Uint32 SPhysicsSettings::m_rigidbodyPositionIters = 4;
Uint32 SPhysicsSettings::m_rigidbodyVelocityIters = 1;
Uint32 SPhysicsSettings::m_ragdollMinPositionIters = 20;
Uint32 SPhysicsSettings::m_ragdollMinVelocityIters = 20;
Float SPhysicsSettings::m_ragdollInVieportSleepThreshold = 0.01f;
#ifdef NO_EDITOR
Uint32 SPhysicsSettings::m_ragdollInVieportMinPositionIters = 40;
Uint32 SPhysicsSettings::m_ragdollInVieportMinVelocityIters = 20;
#else
Uint32 SPhysicsSettings::m_ragdollInVieportMinPositionIters = 255;
Uint32 SPhysicsSettings::m_ragdollInVieportMinVelocityIters = 20;
#endif
Float SPhysicsSettings::m_actorSleepThreshold = 0.25f;
Float SPhysicsSettings::m_destructibleUnfracturedSleepThreshold = 0.05f;
Float SPhysicsSettings::m_actorAngularVelocityLimit = 7.0f;
Float SPhysicsSettings::m_rigidbodyLinearVelocityClamp = 1.5f;
Bool SPhysicsSettings::m_onlyPlayerCanPush = true;
Bool SPhysicsSettings::m_nonplayersCanPushOnlyNonsleepers = false;
Float SPhysicsSettings::m_characterStepBigKinematicClamp = 0.5f;
Float SPhysicsSettings::m_characterStepBigDynamicsClamp = 0.0f;
Float SPhysicsSettings::m_characterPushingMultipler = 20.0f;
Float SPhysicsSettings::m_characterPushingMaxClamp = 20.0f;
Float SPhysicsSettings::m_characterFootstepWaterLevelLimit = 0.4f;
Bool SPhysicsSettings::m_characterManualCacheInvalidation = false;
Bool SPhysicsSettings::m_characterForcePositionCacheInvalidation = false;
Bool SPhysicsSettings::m_ragdollJointsProjection = false;
Float SPhysicsSettings::m_ragdollProjectionLinearTolerance = 0.01f;
Float SPhysicsSettings::m_ragdollProjectionAngularTolerance = 0.01f;
Float SPhysicsSettings::m_fluidBuoyancyMinimalDepth = 0.1f;
Float SPhysicsSettings::m_fluidLinearDamping = 1.0f;
Float SPhysicsSettings::m_fluidLinearForceMultipler = 0.25f;
Float SPhysicsSettings::m_fluidAngularDamping = 2.0f;
Float SPhysicsSettings::m_fluidAngularForceMultipler = 10.0f;
Float SPhysicsSettings::m_fluidAngularForceMaxClamp = 2.0f;
Float SPhysicsSettings::m_fluidAngularPredefinedRadius = 0.5f;
Float SPhysicsSettings::m_simpleBodyLinearDamper = 0.25f;
Float SPhysicsSettings::m_simpleBodyAngularDamper = 0.5f;
Float SPhysicsSettings::m_simpleBodySimulationDistanceLimit = 50.0f;
Float SPhysicsSettings::m_staticBodiesDistanceLimit = 50.0f;
Float SPhysicsSettings::m_tilesDistanceLimit = 512.0f;
Float SPhysicsSettings::m_ragdollJointedLinearDamper = 0.5f;
Float SPhysicsSettings::m_ragdollJointedAngularDamper = 2.0f;
Float SPhysicsSettings::m_ragdollChainedLinearDamper = 0.0f;
Float SPhysicsSettings::m_ragdollChainedAngularDamper = 20.0f;
Float SPhysicsSettings::m_ragdollJointedSleepThreshold = 0.25f;
Float SPhysicsSettings::m_ragdollChainedSleepThreshold = 0.75f;
Float SPhysicsSettings::m_ragdollMaxLinearDamper = 20.0f;
Float SPhysicsSettings::m_ragdollMaxAngularDamper = 20.0f;
Float SPhysicsSettings::m_ragdollMaxSimulationTime = 30.0f;
Float SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit = 50.0f;
Float SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit = 25.0f;
Float SPhysicsSettings::m_destructionLinearDamper = 0.0f;
Float SPhysicsSettings::m_destructionAngularDamper = 0.05f;
Float SPhysicsSettings::m_destructionSimulationDistanceLimit = 30.0f;
Float SPhysicsSettings::m_clothSimulationDistanceLimit = 25.0f;
Float SPhysicsSettings::m_ragdollGlobalWindScaler = 5.0f;
Float SPhysicsSettings::m_clothWindScaler = 20.0f;
Float SPhysicsSettings::m_clothColiderSphereMinimalRadius = 0.15f;
Float SPhysicsSettings::m_clothColiderCapsuleMinimalRadius = 0.15f;
Float SPhysicsSettings::m_ragdollSpeedForSleep = 0.2f;
Float SPhysicsSettings::m_ragdollSleepFallSpeed = 5.0f;
Bool SPhysicsSettings::m_kinematicsContactsEnabled = false;
Float SPhysicsSettings::m_contactReportsThreshold = 30.0f;

Bool SPhysicsSettings::m_doAsyncTerrainCollisionInEditor = true;

//Camera
Float SPhysicsSettings::m_cameraOccludablesRadiusRatio = 1.0f;
Float SPhysicsSettings::m_cameraOccludablesStartRatio = 0.5f;

Uint32 SPhysicsSettings::m_contactBlocksReservation = 32;
Uint32 SPhysicsSettings::m_maxContactBlocks = 512;
Uint32 SPhysicsSettings::m_maxActorCount = 6 * 1024;
Uint32 SPhysicsSettings::m_maxStaticShapes = 4 * 1024;
Uint32 SPhysicsSettings::m_maxDynamicShapes = 2 * 1024;
Uint32 SPhysicsSettings::m_maxConstraints = 1 * 1024;
Uint32 SPhysicsSettings::m_physxScratchMemory = 4 * ( 16 * 1024 ); // Must be multiple of 16KB

#ifndef NO_DEBUG_PAGES
Float SPhysicsStatistics::StaticBodiesInstanced = 0.0f;
Float SPhysicsStatistics::StaticBodiesSimulated = 0.0f;
Float SPhysicsStatistics::TerrainTilesInstanced = 0.0f;
Float SPhysicsStatistics::SimpleBodiesInstanced = 0.0f;
Float SPhysicsStatistics::RagdollsInstanced = 0.0f;
Float SPhysicsStatistics::SimpleBodiesSimulated = 0.0f;
Float SPhysicsStatistics::RagdollsSimulated = 0.0f;	
Float SPhysicsStatistics::DestructionsInstanced = 0.0f;
Float SPhysicsStatistics::DestructionsSimulated = 0.0f;
Float SPhysicsStatistics::ClothsInstanced = 0.0f;
Float SPhysicsStatistics::ClothsSimulated = 0.0f;
Float SPhysicsStatistics::ClothsRendered = 0.0f;
Float SPhysicsStatistics::WaterCount = 0.0f;
Float SPhysicsStatistics::WindCount = 0.0f;
Float SPhysicsStatistics::CameraCount = 0.0f;
Float SPhysicsStatistics::CharacterCount = 0.0f;
Float SPhysicsStatistics::PhysicalSoundsCount = 0.0f;
#endif

