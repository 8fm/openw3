/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "slot.h"

/// Slot that represents a bone in skeleton
class CSkeletonBoneSlot : public ISlot
{
	DECLARE_ENGINE_CLASS( CSkeletonBoneSlot, ISlot, 0 );
	NO_DEFAULT_CONSTRUCTOR( CSkeletonBoneSlot );
	
protected:
	Uint32		m_boneIndex;			//!< Index to the bone

public:
	Matrix		m_lastBoneTransform;	//!< Bone transform from last time this slot was updated

public:
	//! Get the bone index
	RED_INLINE Uint32 GetBoneIndex() const { return m_boneIndex; }

public:
	// Constructor
	CSkeletonBoneSlot( Uint32 boneIndex );

	// Calculate slot matrix
	virtual Matrix CalcSlotMatrix() const;
};

BEGIN_CLASS_RTTI( CSkeletonBoneSlot );
	PARENT_CLASS( ISlot );
	PROPERTY_RO( m_boneIndex, TXT("Bone index") );
END_CLASS_RTTI();
