/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//--------------------------------------------------------------------------

#include "triggerManager.h"
#include "triggerMaskBuffer.h"
#include "globalSpatialTree.h"
#include "globalEventsManager.h"
#include "areaComponent.h"

//--------------------------------------------------------------------------

/// Debug event listener
class ITriggerManagerDebugEventListener
{
public:
	virtual ~ITriggerManagerDebugEventListener() {};

	//! Activator created
	virtual void OnActivatorCreated(const class CTriggerActivator* activator) = 0;

	//! Activator removed
	virtual void OnActivatorRemoved(const class CTriggerActivator* activator) = 0;

	//! Trigger object added
	virtual void OnTriggerCreated(const class CTriggerObject* object) = 0;

	//! Trigger object removed
	virtual void OnTriggerRemoved(const class CTriggerObject* object) = 0;

	//! Activator entered trigger
	virtual void OnTriggerEntered(const class CTriggerObject* object, const class CTriggerActivator* activator) = 0;

	//! Activator entered exited
	virtual void OnTriggerExited(const class CTriggerObject* object, const class CTriggerActivator* activator) = 0;
};

//--------------------------------------------------------------------------

class CTriggerConvex;
typedef TGlobalSpatialTree< CTriggerConvex, MC_TriggerSystem > TTriggerCovnexTree;

class CTriggerActivator;
typedef TGlobalSpatialTree< CTriggerActivator, MC_TriggerSystem > TTriggerActivatorTree;

/// Source trigger convex shape
class CTriggerConvex
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_TriggerSystem );

public:
	// Parent trigger object
	class CTriggerObject* m_object;

	// Source gemometry reference
	const class CAreaShape* m_shape;

	// Index of the convex shape in the geometry
	Uint32 m_convexIndex;

	// Convex reference point (for planes)
	IntegerVector4 m_referencePosition;

	// Cached world space planes (but with local offset)
	Vector* m_referencePlanes;
	Uint32 m_numReferencePlanes;

	// Placement token
	typedef TTriggerCovnexTree::Token TTreeToken;
	TTreeToken* m_token;

public:
	CTriggerConvex(class CTriggerObject* object, const class CAreaShape* shape, const Uint32 convexIndex);
	~CTriggerConvex();

	// Remove from the tree
	void RemovePlacement(TTriggerCovnexTree& tree);

	// Recalculate plane equations in reference frame and bounds in world space
	void UpdatePlacement(TTriggerCovnexTree& tree);

	// Test simple box intersection
	Bool TestOverlap( const IntegerBox& worldBox ) const;

	// Test point overlap
	Bool TestOverlap( const Vector& worldPoint ) const;

	// Intersect a line segment, returns entry and exit times
	Bool IntersectSegment( const IntegerVector4& start, const IntegerVector4& end, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const;

	// Debug functionality - render outline of this convex in world space
	void RenderOutline( CRenderFrame* frame, const Color& color, Bool overlap ) const;

	// Calculate memory usage for this convex object
	const Uint32 CalcMemoryUsage() const;
};

//---------------------------------------------------------------------------

/// Base interface for trigger object
class CTriggerObject : public ITriggerObject
{
	friend class CTriggerManager;

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_TriggerSystem );

private:
	// Parent manager
	class CTriggerManager* m_manager;

	// Source gemometry reference
	class CAreaShape* m_shape;

	// Callback interface
	class ITriggerCallback* m_callback;

	// Current position
	IntegerVector4 m_position;

	// Extra transfrom (no translation, only rotation & scaling)
	Matrix m_localToReference;
	Matrix m_localToReferenceInvTrans;

	// Last local to world matrix (used for filtering the updates)
	Matrix m_localToWorld;

	// List of shapes
	typedef TDynArray<CTriggerConvex*> TConvexList;
	TConvexList m_convexList;

	// Trigger object ID (unique during the trigger manager lifetime)
	Uint32 m_objectID;

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	// Internal name for debugging
	const String m_debugName;
#endif

	// Trigger channel mask
	Uint32 m_includedChannels;
	Uint32 m_excludedChannels;

	// Beveling (world space)
	Float m_bevelRadius;

	// Beveling radius (world space, Z only)
	Float m_bevelRadiusVertical;

	// We need position update
	Uint8 m_flagHasMoved:1;

	// Are we using CCD for this object ?
	Uint8 m_flagUseCCD:1;

	// Can this trigger be activated above the terrain
	Uint8 m_flagTerrainMask:2; // bit 0-above, bit 1-below

	// Bounded component
	class CAreaComponent* m_component;

	// Internal reference count
	Red::Threads::CAtomic< Int32 >	m_refCount;

