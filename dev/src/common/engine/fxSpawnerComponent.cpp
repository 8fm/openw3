/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxSpawnerComponent.h"
#include "entityTemplate.h"
#include "animatedComponent.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CFXSpawnerComponent );

CFXSpawnerComponent::CFXSpawnerComponent()
	: m_copyRotation( true )
	, m_percentage( 1.0f )
{
}

Bool CFXSpawnerComponent::Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber )
{
	// No parent entity, we cannot calculate
	if ( !parentEntity )
	{
		WARN_ENGINE( TXT("FX spawner '%ls' error: no parent entity"), GetFriendlyName().AsChar() );
		return false;
	}

	// Try to resolve as slot
	if ( parentEntity->GetTemplate() )
	{
		// Slots are defined at entity template level
		const EntitySlot* slot = parentEntity->GetEntityTemplate()->FindSlotByName( m_componentName, true );
		if ( slot )
		{
			// Calculate slot crap
			Matrix slotMatrix;
			String outError;
			if ( !slot->CalcMatrix( parentEntity, slotMatrix, &outError ) )
			{
				WARN_ENGINE( TXT("FX spawner '%ls' slot error: %s"), GetFriendlyName().AsChar(), outError.AsChar() );
				return false;
			}

			// Extract position
			position = slotMatrix.GetTranslation();

			// Extract rotation
			if ( m_copyRotation )
			{
				rotation = slotMatrix.ToEulerAnglesFull();
			}

			// Valid
			return true;
		}
	}

	// Find component
	CComponent* component = parentEntity->FindComponent( m_componentName );
	if ( !component )
	{
		WARN_ENGINE( TXT("FX spawner '%ls' error: no component '%ls' in '%ls'"), GetFriendlyName().AsChar(), m_componentName.AsString().AsChar(), parentEntity->GetFriendlyName().AsChar() );
		return false;
	}

	// If it's a physics component do some more stuff here
/*	IPhysicsComponentInterface* physics = component->QueryPhysicsInterface();
	if ( physics && ( m_parentSlotName == CName::NONE ) )
	{
		// Extract body position
		physics->GetVectorValueOfRigidBody( pcNumber, BODY_CenterOfMass, position );

		// Derive rotation
		if ( m_copyRotation )
		{
			Matrix bodyMatrix;
			physics->GetRigidBodyWorldTransformMatrix( pcNumber, bodyMatrix );
			rotation = bodyMatrix.ToEulerAngles();
		}

		// We're done here.
		return true;	
	}*/

	// Ask slot for the valid position
	if ( m_parentSlotName != CName::NONE && !m_attach )
	{
		CAnimatedComponent* animComponent = Cast<CAnimatedComponent>( component );
		if ( animComponent )
		{
			const ISkeletonDataProvider* provider = animComponent->QuerySkeletonDataProvider();
			ASSERT( provider );
			Int32 boneIndex = provider->FindBoneByName( m_parentSlotName.AsString().AsChar() );
			if ( boneIndex != -1 )
			{
				const Matrix& boneMatrix = provider->GetBoneMatrixWorldSpace( boneIndex );
				position = boneMatrix.GetTranslation();

				// Derive rotation
				if ( m_copyRotation )
				{
					rotation = boneMatrix.ToEulerAngles();
				}

				// Done
				return true;
			}	
			else
			{
				WARN_ENGINE( TXT("FX spawner '%ls' error: no bone %s in animated component %s"), GetFriendlyName().AsChar(), m_parentSlotName.AsString().AsChar(), m_componentName.AsString().AsChar() );
				
				return false;
			}
		}
		else
		{
			WARN_ENGINE( TXT("FX spawner '%ls' error: component %s is not an animated component, specified bone name %s will be ginored"), GetFriendlyName().AsChar(), m_componentName.AsString().AsChar(), m_parentSlotName.AsString().AsChar() );
		}	
	}

	// Derive position
	position = component->GetWorldPosition();

	// Derive rotation
	if ( m_copyRotation )
	{
		rotation = component->GetWorldRotation();
	}

	// Done
	return true;
}

