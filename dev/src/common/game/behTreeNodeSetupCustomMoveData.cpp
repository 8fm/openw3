/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSetupCustomMoveData.h"

#include "aiExplorationParameters.h"
#include "behTreeInstance.h"
#include "doorComponent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSetupCustomMoveTargetToPositionDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeNotifyDoorDefinition )

////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeSetupCustomMoveDataInstance
////////////////////////////////////////////////////////////////////////////
IBehTreeNodeSetupCustomMoveDataInstance::IBehTreeNodeSetupCustomMoveDataInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_targetPtr( owner )
	, m_setTargetForEvaluation( def.m_setTargetForEvaluation )
{}

Bool IBehTreeNodeSetupCustomMoveDataInstance::Activate()
{
	CBehTreeCustomMoveData* data = m_targetPtr.Get();

	Vector target;
	Float heading;

	if ( !ComputeTargetAndHeading( target, heading ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	data->SetTarget( target );
	data->SetHeading( heading );
	
	return Super::Activate();
}

Bool IBehTreeNodeSetupCustomMoveDataInstance::IsAvailable()
{
	if ( m_setTargetForEvaluation && !m_isActive )
	{
		CBehTreeCustomMoveData* data = m_targetPtr.Get();

		Vector prevTarget = data->GetTarget();
		Float prevHeading = data->GetHeading();

		Vector target;
		Float heading;

		if ( !ComputeTargetAndHeading( target, heading ) )
		{
			DebugNotifyAvailableFail();
			return false;
		}

		data->SetTarget( target );
		data->SetHeading( heading );

		Bool ret = Super::IsAvailable();

		data->SetTarget( prevTarget );
		data->SetHeading( prevHeading );

		if( !ret )
		{
			DebugNotifyAvailableFail();
		}

		return ret;
	}

	return Super::IsAvailable();
}

Int32 IBehTreeNodeSetupCustomMoveDataInstance::Evaluate()
{
	if ( m_setTargetForEvaluation && !m_isActive )
	{
		CBehTreeCustomMoveData* data = m_targetPtr.Get();

		Vector prevTarget = data->GetTarget();
		Float prevHeading = data->GetHeading();

		Vector target;
		Float heading;

		if ( !ComputeTargetAndHeading( target, heading ) )
		{
			DebugNotifyAvailableFail();
			return -1;
		}

		data->SetTarget( target );
		data->SetHeading( heading );

		Int32 ret = Super::Evaluate();

		data->SetTarget( prevTarget );
		data->SetHeading( prevHeading );

		if( ret <= 0 )
		{
			DebugNotifyAvailableFail();
		}

		return ret;
	}
	
	return Super::Evaluate();
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSetupCustomMoveTargetToPositionInstance
////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSetupCustomMoveTargetToPositionInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	outTarget = m_target;
	outHeading = m_heading;
	return true;
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance
////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	outTarget.AsVector3() = m_target;
	outHeading = m_heading;
	return true;
}

CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance::CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
{
	Vector3 interactionPoint( 0.f, 0.f, 0.f );
	Vector3 destinationPoint( 0.f, 0.f, 0.f );
	context.GetValRef( IAIExplorationTree::GetInteractionPointParamName(), interactionPoint );
	context.GetValRef( IAIExplorationTree::GetDestinationPointParamName(), destinationPoint );
	m_target = interactionPoint;
	Vector2 diff = destinationPoint.AsVector2() - interactionPoint.AsVector2();
	m_heading = EulerAngles::YawFromXY( diff.X, diff.Y );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance
////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance::ComputeTargetAndHeading( Vector& outTarget, Float& outHeading )
{
	outTarget.AsVector3() = m_target;
	outHeading = m_heading;
	return true;
}

CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance::CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
{
	Vector3 interactionPoint( 0.f, 0.f, 0.f );
	Vector3 destinationPoint( 0.f, 0.f, 0.f );
	context.GetValRef( IAIExplorationTree::GetInteractionPointParamName(), interactionPoint );
	context.GetValRef( IAIExplorationTree::GetDestinationPointParamName(), destinationPoint );
	m_target = destinationPoint;
	Vector2 diff = destinationPoint.AsVector2() - interactionPoint.AsVector2();
	m_heading = EulerAngles::YawFromXY( diff.X, diff.Y );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeNotifyDoorDefinition
////////////////////////////////////////////////////////////////////////////
CBehTreeNodeNotifyDoorInstance::CBehTreeNodeNotifyDoorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_metalinkComponent( NULL )
{
	// get meta link component
	context.GetValRef( IAIExplorationTree::GetMetalinkParamName(), m_metalinkComponent );
}

Bool CBehTreeNodeNotifyDoorInstance::Activate()
{
	Bool result = IBehTreeNodeDecoratorInstance::Activate();

	if( m_metalinkComponent.Get() )
	{
		// get door component (should have same parent)
		CEntity* entity = m_metalinkComponent.Get()->GetEntity();
		CDoorComponent* door = entity->FindComponent<CDoorComponent>();

		// got a door to notify?
		if( door )
		{
			// notify door
			door->AddDoorUser( GetOwner()->GetNPC() );
			if( door->IsClosed() )
			{
				door->Open();
			}
		}
	}

	return result;
}