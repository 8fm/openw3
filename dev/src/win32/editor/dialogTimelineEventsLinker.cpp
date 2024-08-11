
#include "build.h"
#include "dialogTimelineEventsLinker.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneIncludes.h"
#include "../../common/core/feedback.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

DialogTimelineEventsLinker::DialogTimelineEventsLinker( CEdDialogTimeline* timeline )
	: m_itemToLink( nullptr )
	, m_timeline( timeline )
{

}

Bool DialogTimelineEventsLinker::IsRunning() const
{
	return m_itemToLink != nullptr;
}

void DialogTimelineEventsLinker::StartLinkingProcess( DialogTimelineItems::CTimelineItemEvent* e )
{
	SCENE_ASSERT( !m_itemToLink );
	m_itemToLink = e;
}

void DialogTimelineEventsLinker::StopLinkingProcess()
{
	m_itemToLink = nullptr;
}

void DialogTimelineEventsLinker::FinilizeLinkingProcess( CStorySceneEvent* e )
{
	DialogTimelineItems::CTimelineItemEvent* pEvt = m_timeline->FindItemEvent( e );

	// TODO
	if ( pEvt->GetEvent()->HasLinkParent() )
	{
		GFeedback->ShowMsg( TXT("Story Scene"), TXT("WIP. Unable to link already linked events yet.") );
		StopLinkingProcess();
		return;
	}

	LinkEvent( pEvt, m_itemToLink );

	StopLinkingProcess();
}

void DialogTimelineEventsLinker::LinkEvent( DialogTimelineItems::CTimelineItemEvent* pEvt, DialogTimelineItems::CTimelineItemEvent* chEvt ) const
{
	SCENE_ASSERT( pEvt != chEvt );
	if ( pEvt == chEvt )
	{
		return;
	}

	CStorySceneEvent* parent = pEvt->GetEvent();
	CStorySceneEvent* child = chEvt->GetEvent();

	SCENE_ASSERT( !parent->HasLinkParent() );
	if ( parent->HasLinkParent() )
	{
		return;
	}

	if ( child->HasBlendParent() )
	{
		return;
	}

	if ( child->GetGUID() == parent->GetBlendParentGUID() )
	{
		return;
	}

	if ( child->GetClass()->IsA< CStorySceneEventBlend >() )
	{
		return;
	}

	const Float timeOffset = chEvt->GetStart() - pEvt->GetStart();

	SCENE_ASSERT( !child->HasLinkParent() );
	Bool ret = chEvt->SetLinkParent( parent->GetGUID(), timeOffset );

	SCENE_ASSERT( !parent->HasLinkChildGUID( child->GetGUID() ) );
	parent->AddLinkChildGUID( child->GetGUID() );

	if ( ret )
	{
		m_timeline->RequestRebuild();
	}
}

void DialogTimelineEventsLinker::UnlinkEvent( DialogTimelineItems::CTimelineItemEvent* item )
{
	CStorySceneEvent* e = item->GetEvent();
	if ( e->HasLinkParent() )
	{
		CStorySceneEvent* parent = m_timeline->FindEvent( e->GetLinkParentGUID() );
		SCENE_ASSERT( parent );

		if ( parent )
		{
			SCENE_VERIFY( parent->RemoveLinkChildGUID( e->GetGUID() ) );
		}

		item->ResetLinkParent();
	}
}

void DialogTimelineEventsLinker::UnlinkAllChildrenEvents( DialogTimelineItems::CTimelineItemEvent* parentItem )
{
	CStorySceneEvent* parentEvt = parentItem->GetEvent();
	if ( parentEvt )
	{
		const TDynArray< CGUID > children = parentEvt->GetLinkChildrenGUID(); // copy
		for ( Uint32 i=0; i<children.Size(); ++i )
		{
			DialogTimelineItems::CTimelineItemEvent* childItem = m_timeline->FindItemEvent( children[ i ] );
			UnlinkEvent( childItem );
		}
	}
}

void DialogTimelineEventsLinker::Reset()
{
	StopLinkingProcess();

	m_selectedChildEvents.ClearFast();
	m_selectedParentEvents.ClearFast();
	m_selectedChildRects.ClearFast();
	m_selectedParentRects.ClearFast();
	m_selectedChildValid.ClearFast();
	m_selectedParentValid.ClearFast();
}

void DialogTimelineEventsLinker::OnSelectionChanged( const TDynArray< ITimelineItem* >& selectedItems )
{
	{
		m_selectedChildEvents.ClearFast();
		m_selectedParentEvents.ClearFast();

		const Uint32 num = selectedItems.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( DialogTimelineItems::CTimelineItemEvent* item = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( selectedItems[ i ] ) )
			{
				if ( item->GetEvent()->HasLinkParent() )
				{
					DialogTimelineItems::CTimelineItemEvent* parentItem = m_timeline->FindItemEvent( item->GetEvent()->GetLinkParentGUID() );
					SCENE_ASSERT( parentItem );

					m_selectedChildEvents.PushBack( item );
					m_selectedParentEvents.PushBack( parentItem );
				}
			}
		}

		const Uint32 size = m_selectedChildEvents.Size();
		m_selectedChildRects.Resize( size );
		m_selectedParentRects.Resize( size );
		m_selectedChildValid.Resize( size );
		m_selectedParentValid.Resize( size );
	}

	if ( selectedItems.Size() != 1 )
	{
		StopLinkingProcess();
	}
	else if ( m_itemToLink )
	{
		DialogTimelineItems::CTimelineItemEvent* item = dynamic_cast< DialogTimelineItems::CTimelineItemEvent* >( selectedItems[ 0 ] );
		if ( item )
		{
			FinilizeLinkingProcess( item->GetEvent() );
		}
	}
}

/*

\param rect Rect in global space (timeline space).
*/
void DialogTimelineEventsLinker::OnDrawItem( const ITimelineItem* item, const wxRect& rect )
{
	{
		const Uint32 num = m_selectedChildEvents.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( m_selectedChildEvents[ i ] == item )
			{
				m_selectedChildRects[ i ] = rect;
				m_selectedChildValid[ i ] = true;
				return;
			}
		}
	}

	{
		const Uint32 num = m_selectedParentEvents.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( m_selectedParentEvents[ i ] == item )
			{
				m_selectedParentRects[ i ] = rect;
				m_selectedParentValid[ i ] = true;
				return;
			}
		}
	}
}

void DialogTimelineEventsLinker::PrintCanvas( const wxPoint& cursorPosition )
{
	if ( IsRunning() )
	{
		// Draw line between m_eventToLink and mouse point
		wxPoint globalPos = m_timeline->GetItemGlobalPosition( m_itemToLink );
		m_timeline->GetDrawGroupVolatile()->GetDrawBuffer()->DrawLine( globalPos.x, globalPos.y, cursorPosition.x, cursorPosition.y, m_itemToLink->GetBorderColor() );
	}

	const Uint32 num = m_selectedChildEvents.Size();
	if ( num > 0 )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( m_selectedChildValid[ i ] && m_selectedParentValid[ i ] )
			{
				const wxRect& rectCh = m_selectedChildRects[ i ];
				const wxRect& rectP = m_selectedParentRects[ i ];

				const wxColor c = m_selectedChildEvents[ i ]->GetBorderColor();

				m_timeline->DrawLink( rectCh, rectP, c, 0.f );
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
