/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/game/behTreeNode.h"

#include "aiTreeComponent.h"
#include "aiActionPerformerAttachment.h"


IMPLEMENT_ENGINE_CLASS( CAITreeComponent );

CAITreeComponent::CAITreeComponent()
	: m_resource( nullptr )
	, m_priorityGroup( TICK_Normal )
{
	void TouchR6BehTreeRTTIFile_CompilerHack(); // temporary, nothing to see here, move along... :)
	TouchR6BehTreeRTTIFile_CompilerHack();
}

void CAITreeComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( m_resource.IsValid() )
	{
		// Create instance
		m_instance = CreateObject< CR6BehTreeInstance > ( this );
		R6_ASSERT( m_instance )

		TDynArray< THandle< IAIParameters > > params( m_parameters.Size() );
		for ( Uint32 i=0; i<params.Size(); ++i )
		{
			params[i] = m_parameters[i];
		}

		// Bind resource to instance 
		m_instance->Bind( m_resource, params, nullptr );
		R6_ASSERT( m_instance->IsBound() )

		// Register this component for tick
		GCommonGame->GetSystem< CR6AISystem > ()->RegisterComponent( this );
	}
	else
	{
		RED_LOG( CNAME( AITree ), TXT("Component '%ls' with null m_resource attached."), GetFriendlyName().AsChar() );
	}
}

void CAITreeComponent::OnDetached( CWorld* world )
{
	if ( m_instance )
	{
		DestroyInstance();

		// Unregister from tick group
		GCommonGame->GetSystem< CR6AISystem > ()->UnregisterComponent( this );
	}

	// clear performers list
	m_attachedPerformers.Clear();

	TBaseClass::OnDetached( world );
}

EAITickPriorityGroup CAITreeComponent::GetCurrentPriorityGroup() const
{
	// TODO: this may actually return different valuest depanding on LOD, etc.
	// BUT changing the group should be handled in ai system.
	return m_priorityGroup;
}

void CAITreeComponent::Update( Float timeSinceLastUpdate, Float thisFrameTimeDelta )
{
	R6_ASSERT( GetCurrentPriorityGroup() != TICK_EveryFrame || timeSinceLastUpdate == thisFrameTimeDelta );	

	m_instance->Update( timeSinceLastUpdate );
}

void CAITreeComponent::OnWorldEnd()
{
	if ( m_instance )
	{
		DestroyInstance();

		// no unregister from tick group here; since the world is ending, the system will clear the tick lists either way
	}
}

void CAITreeComponent::DestroyInstance()
{
	// Stop running
	IBehTreeNodeInstance* node = m_instance->GetInstanceRootNode();
	if ( nullptr != node && node->IsActive() )
	{
		node->Deactivate();
	}

	// Unbind the risource and destroy the instance
	m_instance->Unbind();
	m_instance->Discard();
	m_instance = nullptr;
}

void CAITreeComponent::OnChildAttachmentAdded( IAttachment* attachment )
{
	CAIActionPerformerAttachment *att = Cast< CAIActionPerformerAttachment > ( attachment );
	if ( att )
	{
		CComponent* child = Cast< CComponent > ( att->GetChild() );
		R6_ASSERT( child );
		R6_ASSERT( false == m_attachedPerformers.Exist( child ) );
		m_attachedPerformers.PushBack( child );
	}

	TBaseClass::OnChildAttachmentAdded( attachment );
}

void CAITreeComponent::OnChildAttachmentBroken( IAttachment* attachment )
{
	CAIActionPerformerAttachment *att = Cast< CAIActionPerformerAttachment > ( attachment );
	if ( att )
	{
		CComponent* child = Cast< CComponent > ( att->GetChild() );
		R6_ASSERT( child );
		R6_ASSERT( m_attachedPerformers.Exist( child ) );
		m_attachedPerformers.RemoveFast( child );
	}

	TBaseClass::OnChildAttachmentBroken( attachment );
}
