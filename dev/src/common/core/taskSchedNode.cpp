/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskSchedNode.h"

CTaskSchedNode::CTaskSchedNode()
	: m_taskScheduleState( TSS_Ready )
	, m_cancelledFlag( false )
{
}

CTaskSchedNode::~CTaskSchedNode()
{
}

Bool CTaskSchedNode::MarkScheduled()
{
	return CompareExchangeScheduleState( TSS_Scheduled, TSS_Ready );
}

Bool CTaskSchedNode::MarkRunning()
{
	return CompareExchangeScheduleState( TSS_Running, TSS_Scheduled );
}

Bool CTaskSchedNode::MarkFinished()
{
	SetScheduleState( TSS_Finished );
	return true;
}

Bool CTaskSchedNode::MarkCancelled()
{
	if ( CompareExchangeScheduleState( TSS_Finished, TSS_Scheduled ) )
	{
		m_cancelledFlag.SetValue( true );
		return true;
	}

	return false;
}
