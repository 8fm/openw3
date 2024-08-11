#pragma once

#include "../core/math.h"

struct CSectorDataStreamingContextThreadData;

// ehh, another thread
// NOTE: this is because we need huge amount of stack space and the normal CTasks they do not have enough
class CSectorDataStreamingThread : public Red::Threads::CThread
{
public:
	CSectorDataStreamingThread( CSectorDataStreamingContextThreadData* threadData );
	~CSectorDataStreamingThread();

	// general interface - start/finish, not secured - match start/finish calls carefully
	// state less, can work with any streaming data wrapper
	void Start( class CSectorDataStreamingContext& context, const Vector& referencePosition );
	void Finish();

private:
	void SendKillSignal();
	virtual void ThreadFunc();

	// thread state
	Red::Threads::CAtomic< Bool >			m_exit;
	Red::Threads::CAtomic< Bool >			m_processing;
	Red::Threads::CSemaphore				m_startLock;
	Red::Threads::CSemaphore				m_finishLock;

	// query information
	Vector									m_queryPosition;
	CSectorDataStreamingContext*			m_queryContext;

	// thread data
	CSectorDataStreamingContextThreadData*	m_threadData;	
};