public:
	//! Get the current positon
	RED_FORCE_INLINE const IntegerVector4& GetPosition() const { return m_position; }

	//! Get current reference transform
	RED_FORCE_INLINE const Matrix& GetLocalToReference() const { return m_localToReference; }

	//! Get current reference transform for normal transform
	RED_FORCE_INLINE const Matrix& GetLocalToReferenceInvTrans() const { return m_localToReferenceInvTrans; }

	//! Get current local to world matrix (not used directly for compuations)
	RED_FORCE_INLINE const Matrix& GetLocalToWorld() const { return m_localToWorld; }

	//! Get beveling radius
	RED_FORCE_INLINE const Float GetBevelRadius() const { return m_bevelRadius; }

	//! Get beveling radius for vertical normals
	RED_FORCE_INLINE const Float GetBevelRadiusVertical() const { return m_bevelRadiusVertical; }

	//! Get the included channels mask
	RED_FORCE_INLINE const Uint32 GetIncludedChannels() const { return m_includedChannels; }

	//! Get the excluded channels mask
	RED_FORCE_INLINE const Uint32 GetExcludedChannels() const { return m_excludedChannels; }

	//! Get ID of the trigger object
	RED_FORCE_INLINE const Uint32 GetID() const { return m_objectID; }

	//! Get number of convex pieces
	RED_FORCE_INLINE const Uint32 GetConvexNum() const { return m_convexList.Size(); }

	//! Get convex piece
	RED_FORCE_INLINE const CTriggerConvex* GetConvex( const Uint32 index ) const { return m_convexList[index]; }

	//! Get the source area shape object
	RED_FORCE_INLINE const CAreaShape* GetSourceShape() const { return m_shape; }

	//! Is the object valid (registered in the manager)
	RED_FORCE_INLINE const Bool IsValid() const { return (NULL != m_manager); }

	//! Get terrain mask
	RED_FORCE_INLINE const Uint8 GetTerrainMask() const { return m_flagTerrainMask; }

	//! Does this object require terrain filtering ?
	RED_FORCE_INLINE const Bool UsesTerrainFiltering() const { return (m_flagTerrainMask != 3); }

public:
	CTriggerObject( class CTriggerManager* manager, const CTriggerObjectInfo& info, const Uint32 uniqueID );
	virtual ~CTriggerObject();

	// ITriggerObject interface
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	virtual const String& GetDebugName() const;
#endif
	virtual class CAreaComponent* GetComponent() const;
	virtual const Bool IsCCDEnabled() const;
	virtual void SetMask(const Uint32 inclusionMask, const Uint32 exclusionMask);
	virtual void SetPosition(const Matrix& matrix);
	virtual void SetTerrainMask(const Bool allowAboveTerrain, const Bool allowBelowTerrain);
	virtual void Render(class CRenderFrame* frame);
	virtual void EnableCCD(const Bool isCCDEnabled);
	virtual Bool TestPoint( const Vector& worldPoint ) const;
	virtual Bool TestBox( const Vector& center, const Vector& extents ) const;
	virtual Bool TraceBox( const Vector& start, const Vector& end, const Vector& extents, Float& inTime, Float& outTime ) const;
	virtual void Remove();
	virtual void Release();

public:
	// Add a reference
	void AddRef();

	// Release manager pointer (makes the object dead)
	void UnlinkManager();

	// Recalculate bounds and placement of the trigger
	void UpdatePlacement(TTriggerCovnexTree& tree);

	// Check if given trigger channel can interact with this trigger type
	Bool CanInteractWith(const Uint32 channel) const;

	// Check if given activator can interact with this trigger type
	Bool CanInteractWith(const class CTriggerActivator& activator) const;

	// Trigger object was entered by an activator
	void OnEnter(const CTriggerActivator* activator) const;

	// Trigger object was exited by an activator
	void OnExit(const CTriggerActivator* activator) const;

	// Invalidate all activators touching this trigger
	void InvalidateNearbyActivators();

	// Intersect a line segment, returns entry and exit times
	Bool IntersectSegment( const IntegerVector4& start, const IntegerVector4& end, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const;

	// Calculate memory usage for this convex object
	const Uint32 CalcMemoryUsage() const;

private:
	CTriggerObject(const CTriggerObject& other) {};
	CTriggerObject& operator=(const CTriggerObject& other) { return *this; }
};

