/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiAction.h"
#include "r6aiSystem.h"
#include "r6behTreeInstance.h"
#include "behTreeNodeAIAction.h"

IMPLEMENT_RTTI_ENUM( EAIActionStatus )
IMPLEMENT_ENGINE_CLASS( CAIAction )

CAIAction::CAIAction() 
	: m_status( ACTION_NotStarted )
{

}

#ifndef FINAL
void CAIAction::SetErrorState( const Char* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	Char buf[ 4096 ];
	vswprintf_s( buf, 4096, format, arglist );
	m_errorMsg = buf;
	va_end( arglist );
	m_status = ACTION_Failed;
}
#endif // ifndef FINAL

EAIActionStatus CAIAction::StartOn( CComponent* component )
{
	m_status = ACTION_InProgress;
	GCommonGame->GetSystem< CR6AISystem > ()->OnPerformAIAction( this );
	return m_status;
}

EAIActionStatus CAIAction::Stop( EAIActionStatus newStatus )
{
	R6_ASSERT( newStatus != ACTION_InProgress && newStatus != ACTION_NotStarted );

	if ( m_status == ACTION_InProgress )
	{
		m_status = newStatus;
		GCommonGame->GetSystem< CR6AISystem > ()->OnStopAIAction( this );

		if ( m_nodeInstance )
		{
			m_nodeInstance->OnStopAIAction( this );
		}
	}

	m_status = newStatus;
	return newStatus;
}

EAIActionStatus CAIAction::Cancel( const Char* reason )
{
	Stop( ACTION_Failed );
	SetErrorState( TXT("Action %s cancelled for reason: %s"), GetClass()->GetName().AsChar(), reason ); 
	return m_status;
}

EAIActionStatus CAIAction::Reset()
{
	m_errorMsg.Clear();
	m_status = ACTION_NotStarted;
	return ACTION_NotStarted;
}

EAIActionStatus CAIAction::Tick( Float timeDelta )
{
	R6_ASSERT( ShouldBeTicked() );
	return m_status;
}

EAIActionStatus CAIAction::RequestInterruption()
{
	return Stop( ACTION_Failed );
}

CNode* CAIAction::FindActionTarget() const
{
	if ( nullptr == m_nodeInstance )
	{
		return nullptr;
	}

	CBehTreeInstance* inst = m_nodeInstance->GetOwner();
	if ( nullptr == inst )
	{
		return nullptr;
	}

	return inst->GetActionTarget().Get();
}


