#include "build.h"
#include "questGraphSocket.h"
#include "questTimeMgmtBlock.h"
#include "../engine/gameTimeManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestTimeManagementBlock )
IMPLEMENT_ENGINE_CLASS( IQuestTimeFunction )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestTimeManagementBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestTimeManagementBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
	
	if ( m_function )
	{
		m_function->Execute();
	}
	ActivateOutput( data, CNAME( Out ) );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CPauseTimeFunction )

void CPauseTimeFunction::Execute()
{
	if ( m_pause )
	{
		GGame->GetTimeManager()->Pause();
	}
	else
	{
		GGame->GetTimeManager()->Resume();
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CSetTimeFunction )

void CSetTimeFunction::Execute()
{
	GameTime curTime = GGame->GetTimeManager()->GetTime();
	GameTime dayTime = GameTime( 0, curTime.Hours(), curTime.Minutes(), curTime.Seconds() );
	Int32 days = curTime.Days();

	if ( m_newTime < dayTime ) //if we want to set earlier hour make next day
	{
		days++;
	}
	curTime = GameTime( days, 0, 0, 0 ) + m_newTime;

	GGame->GetTimeManager()->SetTime( curTime, m_callEvents );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CShiftTimeFunction )

void CShiftTimeFunction::Execute()
{
	GameTime newTime = GGame->GetTimeManager()->GetTime();
	newTime += m_timeShift;
	GGame->GetTimeManager()->SetTime( newTime, m_callEvents );
}
