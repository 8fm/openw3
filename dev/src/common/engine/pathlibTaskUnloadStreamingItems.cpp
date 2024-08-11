/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskUnloadStreamingItems.h"

#include "pathlibStreamingManager.h"

namespace PathLib
{

CTaskUnloadStreamingItems::CTaskUnloadStreamingItems( PathLib::CTaskManager& taskManager, CStreamingManager& streamingManager, IStreamingItem::List::Head&& items )
	: Super( taskManager, T_MODYFICATION, FLAG_USE_PREPROCESSING | FLAG_USE_POSTPROCESSING, EPriority::UnloadStreamingItems )
	, m_streamingManager( streamingManager )
	, m_itemsToUnload( Move( items ) )
	, m_lockedStreaming( true )
{
	streamingManager.AddStreamingLock();
}

CTaskUnloadStreamingItems::~CTaskUnloadStreamingItems()
{
	if ( m_lockedStreaming )
	{
		m_streamingManager.ReleaseStreamingLock();
	}
}

Bool CTaskUnloadStreamingItems::PreProcessingSynchronous()
{
	for ( auto it = m_itemsToUnload.Begin(), end = m_itemsToUnload.End(); it != end; ++it )
	{
		IStreamingItem& item = *it;
		item.PreUnload();
	}
	return true;
}
void CTaskUnloadStreamingItems::Process()
{
	for ( auto it = m_itemsToUnload.Begin(), end = m_itemsToUnload.End(); it != end; ++it )
	{
		IStreamingItem& item = *it;
		item.Unload();
	}
	if ( m_lockedStreaming )
	{
		m_streamingManager.ReleaseStreamingLock();
		m_lockedStreaming = false;
	}
}
void CTaskUnloadStreamingItems::PostProcessingSynchronous()
{
	for ( auto it = m_itemsToUnload.Begin(), end = m_itemsToUnload.End(); it != end; )
	{
		IStreamingItem& item = *it;
		++it;
		item.Detach( &m_streamingManager );
	}

	m_streamingManager.GetUnloadedItems().ListJoin( Move( m_itemsToUnload ) );
}

void CTaskUnloadStreamingItems::DescribeTask( String& outName ) const
{
	outName = TXT("Unload streaming items");
}

};			// namespace PathLib