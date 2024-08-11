
#include "build.h"
#include "fxSimpleSpawner.h"
#include "hardAttachment.h"
#include "entityTemplate.h"
#include "entity.h"
#include "animatedComponent.h"
#include "../physics/physicsWorld.h"

IMPLEMENT_ENGINE_CLASS ( CFXSimpleSpawner )

void GetAllSlotsUnder( CEntity* parentEntity, CName rootSlotName, TDynArray< const EntitySlot* >& outSlots )
{
	if ( parentEntity == nullptr )
	{
		return;
	}

	if ( CEntityTemplate* entityTemplate = parentEntity->GetEntityTemplate() )
	{
		// Slots are defined at entity template level
		if ( const EntitySlot* rootSlot = entityTemplate->FindSlotByName( rootSlotName, true ) )
		{
			TDynArray< CName > boneNames;
			boneNames.PushBack( rootSlot->GetBoneName() );

			if ( CAnimatedComponent* animComponent = parentEntity->GetRootAnimatedComponent() )
			{
				TDynArray< ISkeletonDataProvider::BoneInfo > bonesInfo;
				animComponent->GetBones( bonesInfo );

				Bool possibleParentAdded;
				do
				{
					possibleParentAdded = false;
					for ( const ISkeletonDataProvider::BoneInfo& b : bonesInfo )
					{
						if ( !boneNames.Exist( b.m_name ) && b.m_parent >=0 && boneNames.Exist( bonesInfo[ b.m_parent ].m_name ) )
						{
							boneNames.PushBack( b.m_name );
							possibleParentAdded = true;
						}
					}
				} 
				while ( possibleParentAdded );
			}

			// translate found bone names into slots

			TDynArray< const EntitySlot* > allSlots;
			entityTemplate->CollectSlots( allSlots, true );

			for ( const EntitySlot* slot : allSlots )
			{
				if ( boneNames.Exist( slot->GetBoneName() ) )
				{
					outSlots.PushBack( slot );
				}
			}
		}
	}
}

CFXSimpleSpawner::CFXSimpleSpawner() 
	: m_cachedEntity( nullptr )
{
}

Bool CFXSimpleSpawner::Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber )
{
	if ( parentEntity )
	{
		if ( pcNumber < m_slotNames.Size() )
		{
			// = SLOT =

			CName slotName = m_slotNames[ pcNumber ];

			if ( CEntityTemplate* templ = parentEntity->GetEntityTemplate() )
			{
				if ( const EntitySlot* slot = templ->FindSlotByName( slotName, true ) )
				{
					Matrix slotMatrix;
					String outError;
					if ( slot->CalcMatrix( parentEntity, slotMatrix, &outError ) )
					{
						position = slotMatrix.GetTranslation();
						rotation = slotMatrix.ToEulerAnglesFull();
						return true; // position extracted
					}
					else
					{
						WARN_ENGINE( TXT("Obtaining position from the slot failed with message: %s"), outError.AsChar() );
					}
				}
			}
		}
		else
		{
			// = BONE =

			pcNumber -= m_slotNames.Size();
			
			if ( pcNumber < m_boneNames.Size() )
			{
				CName boneName = m_boneNames[ pcNumber ];

				if ( CAnimatedComponent* animComponent = parentEntity->GetRootAnimatedComponent() )
				{
					Int32 boneIndex = animComponent->FindBoneByName( boneName );
					if ( boneIndex != -1 )
					{
						const Matrix& boneMatrix = animComponent->GetBoneMatrixWorldSpace( boneIndex );
						position = boneMatrix.GetTranslation();
						rotation = boneMatrix.ToEulerAngles();
						return true;
					}	
					else
					{
						WARN_ENGINE( TXT("No bone with found") );
					}
				}
			}
		}

		// here is the case if there is no slot nor bone specified (or they are not found)
		rotation = parentEntity->GetWorldRotation();
		position = parentEntity->GetWorldPosition();
		return true;
	}
	else
	{
		return false;
	}
}

Bool CFXSimpleSpawner::PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 pcNumber )
{ 
	if ( parentEntity )
	{
		if ( pcNumber < m_slotNames.Size() )
		{
			// = SLOT =

			CName slotName = m_slotNames[ pcNumber ];

			if ( CEntityTemplate* templ = parentEntity->GetEntityTemplate() )
			{
				if ( const EntitySlot* slot = templ->FindSlotByName( slotName, true ) )
				{
					HardAttachmentSpawnInfo spawnInfo;
					spawnInfo.m_relativePosition  = m_relativePos;
					spawnInfo.m_relativeRotation  = m_relativeRot;
					spawnInfo.m_freePositionAxisX = false;
					spawnInfo.m_freePositionAxisY = false;
					spawnInfo.m_freePositionAxisZ = false;
					spawnInfo.m_freeRotation      = false;

					String attachementErrorStr;
					if ( slot->CreateAttachment( spawnInfo, parentEntity, createdComponent, &attachementErrorStr ) == nullptr )
					{
						return false;
					}
				}
			}
		}
		else
		{
			// = BONE = 

			pcNumber -= m_slotNames.Size();

			if ( pcNumber < m_boneNames.Size() )
			{
				CName boneName = m_boneNames[ pcNumber ];

				if ( CAnimatedComponent* animComponent = parentEntity->GetRootAnimatedComponent() )
				{
					HardAttachmentSpawnInfo spawnInfo;
					spawnInfo.m_relativePosition  = m_relativePos;
					spawnInfo.m_relativeRotation  = m_relativeRot;
					spawnInfo.m_freePositionAxisX = false;
					spawnInfo.m_freePositionAxisY = false;
					spawnInfo.m_freePositionAxisZ = false;
					spawnInfo.m_freeRotation      = false;
					spawnInfo.m_parentSlotName    = boneName;

					if ( animComponent->Attach( createdComponent, spawnInfo ) == nullptr )
					{
						return false;
					}
				}
			}
		}
	}

	return true; 
}

Uint32 CFXSimpleSpawner::AmountOfPC( CEntity* parentEntity, TDynArray< Uint32 > &indices )
{ 
	if ( m_slotNames.Empty() && m_boneNames.Empty() )
	{
		indices.PushBack( 0 );
		return 1;
	}
	else
	{
		Uint32 total = m_slotNames.Size() + m_boneNames.Size();

		for ( Uint32 i = 0; i < total; ++i )
		{
			indices.PushBack( i );
		}

		return total;
	}
}

Bool CFXSimpleSpawner::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == TXT("slotName") )
	{
		CName name;
		readValue.AsType( name );
		m_slotNames.PushBack( name );
		return true;
	}

	return false;
}
