
#include "build.h"
#include "switch.h"

IMPLEMENT_ENGINE_CLASS( CSwitch );

CSwitch::CSwitch()
{
}

void CSwitch::funcCallEventSwitch( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	for( Uint32 i = 0, end = m_TargetEntities.Size(); i < end; ++i )
	{
		m_TargetEntities[i].Get()->CallEvent( CNAME( OnSwitched ), m_SwitchId );
	}
	// Call on self
	CallEvent( CNAME( OnSwitched ), m_SwitchId );
}

void CSwitch::funcCallEventSwitchOn( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	for( Uint32 i = 0, end = m_TargetEntities.Size(); i < end; ++i )
	{
		m_TargetEntities[i].Get()->CallEvent( CNAME( OnSwitchedOn ), m_SwitchId );
	}
	// Call on self
	CallEvent( CNAME( OnSwitchedOn ), m_SwitchId );
}

void CSwitch::funcCallEventSwitchOff( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	for( Uint32 i = 0, end = m_TargetEntities.Size(); i < end; ++i )
	{
		m_TargetEntities[i].Get()->CallEvent( CNAME( OnSwitchedOff ), m_SwitchId );
	}
	// Call on self
	CallEvent( CNAME( OnSwitchedOff ), m_SwitchId );
}