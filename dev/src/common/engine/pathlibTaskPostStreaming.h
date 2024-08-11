/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibStreamingItem.h"
#include "pathlibTaskManager.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Runs OnPostLoad() on streaming items (areas) asynchronously, and Attach()
// on synchronous post processing.
// We don't block resource loading task queue with our internal, possibly
// heavy processing. Instead as soon as CStreamingJob ends, we run
// this CTaskPostStreaming task to finish up and post-process loaded data,
// that is integrated with our pathlib task queue, so it doesn't overlap
// with any kind of asynchoronus processing.
////////////////////////////////////////////////////////////////////////////
class CTaskPostStreaming : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	IStreamingItem::List::Head		m_requestsList;

public:
	CTaskPostStreaming( PathLib::CTaskManager& taskManager, IStreamingItem::List::Head&& requestsList );

	// subclass interface task 
	virtual void			Process();
	virtual void			PostProcessingSynchronous() override;

	virtual void			DescribeTask( String& outName ) const override;
};

};			// namespace PathLib