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

#ifndef NX_DESTRUCTIBLE_ACTOR_H
#define NX_DESTRUCTIBLE_ACTOR_H

#include "foundation/Px.h"
#include "NxApexActor.h"
#include "NxApexRenderable.h"
#include "NxModuleDestructible.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxBox.h"
#include "NxRay.h"
#elif NX_SDK_VERSION_MAJOR == 3
namespace physx
{
	class PxRigidDynamic;
};
#endif

#define NX_DESTRUCTIBLE_ACTOR_TYPE_NAME "DestructibleActor"

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

struct NxDestructibleParameters;
class NxDestructibleJoint;
class NxApexRenderVolume;
class NxApexEmitterActor;
class NxDestructibleRenderable;
class NxDestructibleBehaviorGroupDesc;

/**
	Flags for NxDestructibleActor::raycast()
*/
struct NxDestructibleActorRaycastFlags
{
	enum Enum
	{
		StaticChunks =	(1 << 0),
		DynamicChunks =	(1 << 1),

		AllChunks =					StaticChunks | DynamicChunks,

		SegmentIntersect =	(1 << 2),	// Do not consider intersect times > 1

		ForceAccurateRaycastsOn =	(1 << 3),
		ForceAccurateRaycastsOff =	(1 << 4),
	};
};


/**
	Determines which type of emitter to associate with a render volume
*/
struct NxDestructibleEmitterType
{
	enum Enum
	{
		Crumble,
		Dust,	// Note: this is a placeholder.  Its implementation has been removed in 1.2.0, and will be restored in later versions.
		Count
	};
};

/**
	Provides access to specific NxParamterized types provided by the destructible actor
*/
struct NxDestructibleParameterizedType
{
	enum Enum
	{
		State,	// The full state of the actor (including params, chunk data, etc...)
		Params,	// The parameters used to initialize the actor
	};
};

/**
	An event structure for an optional chunk event buffer.  Contains a chunk index and an event field.
*/
struct NxDestructibleChunkEvent
{
	enum EventMask
	{
		VisibilityChanged	=	(1 << 0),
		ChunkVisible	=		(1 << 1)
	};

	physx::PxU16	chunkIndex;
	physx::PxU16	event;
};

/**
	Hit chunk info.  Hit chunks are those directly affected by fracture events.  See getHitChunkHistory and forceChunkHits.
*/
struct NxDestructibleHitChunk
{
	physx::PxU32 chunkIndex;
	physx::PxU32 hitChunkFlags;
};

/**
	Flags for managing the sync state of the destructible actor
*/
struct NxDestructibleActorSyncFlags
{
    enum Enum
    {
		None				= 0,
		CopyDamageEvents	= (1 << 0),
		ReadDamageEvents	= (1 << 1),
		CopyFractureEvents	= (1 << 2),
		ReadFractureEvents	= (1 << 3),
		CopyChunkTransform	= (1 << 4),
		ReadChunkTransform	= (1 << 5),
		Last				= (1 << 6),
    };
};

/**
	Flags which control which actors are returned by NxDestructibleActor::acquirePhysXActorBuffer
*/
struct NxDestructiblePhysXActorQueryFlags
{
	enum Enum
	{
		None				= 0,

		Static				= (1 << 0),	// Destructible-static, which is a kinematic PhysX actor that hasn't been turned dynamic yet
		Dynamic				= (1 << 1),	// Dynamic, not dormant (not kinematic)
		Dormant				= (1 << 2),	// Dynamic, but dormant (had been made dynamic, but is now in a dormant, PhysX-kinematic state)

		All					= Static | Dynamic | Dormant
	};
};

/**
	Tweak-able parameters on source-side for controlling the actor sync state.
*/
struct NxDestructibleActorSyncState
{
	physx::PxU32	damageEventFilterDepth;		// dictates the (inclusive) maximum depth at which damage events will be buffered.
	physx::PxU32	fractureEventFilterDepth;	// dictates the (inclusive) maximum depth at which fracture events will be buffered.
};

