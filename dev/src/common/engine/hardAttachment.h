/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "attachment.h"
#include "../core/bitFieldBuilder.h"
#include "../core/engineTransform.h"

class ISlot;

/// Hard attachment spawn info
class HardAttachmentSpawnInfo : public AttachmentSpawnInfo
{
public:
	Vector			m_relativePosition;		//!< Local relative position
	EulerAngles		m_relativeRotation;		//!< Local relative rotation
	CName			m_parentSlotName;		//!< Name of the slot
	Bool			m_freePositionAxisX;	//!< Free position axis X
	Bool			m_freePositionAxisY;	//!< Free position axis Y
	Bool			m_freePositionAxisZ;	//!< Free position axis Z
	Bool			m_freeRotation;			//!< Do not lock rotation to the parent slot

public:
	HardAttachmentSpawnInfo();
};

/// Hard attachment flags
enum EHardAttachmentFlags
{
	HAF_FreePositionAxisX		= FLAG( 0 ),		//!< Free movement in axis X
	HAF_FreePositionAxisY		= FLAG( 1 ),		//!< Free movement in axis Y
	HAF_FreePositionAxisZ		= FLAG( 2 ),		//!< Free movement in axis Z
	HAF_FreeRotation			= FLAG( 3 ),		//!< Free rotation
};

BEGIN_BITFIELD_RTTI( EHardAttachmentFlags, 1 );
	BITFIELD_OPTION( HAF_FreePositionAxisX );
	BITFIELD_OPTION( HAF_FreePositionAxisY );
	BITFIELD_OPTION( HAF_FreePositionAxisZ );
	BITFIELD_OPTION( HAF_FreeRotation );
END_BITFIELD_RTTI();

/// Hard attachment
class CHardAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CHardAttachment, IAttachment, 0 )

protected:
	EngineTransform		m_relativeTransform;	//!< Relative attachment transform
	CName				m_parentSlotName;		//!< Name of the slot in the parent component
	Uint8				m_attachmentFlags;		//!< Attachment flags
	ISlot*				m_parentSlot;			//!< Parent slot we are attached to

public:
	CHardAttachment();

	//! Get parent slot ( if used )
	RED_INLINE ISlot* GetParentSlot() const { return m_parentSlot; }

	//! Get parent slot name ( if used )
	RED_INLINE CName GetParentSlotName() const { return m_parentSlotName; }

	//! Get relative transform
	RED_INLINE const EngineTransform& GetRelativeTransform() const { return m_relativeTransform; }

	//! Get attachment flags
	RED_INLINE Uint8 GetAttachmentFlags() const { return m_attachmentFlags; }

public:
	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info );

	void Break() override;

	// Property modified
	virtual void OnPropertyPostChange( IProperty* property );
	
	// Calculate final localToWorld matrix for attached component
	virtual void CalcAttachedLocalToWorld( Matrix& out ) const;

public:
	// Set relative attachment position
	void SetRelativePosition( const Vector& position );

	// Set relative attachment rotation
	void SetRelativeRotation( const EulerAngles& rotation );

	// Set relative scale
	void SetRelativeScale( const Vector& scale );
		
	// Remove relative transform from attachment
	void RemoveRelativeTransform();

public:
	// Create slot
	Bool SetupSlot( CNode* parent, CName slotName );
};

BEGIN_CLASS_RTTI( CHardAttachment );
	PARENT_CLASS( IAttachment );
	PROPERTY_EDIT( m_relativeTransform, TXT("Relative attachment transform") );
	PROPERTY_CUSTOM_EDIT( m_parentSlotName, TXT("Name of the slot in the parent component"), TXT("EdHardAttachmentBonePicker") );
	PROPERTY_BITFIELD_EDIT( m_attachmentFlags, EHardAttachmentFlags, TXT("Attachment flags") );
	PROPERTY_INLINED_RO( m_parentSlot, TXT("Slot") );
END_CLASS_RTTI();