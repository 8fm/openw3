/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/object.h"

class CComponent;
class CHardAttachment;
class CMeshSkinningAttachment;
class CAnimatedAttachment;

/// Attachment spawn info
class AttachmentSpawnInfo
{
protected:
	CClass*			m_attachmentClass;		//!< Type of attachment to spawn

	// Constructor, ensures that attachmentClass member is initialized
	AttachmentSpawnInfo( CClass* attachmentClass )
		: m_attachmentClass( attachmentClass )
	{};

public:
	// Get attachment class
	RED_INLINE CClass* GetAttachmentClass() const { return m_attachmentClass; }
};

/// Attachment between two nodes
class IAttachment : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IAttachment, CObject );

	friend class CNode;
	friend class CAppearanceComponent;
	friend struct SSavedAttachments;

protected:
	CNode*		m_parent;		//!< Component that drives the attachment
	CNode*		m_child;		//!< Attached component
	Bool		m_isBroken;		//!< Attachment is broken ( usually means it's dead )
	
	// TODO - use flags
	Bool		m_isHardAttachment;
	Bool		m_isMeshSkinningAttachment;
	Bool		m_isAnimatedAttachment;

public:
	IAttachment()
		: m_parent( NULL )
		, m_child( NULL )
		, m_isBroken( false )
		, m_isHardAttachment( false )
		, m_isMeshSkinningAttachment( false )
		, m_isAnimatedAttachment( false )
	{}

	~IAttachment();

	// Get master component of this attachment
	RED_INLINE CNode* GetParent() const { return m_parent; }

	// Get slave component of this attachment
	RED_INLINE CNode* GetChild() const { return m_child; }

	// Is attachment broken ?
	RED_INLINE Bool IsBroken() const { return m_isBroken; }

public:
	// One of the components that drives this attachments was destroyed
	virtual void OnComponentDestroyed( CComponent* component );

	// Property conversion
	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// Setup attachment from attachment info
	virtual Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )=0;

	// Break the attachment
	virtual void Break();

	// Nullify the parent and child
	virtual void Nullify();

	virtual void OnFinalize();

public:
	//! Convert to transform attachment
	CHardAttachment* ToHardAttachment();

	//! Convert to skinning attachment
	CMeshSkinningAttachment* ToSkinningAttachment();

	//! Convert to animated attachment
	CAnimatedAttachment* ToAnimatedAttachment();
};

BEGIN_ABSTRACT_CLASS_RTTI( IAttachment );
	PARENT_CLASS( CObject );
	PROPERTY( m_parent );
	PROPERTY( m_child );
	PROPERTY( m_isBroken );
END_CLASS_RTTI();