/**
	Tweak-able parameters on source-side for controlling the chunk sync state. NxDestructibleActorSyncFlags::CopyChunkTransform must first be set.
*/
struct NxDestructibleChunkSyncState
{
	bool			disableTransformBuffering;	// a handy switch for controlling whether chunk transforms will be buffered this instance.
	bool			excludeSleepingChunks;		// dictates whether chunks that are sleeping will be buffered.
	physx::PxU32	chunkTransformCopyDepth;	// dictates the (inclusive) maximum depth at which chunk transforms will be buffered.
};

/**
	Destructible actor API.  The destructible actor is instanced from an NxDestructibleAsset.
*/
class NxDestructibleActor : public NxApexActor, public NxApexActorSource, public NxApexRenderable
{
public:
	/**
		Get the render mesh actor for the specified mesh type.
	*/
	virtual const NxRenderMeshActor* getRenderMeshActor(NxDestructibleActorMeshType::Enum type = NxDestructibleActorMeshType::Skinned) const = 0;

	/**
		Gets the destructible's NxDestructibleParameter block of parameters.  These are initially set from the asset.
	*/
	virtual const NxDestructibleParameters& getDestructibleParameters() const = 0;

	/**
		Sets the destructible's NxDestructibleParameter block of parameters.  These may be set at runtime.
	*/
	virtual void setDestructibleParameters(const NxDestructibleParameters& destructibleParameters) = 0;

	/**
		Gets the global pose used when the actor is added to the scene, in the NxDestructibleActorDesc
	*/
	virtual physx::PxMat44			getInitialGlobalPose() const = 0;

	/**
		Resets the initial global pose used for support calculations when the first simulation step is run.
	*/
	virtual void					setInitialGlobalPose(const physx::PxMat44& pose) = 0;

	/**
		Gets the destructible actor's 3D (possibly nonuniform) scale
	*/
	virtual physx::PxVec3			getScale() const = 0;

	/**
		Returns true iff the destructible actor starts off life dynamic.
	*/
	virtual bool					isInitiallyDynamic() const = 0;

	/**
		Returns an array of visibility data for each chunk.  Each byte in the array is 0 if the
		corresponding chunkIndex is invisible, 1 if visibile.
			visibilityArray = a pointer to the byte buffer to hold the visibility values.
			visibilityArraySize = the size of the visibilityArray
	*/
	virtual void					getChunkVisibilities(physx::PxU8* visibilityArray, physx::PxU32 visibilityArraySize) const = 0;

	/**
		Returns the number of visible chunks.  This is the number of 1's written to the visibilityArray by getChunkVisibilities.
	*/
	virtual physx::PxU32			getNumVisibleChunks() const = 0;

	/**
		Returns a pointer to an array of visible chunk indices.
	*/
	virtual const physx::PxU16*		getVisibleChunks() const = 0;

	/**
		Locks the chunk event buffer, and (if successful) returns the head of the chunk event buffer in the buffer field and the length
		of the buffer (the number of events) in the bufferSize field.  To unlock the buffer, use releaseChunkEventBuffer().
		See NxDestructibleChunkEvent.  This buffer is filled with chunk events if the NxDestructibleActor parameter createChunkEvents is set to true.
		This buffer is not automatically cleared by APEX.  The user must clear it using releaseChunkEventBuffer(true).
		Returns true if successful, false otherwise.
	*/
	virtual bool					acquireChunkEventBuffer(const physx::NxDestructibleChunkEvent*& buffer, physx::PxU32& bufferSize) = 0;

	/**
		Releases the chunk event buffer, which may have been locked by acquireChunkEventBuffer().
		If clearBuffer is true, the buffer will be erased before it is unlocked.
		Returns true if successful, false otherwise.
	*/
	virtual bool					releaseChunkEventBuffer(bool clearBuffer = true) = 0;

