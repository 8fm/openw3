#include "build.h"

#include "behTreeNodeCondition.h"

String CBehTreeNodeConditionDefinition::DecorateCaption( const String& baseCaption ) const
{
	return
		String::Printf(TXT("%s%s%s%s%s")
		, m_invertAvailability ? TXT("!") : TXT("")
		, baseCaption.AsChar()
		, m_forwardAvailability ? TXT(" &") : TXT("")
		, m_forwardTestIfNotAvailable ? TXT(" |") : TXT("")
		, m_skipIfActive ? TXT(" :)") : TXT("")
		);
}
String CBehTreeNodeConditionDefinition::GetNodeCaption() const
{
	String baseCaption = TBaseClass::GetNodeCaption();
	return DecorateCaption( baseCaption );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionInstance::IsAvailable()
{
	Bool available;
	if ( m_skipIfActive && m_isActive )
	{
		available = true;
	}
	else
	{
		available = ConditionCheck();
		if ( m_invertAvailability )
			available = !available;
	}

	if ( available )
	{
		if ( m_forwardAvailability )
		{
			if( Super::IsAvailable() )
			{
				return true;
			}
			else
			{
				DebugNotifyAvailableFail();
				return false;
			}
		}
		return true;
	}
	else
	{
		if ( m_forwardTestIfNotAvailable )
		{
			if( Super::IsAvailable() )
			{
				return true;
			}
			else
			{
				DebugNotifyAvailableFail();
				return false;
			}
		}

		DebugNotifyAvailableFail();
		return false;
	}
}
Int32 CBehTreeNodeConditionInstance::Evaluate()
{
	Bool available;
	if ( m_skipIfActive && m_isActive )
	{
		available = true;
	}
	else
	{
		available = ConditionCheck();
		if ( m_invertAvailability )
			available = !available;
	}

	if ( available )
	{
		if ( m_forwardAvailability )
		{
			return Super::Evaluate();
		}
		return m_priority;
	}
	else
	{
		if ( m_forwardTestIfNotAvailable )
		{
			return Super::Evaluate();
		}
		return -1;
	}
}
