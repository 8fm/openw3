#include "build.h"
#include "r6Player.h"

#include "explorationCameraComponent.h"
#include "aimHelpTargetGatherer.h"
#include "r6InteractionComponent.h"
#include "r6InventoryItemComponent.h"

#include "r6damageData.h"
#include "../../common/engine/PhysicsRagdollWrapper.h"
#include "playerInventoryPanel.h"
#include "eventRouterComponent.h"

#include "idInterlocutor.h"
#include "idInstance.h"

#include "../../common/game/movingPhysicalAgentComponent.h"

#include "aimTarget.h"
#include "r6CameraComponent.h"
#include "../../common/core/dataError.h"

#include "../../common/engine/utils.h"


IMPLEMENT_ENGINE_CLASS( CR6Player );

//------------------------------------------------------------------------------------------------------------------
CR6Player::CR6Player()
	: m_aimYawF( 0.0f )
	, m_aimPitchF( 0.0f )
	, m_defaultCameraComponent( NULL )
{
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::OnTick( Float timeDelta )
{
	{
		// if the player is in the AV, it must be updated before the player is updated,
		// so that the player doesn't flicker behind the AV.

		auto avNode = GetAttachedAVNode();
		if(avNode)
		{
			avNode->ForceUpdateTransform();
		}
	}

	TBaseClass::OnTick( timeDelta );
	CallEvent(CNAME( OnTickScript ), timeDelta);

	//UpdateCamHeight();

	ForceUpdateTransform();
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );	
	
	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if( mac )
	{
		mac->SnapToNavigableSpace( false );
		CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( mac );
		mpac->SetGravity(false);
	}


	// Dialog
	auto interlocutorComponent = GetInterlocutorComponent();
	if( interlocutorComponent )
	{
		interlocutorComponent->SetWantsToDisplayOnHUD( true );
	}
	
	m_inventoryPanel = CreateObject< CPlayerInventoryPanel >( this );
	m_inventoryPanel.Get()->SetPlayer( this );	
	
	// TODO: move crafting somewhere else than the player
	m_craftingPanel = CreateObject< CPlayerCraftingPanel >( this );
	m_craftingPanel.Get()->SetPlayer( this );	
}

//------------------------------------------------------------------------------------------------------------------
Bool CR6Player::Teleport( const Vector& position, const EulerAngles& rotation )
{
	Bool ret = TBaseClass::Teleport( position, rotation );
	
	// notify script
	CallEvent( CNAME( OnPlayerTeleported ) );

	return ret;
}

//------------------------------------------------------------------------------------------------------------------
CR6InventoryItemComponent* CR6Player::GetEquippedItem()
{ 
	return m_equippedItem.Get(); 
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::RecreateCamera()
{	
}

//------------------------------------------------------------------------------------------------------------------
CR6InteractionComponent* CR6Player::GetInteractionTarget()
{
	if( m_interactionQueue.Size() > 0 )
	{
		return m_interactionQueue[ 0 ].Get();
	}

	return NULL;
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::GenerateDebugFragments( CRenderFrame* frame )
{
	TBaseClass::GenerateDebugFragments( frame );

	if( m_inventoryPanel.Get() )
		m_inventoryPanel.Get()->DrawInventory( frame );

	// TODO: Replace with proper GUI
	if( m_craftingPanel.Get() )
		m_craftingPanel.Get()->DrawCrafting( frame );
	
	//frame->AddDebugLine( m_TargetOrigin, m_TargetEnd, Color::RED );


	//Vector pos = GetPosition();
	//frame->AddDebugScreenFormatedText(100, 160, TXT("Player.position = [ %f , %f , %f ]"), pos.X, pos.Y, pos.Z);

	//Vector wPos = GetWorldPosition();
	//frame->AddDebugScreenFormatedText(100, 140, TXT("Player.worldPosition = [ %f , %f , %f ]"), wPos.X, wPos.Y, wPos.Z);
}



CR6CameraComponent* CR6Player::GetCurrentAutoCameraComponent()
{
	// try to get default parent camera if exists
	{
		auto playerAttachments = GetParentAttachments();
		auto it = playerAttachments.Begin();
		auto endIt = playerAttachments.End();
		for ( ; it != endIt; ++it )
		{
			CEntity* parentEntity = Cast< CEntity >( (*it)->GetParent() );
			if( !parentEntity  )
			{
				continue;
			}

			CR6CameraComponent* parentCam = GetDefaultCameraFromEntity( parentEntity );
			if( parentCam )
			{
				return parentCam;
			}
		}
	}

	return GetDefaultCameraComponent();
}


CR6CameraComponent* CR6Player::GetDefaultCameraFromEntity( CEntity* parentEntity )
{
	ComponentIterator< CR6CameraComponent > cit( parentEntity );
	for( ; cit; ++cit )
	{
		RED_ASSERT( cit && *cit );

		if( (*cit)->IsDefault() )
		{
			return *cit;
		}
	}

	// no default found
	return NULL;
}



CR6CameraComponent* CR6Player::GetDefaultCameraComponent()
{
	if( m_defaultCameraComponent )
	{
		return m_defaultCameraComponent;
	}

	m_defaultCameraComponent = GetDefaultCameraFromEntity( this );
	if( m_defaultCameraComponent )
	{
		return m_defaultCameraComponent;
	}

	DATA_HALT( DES_Major, GetPlayerResource(), TXT( "Camera" ), TXT( "None default CR6CameraComponent was created for CR6Player - create at least one component and mark as default!" ) );

	// we want the game to work anyway, so lets create exploration camera as default
	m_defaultCameraComponent = Cast< CR6CameraComponent >( CreateComponent( CExplorationCameraComponent::GetStaticClass(), SComponentSpawnInfo() ) );
	RED_ASSERT( m_defaultCameraComponent );
	return m_defaultCameraComponent;
}

const CResource* CR6Player::GetPlayerResource() const
{
	return CResourceObtainer::GetResource( this );
}



//------------------------------------------------------------------------------------------------------------------
void CR6Player::funcRayTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, origin, Vector::ZEROS );
	GET_PARAMETER( Vector, end, Vector::ZEROS );
	FINISH_PARAMETERS;
	
	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	if ( !physicsWorld )
	{
		RETURN_BOOL( false );
		return;
	}

	SPhysicsContactInfo contactInfo;
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )  | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );

	// Result
	if ( physicsWorld->RayCastWithSingleResult( origin, end, include, 0, contactInfo ) == TRV_Hit )
	{
		RETURN_BOOL(true); 
		return;
	}	
	
	RETURN_BOOL(false); 
}