	/**
		Locks a PhysX actor buffer, and (if successful) returns the head of the buffer in the buffer field and the length
		of the buffer (the number of PhysX actors) in the bufferSize field.
		To unlock the buffer, use releasePhysXActorBuffer().
		The user must release this buffer before another call to releasePhysXActorBuffer.
			buffer = returned buffer, if successful
			bufferSize = returned buffer size, if successful
			flags = flags which control which actors are returned.  See NxDestructiblePhysXActorQueryFlags.
			eliminateRedundantActors = whether or not to ensure that PhysX actors are not listed more than once when this NxDestructibleActor is
				part of an extended structure.  If this is true, then some NxDestructibleActors may not return all PhysX actors associated with
				all of their chunks (and in fact may return no PhysX actors), but after querying all NxDestructibleActors in a given structure,
				every PhysX actor will be accounted for.  Default = true.
		Returns true if successful, false otherwise.
	*/
#if NX_SDK_VERSION_MAJOR == 2
	virtual bool					acquirePhysXActorBuffer(NxActor**& buffer, physx::PxU32& bufferSize, physx::PxU32 flags = NxDestructiblePhysXActorQueryFlags::All, bool eliminateRedundantActors = true) = 0;
#elif NX_SDK_VERSION_MAJOR == 3
	virtual bool					acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, physx::PxU32& bufferSize, physx::PxU32 flags = NxDestructiblePhysXActorQueryFlags::All, bool eliminateRedundantActors = true) = 0;
#endif

	/**
		Releases the PhysX actor buffer, which may have been locked by acquirePhysXActorBuffer().
		The buffer will be erased before it is unlocked.
		Returns true if successful, false otherwise.
	*/
	virtual bool					releasePhysXActorBuffer() = 0;

	/**
		Returns the PhysX actor associated with the given chunk.  Note, more than one chunk may be associated with a given PhysX actor, and
		chunks from different NxDestructibleActors may even be associated with the same PhysX actor.
		Caution is recommended when using this function.  During APEX scene simulate/fetchResults, this actor may be deleted, replaced, or tampered with.
			index = the chunk index within the actor
	*/
#if NX_SDK_VERSION_MAJOR == 2
	virtual NxActor*				getChunkPhysXActor(physx::PxU32 chunkIndex) = 0;
#elif NX_SDK_VERSION_MAJOR == 3
	virtual physx::PxRigidDynamic*	getChunkPhysXActor(physx::PxU32 chunkIndex) = 0;
