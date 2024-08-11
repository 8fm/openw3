/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "hardAttachment.h"
#include "slot.h"
#include "slotProvider.h"
#include "node.h"
#include "utils.h"
#include "../core/dataError.h"

IMPLEMENT_ENGINE_CLASS( CHardAttachment );
IMPLEMENT_RTTI_BITFIELD( EHardAttachmentFlags );

#if !(defined NO_EDITOR) && !(defined NO_DATA_ASSERTS)
static Bool HasCyclicAttachments( CNode* base )
{
	THashSet< CNode* > visitedNodes;

	struct {
		Bool Follow( THashSet< CNode* >& visitedNodes, CNode* node ) const
		{
			if ( !node || !visitedNodes.Insert( node ) )
			{
				return false;
			}


			const auto& attachments = node->GetChildAttachments();
			for ( auto it=attachments.Begin(); it != attachments.End(); ++it )
			{
				IAttachment* attachment = *it;
				if ( !attachment->IsA< CHardAttachment >() || attachment->IsBroken() )
				{
					continue;
				}

				if ( Follow( visitedNodes, attachment->GetChild() ) )
				{
					return true;
				}
			}

			return false;
		}
	} local;

	return local.Follow( visitedNodes, base );
}
#endif

HardAttachmentSpawnInfo::HardAttachmentSpawnInfo()
	: AttachmentSpawnInfo( ClassID< CHardAttachment >() )
	, m_relativePosition( 0,0,0 )
	, m_relativeRotation( 0,0,0 )
	, m_freePositionAxisX( false )
	, m_freePositionAxisY( false )
	, m_freePositionAxisZ( false )
	, m_freeRotation( false )
{
}

CHardAttachment::CHardAttachment()
{
	m_isHardAttachment = true;
}

void CHardAttachment::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Slot changed
	if ( property->GetName() == TXT("parentSlotName") )
	{
		SetupSlot( GetParent(), m_parentSlotName );
	}

	// Force transform update
	GetChild()->ScheduleUpdateTransformNode();
}

struct OldHardAttachmentFlags
{
	Bool m_freePositionAxisX;
	Bool m_freePositionAxisY;
	Bool m_freePositionAxisZ;
	Bool m_freeRotation;
	Vector m_relativePosition;
	EulerAngles m_relativeRotation;

	void Reset()
	{
		m_freePositionAxisX = false;
		m_freePositionAxisY = false;
		m_freePositionAxisZ = false;
		m_freeRotation = false;
		m_relativePosition = Vector::ZERO_3D_POINT;
		m_relativeRotation = EulerAngles::ZEROS;
	}

	Uint8 TranslateAttachmentFlags() const
	{
		Uint8 flags = 0;
		if ( m_freePositionAxisX ) flags |= HAF_FreePositionAxisX;
		if ( m_freePositionAxisY ) flags |= HAF_FreePositionAxisY;
		if ( m_freePositionAxisZ ) flags |= HAF_FreePositionAxisZ;
		if ( m_freeRotation ) flags |= HAF_FreeRotation;
		return flags;
	}

	EngineTransform TranslateRelativeTransform() const
	{
		return EngineTransform( m_relativePosition, m_relativeRotation );
	}
};

void CHardAttachment::SetRelativePosition( const Vector& position )
{
	// Set new relative position
	m_relativeTransform.SetPosition( position );

	// Schedule transform update for child component
	GetChild()->ScheduleUpdateTransformNode();
}

void CHardAttachment::SetRelativeRotation( const EulerAngles& rotation )
{
	// Set new relative rotation
	m_relativeTransform.SetRotation( rotation );

	// Schedule transform update for child component
	GetChild()->ScheduleUpdateTransformNode();
}

void CHardAttachment::SetRelativeScale( const Vector& scale )
{
	// Set new relative scale
	m_relativeTransform.SetScale( scale );

	// Schedule transform update for child component
	GetChild()->ScheduleUpdateTransformNode();
}

void CHardAttachment::RemoveRelativeTransform()
{
	// Mark as not using relative transform
	m_relativeTransform.Identity();

	// Schedule transform update for child component
	GetChild()->ScheduleUpdateTransformNode();
}

Bool CHardAttachment::SetupSlot( CNode* parent, CName slotName )
{
	// Discard current slot
	if ( m_parentSlot )
	{
		m_parentSlot->Discard();
		m_parentSlot = NULL;
	}

	// We need parent to query for slot
	if ( parent )
	{
		// Query slot provider for parent component
		ISlotProvider* slotProvider = parent->QuerySlotProvider();
		if ( !slotProvider )
		{
			WARN_ENGINE( TXT("Cannot create HardAttachment because '%ls' does not implement slot provider interface"), parent->GetName().AsChar() );
			return false;
		}

		// Ask for slot
		m_parentSlot = slotProvider->CreateSlot( slotName.AsString().AsChar() );
		if ( !m_parentSlot )
		{
			WARN_ENGINE( TXT("Cannot create HardAttachment because slot '%ls' does not exist in '%ls'"), slotName.AsString().AsChar(), parent->GetName().AsChar() );
			return false;
		}

		// Set new slot
		m_parentSlotName = slotName;
	}

	// Valid
	return true;
}