//------------------------------------------------------------------------------------------------------------------
//void CR6Player::funcRayTestNormal( CScriptStackFrame& stack, void* result )
//{
//	GET_PARAMETER( Vector, origin, Vector::ZEROS );
//	GET_PARAMETER( Vector, end, Vector::ZEROS );
//	FINISH_PARAMETERS;
//	
//	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
//	if ( !physicsWorld )
//	{
//		RETURN_STRUCT( Vector, Vector::ZEROS );
//		return;
//	}
//
//	SPhysicsContactInfo contactInfo;
//	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
//
//	// Result
//	if ( physicsWorld->RayCastWithSingleResult( origin, end, include, 0, contactInfo ))
//	{
//		origin.X = contactInfo.m_normal.X;
//		origin.Y = contactInfo.m_normal.Y;
//		origin.Z = contactInfo.m_normal.Z;
//		origin.W = contactInfo.m_distance;
//
//		RETURN_STRUCT( Vector, origin);
//		return;
//	}
//
//	// Not collided
//	RETURN_STRUCT( Vector, Vector::ZEROS );
//}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::funcGetDialogInterlocutor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE(CIDInterlocutorComponent, GetInterlocutorComponent() );
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::funcGetEntityInteractions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, _entity, NULL );
	FINISH_PARAMETERS;

	if( !_entity.Get() || !result )
	{
		return;
	}

	if( !GGame->GetActiveWorld() )
	{
		return;
	}

	CCameraDirector* cameraDirector = GGame->GetActiveWorld()->GetCameraDirector();
	RED_ASSERT( cameraDirector );

	// return as list of handles
	TDynArray< THandle<CR6InteractionComponent> > & resultArr = *(TDynArray< THandle<CR6InteractionComponent> >*) result;
	ComponentIterator<CR6InteractionComponent> it( _entity.Get() );
	for( ; it; ++it )
	{
		CR6InteractionComponent* interaction = (*it);
		if( !interaction )
		{
			continue;
		}

		// Interaction enabled?
		if( !interaction->IsEnabled() )
		{
			continue;
		}

		// check distance to interaction
		if( ( cameraDirector->GetCameraPosition() - _entity.Get()->GetWorldPosition() ).Mag3() > interaction->GetRange() )
		{
			continue;
		}

		// can interact?
		Bool canInteract = false;
		CallFunctionRet< Bool, THandle<CActor> >( interaction, CNAME( CanInteract ), THandle<CActor>( this ), canInteract );
		if( !canInteract )
		{
			continue;
		}

		// doable!
		resultArr.PushBack( THandle< CR6InteractionComponent >( interaction ) );
	}
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::funcEnableCharacterControllerPhysics( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mpac )
	{
		mpac->SetGravity( enable );
		mpac->EnableStaticCollisions( enable );
		mpac->EnableCharacterCollisions( enable );
		mpac->EnableDynamicCollisions( enable );
		mpac->InvalidatePhysicsCache();
	}
}

//------------------------------------------------------------------------------------------------------------------
void CR6Player::funcGetInteractionTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CR6InteractionComponent, GetInteractionTarget() );
}




//------------------------------------------------------------------------------------------------------------------
Bool CR6Player::IsAttachedToAV()
{
	return ( GetAttachedAVNode() != NULL );
}




//------------------------------------------------------------------------------------------------------------------
CNode* CR6Player::GetAttachedAVNode()
{
	/// @todo MS: Check tag maybe?

	auto playerAttachments = GetParentAttachments();
	if( playerAttachments.Size() != 1 )
	{
		return NULL;
	}

	auto att = *(playerAttachments.Begin());

	return ( att ? att->GetParent() : NULL 	);
}

CIDInterlocutorComponent* CR6Player::GetInterlocutorComponent()
{
	ComponentIterator< CIDInterlocutorComponent > it( this );
	
	if ( it )
	{
		CIDInterlocutorComponent* ic = Cast< CIDInterlocutorComponent >( *it );
		if( ic )
		{
			return ic;
		}
	}
	
	DATA_HALT( DES_Major, GetPlayerResource(), TXT( "Component missing" ), TXT( "R6Player needs a CIDInterlocutorComponent component in the player entity" ) );
	return NULL;
}