#endif

	/**
		Returns a chunk's pose, without scaling.
			index = the chunk index within the actor
	*/
	virtual physx::PxMat44			getChunkPose(physx::PxU32 chunkIndex) const = 0;

	/**
		Returns a chunk's linear velocity in world space.
			index = the chunk index within the actor
	*/
	virtual physx::PxVec3			getChunkLinearVelocity(physx::PxU32 chunkIndex) const = 0;

	/**
		Returns a chunk's angular velocity in world space.
			index = the chunk index within the actor
	*/
	virtual physx::PxVec3			getChunkAngularVelocity(physx::PxU32 chunkIndex) const = 0;

	/**
		Returns the transform of the chunk's graphical representation.  This may have
		a scale component.
			index = the chunk index within the actor
	*/
	virtual const physx::PxMat44 	getChunkTM(physx::PxU32 chunkIndex) const = 0;

	/**
		Returns the behavior group index associated with the chunk.  Use getBehaviorGroup() to access the behavior group data.
		The index will either be -1, in which case it is the default behavior group, or in the range [0, getCustomBehaviorGroupCount()-1].
		Any of those values is valid for getBehaviorGroup().
	*/
	virtual physx::PxI32			getBehaviorGroupIndex(physx::PxU32 chunkIndex) const = 0;

	/**
		Set the destructible actor's global pose.  This will only be applied to the physx NxActor or PxActor
		for the static chunks, and therefore will apply to all static chunks in the structure which contains
		the this destructible actor.  This pose should not contain scale, as the scale is already contained
		in the actor's scale parameter.
	*/
	virtual void					setGlobalPose(const physx::PxMat44& pose) = 0;

	/**
		Get the destructible actor's global pose.  This will be the pose of the physx NxActor or PxActor
		for the static chunks in the structure containing this actor, if there are static chunks.  If there
		are no static chunks in the structure, pose will not be modified and false will be returned.  Otherwise
		pose will be filled in with a scale-free transform, and true is returned.
	*/
	virtual bool					getGlobalPose(physx::PxMat44& pose) = 0;

	/**
		Sets the linear velocity of every dynamic chunk to the given value.
	*/
	virtual void					setLinearVelocity(const physx::PxVec3& linearVelocity) = 0;

	/**
		Sets the angular velocity of every dynamic chunk to the given value.
	*/
	virtual void					setAngularVelocity(const physx::PxVec3& angularVelocity) = 0;

	/**
		If the indexed chunk is visible, it is made dynamic (if it is not already).
		If NxModuleDestructibleConst::INVALID_CHUNK_INDEX is passed in, all visible chunks in the
		destructible actor are made dynamic, if they are not already.
	*/
	virtual void					setDynamic(physx::PxI32 chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX) = 0;

	/**
		Returns true if the chunkIndex is valid, and the indexed chunk is dynamic.  Returns false otherwise.
	*/
	virtual bool					isDynamic(physx::PxU32 chunkIndex) const = 0;

	/**
		If "hard sleeping" is enabled, physx actors for chunk islands that go to sleep will be turned kinematic.  If a chunk
		island has chunks from more than one NxDestructibleActor, then hard sleeping will be used if ANY of the destructibles
		have hard sleeping enabled.
	*/
	virtual void					enableHardSleeping() = 0;

	/**
		See the comments for enableHardSleeping() for a description of this feature.  The disableHardSleeping function takes
		a "wake" parameter, which (if true) will not only turn kinematic-sleeping actors dynamic, but wake them as well.
	*/
	virtual void					disableHardSleeping(bool wake = false)= 0;

	/**
		Returns true iff hard sleeping is selected for this NxDestructibleActor.
	*/
	virtual bool					isHardSleepingEnabled() const = 0;

	/**
		Sets the override material.
	*/
	virtual void					setSkinnedOverrideMaterial(PxU32 submeshIndex, const char* overrideMaterialName) = 0;

	/**
		Sets the override material.
	*/
	virtual void					setStaticOverrideMaterial(PxU32 submeshIndex, const char* overrideMaterialName) = 0;

	/**
		Sets the override fracture pattern.
	*/
	virtual void					setRuntimeFractureOverridePattern(const char* overridePatternName) = 0;

	/**
		Damage
	*/

	/**
		Apply damage at a point.  Damage will be propagated into the destructible based
		upon its NxDestructibleParameters.
			damage = the amount of damage at the damage point
			momentum = the magnitude of the impulse to transfer to the actor
			position = the damage location
			direction = direction of impact.  This is valid even if momentum = 0, for use in deformation calculations.
			chunkIndex = which chunk to damage (returned by rayCast and NxModuleDestructible::getDestructibleAndChunk).
				If chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX, then the nearest visible chunk hit is found.
			userData = pointer which will be returned in damage and fracture event callbacks
	*/
	virtual void					applyDamage(physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, const physx::PxVec3& direction, physx::PxI32 chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX, void* userData = NULL) = 0;

	/**
		Apply damage to all chunks within a radius.  Damage will also propagate into the destructible
		based upon its NxDestructibleParameters.
			damage = the amount of damage at the damage point
			momentum = the magnitude of the impulse to transfer to each chunk
			position = the damage location
			radius = distance from damage location at which chunks will be affected
			falloff = whether or not to decrease damage with distance from the damage location.  If true,
				damage will decrease linearly from the full damage (at zero distance) to zero damage (at radius).
				If false, full damage is applied to all chunks within the radius.
			userData = pointer which will be returned in damage and fracture event callbacks
	*/
	virtual void					applyRadiusDamage(physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, physx::PxF32 radius, bool falloff, void* userData = NULL) = 0;

	/**
		Register a rigid body impact for impact-based damage.  Much like applyDamage, but multplies the input 'force' by the destructible's forceToDamage parameter, and also allows the user
		to report the impacting PhysX actor
	*/
#if NX_SDK_VERSION_MAJOR == 2
	virtual void					takeImpact(const physx::PxVec3& force, const physx::PxVec3& position, physx::PxU16 chunkIndex, NxActor const* damageImpactActor) = 0;
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void					takeImpact(const physx::PxVec3& force, const physx::PxVec3& position, physx::PxU16 chunkIndex, PxActor const* damageImpactActor) = 0;
#endif

