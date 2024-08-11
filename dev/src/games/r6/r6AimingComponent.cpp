/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r6AimingComponent.h"
#include "aimHelpTargetGatherer.h"
#include "idInterlocutor.h"
#include "eventRouterComponent.h"
#include "aimTarget.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/physicsWorldUtils.h"
#include "../../common/engine/physicsBodyWrapper.h"
#include "../../common/engine/tickManager.h"
#include "../../common/engine/utils.h"



IMPLEMENT_ENGINE_CLASS( CR6AimingComponent );










CR6AimingComponent::CR6AimingComponent()
	: m_aimedEntityChangedSinceLastFrame( false )
{
}












void CR6AimingComponent::OnAttached( CWorld* world )
{
	
	TBaseClass::OnAttached( world );

	// @todo MS: Maybe remove CAimHelpTargetsGatherer and combine CR6AimingComponent with CAimHelpTargetsGatherer?
	ComponentIterator< CAimHelpTargetsGatherer > targetsGatherer( GetEntity() );
	if ( targetsGatherer && *targetsGatherer)
	{
		m_aimHelpGatherer = *targetsGatherer;

		// Temp aim help params hack (copied from old code)
		// @todo MS: remove this hack!!!!!
		SAimHelpParams aimHelpParams;
		aimHelpParams.m_nearAngle		= 35.0f;
		aimHelpParams.m_farAngle		= 15.0f;
		aimHelpParams.m_nearDistance	= 7.0f;
		aimHelpParams.m_farDistance		= 50.0f;
		m_aimHelpGatherer.Get()->AddAimingHelpParams( aimHelpParams );
	}
	else
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT( "Component missing" ), TXT( "R6Player needs a CAimHelpTargetsGatherer component in the player entity" ) );
	}


	// Register this component for tick
	R6_ASSERT( world );
	world->GetTickManager()->AddToGroup( this, TICK_Main );
}











void CR6AimingComponent::OnDetached( CWorld* world )
{
	R6_ASSERT( world );

	/// @todo MS: when tick group is specified finally in OnAttached - set this to RemoveFromGroup for optimisation
	world->GetTickManager()->Remove( this );

	TBaseClass::OnDetached( world );
}










void CR6AimingComponent::OnTick( Float timeDelta )
{
	UpdateAimedEntity( timeDelta );
}










void CR6AimingComponent::UpdateAimedEntity( Float timeDelta )
{
	R6_ASSERT( GGame );

	if( !GGame->GetActiveWorld() || !GGame->IsActive() )
	{
		return;
	}


	CCameraDirector* cameraDirector = GGame->GetActiveWorld()->GetCameraDirector();
	R6_ASSERT( cameraDirector );



	/// @todo MS: Change this so that it is taken from somewhere not hardcoded
	CPhysicsWorld* physicsWorld = GGame->GetActiveWorld()->GetPhysicsWorld();
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include =	GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) 
															|	GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) )  
															|	GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask exclude =	GPhysicEngine->GetCollisionTypeBit( CNAME( Player ) );


	/// @todo MS: this was allready marked as fixme (old comment: 'Offset to improve gathering ( Hack to be fixed )')
	Float extraOffset	= -0.3f;

	/// @todo MS: some other hardcoded values, fix this
	Vector m_targetOrigin	= cameraDirector->GetCameraPosition() + cameraDirector->GetCameraForward() * extraOffset;
	Vector m_targetEnd		= m_targetOrigin + cameraDirector->GetCameraForward() * 100.0f;

	/// @todo MS: some other hardcoded values, fix this
	const Uint32		TARGET_MAX_RESULTS	= 20;
	SPhysicsContactInfo	contactInfo[ TARGET_MAX_RESULTS ];

	Uint32 l_NumResults =  0;
	const ETraceReturnValue retVal = physicsWorld->RayCastWithMultipleResults( m_targetOrigin, m_targetEnd, include, exclude, contactInfo, l_NumResults, TARGET_MAX_RESULTS );
	
	
	for ( Uint32 i = 0; i < l_NumResults; ++i )
	{
		CComponent* component = nullptr;
		contactInfo[ i ].m_userDataA->GetParent( component );
		if ( contactInfo[ i ].m_userDataA && component && component->GetEntity() )
		{
			CEntity* ent = component->GetEntity();

			// filter out this player (we shouldn't interact with ourselves)
			if ( !ent ||  ( ent == GetEntity() ) )
			{
				continue;
			}

			// @todo MS: fix this - we have to take under consideration equipped items for aiming
			//// filter out equipped item
			//if( m_equippedItem.Get() != NULL && m_equippedItem.Get()->GetEntity() == ent )
			//{
			//	continue;
			//}

			// Request the aimTarget component
			ComponentIterator< CAimTarget > comp( ent );
			if ( !comp )
			{
				continue;
			}

			
			SetNewAimedEntity( ent );
			return;
		}
	}


	// @todo MS: this part of code is potentially wrong, didn't have time to look at it deeper now.
	// Aim target was not found, so try getting a close one with the aim help gatherer
	if( m_aimHelpGatherer.Get() )
	{	
		m_aimHelpGatherer.Get()->PreUpdate();

		/// @todo MS: this doesn't make sense really, should be changed probably? End is close to the origin of the world?
		Vector end		= cameraDirector->GetCameraForward();
		CGameplayEntity* ge	= m_aimHelpGatherer.Get()->GetClosestValidEntityInLineOfSight( m_targetOrigin, end );
		SetNewAimedEntity( ge );
	}
}















void CR6AimingComponent::SetNewAimedEntity( CEntity* currAimEntity )
{
	m_aimedEntityChangedSinceLastFrame = ( currAimEntity != m_currAimedEntity.Get() );

	if( !m_aimedEntityChangedSinceLastFrame )
	{
		return;
	}




	CEventRouterComponent* router = NULL;
	if( m_currAimedEntity.Get() )
	{
		router = m_currAimedEntity.Get()->FindComponent< CEventRouterComponent >();
	}

	R6_ASSERT( GetEntity() );
	CIDInterlocutorComponent* dialogInterlocutor = GetEntity()->FindComponent< CIDInterlocutorComponent >();



	if( currAimEntity )				// found new aiming entity?
	{
		if( router )
		{
			router->RouteEvent( CNAME( OnLookAt ) );
		}
		
		if ( dialogInterlocutor )
		{
			dialogInterlocutor->SetAttentionToEntity( currAimEntity );
		}
	}
	else if( m_currAimedEntity.Get() ) // didn't find a new one, but lost a previous one?
	{
		if( router )
		{
			router->RouteEvent( CNAME( OnLookAway ) );
		}

		if ( dialogInterlocutor )
		{
			dialogInterlocutor->SetAttentionToEntity( nullptr );
		}
	}



	m_currAimedEntity = currAimEntity;
}













void CR6AimingComponent::funcGetAimedEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CEntity, m_currAimedEntity );
}













void CR6AimingComponent::funcGetAimedEntityChangedSinceLastFrame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_aimedEntityChangedSinceLastFrame );
}



