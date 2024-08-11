
#include "build.h"
#include "expManualStatesExecutor.h"
#include "expBaseExecutor.h"
#include "expSlideExecutor.h"
#include "..\engine\inputManager.h"
#include "../physics/physicsWorldUtils.h"
#include "..\physics\physicsWorld.h"
#include "../engine/physicsCharacterWrapper.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "movingPhysicalAgentComponent.h"
#include "movementAdjustor.h"

//////////////////////////////////////////////////////////////////////////
ExpManualStepsExecutor::ExpManualStepsExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup  )
	: m_entity( setup.m_entity )
	, m_ExplorationDescription( desc )
	, m_shouldGetOffAtBottom( false )
	, m_autoGoInDir( 0.0f )
{

}

void ExpManualStepsExecutor::ProcessGettingOff( ExpExecutorUpdateResult& result )
{
	if ( m_shouldGetOffAtBottom )
	{
		result.m_finished = true;
		result.m_finishPoint = -1;
	}
}

Bool ExpManualStepsExecutor::ContinueLoop( const ExpExecutorContext& context, ExpStepsExecutor::InternalDir& dir )
{
	m_shouldGetOffAtBottom = false;
	Bool checkRay = false;
	Vector rayStartWS = m_entity? ( m_entity->GetWorldPosition() + m_entity->GetWorldUp() * 0.5f ) : Vector::ZEROS;
	Vector rayEndWS = rayStartWS;
	dir = ID_None;
	Float verticalAxis	= m_autoGoInDir;
	static const Bool useCameraInput	= false;

	if ( m_entity->IsPlayer() )
	{
		CInputManager * inputMgr = GGame->GetInputManager();
		if( !inputMgr )
		{
			return false;
		}
		verticalAxis = inputMgr->GetActionValue( CNAME( GI_AxisLeftY ) );
		// Convert input to relative to camera
		if( useCameraInput && abs( verticalAxis )  > 0.5f )
		{
			CCameraDirector* cameraDirector = GGame->GetActiveWorld()->GetCameraDirector();
			if( cameraDirector )
			{
				Vector	inputDir	= cameraDirector->GetCameraForward() * verticalAxis;
				inputDir.Z			= 0.0f;
				inputDir			= inputDir.Normalized3();
				Vector toLadderDir	=  m_entity->GetWorldForward(); // The actor is facing the ladder

				 verticalAxis	= Abs( verticalAxis );
				if( Vector::Dot3( inputDir, toLadderDir ) < 0.0f )
				{
					verticalAxis		*= -1.0f;
				}
			}
		}
	}
	if ( verticalAxis > 0.5f )
	{
		dir = ID_Positive;
		checkRay = true;
		rayEndWS.Z += 1.9f; // but it would feel better with 2.3 if camera would not try to go into object that blocks
	}
	else if ( verticalAxis < -0.5f )
	{
		dir = ID_Negative;
		checkRay = true; // to allow getting off ladder
		rayEndWS.Z -= 1.0f;
		m_shouldGetOffAtBottom = true; // to get off ladder when hit something
	}

	if ( ( checkRay  && m_entity->IsPlayer() ) || dir == ID_Negative ) //hack for Ciri on path in prologue
	{
		if ( CLayer* lay = m_entity->GetLayer() )
		{
			if ( CWorld* world = lay->GetWorld() )
			{
				CPhysicsWorld* physWorld = nullptr;
				if ( world->GetPhysicsWorld( physWorld ) )
				{
					CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
					CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
					SPhysicsContactInfo contactInfo;
					if( physWorld->RayCastWithSingleResult( rayStartWS, rayEndWS, include, exclude, contactInfo ) == TRV_Hit )
					{
						// collision
						dir = ID_None;
					}
				}
			}
		}
	}

	if (dir != ID_None)
	{
		m_shouldGetOffAtBottom = false;
		StepManualAdjustmentPrepare( dir == ID_Positive );
	}

	return dir != ID_None;
}

