/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "rigidMeshComponent.h"
#include "soundStartData.h"

#include "../physics/physicsSimpleBodyWrapper.h"
#include "mesh.h"
#include "../physics/compiledCollision.h"

#include "../../common/core/scriptStackFrame.h"
#include "../../common/core/dataError.h"
#include "collisionMesh.h"
#include "hardAttachment.h"
#include "entity.h"
#include "world.h"
#include "layer.h"
#include "collisionCache.h"
#include "../physics/physicsWorld.h"
#include "renderFrame.h"
#include "../physics/physicsSettings.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CRigidMeshComponent );
IMPLEMENT_RTTI_ENUM( EMotionType );

RED_DEFINE_STATIC_NAME( linearDamping )
RED_DEFINE_STATIC_NAME( angularDamping )

CRigidMeshComponent::CRigidMeshComponent()
	: m_motionType( MT_Dynamic )
	, m_angularDamping( -1.0f )
	, m_linearVelocityClamp( -1.0f )
	, m_linearDamping( -1.0f )
	, m_physicsBodyWrapper( 0 )
{
	m_physicalCollisionType = CPhysicalCollision( CNAME( RigidBody ) );
	m_pathLibCollisionType = PLC_Disabled;
}

void CRigidMeshComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Mass changed
	if ( property->GetName() == CNAME( motionType ) )
	{
		if( m_physicsBodyWrapper )
		{
			m_physicsBodyWrapper->SwitchToKinematic( m_motionType == MT_KeyFramed );
		}
	}

	// Friction changed in editor
	if ( property->GetName() == CNAME( linearDamping ) )
	{
		if( m_physicsBodyWrapper )
		{
			m_physicsBodyWrapper->SetDampingLinear( m_linearDamping );
		}
	}

	// Restitution changed
	if ( property->GetName() == CNAME( angularDamping ) )
	{
		if( m_physicsBodyWrapper )
		{
			m_physicsBodyWrapper->SetDampingAngular( m_angularDamping );
		}
	}
}

Bool CRigidMeshComponent::UsesAutoUpdateTransform()
{
	// If we are attached via skinning attachment to not automatically update transform
	if ( m_transformParent && m_transformParent->ToSkinningAttachment() )
	{
		return false;
	}

	// Auto update transform
	return true;
}

void CRigidMeshComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentAdded( attachment );

	// This entity is in game and was attached to something
	if ( GetEntity() && GetEntity()->IsInGame() && attachment->IsA< CHardAttachment >() && m_physicsBodyWrapper && m_motionType == MT_Dynamic )
	{
		m_physicsBodyWrapper->SwitchToKinematic( true );
		m_physicsBodyWrapper->SetFlag( PRBW_TrackKinematic, true );
	}
}

void CRigidMeshComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentBroken( attachment );

	// Hard attachment was broken
	if ( GetEntity() && GetEntity()->IsInGame() && attachment->IsA< CHardAttachment >() && m_physicsBodyWrapper && m_motionType == MT_Dynamic )
	{
		m_physicsBodyWrapper->SwitchToKinematic( false );
		m_physicsBodyWrapper->SetFlag( PRBW_TrackKinematic, false );
	}
}

void CRigidMeshComponent::OnAttached( CWorld* world )
{
	// Pass to the base class
	TBaseClass::OnAttached( world );

	// patch 1.1 hack fix. needs to be removed once asset will be fixed.
	if ( GetEntity() != nullptr && GetEntity()->HACK_ForceInitialKinematicMotion() )
	{
		m_motionType = MT_KeyFramed;
	}
}

void CRigidMeshComponent::OnDetached( CWorld* world )
{
	if ( m_physicsBodyWrapper )
	{
		m_physicsBodyWrapper->Release();
		m_physicsBodyWrapper = 0;
	}

	// Pass to the base class
	TBaseClass::OnDetached( world );
}

void CRigidMeshComponent::OnStreamIn()
{
	CEntity* ent = GetEntity();
	if ( ent && ent->GetTransformParent() && m_physicsBodyWrapper && !m_physicsBodyWrapper->IsKinematic() )
	{
		m_physicsBodyWrapper->SwitchToKinematic( true );
	}	
}

void CRigidMeshComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CRigidMeshComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_physicsBodyWrapper && m_physicsBodyWrapper->IsKinematic() && prevLocalToWorld != GetLocalToWorld() )
	{
		m_physicsBodyWrapper->SetFlag( PRBW_PoseIsDirty, true );
	}
}

