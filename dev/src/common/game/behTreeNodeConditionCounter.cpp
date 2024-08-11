#include "build.h"
#include "behTreeNodeConditionCounter.h"
#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionCounterDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionCounterNewDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCounterDefinition
///////////////////////////////////////////////////////////////////////////////
String CBehTreeNodeConditionCounterDefinition::GetNodeCaption() const
{
	String lowLimit =
		m_counterLowerBound.m_varName.Empty()
		? String::Printf( TXT("%d"), m_counterLowerBound.m_value )
		: m_counterLowerBound.m_varName.AsString();
	String highLimit =
		m_counterUpperBound.m_varName.Empty()
		? String::Printf( TXT("%d"), m_counterUpperBound.m_value )
		: m_counterUpperBound.m_varName.AsString();
	return
		String::Printf( TXT("Semaphore %s <= %s <= %s")
		, lowLimit.AsChar()
		, m_counterName.m_varName.Empty() ? m_counterName.m_value.AsChar() : m_counterName.m_varName.AsChar()
		, highLimit.AsChar() );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCounterInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionCounterInstance::CBehTreeNodeConditionCounterInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_counter( owner, def.m_counterName.GetVal( context ) )
	, m_counterLowerBound( def.m_counterLowerBound.GetVal( context ) )
	, m_counterUpperBound( def.m_counterUpperBound.GetVal( context ) )
{
}

Bool CBehTreeNodeConditionCounterInstance::ConditionCheck()
{
	Int32 counter = m_counter->GetCounter();
	return counter >= m_counterLowerBound && counter <= m_counterUpperBound;
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCounterNewDefinition
///////////////////////////////////////////////////////////////////////////////
String CBehTreeNodeConditionCounterNewDefinition::GetNodeCaption() const
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
		String::Printf( TXT("Semaphore %s %s %s")
		, m_counterName.m_varName.Empty() ? m_counterName.m_value.AsChar() : m_counterName.m_varName.AsChar()
		, funcName
		, value.AsChar()
		);
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCounterNewInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionCounterNewInstance::CBehTreeNodeConditionCounterNewInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
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

Bool CBehTreeNodeConditionCounterNewInstance::ConditionCheck()
{
	Int32 counter = m_counter->GetCounter();

	switch ( m_comparison )
	{
	case CF_Equal:
		return counter == m_counterValue;
	case CF_NotEqual:
		return counter != m_counterValue;
	case CF_Less:
		return counter < m_counterValue;
	case CF_LessEqual:
		return counter <= m_counterValue;
	case CF_Greater:
		return counter > m_counterValue;
	case CF_GreaterEqual:
		return counter >= m_counterValue;
	default:
		ASSUME( false );
	}
	return false;
}