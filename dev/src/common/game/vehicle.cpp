
#include "build.h"
#include "vehicle.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "../engine/tickManager.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CVehicleComponent );

RED_DEFINE_STATIC_NAME( OnInit )
RED_DEFINE_STATIC_NAME( OnDeinit )

void CVehicleComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CVehicleComponent_OnAttached );

	world->GetTickManager()->AddToGroup( this, TICK_Main );

	// Flag gameplay entity as a vehicle container
	CGameplayEntity *gmplent = Cast< CGameplayEntity > ( GetEntity() );
	if ( gmplent )
	{
		gmplent->SetGameplayFlags( FLAG_HasVehicle );
	}
	
	CallEvent( CNAME( OnInit ) );
}

void CVehicleComponent::OnDetached( CWorld* world )
{
	CallEvent( CNAME( OnDeinit ) );
	world->GetTickManager()->Remove( this );
	
	TBaseClass::OnDetached( world );
}

void CVehicleComponent::OnTick( Float timeDelta )
{
	if( m_commandToMountDelay )
	{
		CallEvent( CName( TXT( "OnDelayedCommandToMount" ) ), timeDelta );
	}

	CallEvent( CNAME( OnTick ), timeDelta );
}

Bool CVehicleComponent::PlayAnimationOnVehicle( const CName& slotName, const CName& animationName, Float blendIn, Float blendOut )
{
	CEntity* entity = GetEntity();

	CAnimatedComponent* animated = entity->GetRootAnimatedComponent();
	if( !animated || !animated->GetBehaviorStack() )
	{
		return false;
	}

	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = blendIn;
	slotSetup.m_blendOut = blendOut;

	if ( !animated->GetBehaviorStack()->PlaySlotAnimation( slotName, animationName, &slotSetup ) )
	{
		return false;
	}

	return true;
}

Bool CVehicleComponent::StopAnimationOnVehicle( const CName& slotName )
{
	CEntity* entity = GetEntity();

	CAnimatedComponent* animated = entity->GetRootAnimatedComponent();
	if( !animated || !animated->GetBehaviorStack() )
	{
		return false;
	}

	return animated->GetBehaviorStack()->StopAllSlotAnimation( slotName );
}

Bool CVehicleComponent::IsPlayingAnimationOnVehicle( const CName& slotName ) const
{
	CEntity* entity = GetEntity();

	CAnimatedComponent* animated = entity->GetRootAnimatedComponent();
	if( !animated || !animated->GetBehaviorStack() )
	{
		return false;
	}

	return animated->GetBehaviorStack()->HasSlotAnimation( slotName );
}

extern Bool GLatentFunctionStart;

void CVehicleComponent::funcPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( Float, blendIn, 0.2f );
	GET_PARAMETER_OPT( Float, blendOut, 0.2f );
	FINISH_PARAMETERS;

	Bool actionResult = PlayAnimationOnVehicle( slotName, animationName, blendIn, blendOut);
	
	RETURN_BOOL( actionResult );
}

void CVehicleComponent::funcPlaySlotAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( Float, blendIn, 0.2f );
	GET_PARAMETER_OPT( Float, blendOut, 0.2f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	if ( GLatentFunctionStart )
	{
		Bool actionResult = PlayAnimationOnVehicle( slotName, animationName, blendIn, blendOut );
		if ( !actionResult )
		{
			RETURN_BOOL( false );
			return;
		}
	}

	if ( IsPlayingAnimationOnVehicle( slotName ) )
	{
		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}

void CVehicleComponent::funcGetDeepDistance( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Vector, vel, Vector::ZEROS );
    FINISH_PARAMETERS;

    SPhysicsContactInfo contactInfo;
    STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );

    // Not the best idea to GetPhysicsWorld like that - todo
	CPhysicsWorld* physicsWorld = nullptr;
	GetLayer()->GetWorld()->GetPhysicsWorld( physicsWorld );
	physicsWorld->RayCastWithSingleResult( vel, vel - Vector( 0, 0, 100 ), include, 0, contactInfo );

    RETURN_FLOAT( Float( contactInfo.m_position.Z ) );
}

void CVehicleComponent::funcGetSlotTransform( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER_REF( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, rot, Vector::ZEROS );
	FINISH_PARAMETERS;

	CEntity* ent = GetEntity();
	ASSERT( ent );
	CEntityTemplate* templ = ent->GetEntityTemplate();
	ASSERT( templ );
	const EntitySlot* slot = templ->FindSlotByName( slotName, true );

	if( slot )
	{
		Matrix l2w;
		slot->CalcMatrix( ent, l2w, NULL );
		pos = l2w.GetTranslation();
		rot = l2w.ToQuat();
	}
}

void CVehicleComponent::funcSetCommandToMountDelayed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, ctmd, false );
	FINISH_PARAMETERS;
	m_commandToMountDelay = ctmd;
}

void CVehicleComponent::funcIsCommandToMountDelayed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_commandToMountDelay );
}

void CVehicleComponent::funcOnDriverMount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	OnDriverMount();
}
