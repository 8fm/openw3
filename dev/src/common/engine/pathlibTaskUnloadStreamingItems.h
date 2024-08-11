/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibStreamingItem.h"
#include "pathlibTaskManager.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Asynchronous un-streaming code.
// We asynchronously detach areas and removes their
// obstacle (& metaconnections) mapping
////////////////////////////////////////////////////////////////////////////
class CTaskUnloadStreamingItems : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	CStreamingManager&				m_streamingManager;
	IStreamingItem::List::Head		m_itemsToUnload;
	Bool							m_lockedStreaming;

public:
	CTaskUnloadStreamingItems( CTaskManager& taskManager, CStreamingManager& streamingManager, IStreamingItem::List::Head&& items );
	~CTaskUnloadStreamingItems();

	// subclass interface task 
	virtual Bool					PreProcessingSynchronous() override;
	virtual void					Process() override;
	virtual void					PostProcessingSynchronous() override;

	virtual void					DescribeTask( String& outName ) const override;
};

};			// namespace PathLib