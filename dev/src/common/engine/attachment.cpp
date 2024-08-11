/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "attachment.h"
#include "node.h"
#include "meshSkinningAttachment.h"
#include "animatedAttachment.h"

IMPLEMENT_ENGINE_CLASS( IAttachment );

void IAttachment::OnComponentDestroyed( CComponent* component )
{
	// Break attachment when one of the components is destroyed
	Break();
}

Bool IAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	ASSERT( !m_parent );
	ASSERT( !m_child );
	ASSERT( parent );
	ASSERT( child );

	ASSERT( !parent->HasChild( child, this->GetClass() ), TXT("Possible duplicate attachment found. Parent %s (%s) and Child %s (%s) are already attached with %s"),
		parent->GetName().AsChar(), parent->GetClass()->GetName().AsString().AsChar(),
		child->GetName().AsChar(), child->GetClass()->GetName().AsString().AsChar(),
		this->GetClass()->GetName().AsString().AsChar() );

	// Setup attachment info
	m_isBroken = false;
	m_parent = parent;
	m_child = child;

	// Attach
	if( m_parent ) m_parent->OnChildAttachmentAdded( this );
	if( m_child ) m_child->OnParentAttachmentAdded( this );

	// Attached
	return true;
}

CHardAttachment* IAttachment::ToHardAttachment()
{
	return m_isHardAttachment ? static_cast< CHardAttachment* >( this ) : nullptr;
}

CMeshSkinningAttachment* IAttachment::ToSkinningAttachment()
{
	return m_isMeshSkinningAttachment ? static_cast< CMeshSkinningAttachment* >( this ) : nullptr;
}

CAnimatedAttachment* IAttachment::ToAnimatedAttachment()
{
	return m_isAnimatedAttachment ? static_cast< CAnimatedAttachment* >( this ) : nullptr;
}

void IAttachment::Break()
{
	if ( !m_isBroken )
	{
		// Mark as broken
		m_isBroken = true;

		// Unregister attachment from components

		if ( m_parent )
		{
			m_parent->OnChildAttachmentBroken( this );
		}
		if ( m_child )
		{
			m_child->OnParentAttachmentBroken( this );
		}

		// Cleanup to prevent recursion
		Nullify();
	}
}

void IAttachment::Nullify()
{
	m_parent = NULL;
	m_child = NULL;
}

IAttachment::~IAttachment()
{
}

Bool IAttachment::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == TXT("child") )
	{
		m_child = *( CNode** ) readValue.GetData(); 
		return true;
	}

	if ( propertyName == TXT("parent") )
	{
		m_parent = *( CNode** ) readValue.GetData(); 
		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

void IAttachment::OnFinalize()
{
	if ( !GIsCooker)
	{
		Break();
	}

	TBaseClass::OnFinalize();
}
