/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTask;

//////////////////////////////////////////////////////////////////////////
// CTaskDeque
//////////////////////////////////////////////////////////////////////////
template< typename TMutex >
class CTaskDeque : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Task, MC_Engine );

	static const Uint32 MAX_TASKS		= 16000;

private:
	Red::Threads::CAtomic< CTask* >			m_tasks[ MAX_TASKS ];

	Red::Threads::CAtomic< Int32 >			m_put;
	Red::Threads::CAtomic< Int32 >			m_get;
	Red::Threads::CAtomic< Int32 >			m_count;
	Red::Threads::CSpinLock					m_lock;

public:
											CTaskDeque();
											~CTaskDeque();

	void									PushBack( CTask* node );
	Bool									PopFront( CTask*& /*[out]*/ outNode );
};

typedef CTaskDeque< Red::Threads::CMutex > TTaskDeque;
typedef CTaskDeque< Red::Threads::CNullMutex > TTaskDequeNTS;
