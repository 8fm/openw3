/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeNodeInputDecorator.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

IMPLEMENT_RTTI_ENUM( EInputDecoratorCondition );
IMPLEMENT_RTTI_ENUM( EInputDecoratorAction );
IMPLEMENT_ENGINE_CLASS( CBehTreeValEInputDecoratorCondition );
IMPLEMENT_ENGINE_CLASS( CBehTreeValEInputDecoratorAction );




////////////////////////////////////////////////////////////////////////
// CBehTreeNodeInputDecoratorDefinition
////////////////////////////////////////////////////////////////////////


CBehTreeNodeInputDecoratorDefinition::CBehTreeNodeInputDecoratorDefinition()
	: m_rangeMin( 0.0f )
	, m_rangeMax( 1.0f )
{
}



IBehTreeNodeDecoratorInstance* CBehTreeNodeInputDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
















////////////////////////////////////////////////////////////////////////
// CBehTreeNodeInputDecoratorInstance
////////////////////////////////////////////////////////////////////////






CBehTreeNodeInputDecoratorInstance::CBehTreeNodeInputDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_inputName	( def.m_inputName.GetVal( context ) )
	, m_rangeMin	( def.m_rangeMin.GetVal( context ) )
	, m_rangeMax	( def.m_rangeMax.GetVal( context ) )
	, m_condition	( def.m_condition.GetVal( context ) )
	, m_action		( def.m_action.GetVal( context ) )
{
	if( m_rangeMin > m_rangeMax )
	{
		DATA_HALT( DES_Minor, GetOwnerResource() , TXT( "Behaviour Tree" ), TXT( "Input range max is smaller than min." ) );
		Swap( m_rangeMin, m_rangeMax );
	}

	m_prevInRange = CheckInRangeCondition( 0.0f );
}











Bool CBehTreeNodeInputDecoratorInstance::IsAvailable()
{
	Bool condition = CheckCondition();

	if( m_action == IDA_PERFORM_ACTION )
	{
		if( !condition )
		{
			DebugNotifyAvailableFail();
			return false;
		}

		return Super::IsAvailable();
	}



	if( m_action == IDA_START_ACTION )
	{
		if( IsActive() || condition )
		{
			return Super::IsAvailable();
		}

		DebugNotifyAvailableFail();
		return false;
	}


	if( m_action == IDA_STOP_ACTION )
	{
		if( IsActive() && condition )
		{
			DebugNotifyAvailableFail();
			return false;
		}

		return Super::IsAvailable();
	}

	DebugNotifyAvailableFail();
	R6_ASSERT( false, TXT( "Unknown IDC Action in behTreeNodeInputDecorator" ) );
	return false;
}










Bool CBehTreeNodeInputDecoratorInstance::CheckCondition()
{
	CInputManager* inputManager = GGame->GetInputManager();
	if ( nullptr == inputManager )
	{
		// can't walk without input
		return false;
	}

	if( m_inputName == CName::NONE )
	{
		DATA_HALT( DES_Major, GetOwnerResource() , TXT( "Behaviour Tree" ), TXT( "Input action name is not specified. (Input Decorator Node)" ) );
		return false;
	}

	SInputAction& inputAction = inputManager->GetInputActionByName( m_inputName );
	if( inputAction == SInputAction::INVALID )
	{
		DATA_HALT( DES_Major, GetOwnerResource(), TXT( "Behaviour Tree" ), TXT( "Input action name specified for decorator is invalid. (Input Decorator Node)" ) );
		return false;
	}


	switch ( m_condition )
	{
		case IDC_IN_RANGE:
		{
			m_prevInRange = CheckInRangeCondition( inputAction.m_value );
			return m_prevInRange;
		}

		case IDC_OUT_OF_RANGE:
		{
			m_prevInRange = CheckInRangeCondition( inputAction.m_value );
			return !m_prevInRange;
		}

		case IDC_RANGE_CHANGED:
		{
			Bool inRange = CheckInRangeCondition( inputAction.m_value );
			Bool toRet = ( inRange != m_prevInRange );
			m_prevInRange = inRange;
			return toRet;
		}

		case IDC_GOT_IN_RANGE:
		{
			Bool inRange = CheckInRangeCondition( inputAction.m_value );
			Bool toRet = inRange && !m_prevInRange;
			m_prevInRange = inRange;
			return toRet;
		}

		case IDC_GOT_OUT_OF_RANGE:
		{
			Bool inRange = CheckInRangeCondition( inputAction.m_value );
			Bool toRet = !inRange && m_prevInRange;
			m_prevInRange = inRange;
			return toRet;
		}
	}

	R6_ASSERT( false, TXT( "Unknown IDC Condition in behTreeNodeInputDecorator" ) );
	return false;
}







Bool CBehTreeNodeInputDecoratorInstance::CheckInRangeCondition( Float val )
{
	R6_ASSERT( m_rangeMax >= m_rangeMin );
	return ( val >= m_rangeMin ) && ( val <= m_rangeMax );
}

const CResource* CBehTreeNodeInputDecoratorInstance::GetOwnerResource() const
{
	return CResourceObtainer::GetResource( GetOwner()->GetActor() );
}

