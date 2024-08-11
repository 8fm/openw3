/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"
#include "../core/softHandleProcessor.h"
#include "../core/sharedPtr.h"
#include "foliageDynamicInstanceService.h"

class CSRTBaseTree;

// CSwitchableFoliageComponent

struct SSwitchableFoliageEntry
{
	DECLARE_RTTI_STRUCT( SSwitchableFoliageEntry );

	CName m_name;
	TSoftHandle< CSRTBaseTree > m_tree;
};

BEGIN_CLASS_RTTI( SSwitchableFoliageEntry );
	PROPERTY_EDIT( m_name, TXT("Entry name") );
	PROPERTY_EDIT( m_tree, TXT("Tree resource") );
END_CLASS_RTTI();

// Resource encapsulating multiple tree resources
class CSwitchableFoliageResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSwitchableFoliageResource, CResource, "w2sf", "Switchable Foliage" );

private:
	mutable TDynArray< SSwitchableFoliageEntry > m_entries;

public:
	static const Uint32 InvalidEntryIndex = 0xFFFFFFFF;

	// Gets tree resource by index; return nullptr if still being streamed or invalid
	CSRTBaseTree* GetAsync( Uint32 index );

	// Gets entry index by name
	Uint32 GetEntryIndex( CName name );

	// Gets entry name under given index
	CName GetEntryName( Uint32 index );
};

BEGIN_CLASS_RTTI( CSwitchableFoliageResource );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_entries, TXT("Named tree entries") )
END_CLASS_RTTI();

// Similar to CDynamicFoliageComponent but optimized for when used in large amounts and made asynchronous
class CSwitchableFoliageComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CSwitchableFoliageComponent, CComponent, 0 )

public:
	CSwitchableFoliageComponent(void);
	~CSwitchableFoliageComponent(void);

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnDestroyed() override;
	virtual Uint32 GetMinimumStreamingDistance() const override;

#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged() override;
	virtual void EditorOnTransformChangeStop() override;
#endif
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	// Sets tree entry (from resource) by name
	void SetEntry( CName name );

private:
	Bool CreateFoliageInstance();
	void DestroyFoliageInstance();
	void UpdateInstancePosition();

	CSRTBaseTree* GetBaseTree( Uint32 entryIndex );
	virtual Bool OnCreateEngineRepresentation() override;
	virtual void OnDestroyEngineRepresentation() override;

	mutable THandle< CSwitchableFoliageResource > m_resource;
	CFoliageDynamicInstanceService m_foliageController;

	Uint32 m_minimumStreamingDistance;

	Bool m_isCreated;				// Is foliage instance in the world created?
	Bool m_isSuppressed;			// Is foliage creation suppressed (due to streaming distances)?
	Uint32 m_entryIndex;			// Current entry index
	Uint32 m_pendingEntryIndex;		// Target entry index (if different from m_entryIndex, then it means we're transitioning to it)

#ifndef NO_EDITOR
	Vector m_oldInstancePosition;	//!< To keep track of position only in editor
	CSRTBaseTree* m_oldBaseTree;	//!< For swap functionality only in editor
#endif

	void funcSetEntry( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CSwitchableFoliageComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_resource, TXT("Resource"))
	PROPERTY_EDIT( m_minimumStreamingDistance, TXT( "Minimum streaming distance" ) );
	NATIVE_FUNCTION( "SetEntry", funcSetEntry );
END_CLASS_RTTI();
