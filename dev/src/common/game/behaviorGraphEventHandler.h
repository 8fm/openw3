#pragma once

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/game/behaviorGraphScriptStateNode.h"

#define W3BEHAVIOR_DECLARE_EVENT_HANDLER_REGISTER_EVENT_CLASS( className, uniqueName )					\
	protected:																							\
	TBehaviorGraphDelayedEventCache< className >	m_## uniqueName ##_events;							\

#define W3BEHAVIOR_IMPLEMENT_EVENT_HANDLER_REGISTER_EVENT_CLASS( className, uniqueName )											\
	template <> RED_INLINE TBehaviorGraphDelayedEventCache< className >& CW3BehaviorGraphEventHandler::GetEventCache< className >()	\
	{																																\
		return m_## uniqueName ##_events;																							\
	}

template < class TEvent >
class TBehaviorGraphDelayedEventCache
{
protected:
	TDynArray< TEvent >				m_events;
public:
	RED_INLINE void IsEmpty() const									{ return m_events.Empty(); }
	RED_INLINE void PushEvent( const TEvent& e )						{ m_events.PushBack( e ); }
	RED_INLINE void Handle( CBehaviorGraphInstance* instance )		{ ForEach( m_events, [ instance ] ( TEvent& e ) { e.Handle( instance ); } ); m_events.ClearFast(); }
	RED_INLINE void Clear()											{ m_events.ClearFast(); }
};

class CW3BehaviorGraphEventHandler : public CBehaviorGraphEventHandler
{
public:
	CW3BehaviorGraphEventHandler()										{}
	~CW3BehaviorGraphEventHandler()										{}

	template< class TEvent >
	TBehaviorGraphDelayedEventCache< TEvent >& GetEventCache();

	template< class TEvent >
	void AddEvent( const TEvent& e )
	{
		GetEventCache< TEvent >().PushEvent( e );
	}

	void HandleDelayedEvents( CBehaviorGraphInstance* instance ) override;

	static void LazyCreate( CBehaviorGraphInstance& instance );
	static CW3BehaviorGraphEventHandler* Get( const CBehaviorGraphInstance& instance ) { return static_cast< CW3BehaviorGraphEventHandler* >( instance.GetEventHandler() ); }

	W3BEHAVIOR_DECLARE_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptStateNode::CEvent, CBehaviorGraphScriptStateNode )
	W3BEHAVIOR_DECLARE_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptStateReportingNode::CEvent, CBehaviorGraphScriptStateReportingNode )
	W3BEHAVIOR_DECLARE_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptComponentStateNode::CEvent, CBehaviorGraphScriptComponentStateNode )
};

W3BEHAVIOR_IMPLEMENT_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptStateNode::CEvent, CBehaviorGraphScriptStateNode )
W3BEHAVIOR_IMPLEMENT_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptStateReportingNode::CEvent, CBehaviorGraphScriptStateReportingNode )
W3BEHAVIOR_IMPLEMENT_EVENT_HANDLER_REGISTER_EVENT_CLASS( CBehaviorGraphScriptComponentStateNode::CEvent, CBehaviorGraphScriptComponentStateNode )

#undef W3BEHAVIOR_DECLARE_EVENT_HANDLER_REGISTER_EVENT_CLASS
#undef W3BEHAVIOR_IMPLEMENT_EVENT_HANDLER_REGISTER_EVENT_CLASS