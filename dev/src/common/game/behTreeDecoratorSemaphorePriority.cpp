/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorSemaphorePriority.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition
///////////////////////////////////////////////////////////////////////////////
String CBehTreeNodeDecoratorPriorityOnSemaphoreDefinition::GetNodeCaption() const
{
	const Char* funcName;

	switch ( m_comparison )
	{
	case CF_Equal:
		funcName = TXT("=");
		break;
	case CF_NotEqual:
		funcName = TXT("<>");
		break;
	case CF_Less:
		funcName = TXT("<");
		break;
	case CF_LessEqual:
		funcName = TXT("<=");
		break;
	case CF_Greater:
		funcName = TXT(">");
		break;
	case CF_GreaterEqual:
		funcName = TXT(">=");
		break;
	default:
		funcName = TXT("wtf");
		break;
	}

	String value =
		m_counterValue.m_varName.Empty()
		? String::Printf( TXT("%d"), m_counterValue.m_value )
		: m_counterValue.m_varName.AsString();

	return
		String::Printf( TXT("PriorityOnSemaphore %s %s %s")
		, m_counterName.m_varName.Empty() ? m_counterName.m_value.AsChar() : m_counterName.m_varName.AsChar()
		, funcName
		, value.AsChar()
		);
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorPriorityOnSemaphoreInstance
///////////////////////////////////////////////////////////////////////////////

CBehTreeNodeDecoratorPriorityOnSemaphoreInstance::CBehTreeNodeDecoratorPriorityOnSemaphoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_counter( owner, def.m_counterName.GetVal( context ) )
	, m_counterValue( def.m_counterValue.GetVal( context ) )
	, m_comparison( def.m_comparison )
{
	// this test allows me to bypass default branch later on
	switch ( m_comparison )
	{
	case CF_Equal:
	case CF_NotEqual:
	case CF_Less:
	case CF_LessEqual:
	case CF_Greater:
	case CF_GreaterEqual:
		break;
	default:
		RED_WARNING( false, "CBehTreeNodeConditionCounterNewInstance invalid m_comparison value!" );
		m_comparison = CF_Equal;
		break;
	}
}

Int32 CBehTreeNodeDecoratorPriorityOnSemaphoreInstance::Evaluate()
{
	Int32 eval = Super::Evaluate();
	if ( eval > 0 )
	{
		Bool isOk;

		Int32 counter = m_counter->GetCounter();

		switch ( m_comparison )
		{
		case CF_Equal:
			isOk = counter == m_counterValue;
			break;
		case CF_NotEqual:
			isOk = counter != m_counterValue;
			break;
		case CF_Less:
			isOk = counter < m_counterValue;
			break;
		case CF_LessEqual:
			isOk = counter <= m_counterValue;
			break;
		case CF_Greater:
			isOk = counter > m_counterValue;
			break;
		case CF_GreaterEqual:
			isOk = counter >= m_counterValue;
			break;
		default:
			isOk = false;
			ASSUME( false );
		}
		if ( isOk )
		{
			return m_priority;
		}
	}
	return eval;
}