Bool CHardAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	RED_FATAL_ASSERT( parent != child, "The attachment of parent and child must be different" );
#ifdef RED_ASSERTS_ENABLED
	{
		const CNode* _parent = parent;
		const CNode* _child = child;

		while ( const CHardAttachment* parentAtt = _parent->GetTransformParent() )
		{
			const CNode* parentParentNode = parentAtt->GetParent();

			RED_FATAL_ASSERT( parentParentNode != child, "Attachments of parent and child has a cycle" );

			_parent = parentParentNode;
		}
	}
#endif

	// Child already has parent transform
	if ( child->GetTransformParent() )
	{
		CNode *tparent = child->GetTransformParent()->GetParent();
		WARN_ENGINE( TXT("Cannot create HardAttachment because '%ls' is already hard attached to '%ls'"), child->GetName().AsChar(), tparent->GetName().AsChar() );
		return false;
	}

	// Check if we had cyclic attachments before
#if !(defined NO_EDITOR) && !(defined NO_DATA_ASSERTS)
	m_isBroken = true;
	m_parent = nullptr;
	m_child = nullptr;
	Bool hadCyclicAttachmentsPreviously = HasCyclicAttachments( parent );
#endif

	// Extra info
	if ( info )
	{
		const HardAttachmentSpawnInfo& hinfo = *( static_cast< const HardAttachmentSpawnInfo* >( info ) );

		// Parent slot name was given, get it
		if ( !hinfo.m_parentSlotName.Empty() )
		{
			if ( !SetupSlot( parent, hinfo.m_parentSlotName ))
			{
				return false;
			}
		}

		// Set relative position and rotation
		m_relativeTransform.Init( hinfo.m_relativePosition, hinfo.m_relativeRotation );
		m_parentSlotName = hinfo.m_parentSlotName;

		// Flags
		m_attachmentFlags = 0;
		if ( hinfo.m_freeRotation ) m_attachmentFlags |= HAF_FreeRotation;
		if ( hinfo.m_freePositionAxisX ) m_attachmentFlags |= HAF_FreePositionAxisX;
		if ( hinfo.m_freePositionAxisY ) m_attachmentFlags |= HAF_FreePositionAxisY;
		if ( hinfo.m_freePositionAxisZ ) m_attachmentFlags |= HAF_FreePositionAxisZ;
	}
    else
    {
        if ( !m_parentSlotName.Empty() )
		{
            if ( !SetupSlot( parent, m_parentSlotName ) )
			{
                return false;
			}
		}
    }

	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ))
	{
		return false;
	}

	// Check if we introduced cyclic attachments
#if !(defined NO_EDITOR) && !(defined NO_DATA_ASSERTS)
	if ( HasCyclicAttachments( parent ) && !hadCyclicAttachmentsPreviously )
	{
		Break();
		DATA_HALT( DES_Uber, CResourceObtainer::GetResource( parent ), TXT("Attachments"), TXT("Attempt to create cyclic hard attachments, you may need a resave") );
		return false;
	}
#endif

	// Schedule transform update for child component
	if ( CNode* child = GetChild() )
	{
		if( child->UsesAutoUpdateTransform() )
		{
			if ( child->IsAttached() && child->HasScheduledUpdateTransform() && child->HasFlag( NF_MarkUpdateTransform ) )
			{
				child->RefreshMarkUpdateTransformChain( child->GetLayer() );
			}
			child->ScheduleUpdateTransformNode();
		}
	}

	// Initialized and valid
	return true;
}

//////////////////////////////////////////////////////////////
void CHardAttachment::Break()
{
	// child is being nullified in TBaseClass::Break()
	CNode* child = GetChild();

	TBaseClass::Break();

	// Refresh child if is marked for update:
	if ( child && child->HasFlag( NF_MarkUpdateTransform ) )
	{
		child->RefreshMarkUpdateTransformChain( child->GetLayer() );
	}
}

void CHardAttachment::CalcAttachedLocalToWorld( Matrix& out ) const
{
	// This method is called VERY often, so please do not submit this line uncommented:
	//PC_SCOPE( AttachmentUpdateTransform );
	ASSERT( !IsBroken() );

	// Get parent component
	if ( m_parentSlot )
	{
		// Use slot to get transformation
		out = m_parentSlot->CalcSlotMatrix();
	}
	else
	{
		// Get the matrix directly from transform parent
		out = GetParent()->GetLocalToWorld();
	}

	// Reset rotation
	if ( m_attachmentFlags & HAF_FreeRotation )
	{
		// Remove rotation and scaling
		Vector translation = out.GetTranslation();
		out.SetIdentity();
		out.SetTranslation( translation );
	}

	// Reset free axes to 0
	if ( m_attachmentFlags & ( HAF_FreePositionAxisX | HAF_FreePositionAxisY | HAF_FreePositionAxisZ ) )
	{
		Vector translation = out.GetTranslation();
		if ( m_attachmentFlags & HAF_FreePositionAxisX ) translation.X = 0.0f;
		if ( m_attachmentFlags & HAF_FreePositionAxisY ) translation.Y = 0.0f;
		if ( m_attachmentFlags & HAF_FreePositionAxisZ ) translation.Z = 0.0f;
		out.SetTranslation( translation );
	}

	// Apply relative transform
	if ( !m_relativeTransform.IsIdentity() )
	{
		// Assemble relative matrix
		Matrix relativeMatrix;
		m_relativeTransform.CalcLocalToWorld( relativeMatrix );

		// Apply relative transform
		out = relativeMatrix * out;
	}
}
