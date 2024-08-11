/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskPostStreaming.h"

#include "pathlibStreamingManager.h"
#include "pathlibWorld.h"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CTaskPostStreaming
///////////////////////////////////////////////////////////////////////////////
CTaskPostStreaming::CTaskPostStreaming( PathLib::CTaskManager& taskManager, IStreamingItem::List::Head&& requestsList )
	: Super( taskManager, T_MODYFICATION, FLAG_USE_POSTPROCESSING, EPriority::PostStreaming )
	, m_requestsList( Move( requestsList ) )
{

}

void CTaskPostStreaming::Process()
{
	IStreamingItem::List::Head::Iterator it( &m_requestsList );
	for ( auto it = m_requestsList.Begin(), end = m_requestsList.End(); it != end; ++it )
	{
		IStreamingItem& item = *it;
		item.PostLoad();
	}
	for ( auto it = m_requestsList.Begin(), end = m_requestsList.End(); it != end; ++it )
	{
		IStreamingItem& item = *it;
		item.PostLoadInterconnection();
	}
}

void CTaskPostStreaming::PostProcessingSynchronous()
{
	CStreamingManager* streamingManager = m_taskManager.GetPathLib().GetStreamingManager();
	IStreamingItem::List::Head::Iterator it( &m_requestsList );
	for ( auto it = m_requestsList.Begin(), end = m_requestsList.End(); it != end; )
	{
		IStreamingItem& item = *it;
		++it;
		item.Attach( streamingManager );
	}
	streamingManager->AttachStreamedItems( Move( m_requestsList ) );
}

void CTaskPostStreaming::DescribeTask( String& outName ) const 
{
	outName = TXT("Post streaming");
}




};			// namespace PathLib