#if (NX_SDK_VERSION_MAJOR == 2) || defined(DOXYGEN)
	/**
		PhysX SDK 2.8.X.
		Returns the index of the first visible chunk hit in the actor by worldRay, if any.
		Otherwise returns NxModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.
			time = (return value) of the time to the hit chunk, if any.
			normal = (return value) the surface normal of the hit chunk's collision volume, if any.
			worldRay = the ray to fire at the actor (the direction need not be normalized)
			flags = raycast control flags (see NxDestructibleActorRaycastFlags)
			parentChunkIndex = (if not equal to NxModuleDestructibleConst::INVALID_CHUNK_INDEX)
				the chunk subhierarchy in which to confine the raycast.  If parentChunkIndex =
				NxModuleDestructibleConst::INVALID_CHUNK_INDEX, then the whole actor is searched.
	*/
	virtual physx::PxI32			rayCast(physx::PxF32& time, physx::PxVec3& normal, const NxRay& worldRay, NxDestructibleActorRaycastFlags::Enum flags, physx::PxI32 parentChunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX) const = 0;
#endif
#if (NX_SDK_VERSION_MAJOR == 3) || defined(DOXYGEN)
	/**
		PhysX SDK 3.X.
		Returns the index of the first visible chunk hit in the actor by worldRay, if any.
		Otherwise returns NxModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.
			time = (return value) of the time to the hit chunk, if any.
			normal = (return value) the surface normal of the hit chunk's collision volume, if any.
			worldRay = the ray to fire at the actor (the direction need not be normalized)
			flags = raycast control flags (see NxDestructibleActorRaycastFlags)
			parentChunkIndex = (if not equal to NxModuleDestructibleConst::INVALID_CHUNK_INDEX)
				the chunk subhierarchy in which to confine the raycast.  If parentChunkIndex =
				NxModuleDestructibleConst::INVALID_CHUNK_INDEX, then the whole actor is searched.
	*/
	virtual physx::PxI32			rayCast(physx::PxF32& time, physx::PxVec3& normal, const PxVec3& worldRayOrig, const PxVec3& worldRayDir, NxDestructibleActorRaycastFlags::Enum flags, physx::PxI32 parentChunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX) const = 0;
#endif

#if (NX_SDK_VERSION_MAJOR == 2) || defined(DOXYGEN)
	/**
		Physx SDK 2.8.X.
		Returns the index of the first visible chunk hit in the actor by swept oriented bounding box, if any.
		Otherwise returns NxModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.
			time = (return value) of the time to the hit chunk, if any.
			normal = (return value) the surface normal of the hit chunk's collision volume, if any.
			worldBox = the obb to sweep against the actor, oriented in world space
			worldDisplacement = the displacement of the center of the worldBox through the sweep, in world space
			flags = raycast control flags (see NxDestructibleActorRaycastFlags)
	*/
	virtual physx::PxI32			obbSweep(physx::PxF32& time, physx::PxVec3& normal, const NxBox& worldBox, const physx::PxVec3& worldDisplacement, NxDestructibleActorRaycastFlags::Enum flags) const = 0;
#endif
#if (NX_SDK_VERSION_MAJOR == 3) || defined(DOXYGEN)
	/**
		Physx SDK 3.X.
		Returns the index of the first visible chunk hit in the actor by swept oriented bounding box, if any.
		Otherwise returns NxModuleDestructibleConst::INVALID_CHUNK_INDEX.
		If a chunk is hit, the time and normal fields are modified.
			time = (return value) of the time to the hit chunk, if any.
			normal = (return value) the surface normal of the hit chunk's collision volume, if any.
			worldBox = the obb to sweep against the actor, oriented in world space
			worldDisplacement = the displacement of the center of the worldBox through the sweep, in world space
			flags = raycast control flags (see NxDestructibleActorRaycastFlags)
	*/
	virtual physx::PxI32			obbSweep(physx::PxF32& time, physx::PxVec3& normal, const physx::PxVec3& worldBoxCenter, const physx::PxVec3& worldBoxExtents, const physx::PxMat33& worldBoxRot, const physx::PxVec3& worldDisplacement, NxDestructibleActorRaycastFlags::Enum flags) const = 0;
