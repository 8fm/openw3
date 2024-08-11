#include "build.h"
#include "scriptedComponent.h"

RED_DEFINE_STATIC_NAME( OnComponentAttachFinished )
RED_DEFINE_STATIC_NAME( OnComponentDetached )

IMPLEMENT_ENGINE_CLASS( CScriptedComponent );

CScriptedComponent::CScriptedComponent()
	: m_useUpdateTransform( true )
{

}

void CScriptedComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CScriptedComponent_OnAttached );
	CallEvent( CNAME( OnComponentAttached ) );
}

void CScriptedComponent::OnAttachFinished( CWorld* world ) 
{
	TBaseClass::OnAttachFinished( world );
	CallEvent( CNAME( OnComponentAttachFinished ) );
}

void CScriptedComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	CallEvent( CNAME( OnComponentDetached ) );
}

void CScriptedComponent::ProcessEvent( CName eventName, CObject* param )
{
	for( Uint32 i=0; i < m_interestingEvents.Size(); ++i )
	{
		CScriptedComponent::SIntrestingEventEntry iEvent = m_interestingEvents[i];
		if( eventName == iEvent.m_eventName )
		{
			if( iEvent.m_reportToScripts )
			{
				if( iEvent.m_eventToCallOnComponent )
				{
					CallFunction( this, iEvent.m_eventToCallOnComponent, THandle< CObject >( param ) );
				}
				else
				{
					CallFunction( this, CNAME( Export_OnEventOccur ), eventName, THandle< CObject >( param ) );
				}
			}
			else
			{
				OnEventOccure( eventName, param );
			}
		}
	}
}

void CScriptedComponent::ListenToEvent( CName eventName, CName scriptEventToCall, Bool reportToScripts )
{
	CScriptedComponent::SIntrestingEventEntry entry;

	entry.m_eventName				= eventName;
	entry.m_eventToCallOnComponent	= scriptEventToCall;
	entry.m_reportToScripts			= reportToScripts;

	m_interestingEvents.PushBack( entry );
}

void CScriptedComponent::funcListenToEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER_OPT( CName, eventToCall, CName::NONE );
	FINISH_PARAMETERS;

	ListenToEvent( eventName, eventToCall, true );
}

void CScriptedComponent::funcUseUpdateTransform( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );

	m_useUpdateTransform = flag;
}