//--------------------------------------------------------------------------

class CTriggerActivator : public ITriggerActivator
{
#ifdef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_TriggerSystem );
#else
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_TriggerSystem );
#endif
	
	friend class CTriggerManager;

private:
	// Parent manager
	class CTriggerManager* m_manager;

	// Internal, unique ID
	Uint32 m_uniqueID;

	// Current position
	IntegerVector4 m_position;

	// New position
	IntegerVector4 m_newPosition;

	// Maximum length of CCD movement
	Float m_maxCCDDistance;

	// We need position update
	Uint8 m_flagHasMoved:1;

	// Are we using CCD for this activator ?
	Uint8 m_flagUseCCD:1;

	// Is the movement a teleportation
	Uint8 m_flagTeleported:1;

	// Extents (size)
	IntegerVector4 m_extents;

	// Trigger channel mask
	Uint32 m_channelMask;

	// List of trigger object IDs that we are "inside"
	typedef TDynArray< Uint32 > TObjectIDs;
	TObjectIDs m_interactions;

#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	// Internal name for debugging
	String m_debugName;
#endif
	
	// Bound component
	class CComponent* m_component;

	// Placement token
	typedef TTriggerActivatorTree::Token TTreeToken;
	TTreeToken* m_token;

	// Internal reference count
	Red::Threads::CAtomic< Int32 >	m_refCount;

public:
	//! Get internal, unique ID
	RED_FORCE_INLINE const Uint32 GetUniqueID() const { return m_uniqueID; }

	//! Get the channel mask
	RED_FORCE_INLINE const Uint32 GetChannels() const { return m_channelMask; }

	//! Get activator extents
	RED_FORCE_INLINE const IntegerVector4& GetExtents() const { return m_extents; }

	//! Get activator position
	RED_FORCE_INLINE const IntegerVector4& GetPosition() const { return m_position; }

	//! Get new activator position
	RED_FORCE_INLINE const IntegerVector4& GetNewPosition() const { return m_newPosition; }

	//! Get number of trigger objects we are listed to be inside 
	RED_FORCE_INLINE const Uint32 GetNumInteractions() const { return m_interactions.Size(); }

	//! Get n-th trigger object ID
	RED_FORCE_INLINE const Uint32 GetInteraction( const Uint32 index ) const { return m_interactions[index]; }

	//! Get maximum allowed distance for CCD tests
	RED_FORCE_INLINE const Float GetCCDMaxDistance() const { return m_maxCCDDistance; }

	//! Is the object valid (registered in the manager)
	RED_FORCE_INLINE const Bool IsValid() const { return (NULL != m_manager); }

public:
	CTriggerActivator(class CTriggerManager* manager, const CTriggerActivatorInfo& initInfo, const Uint32 uniqueID );
	virtual ~CTriggerActivator();

	// Remove placement from the tree
	void RemovePlacement( TTriggerActivatorTree& tree );

	// Update placement of trigger activator in the tree
	void UpdatePlacement( TTriggerActivatorTree& tree );

	// Invalidate position of this activator ( schedules entry/exit tests on the next update )
	void InvalidatePlacement();

	// Object (a trigger) was removed, make sure we have no references to it
	void UnregisterTriggerObject( const CTriggerObject* object );

	// Add a reference
	void AddRef();

	// Release manager pointer (makes the object dead)
	void UnlinkManager();

	// ITriggerActivator interface
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
	virtual const String& GetDebugName() const;
#endif
	virtual class CComponent* GetComponent() const;
	virtual const Uint32 GetMask() const;
	virtual const Bool IsCCDEnabled() const;
	virtual void SetExtents(const Vector& extents);
	virtual void SetMask(const Uint32 mask);
	virtual void EnableCCD(const Bool isCCDEnabled);
	virtual void Move(const IntegerVector4& position, const Bool teleporatation = false);
	virtual void Render(class CRenderFrame* frame);
	virtual void Remove();
	virtual void Release();
	virtual const ITriggerObject* GetOccupiedTrigger(const Uint32 index) const;
	virtual const Uint32 GetNumOccupiedTrigger() const;
	virtual const Bool IsInsideTrigger( const ITriggerObject* trigger ) const override;
};

