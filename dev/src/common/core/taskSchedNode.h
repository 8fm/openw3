/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskSched.h"

class CTaskSchedNode
{
public:
	enum ETaskScheduleState
	{
		TSS_Ready,
		TSS_Scheduled,
		TSS_Running,
		TSS_Finished,
	};

private:
	Red::Threads::CAtomic< ETaskScheduleState > m_taskScheduleState;
	Red::Threads::CAtomic< Bool >				m_cancelledFlag;

public:
												CTaskSchedNode();
	virtual										~CTaskSchedNode();

public:
	Bool										IsReady() const { return m_taskScheduleState.GetValue() == TSS_Ready; }
	Bool										IsScheduled() const { return m_taskScheduleState.GetValue() == TSS_Scheduled; }
	Bool										IsRunning() const { return m_taskScheduleState.GetValue() == TSS_Running; }
	Bool										IsFinished() const { return m_taskScheduleState.GetValue() == TSS_Finished; }
	Bool										IsCancelled() const { return m_cancelledFlag.GetValue(); }

public:
	Bool										MarkScheduled();
	Bool										MarkRunning();
	Bool										MarkFinished();
	Bool										MarkCancelled();

private:
	void										SetScheduleState( ETaskScheduleState state ) { m_taskScheduleState.SetValue( state ); }
	Bool										CompareExchangeScheduleState( ETaskScheduleState exchange, ETaskScheduleState comparand ) { return m_taskScheduleState.CompareExchange( exchange, comparand ) == comparand; }
};