Bool CFXSpawnerComponent::PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 i )
{
	ASSERT( createdComponent );

	// Attach to target component
	if ( m_attach )
	{
		// No parent entity, we cannot calculate
		if ( !parentEntity )
		{
			WARN_ENGINE( TXT("FX spawner '%ls' error: no parent entity"), GetFriendlyName().AsChar() );
			return false;
		}

		// Build attachment info
		HardAttachmentSpawnInfo spawnInfo;
		spawnInfo.m_relativePosition = m_relativePosition;
		spawnInfo.m_relativeRotation = m_relativeRotation;
		spawnInfo.m_freePositionAxisX = m_freePositionAxisX;
		spawnInfo.m_freePositionAxisY = m_freePositionAxisY;
		spawnInfo.m_freePositionAxisZ = m_freePositionAxisZ;
		spawnInfo.m_freeRotation = m_freeRotation;

		// Try to resolve as slot
		if ( parentEntity->GetTemplate() )
		{
			// Slots are defined at entity template level
			const EntitySlot* slot = parentEntity->GetEntityTemplate()->FindSlotByName( m_componentName, true );
			if ( slot )
			{
				String outError;
				IAttachment* att = slot->CreateAttachment(	spawnInfo, parentEntity, createdComponent, &outError );
				if ( !att )
				{
					WARN_ENGINE( TXT("FX spawner '%ls' error: failed to attach to slot '%ls': %s"), GetFriendlyName().AsChar(), m_componentName.AsString().AsChar(), outError.AsChar() );
					
					return false;
				}

				// Attached
				return true;
			}
		}

		// Find component
		CComponent* component = parentEntity->FindComponent( m_componentName );
		if ( !component )
		{
			WARN_ENGINE( TXT("FX spawner '%ls' error: no component '%ls' in '%ls'"), GetFriendlyName().AsChar(), m_componentName.AsString().AsChar(), parentEntity->GetFriendlyName().AsChar() );
			
			return false;
		}

		// Attach to slot, get the slot name
/*		IPhysicsComponentInterface* physics = component->QueryPhysicsInterface();
		if ( physics && ( m_parentSlotName == CName::NONE) )
		{
			spawnInfo.m_parentSlotName = physics->GetRigidBodyName( i );
		}
		else*/
		{
			spawnInfo.m_parentSlotName = m_parentSlotName;
		}

		// Attach
		IAttachment* att = component->Attach( createdComponent, spawnInfo );		
		return att != NULL;
	}

	// Not attached
	return false;
}

const CEntityTemplate* CFXSpawnerComponent::GetEntityTemplate() const
{
	return FindParent< CEntityTemplate > ();
}

Uint32 CFXSpawnerComponent::AmountOfPC( CEntity* parentEntity, TDynArray<Uint32> &indices )
{
	// Find component
	CComponent* component = parentEntity->FindComponent( m_componentName.AsString().AsChar() );
	if ( component )
	{
		// Do some calculations based on the intenral crap
/*		IPhysicsComponentInterface*	physicsInterface = component->QueryPhysicsInterface();
		if ( physicsInterface && ( m_parentSlotName == CName::NONE ) )
		{
			Uint32 rigidBodiesCount = physicsInterface->GetRigidBodiesCount();
			Uint32 targetNum = Clamp< Uint32 >( static_cast< Uint32 >( rigidBodiesCount * m_percentage ), 0, 16);
			indices.Resize( targetNum );

			for ( Uint32 i = 0; i < targetNum; ++i )
			{
				indices[i] = IRand() % rigidBodiesCount;
			}

			return targetNum;
		}*/

		// Defaults - one piece
		indices.Resize( 1 );
		indices[0] = 0;
		return 1;
	}

	// Try to resolve as slot
	if ( parentEntity->GetTemplate() )
	{
		// Slots are defined at entity template level
		const EntitySlot* slot = parentEntity->GetEntityTemplate()->FindSlotByName( m_componentName, true );
		if ( slot )
		{
			// Defaults - one piece
			indices.Resize( 1 );
			indices[0] = 0;
			return 1;
		}
	}

	// No pieces
	return false;
}

Bool CFXSpawnerComponent::UsesComponent( const CName& componentName ) const
{
	return m_componentName == componentName;
}
