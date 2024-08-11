#include "build.h"
#include "behTreeNodeUseExplorationAction.h"

#include "../engine/tagManager.h"

#include "aiExplorationParameters.h"
#include "explorationScriptSupport.h"
#include "movementAdjustor.h"
#include "movingPhysicalAgentComponent.h"


BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeUseExplorationActionDefinition );
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeTeleportToMetalinkDestinationDefinition );

//Int32 CBehTreeNodeUseExplorationActionInstance::s_counter;

CBehTreeNodeUseExplorationActionInstance::CBehTreeNodeUseExplorationActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_entityTag( def.m_entityTag.GetVal( context ) )
	, m_metalinkComponent( NULL )
	, m_explorationType( def.m_explorationType.GetVal( context ) )
	, m_traverser( nullptr )	
{
	// get meta link component
	context.GetValRef( IAIExplorationTree::GetMetalinkParamName(), m_metalinkComponent );
}

Bool CBehTreeNodeUseExplorationActionInstance::IsAvailable()
{
	return m_metalinkComponent.Get() != nullptr || m_entityTag;
}

Bool CBehTreeNodeUseExplorationActionInstance::Activate()
{
	Super::Activate();

	CEntity* entity = NULL;
	
	if( !m_entityTag.Empty() )
	{
		CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
		entity				= tagMgr->GetTaggedEntity( m_entityTag );
	}
	else
	{
		entity = m_metalinkComponent.Get()->GetEntity();
	}	

	//CEntity* entity = ( CEntity* ) m_owner->GetActionTarget().Get();
	SExplorationQueryContext eContext;
	m_explorationToken = GCommonGame->QueryExplorationSync( m_owner->GetActor(), eContext );

	if( m_explorationToken.IsValid() )
	{	
		CActor* ownerActior = m_owner->GetActor();
		ownerActior->ActionTraverseExploration( m_explorationToken );
		m_explorationAction = ownerActior->GetActionExploration();
		
		m_traverser = m_explorationAction->GetTraverser();
		
		CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( ownerActior->GetMovingAgentComponent() );
		if ( mpac )
		{			
			mpac->SetAnimatedMovement( true );
			mpac->SnapToNavigableSpace( false );
			mpac->EnableDynamicCollisions( false );
			mpac->EnableStaticCollisions( false );
			// change force into EnableEntity, Enable PHysics
			mpac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_JobTree );
			mpac->InvalidatePhysicsCache();				
		}
	}

	m_prevLocalTime			= m_owner->GetLocalTime();	
	m_waitingForComplete	= false;
	return true;
}

void CBehTreeNodeUseExplorationActionInstance::EnableProperWalking()
{
	CActor* ownerActior = m_owner->GetActor();

	CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( ownerActior->GetMovingAgentComponent() );
	if ( mpac )
	{					
		mpac->SetAnimatedMovement( false );
		mpac->SnapToNavigableSpace( true );
		mpac->EnableDynamicCollisions( true );
		mpac->EnableStaticCollisions( true );
		// change force into EnableEntity, Enable PHysics
		mpac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_JobTree );
		mpac->InvalidatePhysicsCache();		
	}
}

void CBehTreeNodeUseExplorationActionInstance::Deactivate()
{
	EnableProperWalking();
	Super::Deactivate();	
}

void CBehTreeNodeUseExplorationActionInstance::Update()
{
	if( m_waitingForComplete )
	{
		Complete( IBehTreeNodeInstance::BTTO_SUCCESS );
		return;
	}

	if( !m_explorationToken.IsValid() )
	{
		Complete( IBehTreeNodeInstance::BTTO_FAILED );		
		return;
	}
	
	EActorActionResult status = m_explorationAction->GetStatus();	

	if( status == ActorActionResult_InProgress )
	{
		if( m_traverser.Get() )
		{
			Float ct		= m_owner->GetLocalTime();
			Float dt		= ct - m_prevLocalTime;
			dt				= dt > 0.04f ? 0.04f : dt;
			m_prevLocalTime = ct;
			
			m_traverser.Get()->Update( dt );		
		}	
		else
		{
			Complete( IBehTreeNodeInstance::BTTO_SUCCESS );
		}
	}	

	if( status == ActorActionResult_Succeeded )
	{
		m_waitingForComplete = true;
		EnableProperWalking();		
	}
	else if( status == ActorActionResult_Failed )
	{
		Complete( IBehTreeNodeInstance::BTTO_FAILED );
	}	
}

Bool CBehTreeNodeUseExplorationActionInstance::Interrupt()
{
	return false;
}

CBehTreeNodeTeleportToMetalinkDestinationInstance::CBehTreeNodeTeleportToMetalinkDestinationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )	
	, m_skipTeleportation( def.m_skipTeleportation.GetVal( context )  )
{	
	Vector3 destinationPoint( 0.f, 0.f, 0.f );	
	Vector3 interactionPoint( 0.f, 0.f, 0.f );	
	context.GetValRef( IAIExplorationTree::GetInteractionPointParamName(), interactionPoint );
	context.GetValRef( IAIExplorationTree::GetDestinationPointParamName(), destinationPoint );
	
	m_target = destinationPoint;

	Vector2 diff = destinationPoint.AsVector2() - interactionPoint.AsVector2();
	m_heading = EulerAngles::YawFromXY( diff.X, diff.Y );
}

void CBehTreeNodeTeleportToMetalinkDestinationInstance::Update()
{
	if( !m_skipTeleportation )
	{
		CActor* actor = m_owner->GetActor();
		EulerAngles angles = EulerAngles::ZEROS;	
		angles.Yaw = m_heading;
		actor->Teleport( m_target,  angles );
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		if ( mac )
		{
			CMovementAdjustor* adjustor = mac->GetMovementAdjustor();
			if ( adjustor )
			{
				adjustor->CancelAll();
			}
		}

	}
	Complete( IBehTreeNodeInstance::BTTO_SUCCESS );
}