/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/renderVisibilityExclusion.h"
#include "../../common/core/idAllocator.h"

/// ID used for object registered in this system, 0 means NULL
typedef Uint32 LocalVisObjectID;

//--------------------------------------------------------

/// List of objects to exclude
class CRenderVisibilityExclusionList : public IRenderVisibilityExclusion
{
public:
	CRenderVisibilityExclusionList( const GlobalVisID* ids, const Uint32 numIDs, const Uint8 renderMask, const Bool isEnabled );
	virtual ~CRenderVisibilityExclusionList();

	//! Is it enabled ?
	RED_INLINE const Bool IsEnabled() const { return m_isEnabled; }

	//! Get exclusion render mask
	RED_INLINE const Uint8 GetRenderMask() const { return m_renderMask; }

	//! Global object IDs (read only)
	RED_INLINE const GlobalVisID* GetGlobalObjects() const { return m_globalObjects.TypedData(); }

	//! Get local object IDs (mappable)
	RED_INLINE LocalVisObjectID* GetLocalObjects() { return m_localObjects.TypedData(); }

	//! Get local object IDs (read only)
	RED_INLINE const LocalVisObjectID* GetLocalObjects() const { return m_localObjects.TypedData(); }

	//! Get number of objects
	virtual const Uint32 GetNumObjects() const override { return m_globalObjects.Size(); }

	//! Set general state (NOTE: refresh in the visibility map is required to reflect this state)
	void SetState( const Bool isEnabled );

	//! Register object in list, returns index under which we got registered or -1 if not found
	Int32 RegisterObject( const GlobalVisID globalId, const LocalVisObjectID localId );

	//! Unregister object, returns true if successful
	Bool UnegisterObject( const GlobalVisID globalId, const LocalVisObjectID localId );

private:
	Bool								m_isEnabled;
	Uint8								m_renderMask;

	TDynArray< LocalVisObjectID, MC_RenderVisibility >		m_localObjects; // 0 if not mapped
	TDynArray< GlobalVisID, MC_RenderVisibility >			m_globalObjects; // used only for remapping

	// mapping between global ID and local table index
	typedef THashMap< GlobalVisID, Uint16 >		TGlobalToIndex;
	TGlobalToIndex						m_globalIndexMap;
};

//--------------------------------------------------------

/// Manages objects and their basic render mask
class CRenderVisibilityExclusionMap
{
public:
	CRenderVisibilityExclusionMap();
	~CRenderVisibilityExclusionMap();

	/// Register proxy with given render mask, returns unique ID - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	LocalVisObjectID RegisterProxy( const IRenderProxyBase* proxy, const GlobalVisID globalID );

	/// Unregister proxy from visibility map - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	void UnregisterProxy( const IRenderProxyBase* proxy, const GlobalVisID globalID, const LocalVisObjectID localID );

	/// Add exclusion list - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	void AddList( CRenderVisibilityExclusionList* objectList );

	/// Remove exclusion list - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	void RemoveList( CRenderVisibilityExclusionList* objectList );

	/// Mark list as dirty - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	void MarkDirty( CRenderVisibilityExclusionList* objectList );

	/// Update lists - render thread only
	// NOTE: no mutex here, caller is responsible for thread safety of this code
	void PrepareForQueries();

	/// Get computed render mask table
	RED_INLINE const Uint8* GetFilterMasks() const { return m_objectFilterMasks.TypedData(); }

	/// Get number of objects in the list
	RED_INLINE const Uint32 GetMaxValidLocalId() const { return m_maxValidLocalId; }
	
private:
	/// Registered rendering exclusion lists - usually not very big array, <100
	TDynArray< CRenderVisibilityExclusionList*, MC_RenderVisibility >		m_exclusionLists;
	Bool																	m_fullRecompute; // reevaluate the whole masks

	/// Free objects IDs
	IDAllocatorDynamic					m_idAllocator;

	/// Maximum used object
	Uint32								m_maxValidLocalId;

	/// Registered objects
	TDynArray< const IRenderProxyBase*, MC_RenderVisibility >					m_objectProxies;			// used only for registering/unregistering
	TDynArray< Uint8, MC_RenderVisibility >										m_objectFilterMasks;		// flattened filter mask for each object (used many times every frame)

	/// GlobalVisID -> LocalVisID map, updated during register/unregister proxy, accessed during remap
	THashMap< GlobalVisID, LocalVisObjectID >			m_globalToLocalMap;
};

//--------------------------------------------------------

/// Cheap ass query system for the visibility exclusion list
class CRenderVisibilityExclusionTester
{
public:
	CRenderVisibilityExclusionTester();
	~CRenderVisibilityExclusionTester();

	/// Setup for given map
	void Setup( const CRenderVisibilityExclusionMap& visibilityMap );

	/// Get merged visibility mask
	RED_INLINE const Uint8 GetMaskFilterForLocalID( const LocalVisObjectID id ) const
	{
		RED_FATAL_ASSERT( id != 0, "Invalid local vis ID" );
		RED_FATAL_ASSERT( id < m_maxObjects, "Invalid local vis ID" );
		return m_mask[ id ];
	}

private:
	Uint32			m_maxObjects;
	const Uint8*	m_mask;
};

//--------------------------------------------------------