Bool CRigidMeshComponent::ShouldScheduleFindCompiledCollision() const
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateRigidBodies ) return false;
#endif

	if ( m_physicsBodyWrapper )
	{
		return false;
	}
	
	return true;
}

void CRigidMeshComponent::OnCompiledCollisionFound( CompiledCollisionPtr collision )
{
	m_compiledCollision = collision;

	if( CWorld* world = GetWorld() )
	{
		CPhysicsWorld* physicsWorld = nullptr;
		if( world->GetPhysicsWorld( physicsWorld ) )
		{
			CPhysicsSimpleWrapper* wrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), m_physicalCollisionType, m_compiledCollision );
			m_physicsBodyWrapper = wrapper;
			if( m_physicsBodyWrapper )
			{
				if( m_physicsBodyWrapper->GetActorsCount() == 0 )
				{
					m_physicsBodyWrapper->Release();
					m_physicsBodyWrapper = 0;
					return ;
				}

				SWrapperContext* context = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContext( wrapper );

				Box box = m_physicsBodyWrapper->CalcLocalBounds( 0 );
				context->m_desiredDistanceSquared = SPhysicsSettings::m_simpleBodySimulationDistanceLimit + box.Max.DistanceTo( box.Min );
				if( CMeshTypeResource* resource = GetMeshTypeResource() )
				{
					Float autohide = resource->GetAutoHideDistance();
					if( context->m_desiredDistanceSquared > autohide )
					{
						context->m_desiredDistanceSquared = autohide;
					}
				}
				context->m_desiredDistanceSquared *= context->m_desiredDistanceSquared;
			}

		}
	}

	if( !m_physicsBodyWrapper ) return;

	// Set linear velocity clamp from entity parameters
	if( m_linearVelocityClamp != -1.0f )
	{
		CPhysicsSimpleWrapper* wrapp = (CPhysicsSimpleWrapper*)m_physicsBodyWrapper;
		wrapp->SetLinearVelocityClamp( m_linearVelocityClamp );
	}

	// Switch to kinematic if this is attached to anything
	// (assumption: transform parent always comes from a hard attachment)
	if ( GetEntity()->IsInGame() && GetTransformParent() != nullptr )
	{
		m_physicsBodyWrapper->SwitchToKinematic( true );
		m_physicsBodyWrapper->SetFlag( PRBW_TrackKinematic, true );
	}
	else if ( m_motionType == MT_KeyFramed )
	{
		m_physicsBodyWrapper->SwitchToKinematic( true );
	}
	if( m_linearDamping >= 0 ) m_physicsBodyWrapper->SetDampingLinear( m_linearDamping );
	if( m_angularDamping >= 0 ) m_physicsBodyWrapper->SetDampingAngular( m_angularDamping );
}

void CRigidMeshComponent::SetEnabled( Bool enabled )
{
	TBaseClass::SetEnabled( enabled );
	if( !m_physicsBodyWrapper ) return;
	m_motionType = enabled ? MT_Dynamic : MT_KeyFramed;
	UpdateKinematicState();
}

void CRigidMeshComponent::UpdateKinematicState()
{
	if( !m_physicsBodyWrapper ) return;
	Bool kinematic = ( m_motionType == MT_KeyFramed );
	m_physicsBodyWrapper->SwitchToKinematic( kinematic );
}

#ifndef NO_EDITOR
void CRigidMeshComponent::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
	EditorRecreateCollision();
}

void CRigidMeshComponent::EditorRecreateCollision()
{
	// recreating collision
	if ( m_physicsBodyWrapper )
	{
		m_physicsBodyWrapper->Release();
		m_physicsBodyWrapper = 0;
	}
	CompiledCollisionPtr compiledCollision = GetCompiledCollision( true );
	if( compiledCollision )
	{
		OnCompiledCollisionFound( compiledCollision );
	}
}
#endif //NO_EDITOR

void CRigidMeshComponent::OnSelectionChanged()
{
	TBaseClass::OnSelectionChanged();
	if ( GetWorld() && !GetWorld()->GetPreviewWorldFlag() && m_physicsBodyWrapper )
	{
		if ( GetFlags() & NF_Selected )
		{
			m_physicsBodyWrapper->SwitchToKinematic( true );
		}
		else
		{
			UpdateKinematicState();
		}
	}
}

void CRigidMeshComponent::funcEnableBuoyancy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	Bool res = false;
	if ( m_physicsBodyWrapper != nullptr )
	{
		m_physicsBodyWrapper->SetFlag( PRBW_DisableBuoyancy, !enable );
		res = true;
	}

	RETURN_BOOL( res );
}