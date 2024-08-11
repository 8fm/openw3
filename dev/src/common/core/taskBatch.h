/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskSched.h"
#include "dynarray.h"

class CTask;
class CTaskBatch;

struct STaskBatchSyncToken
{
	friend CTaskBatch;

private:
	Red::Threads::CMutex				m_mutex;
	Red::Threads::CConditionVariable	m_condVar;

private:
	Int32								m_syncCount;
	Int32								m_waitCount;

	STaskBatchSyncToken();
	~STaskBatchSyncToken();

public:
	void								Sync();

public:
	void								Wait();
};

//////////////////////////////////////////////////////////////////////////
// CTaskBatch
//////////////////////////////////////////////////////////////////////////
class CTaskBatch
{
public:
	

private:
	STaskBatchSyncToken						m_syncToken;
	TDynArray< CTask* >						m_taskStack;

public:
											CTaskBatch( Uint32 reserveCount );
											~CTaskBatch();
public:
	void									Add( CTask& task );

public:
	STaskBatchSyncToken&					GetSyncToken() { return m_syncToken; }
	const STaskBatchSyncToken&				GetSyncToken() const { return m_syncToken; }

public:
	void									IssueAndWait( ETaskSchedulePriority taskSchedulePriority = TSP_Normal );
	void									Issue( ETaskSchedulePriority taskSchedulePriority = TSP_Normal );
	void									IssueWithoutInlining( ETaskSchedulePriority taskSchedulePriority = TSP_Normal );
	void									FinishTasksAndWait();

private:
	void									DoIssue( ETaskSchedulePriority taskSchedulePriority );
	void									DoIssueWithoutInlining( ETaskSchedulePriority taskSchedulePriority );
	void									RunTasksInline();
};