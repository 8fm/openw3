#include "build.h"

#include "npcMovementControllerComponent.h"
#include "enemyAwareComponent.h"

#include "..\..\common\game\movableRepresentationPathAgent.h"

IMPLEMENT_ENGINE_CLASS( CNPCMovementControllerComponent );


#define CMP_ACTION_START_TEST if( !GetEntity()->CanPerformActionFromScript( stack ) ) { RETURN_BOOL( false ); return; }


void CNPCMovementControllerComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	m_safePathReater.SetOwnetEntity( GetEntity() );
}

void CNPCMovementControllerComponent::funcSafeMoveToAsync( CScriptStackFrame& stack, void* result )
{

	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Int32, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );	
	FINISH_PARAMETERS;

	// Start test
	CMP_ACTION_START_TEST;


	CActor* parentActor =  Cast< CActor >( GetEntity() );
	if( !parentActor )
	{
		return;
	}

	CMovingAgentComponent* mac = parentActor->GetMovingAgentComponent();
	if( !mac )
	{	
		return;
	}
	CEnemyAwareComponent* enemyAwareComponent = parentActor->FindComponent< CEnemyAwareComponent >();
	if( enemyAwareComponent )
	{
		
		if( enemyAwareComponent->IsEnemyiesPresent() )
		{
			CPathAgent* pathAgent = mac->GetPathAgent();
			pathAgent->SetPrecisePathFollowing( true );
			pathAgent->SetPathRater( &m_safePathReater );
		}
	}

	Bool actionResult = parentActor->ActionMoveToChangeTarget( target, (EMoveType)moveType, absSpeed, range );
	RETURN_BOOL( actionResult );
}

void CNPCMovementControllerComponent::funcActionEnded( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	CActor* parentActor =  Cast< CActor >( GetEntity() );
	if( parentActor )
	{
		CMovingAgentComponent* mac = parentActor->GetMovingAgentComponent();
		CPathAgent* pathAgent = mac->GetPathAgent();
		pathAgent->SetPrecisePathFollowing( false );
		pathAgent->SetPathRater( NULL );
	}	
}