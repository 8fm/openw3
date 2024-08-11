/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "slot.h"
#include "slotProvider.h"
#include "skeleton.h"
#include "component.h"

/// Slot component specific slot type
class CIndirectSlot : public ISlot
{
	DECLARE_ENGINE_CLASS( CIndirectSlot, ISlot, 0 );
	NO_DEFAULT_CONSTRUCTOR( CIndirectSlot );

protected:
	Uint32		m_slotIndex;		//!< Index to the slot

public:
	//! Get the slot index
	RED_INLINE Uint32 GetSlotIndex() const { return m_slotIndex; }

public:
	// Constructor
	CIndirectSlot( Uint32 slotIndex );

	// Calculate slot matrix
	virtual Matrix CalcSlotMatrix() const;
};

BEGIN_CLASS_RTTI( CIndirectSlot );
PARENT_CLASS( ISlot );
PROPERTY_RO( m_slotIndex, TXT("Slot index") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct SSlotInfo
{
	DECLARE_RTTI_STRUCT( SSlotInfo );

	CName	m_slotName;				//!< Name of this slot, copied from the bone name in extender skeleton
	CName	m_parentSlotName;		//!< Name of the parent slot, expected to be available in parent entity's animated component, can be None
	Int32		m_parentSlotIndex;		//!< Index of the parent slot
	Vector	m_relativePosition;		//!< Relative to parent bone position
	EulerAngles m_relativeRotation;	//!< Relative to parent bone rotation

	void Set( CName slotName, CName parentSlotName, Int32 parentSlotIndex, const Vector& pos, const EulerAngles& rot )
	{
		m_slotName = slotName;
		m_parentSlotName = parentSlotName;
		m_relativePosition = pos;
		m_relativeRotation = rot;
		m_parentSlotIndex = parentSlotIndex;
	}
};

BEGIN_CLASS_RTTI( SSlotInfo );
	PROPERTY_RO( m_slotName, TXT("Name of the slot") );
	PROPERTY_RO( m_parentSlotName, TXT("Name of the parent slot") );
	PROPERTY_RO( m_parentSlotIndex, TXT("Index of the parent slot") );
	PROPERTY_RO( m_relativePosition, TXT("Position offset") );
	PROPERTY_RO( m_relativeRotation, TXT("Rotation offset") );
END_CLASS_RTTI();

class CSlotComponent : public CComponent, protected ISlotProvider
{
	DECLARE_ENGINE_CLASS( CSlotComponent, CComponent, 0 );

	struct SBoneInfo
	{
		Int32		m_index;		//!< Bone index in extended skeleton
		CName		m_name;			//!< Bone name
		Int32		m_parentIndex;	//!< Parent bone index in root skeleton
		CName		m_parentName;	//!< Parent bone name

		SBoneInfo() : m_parentIndex( -1 ) {}

		friend Bool operator==( const SBoneInfo& lhs, const SBoneInfo& rhs )
		{
			return ( lhs.m_name == rhs.m_name ) && 
				( lhs.m_parentIndex == rhs.m_parentIndex ) &&
				( lhs.m_parentName == rhs.m_parentName );
		}
	};

protected:
	TDynArray< SSlotInfo >			m_slots;			//!< Array of slots this component provides
	TSoftHandle< CSkeleton >		m_sourceSkeleton;	//!< Skeleton being the source for slots, not serialized at all

public:
	//! Get slots array
	RED_INLINE const TDynArray< SSlotInfo >& GetSlots() const { return m_slots; }

public:
	//! Create a new set of slot infos based on given skeleton and root animated component of parent entity
	Bool ParseSkeleton( const CSkeleton* skeleton );	

	//! Parse all given skeleton bones as slots
	Bool ParseSkeletonNoRootSkeleton( const CSkeleton* skeleton );

	//! Get current skeleton, source skeleton is nullified on save, so this can return NULL
	CSkeleton* GetSourceSkeleton() const { return m_sourceSkeleton.Get(); }

	//! Get slot by name
	const SSlotInfo* GetSlotByName( CName slotName ) const;

	//! Get number of slots
	Uint32 GetNumberOfSlots() const;

	//! Get world transform matrix for the slot
	Matrix GetSlotWorldTransform( Uint32 slotIndex ) const;

	//! Get transform matrix for the slot
	Matrix GetSlotTransform( Uint32 slotIndex ) const;

	//! Setup based on animated component
	void BaseOn( const CAnimatedComponent* animComponent );

public:
	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	
#ifndef NO_DATA_VALIDATION
	// Validation
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

	// Called after object's property is changed in the editor
	virtual void OnPropertyPostChange( IProperty* property );

	//! Get slot provider interface
	virtual ISlotProvider* QuerySlotProvider();
	
	// Create slot
	virtual ISlot* CreateSlot( const String& slotName );

	// Enumerate all available slots
	virtual void EnumSlotNames( TDynArray< String >& slotNames ) const;

protected:
	//! Extract bones info from skeleton
	Bool ExtractBonesInfo( const CSkeleton* skeleton, TDynArray< SBoneInfo >& bones ) const;

	//! Check if bone is present in given skeleton
	Int32 FindBoneIndex( const CSkeleton* skeleton, CName bone ) const;
};

BEGIN_CLASS_RTTI( CSlotComponent )
	PARENT_CLASS( CComponent )
	PROPERTY_RO( m_slots, TXT("Slots in this component") );
	PROPERTY_EDIT_NOT_COOKED( m_sourceSkeleton, TXT("Source skeleton") );
END_CLASS_RTTI()