//--------------------------------------------------------------------------

/// Trigger manager
class CTriggerManager : public ITriggerManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_TriggerSystem );

private:
	// Registered triggers
	typedef TDynArray< CTriggerObject* > TObjects;
	TObjects m_objects;
	TObjects m_movedObjects;

	// Registered activators
	typedef TDynArray< CTriggerActivator* > TActivators;
	TActivators m_activators;
	TActivators m_movedActivators;

	// Object ID managment
	typedef TDynArray< Uint32 > TFreeObjectIDs;
	TFreeObjectIDs m_freeObjectIDs;
	Uint32 m_nextFreeObjectID;

	// ActivatorID management
	Uint32 m_nextFreeActivatorID;

	// General tree for the trigger convex parts
	TTriggerCovnexTree m_convexTree;

	// General tree for the trigger activators
	TTriggerActivatorTree m_activatorTree;

	// Temporary buffer mask
	CTriggerMaskBuffer m_maskBuffer;

	// Special debug listener
	ITriggerManagerDebugEventListener* m_debugListener;

	// Terrain system integration interface
	ITriggerSystemTerrainQuery* m_terrainQuery;

	// Internal lock
	Red::Threads::CMutex m_lock;

	// List of objects to remove
	TObjects m_objectsToRemove;

	// List of activators to remove
	TActivators m_activatorsToRemove;

	// To prevent from removing objects / activators inside Update (quick fix)
	Bool m_isUpdating;

	// For reporting global events
	typedef TGlobalEventsReporterImpl< CComponent*, TGlobalEventsReporterStorageByHandle< CComponent > > TTriggerGlobalEventsReporter;
	TTriggerGlobalEventsReporter*	m_globalEventsReporter;

public:
	//! Get the tree for the conviex pieces
	RED_INLINE TTriggerCovnexTree& GetConvexTree() { return m_convexTree; }

	//! Get size of the object table (may contain empty slots)
	RED_INLINE const Uint32 GetNumObjects() const { return m_objects.Size(); }

	//! Get n-th object (can be NULL due to empty places in the table)
	RED_INLINE const CTriggerObject* GetObject( const Uint32 index ) const { return m_objects[ index ]; }

	//! Get number of registered trigger activators
	RED_INLINE const Uint32 GetNumActivators() const { return m_activators.Size(); }

	//! Get n-th activator
	RED_INLINE const CTriggerActivator* GetActivator( const Uint32 index ) const { return m_activators[ index ]; }

public: 
	CTriggerManager();
	virtual ~CTriggerManager();

	//! ITerrainManager interface
	virtual void SetTerrainIntegration( ITriggerSystemTerrainQuery* queryInterface ) override;
	virtual CTriggerObject* CreateTrigger(const CTriggerObjectInfo& initInfo) override;
	virtual CTriggerActivator* CreateActivator(const CTriggerActivatorInfo& initInfo) override;
	virtual void Update() override;
	virtual void Render(class CRenderFrame* frame) override;
	virtual void GetTriggersAtPoint( const Vector& worldPosition, const Uint32 triggerChannelMask, TResultTriggers& outTriggers ) override;

public:
	//! Lock updates to the tree (thread safety)
	void LockUpdates();

	//! Unlock updates to the tree (thread safety)
	void UnlockUpdates();

	//! Register object for removal
	void RemoveObject(CTriggerObject* obj);

	//! Register activator for removal
	void RemoveActivator(CTriggerActivator* activator);

private:

	//! Remove object from manager
	void RemoveObjectInternal(CTriggerObject* obj);

	//! Remove activator from manager
	void RemoveActivatorInternal(CTriggerActivator* activator);

public:

	//! Register trigger object in the movement list
	void RegisterObjectMovement(CTriggerObject* obj);

	//! Register trigger activator movement
	void RegisterActivatorMovement(CTriggerActivator* obj);

	//! Process movement of all trigger objetcts (convex tree update)
	void ProcessObjectsMovement();

	//! Process movement of all trigger activators (will generate entry/exit events)
	void ProcessActivatorsMovement();

	//! Invalidate activators in the area (force them to reevaluate entry/extis on next updade)
	void InvalidateActivatorsFromArea( const IntegerBox& worldBox );

public:
	//! Set custom debug event listener
	void SetDebugEventListener( ITriggerManagerDebugEventListener* debugListener );
};

//--------------------------------------------------------------------------