void ExpManualStepsExecutor::StepManualAdjustmentPrepare( Bool goingUp )
{
	// Prepare adjustment for granulated explorations, so each loop matches the @grains@
	Float granularity;
	m_ExplorationDescription->UseEdgeGranularity( granularity );
	if( granularity > 0.0f )
	{
		// Find future target position in a proper "grain"
		Vector targetPosition	= m_entity->GetWorldPosition();
		m_exploration->SetPointOnClosestGrain( targetPosition, granularity );
	
		// Proper offset
		//Vector	offsetToEdge	= m_ExplorationDescription->GetOffset();
		//offsetToEdge			= m_entity->GetWorldForward() * offsetToEdge.X +  m_entity->GetWorldRight() * offsetToEdge.Y + m_entity->GetWorldUp() * offsetToEdge.Z;
		//Vector	displacement	= targetPosition + offsetToEdge - m_entity->GetWorldPosition();

		// Find displacement
		Vector	displacement	= targetPosition - m_entity->GetWorldPosition();
		displacement.X			= 0.0f;
		displacement.Y			= 0.0f;

		Float	timeAdjusting	= 0.3f;

		const CActor* actor = Cast< CActor >( m_entity );
		if( actor )
		{
			if( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
			{
				CMovementAdjustor* ma = mac->GetMovementAdjustor();
				ma->CancelByName( CNAME( LadderLoopAdjustment ) );
				SMovementAdjustmentRequest* request = ma->CreateNewRequest( CNAME( LadderLoopAdjustment ) );
				request->AdjustLocationVertically( true );
				request->SlideBy( displacement );
				request->AdjustmentDuration( timeAdjusting );
			}
		}
	}
}

Bool ExpManualStepsExecutor::IsBreakRequest( const ExpExecutorContext& context ) const
{
	if ( !m_entity->IsPlayer() )
	{
		return false;
	}

	CInputManager * inputMgr = GGame->GetInputManager();
	if( !inputMgr )
	{
		return false;
	}


	// Jumping out
	if( inputMgr->GetActionValue( CNAME( ExplorationInteraction ) ) > 0.5 )
	{
		return true;
	}

	// Down to water
	if( inputMgr && inputMgr->GetActionValue( CNAME( GI_AxisLeftY ) ) < 0.0f )
	{
		CActor*	actor	= ( CActor* ) m_entity;
		CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
		if ( mac )
		{
			const CMRPhysicalCharacter* character = mac->GetPhysicalCharacter();
			if ( character )
			{
#ifdef USE_PHYSX
				CPhysicsCharacterWrapper* controller = character->GetCharacterController();
				if ( controller )
				{
#ifdef USE_PHYSX
					if( controller->GetWaterLevel() >= m_entity->GetWorldPosition().Z + 0.5f )
					{
						return true;
					}
#endif
				}
#endif // USE_PHYSX
			}
		}
	}
		
	return false;
}

Bool ExpManualStepsExecutor::IsTransitionRequest( const ExpExecutorContext& context ) const
{
	// Ed: We don't use transitions and they seem to cause some bug
	//CInputManager * inputMgr = GGame->GetInputManager();
	//return inputMgr && inputMgr->GetActionValue( CNAME( X ) ) > 0.5;
	return false;
}

ExpRelativeDirection ExpManualStepsExecutor::GetQueryDirection( const ExpExecutorContext& context ) const
{
	ExpRelativeDirection dir = ERD_None;
	Float horizAxis = 0.0f;

	if ( m_entity->IsPlayer() )
	{
		CInputManager * inputMgr = GGame->GetInputManager();
		if( !inputMgr )
		{
			return dir;
		}

		horizAxis = inputMgr->GetActionValue( CNAME( GI_AxisLeftX ) );
	}

	if ( horizAxis > 0.5f )
	{
		dir = ERD_Right;
	}
	else if ( horizAxis < -0.5f )
	{
		dir = ERD_Left;
	}

	return dir;
}

//////////////////////////////////////////////////////////////////////////