#endif

	/**
		Misc.
	*/
	virtual void					setLODWeights(physx::PxF32 maxDistance, physx::PxF32 distanceWeight, physx::PxF32 maxAge, physx::PxF32 ageWeight, physx::PxF32 bias) = 0;

	/**
		Enable/disable the crumble emitter
	*/
	virtual void					setCrumbleEmitterState(bool enable) = 0;

	/**
		Enable/disable the dust emitter
		Note - this is a placeholder API.  The dust feature is disabled in 1.2.0.
	*/
	virtual void					setDustEmitterState(bool enable) = 0;

	/**
		Sets a preferred render volume for a dust or crumble emitter
		Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual void					setPreferredRenderVolume(NxApexRenderVolume* volume, NxDestructibleEmitterType::Enum type) = 0;


	/**
		Returns the NxApexEmitterActor of either a dust or crumble emitter
		Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual NxApexEmitterActor*		getApexEmitter(NxDestructibleEmitterType::Enum type) = 0;

	/**
	*   Recreates the Apex Emitter, if necessary.  Use this method to re-initialize the crumble or dust emitter after a change has been made to a dependent asset
	*	Note - only crumble emitters are implemented in 1.2.0
	*/
	virtual bool					recreateApexEmitter(NxDestructibleEmitterType::Enum type) = 0;

	/**
	 * \brief Returns the actor's NxParamaterized interface
	 * This cannot be directly modified!  It is read only to the user.
	 * This handle can be used to directly serialize the complete actor state.
	 */
	virtual const ::NxParameterized::Interface*	getNxParameterized(NxDestructibleParameterizedType::Enum type = NxDestructibleParameterizedType::State) const = 0;

	/**
	 * \brief Sets the actor's state via the NxParameterized object
	 * This can be used to update the state from deserialized data.
	 * The actor assumes control of the interface.
	 */
	virtual void					setNxParameterized(::NxParameterized::Interface*) = 0;

	/**
		Set the syncing properties of the destructible actor.

		userActorID is a user-defined value used to identify the syncing actor. This value will be used to identify destructible actors between the server and client.
		userActorID = 0 is used for unregistering the actor as a syncing actor, and is the default value. The other arguments will then be forcibly set to the default (non-participating) values.
		userActorID != 0 registers the actor as a participating syncing actor.
		userActorID can be overwritten. In this case, the destructible actor which used to hold this userActorID will behave exactly like a call to set userActorID to 0.

		actorSyncFlags describes the kind of actor information that participates in syncing. See struct NxDestructibleActorSyncFlags

		actorSyncState describes information that allows finer control over the actor that participates in syncing. See struct NxDestructibleActorSyncState

		chunkSyncState describes information that allows finer control over the chunk that participates in syncing. See struct NxDestructibleChunkSyncState

		Returns true if arguments are accepted. Any one invalid argument will cause a return false. In such a case, no changes are made.
    */
    virtual bool                    setSyncParams(physx::PxU32 userActorID, physx::PxU32 actorSyncFlags = 0, const NxDestructibleActorSyncState * actorSyncState = NULL, const NxDestructibleChunkSyncState * chunkSyncState = NULL) = 0;

	/**
		Set the tracking properties of the actor for chunks that are hit. Chunks that are hit are chunks directly affected by fracture events.

		flushHistory == true indicates that both the cached chunk hit history and the cached damage event core data will be cleared. 
							To get the chunk hit history of this actor, see getHitChunkHistory()
							To get the damage coloring history of this actor, see getDamageColoringHistory()
		startTracking == true indicates that chunk hits and damage coloring will begin caching internally. The actor does not cache chunk hits by default.
		trackingDepth == the depth at which hit chunks will be cached. This value should not exceed the maximum depth level.
		trackAllChunks == true indicates that all the chunks will be cached, trackAllChunks == false indicates that only static chunks will be cached.

		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					setHitChunkTrackingParams(bool flushHistory, bool startTracking, physx::PxU32 trackingDepth, bool trackAllChunks = true) = 0;

	/**
		Get the chunk hit history of the actor. To start caching chunk hits, see setHitChunkTrackingParams()

		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					getHitChunkHistory(const NxDestructibleHitChunk *& hitChunkContainer, physx::PxU32 & hitChunkCount) const = 0;

	/**
		Force the actor to register chunk hits.

		hitChunkContainer should take in an argument that was generated from another destructible actor. See getHitChunkHistory()
		removeChunks == true indicates that the chunks given by hitChunkContainer will be forcibly removed.
		deferredEvent = whether to enable deferred event mode. If true, fracture events won�t get processed until the next tick.
		damagePosition and damageDirection: passed through to NxApexDamageEventReportData::hitPosition and hitDirection in the damage notify output by APEX.
	*/
	virtual bool					forceChunkHits(const NxDestructibleHitChunk * hitChunkContainer, physx::PxU32 hitChunkCount, bool removeChunks = true, bool deferredEvent = false, physx::PxVec3 damagePosition = physx::PxVec3(0.0f), physx::PxVec3 damageDirection = physx::PxVec3(0.0f)) = 0;

	/**
		Get the damage coloring history of the actor. To start caching damage coloring, see setHitChunkTrackingParams()

		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					getDamageColoringHistory(const NxDamageEventCoreData *& damageEventCoreDataContainer, physx::PxU32 & damageEventCoreDataCount) const = 0;

	/**
		Force the actor to register damage coloring.

		damageEventCoreDataContainer should take in an argument that was generated from another destructible actor. See getDamageColoringHistory()
		damageEventCoreDataCount is the count of damageEventCoreDataContainer.

		Returns true if the function executes correctly with the given arguments.
	*/
	virtual bool					forceDamageColoring(const NxDamageEventCoreData * damageEventCoreDataContainer, physx::PxU32 damageEventCoreDataCount) = 0;

	/**
		Accessor to get the initial locally-aligned bounding box of a destructible actor.
	*/
	virtual physx::PxBounds3		getLocalBounds() const = 0;

	/**
		Accessor to get the initial world axis-aligned bounding box of a destructible actor.
	*/
	virtual physx::PxBounds3		getOriginalBounds() const = 0;

	/**
		Accessor to query if a chunk is part of a detached island.
	*/
	virtual bool					isChunkSolitary(physx::PxI32 chunkIndex) const = 0;

	/**
		Accessor to query the axis aligned bounding box of a given chunk in world-space.
	*/
	virtual physx::PxBounds3		getChunkBounds(physx::PxU32 chunkIndex) const = 0;

	/**
		Accessor to query the axis aligned bounding box of a given chunk in chunk local-space.
	*/
	virtual physx::PxBounds3		getChunkLocalBounds(physx::PxU32 chunkIndex) const = 0;

	/**
		Accessor to query if a chunk has been destroyed.
	*/
	virtual bool					isChunkDestroyed(physx::PxI32 chunkIndex) const = 0;

	/**
		Accessor to get the array of chunk indices at the support depth.
	*/
	virtual physx::PxU32			getSupportDepthChunkIndices(physx::PxU32* const OutChunkIndices, physx::PxU32 MaxOutIndices) const = 0;

	/**
		Query the actor's support depth.
	*/
	virtual physx::PxU32			getSupportDepth() const = 0;

	/**
		Set the actor to delete its fractured chunks instead of simulating them.
    */
	virtual void					setDeleteFracturedChunks(bool inDeleteChunkMode) = 0;

	/**
		Acquire a pointer to the destructible's renderable proxy and increment its reference count.  The NxDestructibleRenderable will
		only be deleted when its reference count is zero.  Calls to NxDestructibleRenderable::release decrement the reference count, as does
		a call to NxDestructibleActor::release().  .
	*/
	virtual NxDestructibleRenderable*	acquireRenderableReference() = 0;

	/**
		Access to behavior groups created for this actor.  Each chunk has a behavior group index associated with it.

		This returns the number of custom (non-default) behavior groups.
	*/
	virtual physx::PxU32			getCustomBehaviorGroupCount() const = 0;

	/**
		Access to behavior groups created for this actor.  Each chunk has a behavior group index associated with it.

		This returns the indexed behavior group.  The index must be either -1 (for the default group) or in the range [0, getCustomBehaviorGroupCount()-1].
		If any other index is given, this function returns false.  Otherwise it returns true and the behavior descriptor is filled in.
	*/
	virtual bool					getBehaviorGroup(physx::NxDestructibleBehaviorGroupDesc& behaviorGroupDesc, physx::PxI32 index = -1) const = 0;

	// ++ LAVA ++
	virtual physx::PxF32			getActorDistanceSquaredFromViewport( physx::PxU32 index ) = 0;

	virtual bool					setActorChunksToBeDestroyed(PxActor& actor) = 0;
	// -- LAVA --
	
	virtual void setVisible(bool enable) = 0;

	virtual bool isVisible() const = 0;

protected:
	virtual							~NxDestructibleActor() {}
};

PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_DESTRUCTIBLE_ACTOR_H
